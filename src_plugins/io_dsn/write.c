/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format write
 *  pcb-rnd Copyright (C) 2018,2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "plug_io.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_bool.h>
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "layer.h"
#include "layer_grp.h"
#include "obj_pstk_inlines.h"

#include "write.h"

#include "../src_plugins/lib_netmap/netmap.h"
#include "../src_plugins/lib_netmap/placement.h"
#include "../src_plugins/lib_netmap/pstklib.h"
#include "../src_plugins/lib_polyhelp/topoly.h"

#define GNAME_MAX 64
typedef char grp_name_t[GNAME_MAX];

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	pcb_netmap_t nmap;
	pcb_pstklib_t protolib;
	pcb_placement_t footprints;
	grp_name_t grp_name[PCB_MAX_LAYERGRP];
} dsn_write_t;

/* Return the parent net of the object if it is not anon */
static pcb_net_t *get_net(dsn_write_t *wctx, pcb_any_obj_t *obj)
{
	pcb_net_t *net = htpp_get(&wctx->nmap.o2n, obj);

	if ((net == NULL) || (net->name == NULL)) return NULL;
	if ((*net->name == 'n') && (strncmp(net->name, "netmap_anon_", 12) == 0)) return NULL;
	return net;
}

static void group_name(char *dst, const char *src, rnd_layergrp_id_t gid)
{
	int n;
	char *d;
	const char*s;

	n = sprintf(dst, "%d__", (int)gid);

	for(d = dst+n, s = src; (*s != '\0') && (n < GNAME_MAX - 1); s++,d++,n++) {
		if ((*s == '"') || (*s < 32) || (*s > 126))
			*d = '_';
		else
			*d = *s;
	}
	*d = '\0';
}

/* Board coords */
#define COORD(c)  (c)
#define COORDX(x) (x)
#define COORDY(y) (PCB->hidlib.size_y - (y))

/* local coords (subc or padstack context) */
#define LCOORD(c)  (c)
#define LCOORDX(x) (x)
#define LCOORDY(y) (-(y))



#define LINELEN 72

#define line_brk(wctx, linelen, indent, sep) \
do { \
		if (linelen > LINELEN-8) { \
			linelen = fprintf(wctx->f, "\n%s", indent); \
			sep = ""; \
		} \
		else \
			sep = " "; \
} while(0)

static void dsn_write_poly_coords(dsn_write_t *wctx, pcb_poly_t *poly, int *linelen_, const char *indent)
{
	int linelen = *linelen_;
	char *sep;
	long n, end = (poly->HoleIndexN > 0) ? poly->HoleIndex[0] : poly->PointN;

	for(n = 0; n < end; n++) {
		line_brk(wctx, linelen, indent, sep);
		linelen += rnd_fprintf(wctx->f, "%s%[4]", sep, COORDX(poly->Points[n].X));
		line_brk(wctx, linelen, indent, sep);
		linelen += rnd_fprintf(wctx->f, "%s%[4]", sep, COORDY(poly->Points[n].Y));
	}

	line_brk(wctx, linelen, indent, sep);
	linelen += rnd_fprintf(wctx->f, "%s%[4]", sep, COORDX(poly->Points[0].X));
	line_brk(wctx, linelen, indent, sep);
	linelen += rnd_fprintf(wctx->f, "%s%[4]", sep, COORDY(poly->Points[0].Y));

	*linelen_ = linelen;
}

/* Write the (boundary) subtrees in a (structure) */
static void dsn_write_boundary(dsn_write_t *wctx)
{
	pcb_poly_t *bp;

	rnd_fprintf(wctx->f, "    (boundary (rect pcb %[4] %[4] %[4] %[4]))\n", 0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y);

	bp = pcb_topoly_1st_outline(wctx->pcb, PCB_TOPOLY_KEEP_ORIG | PCB_TOPOLY_FLOATING);
	if (bp != NULL) {
		int linelen = fprintf(wctx->f, "    (boundary (path signal 0");
		dsn_write_poly_coords(wctx, bp, &linelen, "      ");
		if (linelen > LINELEN-2)
			fprintf(wctx->f, "\n      ))");
		else
			fprintf(wctx->f, "))\n");
		pcb_poly_free(bp);
	}
}

