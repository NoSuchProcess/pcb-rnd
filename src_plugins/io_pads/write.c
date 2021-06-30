/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  PADS IO plugin - write: convert pcb-rnd data model to PADS ASCII
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020 and 2021)
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

#include <stdio.h>

#include <genvector/vti0.h>
#include <genvector/vtp0.h>
#include <librnd/core/unit.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/compat_misc.h>

#include "../src_plugins/lib_netmap/map_2nets.h"
#include "../src_plugins/lib_compat_help/pstk_compat.h"
#include "../src_plugins/lib_netmap/placement.h"

#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "data_it.h"
#include "plug_io.h"

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	double ver;

	/* layer mapping */
	vti0_t gid2plid; /* group ID to pads layer ID */
	vtp0_t plid2grp; /* pads layer ID to pcb-rnd group */

	/* internal caches */
	pcb_placement_t footprints;
} write_ctx_t;

#define CRD(c)   (c)
#define CRDX(c)  CRD(c)
#define CRDY(c)  CRD(wctx->pcb->hidlib.size_y - (c))
#define ROT(r)   (r)

#include "write_layer.c"

static int pads_write_blk_pcb(write_ctx_t *wctx)
{
	int coplyn;
	int gridu = 1; /* default to metric - it's 2021 after all... */
	static const rnd_unit_t *unit_mm = NULL, *unit_mil;

	if (unit_mm == NULL) { /* cache mm and mil units to save on the lookups */
		unit_mm  = rnd_get_unit_struct("mm");
		unit_mil = rnd_get_unit_struct("mil");
	}

	if (rnd_conf.editor.grid_unit == unit_mm)        gridu = 1;
	else if (rnd_conf.editor.grid_unit == unit_mil)  gridu = 0;
	/* we don't ever set 2 for Inches */

	coplyn = pcb_layergrp_list(wctx->pcb, PCB_LYT_COPPER, NULL, 0);

	rnd_fprintf(wctx->f, "*PCB*        GENERAL PARAMETERS OF THE PCB DESIGN\r\n\r\n");
	rnd_fprintf(wctx->f, "UNITS             %d              2=Inches 1=Metric 0=Mils\r\n", gridu);
	rnd_fprintf(wctx->f, "USERGRID     %[4]  %[4]      Space between USER grid points\r\n", CRD(rnd_conf.editor.grid), CRD(rnd_conf.editor.grid));
	rnd_fprintf(wctx->f, "MAXIMUMLAYER     % 2d              Maximum routing layer\r\n", coplyn);
	rnd_fprintf(wctx->f, "WORKLEVEL         0              Level items will be created on\r\n");
	rnd_fprintf(wctx->f, "DISPLAYLEVEL      1              toggle for displaying working level last\r\n");
	rnd_fprintf(wctx->f, "LAYERPAIR         1       2      Layer pair used to route connection\r\n");
	rnd_fprintf(wctx->f, "VIAMODE           T              Type of via to use when routing between layers\r\n");
	rnd_fprintf(wctx->f, "LINEWIDTH    %[4]              Width items will be created with\r\n", CRD(conf_core.design.line_thickness));
	rnd_fprintf(wctx->f, "TEXTSIZE     %[4]  %[4]      Height and LineWidth text will be created with\r\n", CRD(conf_core.design.text_scale), CRD(conf_core.design.text_thickness));
	rnd_fprintf(wctx->f, "JOBTIME           0              Amount of time spent on this PCB design\r\n");
	rnd_fprintf(wctx->f, "DOTGRID      %[4]  %[4]      Space between graphic dots\r\n", CRD(rnd_conf.editor.grid), CRD(rnd_conf.editor.grid));
	rnd_fprintf(wctx->f, "SCALE        10.000              Scale of window expansion\r\n");
	rnd_fprintf(wctx->f, "ORIGIN       %[4]  %[4]      User defined origin location\r\n", CRD(wctx->pcb->hidlib.grid_ox), CRD(wctx->pcb->hidlib.grid_oy));
	rnd_fprintf(wctx->f, "WINDOWCENTER %[4] %[4]     Point defining the center of the window\r\n", CRD(wctx->pcb->hidlib.size_x/2.0), CRD(wctx->pcb->hidlib.size_y/2.0));
	rnd_fprintf(wctx->f, "BACKUPTIME       20              Number of minutes between database backups\r\n");
	rnd_fprintf(wctx->f, "REAL WIDTH        2              Widths greater then this are displayed real size\r\n");
	rnd_fprintf(wctx->f, "ALLSIGONOFF       1              All signal nets displayed on/off\r\n");
	rnd_fprintf(wctx->f, "REFNAMESIZE  %[4]  %[4]      Height and LineWidth used by part ref. names\r\n", CRD(conf_core.design.text_scale), CRD(conf_core.design.text_thickness));
	rnd_fprintf(wctx->f, "HIGHLIGHT         0              Highlight nets flag\r\n");

#if 0
these are not yet exported - need to check if we need them:
JOBNAME      foo.pcb
CONCOL 5
FBGCOL 1 0
HATCHGRID    10             Copper pour hatching grid
TEARDROP     2713690        Teardrop tracks
THERLINEWID  15             Copper pour thermal line width
PSVIAGRID    1      1       Push & Shove Via Grid
PADFILLWID   10             CAM finger pad fill width
THERSMDWID   15             Copper pour thermal line width for SMD
MINHATAREA   1              Minimum hatch area
HATCHMODE    0              Hatch generation mode
HATCHDISP    1              Hatch display flag
DRILLHOLE    6              Drill hole checking spacing
MITRERADII   0.5 1.0 1.5 2.0 2.5 3.0 3.5
MITRETYPE    1              Mitring type
HATCHRAD     0.500000       Hatch outline smoothing radius
MITREANG     180 180 180 180 180 180 180
HATCHANG     45             Hatch angle
THERFLAGS    65536          Copper pour thermal line flags
DRLOVERSIZE  3              Drill oversize for plated holes
PLANERAD     0.000000       Plane outline smoothing radius
PLANEFLAGS   ALL OUTLINE Y Y Y N N Y Y N N N Y Y N Y Y N N   Plane and Test Points flags
COMPHEIGHT   0              Board Top Component Height Restriction
KPTHATCHGRID 100            Copper pour hatching grid
BOTCMPHEIGHT 0              Board Bottom Component Height Restriction
FANOUTGRID   1      1       Fanout grid
FANOUTLENGTH 250            Maximum fanout length
ROUTERFLAGS  83879441       Autorouter specific flags
VERIFYFLAGS  1373           Verify Design flags
PLNSEPGAP    20             Plane separation gap
IDFSHAPELAY  0              IDF shapes layer

TEARDROPDATA     90     90 
#endif

	rnd_fprintf(wctx->f, "\r\n");
	return 0;
}

