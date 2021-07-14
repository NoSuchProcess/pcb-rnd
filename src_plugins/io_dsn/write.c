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
#include "data.h"
#include "layer.h"
#include "layer_grp.h"
#include "obj_pstk_inlines.h"

#include "write.h"

#include "../src_plugins/lib_netmap/netmap.h"
#include "../src_plugins/lib_netmap/pstklib.h"
#include "../src_plugins/lib_polyhelp/topoly.h"

#define GNAME_MAX 64
typedef char grp_name_t[GNAME_MAX];

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	pcb_netmap_t nmap;
	pcb_pstklib_t protolib;
	grp_name_t grp_name[PCB_MAX_LAYERGRP];
} dsn_write_t;


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
	long n;
	int linelen = *linelen_;
	char *sep;

	for(n = 0; n < poly->PointN; n++) {
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

static int dsn_write_library(dsn_write_t *wctx)
{
	int res = 0;

	fprintf(wctx->f, "  (library\n");
	res |= dsn_write_library_pstk_protos(wctx);
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
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(padstack);
		pcb_pstklib_entry_t *pe;

		if (proto == NULL) {
			pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "pstk-inv-proto", "invalid padstack prototype", "Failed to look up padstack prototype (board context)");
			continue;
		}
		pe = pcb_pstklib_get(&wctx->protolib, proto);
		if (pe == NULL) {
			pcb_io_incompat_save(PCB->Data, (pcb_any_obj_t *)padstack, "pstk-inv-proto", "invalid padstack prototype", "Failed to look up padstack prototype (padstack hash)");
			continue;
		}
		rnd_fprintf(wctx->f, "    (via pstk_%ld %[4] %[4])\n", pe->id, COORDX(padstack->x), COORDY(padstack->y));
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
			net = htpp_get(&wctx->nmap.o2n, (pcb_any_obj_t *)line);
			rnd_fprintf(wctx->f,"    (wire (path \"%s\" %[4] %[4] %[4] %[4] %[4])", gname, line->Thickness,
				COORDX(line->Point1.X), COORDY(line->Point1.Y),
				COORDX(line->Point2.X), COORDY(line->Point2.Y));
			if (net != NULL)
				fprintf(wctx->f, " (net \"%s\")", net->name);
			fprintf(wctx->f, " (type protect))\n");
		}
		PCB_END_LOOP;


	}
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

	fprintf(wctx->f, ")\n");

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