static int dsn_write_structure(dsn_write_t *wctx)
{
	rnd_layergrp_id_t gid;
	pcb_layergrp_t *lg;

	fprintf(wctx->f, "  (structure\n");
	dsn_write_boundary(wctx);



	for(gid = 0, lg = wctx->pcb->LayerGroups.grp; gid < wctx->pcb->LayerGroups.len; gid++,lg++) {
		if (lg->ltype & PCB_LYT_BOUNDARY) {
			continue;
		}

		if (!(lg->ltype & PCB_LYT_COPPER))
			continue;
		group_name(wctx->grp_name[gid], lg->name, gid);
		fprintf(wctx->f, "    (layer \"%s\" (type signal))\n", wctx->grp_name[gid]);
	}
	fprintf(wctx->f, "  )\n");

	return 0;
}

static int dsn_write_library_pstk_shape(dsn_write_t *wctx, const char *kw, pcb_pstk_shape_t *shp, const char *lyn, pcb_pstk_shape_t *slotshp, rnd_coord_t hdia)
{
	switch(shp->shape) {
		case PCB_PSSH_CIRC:
			rnd_fprintf(wctx->f, "      (%s (circle %s %[4] %[4] %[4]))\n", kw, lyn, LCOORD(shp->data.circ.dia), LCOORDX(shp->data.circ.x), LCOORDY(shp->data.circ.y));
			break;
		case PCB_PSSH_LINE:
			{
				const char *aperture = shp->data.line.square ? " (aperture_type square)" : "";
				rnd_fprintf(wctx->f, "      (%s (path %s %[4] %[4] %[4] %[4] %[4]%s))\n", kw, lyn, LCOORD(shp->data.line.thickness), LCOORDX(shp->data.line.x1), LCOORDY(shp->data.line.y1), LCOORDX(shp->data.line.x2), LCOORDY(shp->data.line.y2), aperture);
			}
			break;
		case PCB_PSSH_POLY:
			{
				int n, linelen;
				const char *indent = "         ", *sep = " ";
				linelen = fprintf(wctx->f, "      (%s (poly %s 0", kw, lyn);
				for(n = 0; n < shp->data.poly.len; n++) {
					line_brk(wctx, linelen, indent, sep);
					linelen += rnd_fprintf(wctx->f, "%s%[4]", sep, LCOORDX(shp->data.poly.x[n]));
					line_brk(wctx, linelen, indent, sep);
					linelen += rnd_fprintf(wctx->f, "%s%[4]", sep, LCOORDY(shp->data.poly.y[n]));
				}
				fprintf(wctx->f, "))\n");
			}
			break;
		case PCB_PSSH_HSHADOW:
			{
				if (slotshp != NULL)
					dsn_write_library_pstk_shape(wctx, kw, slotshp, lyn, NULL, hdia);
				else
					rnd_fprintf(wctx->f, "      (%s (circle %s %[4]))\n", kw, lyn, LCOORD(hdia));
			}
			break;
	}
}

static int dsn_write_library_pstk_protos(dsn_write_t *wctx)
{
	htprp_entry_t *e;
	for(e = htprp_first(&wctx->protolib.protos); e != NULL; e = htprp_next(&wctx->protolib.protos, e)) {
		pcb_pstklib_entry_t *pe = (pcb_pstklib_entry_t *)e->value;
		pcb_pstk_tshape_t *ts = &pe->proto.tr.array[0];
		pcb_pstk_shape_t *slotshp = NULL;
		int n, has_hole;

		fprintf(wctx->f, "    (padstack pstk_%ld\n", pe->id);
		for(n = 0; n < ts->len; n++) {
			if (ts->shape[n].layer_mask & PCB_LYT_MECH) {
				slotshp = &ts->shape[n];
				break;
			}
		}

		if ((pe->proto.hdia > 0) && (slotshp == NULL)) {
			rnd_fprintf(wctx->f, "      (hole (circle signal %[4] 0 0))\n", LCOORD(pe->proto.hdia));
			rnd_fprintf(wctx->f, "      (hole (circle power %[4] 0 0))\n", LCOORD(pe->proto.hdia));
			has_hole = 1;
		}
		else if (slotshp != NULL) {
			dsn_write_library_pstk_shape(wctx, "hole", slotshp, "signal", slotshp, pe->proto.hdia);
			dsn_write_library_pstk_shape(wctx, "hole", slotshp, "power", slotshp, pe->proto.hdia);
			has_hole = 1;
		}
		else
			has_hole = 0;

		if (has_hole)
			fprintf(wctx->f, "      (plating %s)\n", (pe->proto.hplated ? "plated" : "nonplated"));

		for(n = 0; n < ts->len; n++) {
			pcb_layergrp_t *lg;
			rnd_layergrp_id_t gid;
			pcb_layer_type_t lyt = ts->shape[n].layer_mask;

			if (!(lyt & PCB_LYT_COPPER))
				continue;

			for(gid = 0, lg = wctx->pcb->LayerGroups.grp; gid < wctx->pcb->LayerGroups.len; gid++,lg++) {
				if ((lg->ltype & lyt) == lyt)
					dsn_write_library_pstk_shape(wctx, "shape", &ts->shape[n], wctx->grp_name[gid], slotshp, pe->proto.hdia);
			}
		}
		fprintf(wctx->f, "      (attach off)\n"); /* no via placed under smd pads */
		fprintf(wctx->f, "    )\n");
	}

	return 0;
}