static int pads_write_blk_reuse(write_ctx_t *wctx)
{
	rnd_fprintf(wctx->f, "*REUSE*\r\n");
	rnd_fprintf(wctx->f, "\r\n");
	rnd_fprintf(wctx->f, "*REMARK* TYPE TYPENAME\r\n");
	rnd_fprintf(wctx->f, "*REMARK* TIMESTAMP SECONDS\r\n");
	rnd_fprintf(wctx->f, "*REMARK* PART NAMING PARTNAMING\r\n");
	rnd_fprintf(wctx->f, "*REMARK* PART NAME\r\n");
	rnd_fprintf(wctx->f, "*REMARK* NET NAMING NETNAMING\r\n");
	rnd_fprintf(wctx->f, "*REMARK* NET MERGE NAME\r\n");
	rnd_fprintf(wctx->f, "*REMARK* REUSE INSTANCENM PARTNAMING NETNAMING X Y ORI GLUED\r\n");
	rnd_fprintf(wctx->f, "\r\n");
	return 0;
}

static void pads_write_text(write_ctx_t *wctx, const pcb_text_t *t, int plid, int is_label)
{
	rnd_coord_t hght = t->BoundingBox.Y2 - t->BoundingBox.Y1;
	char mir  = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, t) ? 'M' : 'N';
	char *alg = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, t) ? "RIGHT DOWN" : "LEFT UP";

	if (is_label)
		rnd_fprintf(wctx->f, "VALUE ");
	else
		rnd_fprintf(wctx->f, "      ");

	rnd_fprintf(wctx->f, "%[4] %[4] %f %d %[4] %[4] %c %s\r\n",
		CRDX(t->X), CRDY(t->Y), ROT(t->rot), plid, CRD(hght), (rnd_coord_t)RND_MM_TO_COORD(0.1), mir, alg);
	rnd_fprintf(wctx->f, "Regular <Romansim Stroke Font>\r\n");

	if (is_label) {
		if (strstr(t->TextString, "%a.parent.refdes%") != 0) fprintf(wctx->f, "Ref.Des.\r\n");
		else if (strstr(t->TextString, "%a.parent.footprint%") != 0) fprintf(wctx->f, "Part Type\r\n");
		else fprintf(wctx->f, "%s\r\n", t->TextString);
	}
	else
		fprintf(wctx->f, "%s\r\n", t->TextString);
}

static int pads_write_blk_text(write_ctx_t *wctx)
{
	pcb_text_t *t;
	int li;
	pcb_layer_t *l;

	rnd_fprintf(wctx->f, "*TEXT*       FREE TEXT\r\n\r\n");
	rnd_fprintf(wctx->f, "*REMARK* XLOC YLOC ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST .REUSE. INSTANCENM\r\n");
	rnd_fprintf(wctx->f, "*REMARK* FONTSTYLE FONTFACE\r\n\r\n");

/*
       1234        5678  90.000 20          70          10 N   LEFT   DOWN
Regular <Romansim Stroke Font>
text string
*/

	for(li = 0, l = wctx->pcb->Data->Layer; li < wctx->pcb->Data->LayerN; li++,l++) {
		int plid = pads_layer2plid(wctx, l);
		if (plid <= 0)
			continue;
		for(t = textlist_first(&l->Text); t != NULL; t = textlist_next(t))
			pads_write_text(wctx, t, plid, 0);
	}

	rnd_fprintf(wctx->f, "\r\n");
	return 0;
}

static void pads_write_piece_line(write_ctx_t *wctx, pcb_line_t *l, int plid)
{
	rnd_fprintf(wctx->f, "OPEN 2   %[4] 0 %d\r\n", CRD(l->Thickness), plid);
	rnd_fprintf(wctx->f, "%[4]   %[4]\r\n", CRDX(l->Point1.X), CRDY(l->Point1.Y));
	rnd_fprintf(wctx->f, "%[4]   %[4]\r\n", CRDX(l->Point2.X), CRDY(l->Point2.Y));
}

static void pads_write_piece_arc(write_ctx_t *wctx, pcb_arc_t *a, int plid)
{
	double sa, da;
	rnd_coord_t x1, y1, x2, y2;

	pcb_arc_get_end(a, 0, &x1, &y1);
	pcb_arc_get_end(a, 1, &x2, &y2);
	sa = (a->StartAngle) - 180;
	da = (a->Delta);

	rnd_fprintf(wctx->f, "OPEN 2   %[4] %d\r\n", CRD(a->Thickness), plid);
	rnd_fprintf(wctx->f, "%[4] %[4]   %d %d    %[4] %[4]    %[4] %[4]\r\n",
		CRDX(x1), CRDY(y1), (int)rnd_round(sa*10), (int)rnd_round(da*10),
		CRDX(a->X - a->Width), CRDY(a->Y - a->Height),
		CRDX(a->X + a->Width), CRDY(a->Y + a->Height));

	rnd_fprintf(wctx->f, "%[4] %[4]\r\n", CRDX(x2), CRDY(y2));
}

static void pads_write_piece_cop_poly(write_ctx_t *wctx, pcb_poly_t *p, int plid)
{
	long n;

	rnd_fprintf(wctx->f, "COPCLS %ld   0 %d\r\n", (long)p->PointN+1, plid);
	for(n = 0; n < p->PointN; n++)
		rnd_fprintf(wctx->f, "%[4] %[4]\r\n", CRDX(p->Points[n].X), CRDY(p->Points[n].Y));
	rnd_fprintf(wctx->f, "%[4] %[4]\r\n", CRDX(p->Points[0].X), CRDY(p->Points[0].Y));
}

static int pads_write_blk_lines(write_ctx_t *wctx)
{
	rnd_layer_id_t lid;
	pcb_layer_t *ly;

	rnd_fprintf(wctx->f, "*LINES*      LINES ITEMS\r\n\r\n");
	rnd_fprintf(wctx->f, "*REMARK* NAME TYPE XLOC YLOC PIECES TEXT SIGSTR\r\n");
	rnd_fprintf(wctx->f, "*REMARK* .REUSE. INSTANCE RSIGNAL\r\n");
	rnd_fprintf(wctx->f, "*REMARK* PIECETYPE CORNERS WIDTHHGHT LINESTYLE LEVEL [RESTRICTIONS]\r\n");
	rnd_fprintf(wctx->f, "*REMARK* XLOC YLOC BEGINANGLE DELTAANGLE\r\n");
	rnd_fprintf(wctx->f, "*REMARK* XLOC YLOC ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST\r\n\r\n");

	for(lid = 0, ly = wctx->pcb->Data->Layer; lid < wctx->pcb->Data->LayerN; lid++,ly++) {
		pcb_line_t *l;
		pcb_arc_t *a;
		pcb_poly_t *p;
		int plid = pads_layer2plid(wctx, ly);

		if (plid <= 0)
			continue;

		l = linelist_first(&ly->Line);
		if (l != NULL) {
			rnd_fprintf(wctx->f, "lines_lid_%ld    LINES    0      0      %ld\r\n", (long)lid, (long)linelist_length(&ly->Line));
			for(; l != NULL; l = linelist_next(l))
				pads_write_piece_line(wctx, l, plid);
		}
		a = arclist_first(&ly->Arc);
		if (a != NULL) {
			rnd_fprintf(wctx->f, "arcs_lid_%ld    LINES    0      0      %ld\r\n", (long)lid, (long)arclist_length(&ly->Arc));
			for(; a != NULL; a = arclist_next(a))
				pads_write_piece_arc(wctx, a, plid);
		}
		p = polylist_first(&ly->Polygon);
		if (p != NULL) {
			rnd_fprintf(wctx->f, "polys_lid_%ld    COPPER   0      0      %ld\r\n", (long)lid, (long)polylist_length(&ly->Polygon));
			for(; p != NULL; p = polylist_next(p))
				pads_write_piece_cop_poly(wctx, p, plid);
		}
	}

	rnd_fprintf(wctx->f, "\r\n");
	return 0;
}