static int dsn_write_pin_via(dsn_write_t *wctx, pcb_pstk_t *padstack, int is_pin)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(padstack);
	pcb_pstklib_entry_t *pe;
	pcb_net_t *net = get_net(wctx, (pcb_any_obj_t *)padstack);

	if (proto == NULL) {
		pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "pstk-inv-proto", "invalid padstack prototype", "Failed to look up padstack prototype (board context)");
		return 0;
	}
	pe = pcb_pstklib_get(&wctx->protolib, proto);
	if (pe == NULL) {
		pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "pstk-inv-proto", "invalid padstack prototype", "Failed to look up padstack prototype (padstack hash)");
		return 0;
	}

	if (is_pin) {
		const char *termid = padstack->term;
		if ((termid == NULL) || (*termid == '\0'))
			termid = "anon";
		rnd_fprintf(wctx->f, "      (pin pstk_%ld %s %[4] %[4]", pe->id, termid, LCOORDX(padstack->x), LCOORDY(padstack->y));
		if (padstack->rot != 0) fprintf(wctx->f, " (rotate %d)", (int)padstack->rot);
/*		if (padstack->xmirror != 0)  pcb_io_incompat_save(PCB->Data, NULL, "pin-xmirror", "geo-mirrored pin not supported", "padstack will be saved unmirrored due to file format limitations"); - this usually happens for subc proto on bottom side */
		if (padstack->smirror != 0)  pcb_io_incompat_save(PCB->Data, NULL, "pin-smirror", "side-mirrored pin not supported", "padstack will be saved unmirrored due to file format limitations");

		fprintf(wctx->f, ")\n");
	}
	else {
		if (padstack->rot != 0)      pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "via-rot", "rotated via not supported", "padstack will be saved with 0 rotation due to file format limitations");
		if (padstack->xmirror != 0)  pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "via-xmirror", "geo-mirrored via not supported", "padstack will be saved unmirrored due to file format limitations");
		if (padstack->smirror != 0)  pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "via-smirror", "side-mirrored via not supported", "padstack will be saved unmirrored due to file format limitations");

		rnd_fprintf(wctx->f, "    (via pstk_%ld %[4] %[4]", pe->id, COORDX(padstack->x), COORDY(padstack->y));
		if (net != NULL)
			fprintf(wctx->f, " (net \"%s\")", net->name);
		fprintf(wctx->f, ")\n");
	}
	return 0;
}

static int dsn_write_library_subc(dsn_write_t *wctx, pcb_subc_t *subc)
{
	fprintf(wctx->f, "    (image subc_%ld\n", subc->ID);
	PCB_PADSTACK_LOOP(subc->data);
	{
		dsn_write_pin_via(wctx, padstack, 1);
	}
	PCB_END_LOOP;
	fprintf(wctx->f, "    )\n");
	return 0;
}

static int dsn_write_library_subcs(dsn_write_t *wctx)
{
	htscp_entry_t *e;
	for(e = htscp_first(&wctx->footprints.subcs); e != NULL; e = htscp_next(&wctx->footprints.subcs, e))
		if (dsn_write_library_subc(wctx, e->value) != 0)
			return -1;
	return 0;
}

static int dsn_write_library(dsn_write_t *wctx)
{
	int res = 0;

	fprintf(wctx->f, "  (library\n");
	res |= dsn_write_library_pstk_protos(wctx);
	res |= dsn_write_library_subcs(wctx);
	fprintf(wctx->f, "  )\n");

	return res;
}