static int pads_write_pstk_proto(write_ctx_t *wctx, long int pid, pcb_pstk_proto_t *proto, const char *termid)
{
	int n;
	pcb_pstk_tshape_t *ts = &proto->tr.array[0];

	if (!proto->in_use)
		return 0;

	if (termid == NULL) /* board context */
		rnd_fprintf(wctx->f, "PSPOTO_%ld     %[4] %d\n", pid, CRD(proto->hdia), ts->len);
	else /* partdecal context */
		rnd_fprintf(wctx->f, "PAD %s %d\n", termid, ts->len);

	for(n = 0; n < ts->len; n++) {
		const pcb_pstk_shape_t *shape = &ts->shape[n];
		int level = -3333;

		if (shape->layer_mask & PCB_LYT_COPPER) {
			if (shape->layer_mask & PCB_LYT_TOP) level = -2;
			else if (shape->layer_mask & PCB_LYT_INTERN) level = -1;
			else if (shape->layer_mask & PCB_LYT_BOTTOM) level = 0;
		}
		else {
			int lvl = pads_lyt2plid(wctx, shape->layer_mask, NULL);
			if (lvl != 0)
				level = lvl;
		}

		if (level <= -3333) {
			char *tmp = rnd_strdup_printf("padstack proto #%ld, shape #%d uses invalid layer\n", pid, n);
			pcb_io_incompat_save(wctx->pcb->Data, NULL, "pstk-proto-layer", tmp, "This shape will not appear properly. Fix the padstack prototype to use standard layers only.");
			free(tmp);
		}

		switch(shape->shape) {
			case PCB_PSSH_CIRC:
				if ((shape->data.circ.x != 0) || (shape->data.circ.y != 0)) {
					char *tmp = rnd_strdup_printf("padstack proto #%ld, shape #%d uses invalid shape offset for circle\n", pid, n);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "pstk-proto-layer", tmp, "This shape will not appear properly. Fix the padstack prototype to use concentric circles only.");
					free(tmp);
				}
				rnd_fprintf(wctx->f, "%d %[4] R\r\n", level, CRD(shape->data.circ.dia));
				break;
			case PCB_PSSH_LINE:
				{
					double ang1 = atan2(shape->data.line.y1, shape->data.line.x1), ang2 = atan2(shape->data.line.y2, shape->data.line.x2);
					rnd_coord_t len = rnd_distance(shape->data.line.y1, shape->data.line.x1, shape->data.line.y2, shape->data.line.x2);
					rnd_coord_t offs = (len/2) - rnd_distance(0, 0, shape->data.line.y1, shape->data.line.x1);
					rnd_fprintf(wctx->f, "%d %[4] OF %.3f %[4] %[4]\r\n", level, CRD(shape->data.line.thickness), ang1 * RND_RAD_TO_DEG, CRD(len), CRD(offs));
					{
						char *tmp = rnd_strdup_printf("padstack proto #%ld, shape #%d uses line (\"oval finger\" in PADS ASCII)\n", pid, n);
						pcb_io_incompat_save(wctx->pcb->Data, NULL, "pstk-proto-layer", tmp, "This shape is untested and may export incorrectly.");
						free(tmp);
					}
				}
				break;
			case PCB_PSSH_POLY:
				{
					double w, h, cx, cy, rot;
					rnd_bool is_rect = pcb_pstk_shape2rect(shape, &w, &h, &cx, &cy, &rot, NULL, NULL, NULL, NULL);
					if (is_rect) {
						if (cy == 0)
							rnd_fprintf(wctx->f, "%d %[4] RF %.3f %[4] %[4]\r\n", level, CRD(h), rot * RND_RAD_TO_DEG, CRD(w), CRD(cx));
						else if (cx == 0)
							rnd_fprintf(wctx->f, "%d %[4] RF %.3f %[4] %[4]\r\n", level, CRD(w), rot * RND_RAD_TO_DEG + 90.0, CRD(h), CRD(cy));
						else {
							char *tmp = rnd_strdup_printf("padstack proto #%ld, shape #%d is non-centered rectangular polygon\n", pid, n);
							pcb_io_incompat_save(wctx->pcb->Data, NULL, "pstk-proto-layer", tmp, "This shape will not appear properly. A rectangle can be uncentered only along one axis.");
							free(tmp);
							goto write_dummy;
						}
					}
					else {
						char *tmp = rnd_strdup_printf("padstack proto #%ld, shape #%d is non-rectangular polygon\n", pid, n);
						pcb_io_incompat_save(wctx->pcb->Data, NULL, "pstk-proto-layer", tmp, "This shape will not appear properly. Use circle, line or rectangular polygon shape only.");
						free(tmp);
						write_dummy:;
						rnd_fprintf(wctx->f, "%d %[4] R\r\n", level, CRD(proto->hdia*2));
					}
				}
				break;

			case PCB_PSSH_HSHADOW:
				rnd_fprintf(wctx->f, "%d %[4] R\r\n", level, CRD(proto->hdia));
				break;

			default:
				if ((shape->data.circ.x != 0) || (shape->data.circ.y != 0)) {
					char *tmp = rnd_strdup_printf("padstack proto #%ld, shape #%d uses unknown shape\n", pid, n);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "pstk-proto-layer", tmp, "This shape will not appear properly. Fix the padstack prototype to use simpler shapes.");
					free(tmp);
				}
				rnd_fprintf(wctx->f, "%d %[4] R\r\n", level, CRD(proto->hdia * 1.1));

		}
	}
	return 0;
}