static int dsn_write_wiring(dsn_write_t *wctx)
{
	rnd_layer_id_t lid;
	pcb_layer_t *ly;

	fprintf(wctx->f, "  (wiring\n");

	PCB_PADSTACK_LOOP(wctx->pcb->Data);
	{
		dsn_write_pin_via(wctx, padstack, 0);
	}
	PCB_END_LOOP;


	for(ly = wctx->pcb->Data->Layer, lid = 0; lid < wctx->pcb->Data->LayerN; lid++,ly++) {
		char gname[GNAME_MAX];
		rnd_layergrp_id_t gid = pcb_layer_get_group_(ly);
		pcb_layergrp_t *lg = pcb_get_layergrp(wctx->pcb, gid);
		pcb_net_t *net;

		if ((lg == NULL) || !(lg->ltype & PCB_LYT_COPPER))
			continue;

		group_name(gname, lg->name, gid);

		{
			pcb_gfx_t *gfx;
			for(gfx = gfxlist_first(&ly->Gfx); gfx != NULL; gfx = gfxlist_next(gfx))
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)gfx, "gfx", "gfx can not be exported", "please use the lihata board format");
		}

		PCB_LINE_LOOP(ly);
		{
			net = get_net(wctx, (pcb_any_obj_t *)line);
			rnd_fprintf(wctx->f,"    (wire (path \"%s\" %[4] %[4] %[4] %[4] %[4])", gname, line->Thickness,
				COORDX(line->Point1.X), COORDY(line->Point1.Y),
				COORDX(line->Point2.X), COORDY(line->Point2.Y));
			if (net != NULL)
				fprintf(wctx->f, " (net \"%s\")", net->name);
			fprintf(wctx->f, " (type protect))\n");
		}
		PCB_END_LOOP;

		PCB_ARC_LOOP(ly);
		{
			rnd_coord_t e1x, e1y, e2x, e2y;
			double sa = fabs(arc->StartAngle);

			if ((arc->Delta != 90) && (arc->Delta != -90)) {
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)arc, "arc-delta-angle", "invalid arc delta (span) angle", "DSN supports only 90 degree arcs. This object is omitted from the output");
				continue;
			}

			if ((sa != 0) && (sa != 90) && (sa != 180) && (sa != 270) && (sa != 360)) {
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)arc, "arc-start-angle", "invalid arc start angle", "DSN supports only \"axis aligned\" arcs, that is start angle must be integer*90. This object is omitted from the output");
				continue;
			}

			net = get_net(wctx, (pcb_any_obj_t *)arc);
			pcb_arc_get_end(arc, 0, &e1x, &e1y);
			pcb_arc_get_end(arc, 1, &e2x, &e2y);

			rnd_fprintf(wctx->f,"    (wire (qarc \"%s\" %[4] %[4] %[4] %[4] %[4] %[4] %[4])", gname, arc->Thickness,
				COORDX(e1x), COORDY(e1y),
				COORDX(e2x), COORDY(e2y),
				COORDX(arc->X), COORDY(arc->Y));
			if (net != NULL)
				fprintf(wctx->f, " (net \"%s\")", net->name);
			fprintf(wctx->f, " (type protect))\n");
		}
		PCB_END_LOOP;

		PCB_POLY_LOOP(ly);
		{
			int linelen;
			net = get_net(wctx, (pcb_any_obj_t *)polygon);
			linelen = rnd_fprintf(wctx->f,"    (wire (polygon \"%s\" 0", gname);
			dsn_write_poly_coords(wctx, polygon, &linelen, "      ");
			if (net != NULL)
				fprintf(wctx->f, " (net \"%s\")", net->name);
			fprintf(wctx->f, " (type protect)))\n");

			if (polygon->HoleIndexN > 0)
				pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)polygon, "poly-hole", "Polygon holes are not supported", "Saving the polygon without hole.");
		}
		PCB_END_LOOP;

	}
	fprintf(wctx->f, "  )\n");
	return 0;
}

static int dsn_write_placement(dsn_write_t *wctx)
{
	fprintf(wctx->f, "  (placement\n");
	fprintf(wctx->f, "    (place_control (flip_style mirror_first))\n");

	PCB_SUBC_LOOP(wctx->pcb->Data);
	{
		pcb_subc_t *proto = pcb_placement_get(&wctx->footprints, subc);
		rnd_coord_t x, y;
		double rot;
		int on_bottom;
		const char *refdes = subc->refdes, *value;

		if (pcb_subc_get_origin(subc, &x, &y) != 0) {
			pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)subc, "subc-placement", "can't get subc coords", "Failed to determine subcircuit coordinates - subc is missing from the output");
			continue;
		}
		if (pcb_subc_get_rotation(subc, &rot) != 0) {
			pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)subc, "subc-placement", "can't get subc rotation", "Failed to determine subcircuit rotation - subc is missing from the output");
			continue;
		}
		if (pcb_subc_get_side(subc, &on_bottom) != 0) {
			pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)subc, "subc-placement", "can't get subc side", "Failed to determine subcircuit board side - subc is missing from the output");
			continue;
		}

		if ((refdes == NULL) || (*refdes == '\0'))
			refdes = "anon";

		value = pcb_attrib_get(subc, "value");

		fprintf(wctx->f, "    (component subc_%ld\n", proto->ID);
		rnd_fprintf(wctx->f, "      (place %s %[4] %[4] %s %d",
			refdes, COORDX(x), COORDY(y), (on_bottom ? "back" : "front"), (int)rot);
		if (value != NULL)
			fprintf(wctx->f, " (property (value '%s'))", value);
		fprintf(wctx->f, ")\n");
		fprintf(wctx->f, "    )\n");
	}
	PCB_END_LOOP;

	fprintf(wctx->f, "  )\n");
	return 0;
}