static int pads_write_blk_pstk_proto(write_ctx_t *wctx, long int pid, pcb_pstk_proto_t *proto)
{
	return pads_write_pstk_proto(wctx, pid, proto, NULL);
}


static int pads_write_blk_vias(write_ctx_t *wctx)
{
	long n;
	int res = 0;

	rnd_fprintf(wctx->f, "*VIA*  ITEMS\r\n\r\n");
	rnd_fprintf(wctx->f, "*REMARK* NAME  DRILL STACKLINES [DRILL START] [DRILL END]\r\n");
	rnd_fprintf(wctx->f, "*REMARK* LEVEL SIZE SHAPE [INNER DIAMETER]\r\n\r\n");

	for(n = 0; n < wctx->pcb->Data->ps_protos.used; n++) {
		if (pads_write_blk_pstk_proto(wctx, n, &wctx->pcb->Data->ps_protos.array[n]) != 0)
			res = -1;
		rnd_fprintf(wctx->f, "\r\n");
	}

	rnd_fprintf(wctx->f, "\r\n");

	return res;
}

#define SUBC_ID_ATTR "io_pads::__decal__id__"

static int invalid_proto(write_ctx_t *wctx, pcb_subc_t *proto)
{
	char *tmp = rnd_strdup_printf("Footprint of subcircuit %s contains invalid/undefined padstack prototype\n", proto->refdes);
	pcb_io_incompat_save(wctx->pcb->Data, NULL, "subc-proto", tmp, "Output will be invalid/unloadable PADS ASCII.");
	free(tmp);
	return -1;
}

static void partdecal_psproto(write_ctx_t *wctx, pcb_subc_t *proto, int protoid, const char *termid, int *res)
{
	if ((protoid >= 0) && (protoid < proto->data->ps_protos.used)) {
		pcb_pstk_proto_t *psp = &proto->data->ps_protos.array[protoid];
		if (psp->in_use) {
			if (pads_write_pstk_proto(wctx, 0, psp, termid) != 0)
				*res = -1;
		}
		else
			*res = invalid_proto(wctx, proto);
	}
	else
		*res = invalid_proto(wctx, proto);
}

static int partdecal_plid(write_ctx_t *wctx, pcb_subc_t *proto, pcb_any_obj_t *o, const char *where)
{
	int plid = 0;
	pcb_layer_type_t lyt;

	if (o->parent.layer == proto->aux_layer)
		return -3333;

	lyt = pcb_layer_flags_(o->parent.layer);

	if (lyt & PCB_LYT_TOP) plid = 1;
	else if (lyt & PCB_LYT_TOP) plid = 2;
	if (!(lyt & PCB_LYT_SILK) || (plid == 0)) {
		char *tmp = rnd_strdup_printf("Footprint of subcircuit %s contains %s not on top or bottom silk\n", proto->refdes, where);
		pcb_io_incompat_save(wctx->pcb->Data, NULL, "subc-proto", tmp, "Moved to top silk.");
			free(tmp);
		plid = 1;
	}

	return plid;
}