static int dsn_write_network(dsn_write_t *wctx)
{
	htsp_entry_t *e;
	htsp_t *nl = &wctx->pcb->netlist[PCB_NETLIST_EDITED];
	char *sep;
	int linelen;

	fprintf(wctx->f, "  (network\n");
	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_t *net = e->value;
		pcb_net_term_t *t;

		fprintf(wctx->f, "    (net %s\n", net->name);
		linelen = fprintf(wctx->f, "      (pins");

		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
			line_brk(wctx, linelen, "      ", sep);
			linelen += rnd_fprintf(wctx->f, " %s%s-%s", sep, t->refdes, t->term);
		}
		line_brk(wctx, linelen, "      ", sep);
		linelen += rnd_fprintf(wctx->f, ")\n");
		fprintf(wctx->f, "    )\n");
	}

	linelen = fprintf(wctx->f, "    (class pcb_rnd_default ");
	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_t *net = e->value;
		line_brk(wctx, linelen, "      ", sep);
		linelen += fprintf(wctx->f, "%s%s", sep, net->name);
	}
	fprintf(wctx->f, "\n");

	fprintf(wctx->f, "      (circuit\n");
	fprintf(wctx->f, "        (use_via pstk_%ld)\n", conf_core.design.via_proto);
	fprintf(wctx->f, "      )\n");
	fprintf(wctx->f, "      (rule\n");
	rnd_fprintf(wctx->f, "        (width %[4])\n", conf_core.design.line_thickness);
	rnd_fprintf(wctx->f, "        (clearance %[4])\n", conf_core.design.clearance);
	fprintf(wctx->f, "      )\n");
	fprintf(wctx->f, "    )\n");
	fprintf(wctx->f, "  )\n");
	return 0;
}

static int dsn_write_board(dsn_write_t *wctx)
{
	const char *s;
	int res = 0;

	if (pcb_netmap_init(&wctx->nmap, wctx->pcb, 0) != 0) {
		rnd_message(RND_MSG_ERROR, "Can not set up net map\n");
		return -1;
	}

	pcb_pstklib_init(&wctx->protolib, wctx->pcb);
	pcb_pstklib_build_pcb(&wctx->protolib, 1);
	pcb_placement_init(&wctx->footprints, wctx->pcb);
	pcb_placement_build(&wctx->footprints, wctx->pcb->Data);

	fprintf(wctx->f, "(pcb ");
	if ((wctx->pcb->hidlib.name != NULL) && (*wctx->pcb->hidlib.name != '\0')) {
		for(s = wctx->pcb->hidlib.name; *s != '\0'; s++) {
			if (isalnum(*s))
				fputc(*s, wctx->f);
			else
				fputc('_', wctx->f);
		}
		fputc('\n', wctx->f);
	}
	else
		fprintf(wctx->f, "anon\n");

	fprintf(wctx->f, "  (parser\n");
	fprintf(wctx->f, "    (string_quote \")\n");
	fprintf(wctx->f, "    (space_in_quoted_tokens on)\n");
	fprintf(wctx->f, "    (host_cad \"pcb-rnd/io_dsn\")\n");
	fprintf(wctx->f, "    (host_version \"%s\")\n", PCB_VERSION);
	fprintf(wctx->f, "  )\n");

	/* set units to mm with nm resolution */
	fprintf(wctx->f, "  (resolution mm 1000000)\n");
	fprintf(wctx->f, "  (unit mm)\n");
	rnd_printf_slot[4] = "%.07mm";

	res |= dsn_write_structure(wctx);
	res |= dsn_write_library(wctx);
	res |= dsn_write_wiring(wctx);
	res |= dsn_write_placement(wctx);
	res |= dsn_write_network(wctx);

	fprintf(wctx->f, ")\n");

	pcb_placement_uninit(&wctx->footprints);
	pcb_pstklib_uninit(&wctx->protolib);
	pcb_netmap_uninit(&wctx->nmap);
	return res;
}

int io_dsn_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, rnd_bool emergency)
{
	dsn_write_t wctx = {0};

	wctx.f = FP;
	wctx.pcb = PCB;

	return dsn_write_board(&wctx);
}