static int pads_write_blk_partdecal(write_ctx_t *wctx, pcb_subc_t *proto, const char *id)
{
	long n, num_pcs = 0, num_terms = 0, num_stacks = 0, num_texts = 0, num_labels = 0;
	pcb_data_it_t it;
	pcb_any_obj_t *o;
	pcb_pstk_t *ps;
	int w_heavy = 0, w_poly = 0, w_via = 0; /* warnings */
	vti0_t psstat = {0}; /* how many times each proto is used */
	int best, res = 0;
	int ps0; /* which proto to use for default "terminal 0" */

	/* count number of pieces, terminals, labels, etc. */
	pcb_subc_cache_find_aux(proto);
	for(o = pcb_data_first(&it, proto->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		int *stat;

		if (o->parent.layer == proto->aux_layer)
			continue;
		switch(o->type) {
			case PCB_OBJ_ARC:
			case PCB_OBJ_LINE:
				if ((o->term != NULL) && !w_heavy) {
					char *tmp = rnd_strdup_printf("Footprint of subcircuit %s contains heavy terminal\n", proto->refdes);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "subc-proto", tmp, "Convert to padstack, PADS ASCII requires padstacks for pins/pads.");
					free(tmp);
					w_heavy = 1;
				}
				num_pcs++;
				break;
			case PCB_OBJ_POLY:
				if (!w_poly) {
					char *tmp = rnd_strdup_printf("Footprint of subcircuit %s contains polygon object\n", proto->refdes);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "subc-proto", tmp, "Can not write polygons in footprints in PADS ASCII");
					free(tmp);
					w_poly = 1;
				}
				break;
			case PCB_OBJ_TEXT:
				if (PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, o))
					num_labels++;
				else
					num_texts++;
				break;

			case PCB_OBJ_PSTK:
				if ((o->term == NULL) && !w_via) {
					char *tmp = rnd_strdup_printf("Footprint of subcircuit %s contains non-terminal padstack\n", proto->refdes);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "subc-proto", tmp, "Omitted from output. Assign dummy terminal ID or convert to objects.");
					free(tmp);
					w_via = 1;
				}
				else
					num_terms++;

				/* calculate statistics of proto usage */
				ps = (pcb_pstk_t *)o;
				stat = vti0_get(&psstat, ps->proto, 1);
				if (stat != NULL)
					(*stat)++;

				break;
			case PCB_OBJ_SUBC:
				TODO("subc-in-subc");
			default:
				{
					char *tmp = rnd_strdup_printf("Footprint of subcircuit %s contains unsupported object\n", proto->refdes);
					pcb_io_incompat_save(wctx->pcb->Data, NULL, "subc-proto", tmp, "only padstack, text, line and arc can go in footprints in PADS ASCII");
					free(tmp);
				}
				break;
		}
	}

	/* count number of padstack prototypes in use */
	best = 0;
	for(n = 0; n < psstat.used; n++) {
		if (psstat.array[n] > 0) {
			num_stacks++;
			if (psstat.array[n] > best) {
				best = psstat.array[n];
				ps0 = n;
			}
		}
	}

	rnd_fprintf(wctx->f, "\r\n%-16s M 1000 1000  %ld %ld %ld %ld %ld\r\n", id, num_pcs, num_terms, num_stacks, num_texts, num_labels);

	/* write pieces */
	for(o = pcb_data_first(&it, proto->data, PCB_OBJ_ARC | PCB_OBJ_LINE); o != NULL; o = pcb_data_next(&it)) {
		int plid = partdecal_plid(wctx, proto, o, "arc/line");
		if (plid > -3333) {
			if (o->type == PCB_OBJ_ARC)
				pads_write_piece_arc(wctx, (pcb_arc_t *)o, plid);
			else if (o->type == PCB_OBJ_LINE)
				pads_write_piece_line(wctx, (pcb_line_t *)o, plid);
		}
	}

	/* write non-label text */
	for(o = pcb_data_first(&it, proto->data, PCB_OBJ_TEXT); o != NULL; o = pcb_data_next(&it)) {
		int plid = partdecal_plid(wctx, proto, o, "text");
		if ((plid > -3333) && (!PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, o)))
			pads_write_text(wctx, (pcb_text_t *)o, plid, 0);
	}

	/* write labels */
	for(o = pcb_data_first(&it, proto->data, PCB_OBJ_TEXT); o != NULL; o = pcb_data_next(&it)) {
		int plid = partdecal_plid(wctx, proto, o, "text");
		if ((plid > -3333) && (PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, o)))
			pads_write_text(wctx, (pcb_text_t *)o, plid, 1);
	}

	/* write terminals */
	for(ps = padstacklist_first(&proto->data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
		rnd_fprintf(wctx->f, "T %[4] %[4] %[4] %[4] %s\r\n",
			CRDX(ps->x), CRDX(ps->y), CRDX(ps->x), CRDX(ps->y), ps->term);
	}

	/* write padstack prototypes for all pins */
	partdecal_psproto(wctx, proto, ps0, "0", &res); /* default */
	for(ps = padstacklist_first(&proto->data->padstack); ps != NULL; ps = padstacklist_next(ps))
		if (ps->proto != ps0)
			partdecal_psproto(wctx, proto, ps->proto, ps->term, &res); /* special */

	rnd_fprintf(wctx->f, "\r\n");
	vti0_uninit(&psstat);
	return res;
}

static int pads_write_blk_partdecals(write_ctx_t *wctx)
{
	int res = 0, cnt = 0;
	htscp_entry_t *e;

	rnd_fprintf(wctx->f, "*PARTDECAL*  ITEMS\r\n\r\n");
	rnd_fprintf(wctx->f, "*REMARK* NAME UNITS ORIX ORIY PIECES TERMINALS STACKS TEXT LABELS\r\n");
	rnd_fprintf(wctx->f, "*REMARK* PIECETYPE CORNERS WIDTHHGHT LEVEL RESTRICTIONS\r\n");
	rnd_fprintf(wctx->f, "*REMARK* PIECETYPE CORNERS WIDTH LEVEL PINNUM\r\n");
	rnd_fprintf(wctx->f, "*REMARK* XLOC YLOC BEGINANGLE DELTAANGLE\r\n");
	rnd_fprintf(wctx->f, "*REMARK* XLOC YLOC ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST\r\n");
	rnd_fprintf(wctx->f, "*REMARK* VISIBLE XLOC YLOC ORI LEVEL HEIGTH WIDTH MIRRORED HJUST VJUST RIGHTREADING\r\n");
	rnd_fprintf(wctx->f, "*REMARK* FONTSTYLE FONTFACE\r\n");
	rnd_fprintf(wctx->f, "*REMARK* T XLOC YLOC NMXLOC NMYLOC PINNUMBER\r\n");
	rnd_fprintf(wctx->f, "*REMARK* PAD PIN STACKLINES\r\n");
	rnd_fprintf(wctx->f, "*REMARK* LEVEL SIZE SHAPE IDIA DRILL [PLATED]\r\n");
	rnd_fprintf(wctx->f, "*REMARK* LEVEL SIZE SHAPE FINORI FINLENGTH FINOFFSET DRILL [PLATED]\r\n");

	for(e = htscp_first(&wctx->footprints.subcs); e != NULL; e = htscp_next(&wctx->footprints.subcs, e)) {
		char tmp[128];
		pcb_subc_t *proto = e->value;
		sprintf(tmp, "subc_%d", cnt++);
		pcb_attribute_put(&proto->Attributes, SUBC_ID_ATTR, tmp);
		if (pads_write_blk_partdecal(wctx, proto, tmp) != 0)
			res = -1;
	}

	rnd_fprintf(wctx->f, "\r\n");
	return res;
}

static int pads_write_blk_parttype(write_ctx_t *wctx)
{
	int res = 0;
	htscp_entry_t *e;

	fprintf(wctx->f, "*PARTTYPE*   ITEMS\r\n");

	fprintf(wctx->f, "*REMARK* NAME DECALNM TYPE GATES SIGPINS UNUSEDPINNMS FLAGS ECO\r\n");
	fprintf(wctx->f, "*REMARK* G/S SWAPTYPE PINS\r\n");
	fprintf(wctx->f, "*REMARK* PINNUMBER SWAPTYPE.PINTYPE\r\n");
	fprintf(wctx->f, "*REMARK* SIGPIN PINNUMBER SIGNAME\r\n");
	fprintf(wctx->f, "*REMARK* PINNUMBER\r\n\r\n");


	for(e = htscp_first(&wctx->footprints.subcs); e != NULL; e = htscp_next(&wctx->footprints.subcs, e)) {
		pcb_subc_t *proto = e->value;
		const char *id = pcb_attribute_get(&proto->Attributes, SUBC_ID_ATTR);
		fprintf(wctx->f, "%s %s UND  0   0   0     0 Y\r\n\r\n", id, id);
	}

	rnd_fprintf(wctx->f, "\r\n");
	return res;
}

static int pads_write_part(write_ctx_t *wctx, pcb_subc_t *subc)
{
	pcb_data_it_t it;
	pcb_any_obj_t *o;
	int numlab = 0, on_bottom;
	pcb_subc_t *proto;
	const char *id;
	rnd_angle_t rot = 0;
	rnd_coord_t x = 0, y = 0;

	proto = htscp_get(&wctx->footprints.subcs, subc);
	assert(proto != NULL);

	for(o = pcb_data_first(&it, proto->data, PCB_OBJ_TEXT); o != NULL; o = pcb_data_next(&it))
		if (PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, o))
			numlab++;

	id = pcb_attribute_get(&proto->Attributes, SUBC_ID_ATTR);

	pcb_subc_get_origin(subc, &x, &y);
	pcb_subc_get_side(subc, &on_bottom);


	rnd_fprintf(wctx->f, "%-16s              %s %[4] %[4] %f U %c 0 -1 0 -1 %d\r\n",
		((subc->refdes == NULL) || (*subc->refdes == '\0')) ? "unknown" : subc->refdes,
		id, x, y, rot, (on_bottom ? 'Y' : 'N'), numlab);

	for(o = pcb_data_first(&it, proto->data, PCB_OBJ_TEXT); o != NULL; o = pcb_data_next(&it))
		if (PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, o))
			fprintf(wctx->f, "**** LAB\r\n");
}

static int pads_write_blk_part(write_ctx_t *wctx)
{
	int res = 0;

	fprintf(wctx->f, "*PART*       ITEMS\r\n\r\n");

	fprintf(wctx->f, "*REMARK* REFNM PTYPENM X Y ORI GLUE MIRROR ALT CLSTID CLSTATTR BROTHERID LABELS\r\n");
	fprintf(wctx->f, "*REMARK* .REUSE. INSTANCE RPART\r\n");
	fprintf(wctx->f, "*REMARK* VISIBLE XLOC YLOC ORI LEVEL HEIGTH WIDTH MIRRORED HJUST VJUST RIGHTREADING\r\n");
	fprintf(wctx->f, "*REMARK* FONTSTYLE FONTFACE\r\n");

	PCB_SUBC_LOOP(wctx->pcb->Data) {
		res |= pads_write_part(wctx, subc);
	}
	PCB_END_LOOP;

	fprintf(wctx->f, "\r\n");
	return res;
}

static int pads_write_pcb_(write_ctx_t *wctx)
{
	if (pads_write_blk_pcb(wctx) != 0) return -1;
	if (pads_write_blk_reuse(wctx) != 0) return -1;
	if (pads_write_blk_text(wctx) != 0) return -1;
	if (pads_write_blk_lines(wctx) != 0) return -1;
	if (pads_write_blk_vias(wctx) != 0) return -1;
	if (pads_write_blk_partdecals(wctx) != 0) return -1;
	if (pads_write_blk_parttype(wctx) != 0) return -1;
	if (pads_write_blk_part(wctx) != 0) return -1;

	if (pads_write_blk_misc_layers(wctx) != 0) return -1;
	return 0;
}

static int io_pads_write_pcb(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency, double ver)
{
	write_ctx_t wctx = {0};
	int res;

	wctx.f = f;
	wctx.pcb = PCB;
	wctx.ver = ver;

	rnd_fprintf(f, "!PADS-POWERPCB-V%.1f-METRIC! DESIGN DATABASE ASCII FILE 1.0\r\n", ver);

	/* We need to use mm because PARTDECAL can't use BASIC */
	rnd_printf_slot[4] = "%04mm";


	pads_map_layers(&wctx);
	pcb_placement_init(&wctx.footprints, wctx.pcb);
	pcb_placement_build(&wctx.footprints, wctx.pcb->Data);

	res = pads_write_pcb_(&wctx);

	pcb_placement_uninit(&wctx.footprints);
	pads_free_layers(&wctx);

	rnd_fprintf(f, "\r\n*END*     OF ASCII OUTPUT FILE\r\n");

	return res;
}

int io_pads_write_pcb_2005(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency)
{
	return io_pads_write_pcb(ctx, f, old_filename, new_filename, emergency, 2005);
}

int io_pads_write_pcb_9_4(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency)
{
	return io_pads_write_pcb(ctx, f, old_filename, new_filename, emergency, 9.4);
}

