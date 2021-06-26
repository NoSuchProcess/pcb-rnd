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

#include "board.h"
#include "conf_core.h"
#include "data.h"

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	double ver;

	/* layer mapping */
	vti0_t gid2plid; /* group ID to pads layer ID */
	vtp0_t plid2grp; /* pads layer ID to pcb-rnd group */
} write_ctx_t;

#define CRD(c)   ((long)rnd_round((c) * 3 / 2))
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

	fprintf(wctx->f, "*PCB*        GENERAL PARAMETERS OF THE PCB DESIGN\r\n\r\n");
	fprintf(wctx->f, "UNITS             %d              2=Inches 1=Metric 0=Mils\r\n", gridu);
	fprintf(wctx->f, "USERGRID     % 6ld  % 6ld      Space between USER grid points\r\n", CRD(rnd_conf.editor.grid), CRD(rnd_conf.editor.grid));
	fprintf(wctx->f, "MAXIMUMLAYER     % 2d              Maximum routing layer\r\n", coplyn);
	fprintf(wctx->f, "WORKLEVEL         0              Level items will be created on\r\n");
	fprintf(wctx->f, "DISPLAYLEVEL      1              toggle for displaying working level last\r\n");
	fprintf(wctx->f, "LAYERPAIR         1       2      Layer pair used to route connection\r\n");
	fprintf(wctx->f, "VIAMODE           T              Type of via to use when routing between layers\r\n");
	fprintf(wctx->f, "LINEWIDTH    % 6ld              Width items will be created with\r\n", CRD(conf_core.design.line_thickness));
	fprintf(wctx->f, "TEXTSIZE     % 6ld  % 6ld      Height and LineWidth text will be created with\r\n", CRD(conf_core.design.text_scale), CRD(conf_core.design.text_thickness));
	fprintf(wctx->f, "JOBTIME           0              Amount of time spent on this PCB design\r\n");
	fprintf(wctx->f, "DOTGRID      % 6ld  % 6ld      Space between graphic dots\r\n", CRD(rnd_conf.editor.grid), CRD(rnd_conf.editor.grid));
	fprintf(wctx->f, "SCALE        10.000              Scale of window expansion\r\n");
	fprintf(wctx->f, "ORIGIN       % 6ld  % 6ld      User defined origin location\r\n", CRD(wctx->pcb->hidlib.grid_ox), CRD(wctx->pcb->hidlib.grid_oy));
	fprintf(wctx->f, "WINDOWCENTER % 6ld % 6ld     Point defining the center of the window\r\n", CRD(wctx->pcb->hidlib.size_x/2.0), CRD(wctx->pcb->hidlib.size_y/2.0));
	fprintf(wctx->f, "BACKUPTIME       20              Number of minutes between database backups\r\n");
	fprintf(wctx->f, "REAL WIDTH        2              Widths greater then this are displayed real size\r\n");
	fprintf(wctx->f, "ALLSIGONOFF       1              All signal nets displayed on/off\r\n");
	fprintf(wctx->f, "REFNAMESIZE  % 6ld  % 6ld      Height and LineWidth used by part ref. names\r\n", CRD(conf_core.design.text_scale), CRD(conf_core.design.text_thickness));
	fprintf(wctx->f, "HIGHLIGHT         0              Highlight nets flag\r\n");

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

	fprintf(wctx->f, "\r\n");
	return 0;
}

static int pads_write_blk_reuse(write_ctx_t *wctx)
{
	fprintf(wctx->f, "*REUSE*\r\n");
	fprintf(wctx->f, "\r\n");
	fprintf(wctx->f, "*REMARK* TYPE TYPENAME\r\n");
	fprintf(wctx->f, "*REMARK* TIMESTAMP SECONDS\r\n");
	fprintf(wctx->f, "*REMARK* PART NAMING PARTNAMING\r\n");
	fprintf(wctx->f, "*REMARK* PART NAME\r\n");
	fprintf(wctx->f, "*REMARK* NET NAMING NETNAMING\r\n");
	fprintf(wctx->f, "*REMARK* NET MERGE NAME\r\n");
	fprintf(wctx->f, "*REMARK* REUSE INSTANCENM PARTNAMING NETNAMING X Y ORI GLUED\r\n");
	fprintf(wctx->f, "\r\n");
	return 0;
}

static int pads_write_blk_text(write_ctx_t *wctx)
{
	pcb_text_t *t;
	int li;
	pcb_layer_t *l;

	fprintf(wctx->f, "*TEXT*       FREE TEXT\r\n\r\n");
	fprintf(wctx->f, "*REMARK* XLOC YLOC ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST .REUSE. INSTANCENM\r\n");
	fprintf(wctx->f, "*REMARK* FONTSTYLE FONTFACE\r\n\r\n");

/**/
/*
       1234        5678  90.000 20          70          10 N   LEFT   DOWN
Regular <Romansim Stroke Font>
text string
*/

	for(li = 0, l = wctx->pcb->Data->Layer; li < wctx->pcb->Data->LayerN; li++,l++) {
		int plid = pads_layer2plid(wctx, l);
		if (plid <= 0)
			continue;
		for(t = textlist_first(&l->Text); t != NULL; t = textlist_next(t)) {
			rnd_coord_t hght = t->BoundingBox.Y2 - t->BoundingBox.Y1;
			char mir  = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, t) ? 'M' : 'N';
			char *alg = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, t) ? "RIGHT DOWN" : "LEFT UP";

			fprintf(wctx->f, "   % 6ld % 6ld %f %d % 6ld 10 %c %s\r\n",
				CRDX(t->X), CRDY(t->Y), ROT(t->rot), plid, CRD(hght), mir, alg);
			fprintf(wctx->f, "Regular <Romansim Stroke Font>\r\n");
			fprintf(wctx->f, "%s\r\n", t->TextString);
		}
	}

	fprintf(wctx->f, "\r\n");
	return 0;
}

static int pads_write_blk_lines(write_ctx_t *wctx)
{
	rnd_layer_id_t lid;
	pcb_layer_t *ly;

	fprintf(wctx->f, "*LINES*      LINES ITEMS\r\n\r\n");
	fprintf(wctx->f, "*REMARK* NAME TYPE XLOC YLOC PIECES TEXT SIGSTR\r\n");
	fprintf(wctx->f, "*REMARK* .REUSE. INSTANCE RSIGNAL\r\n");
	fprintf(wctx->f, "*REMARK* PIECETYPE CORNERS WIDTHHGHT LINESTYLE LEVEL [RESTRICTIONS]\r\n");
	fprintf(wctx->f, "*REMARK* XLOC YLOC BEGINANGLE DELTAANGLE\r\n");
	fprintf(wctx->f, "*REMARK* XLOC YLOC ORI LEVEL HEIGHT WIDTH MIRRORED HJUST VJUST\r\n\r\n");

	for(lid = 0, ly = wctx->pcb->Data->Layer; lid < wctx->pcb->Data->LayerN; lid++,ly++) {
		pcb_line_t *l;
		pcb_arc_t *a;
		pcb_poly_t *p;
		int plid = pads_layer2plid(wctx, ly);

		if (plid <= 0)
			continue;

		l = linelist_first(&ly->Line);
		if (l != NULL) {
			fprintf(wctx->f, "lines_lid_%ld    LINES    0      0      %ld\r\n", (long)lid, (long)linelist_length(&ly->Line));
			for(; l != NULL; l = linelist_next(l)) {
				fprintf(wctx->f, "OPEN 2   %ld %d\r\n", CRD(l->Thickness), plid);
				fprintf(wctx->f, "%ld   %ld\r\n", CRDX(l->Point1.X), CRDY(l->Point1.Y));
				fprintf(wctx->f, "%ld   %ld\r\n", CRDX(l->Point2.X), CRDY(l->Point2.Y));
			}
		}
		a = arclist_first(&ly->Arc);
		if (a != NULL) {
			fprintf(wctx->f, "arcs_lid_%ld    LINES    0      0      %ld\r\n", (long)lid, (long)arclist_length(&ly->Arc));
			for(; a != NULL; a = arclist_next(a)) {
				double sa, da;
				rnd_coord_t x1, y1, x2, y2;

				pcb_arc_get_end(a, 0, &x1, &y1);
				pcb_arc_get_end(a, 1, &x2, &y2);
				sa = (a->StartAngle) - 180;
				da = (a->Delta);

				fprintf(wctx->f, "OPEN 2   %ld %d\r\n", CRD(a->Thickness), plid);
				fprintf(wctx->f, "%ld %ld   %d %d    %ld %ld    %ld %ld\r\n",
					CRDX(x1), CRDY(y1), (int)rnd_round(sa*10), (int)rnd_round(da*10),
					CRDX(a->X - a->Width), CRDY(a->Y - a->Height),
					CRDX(a->X + a->Width), CRDY(a->Y + a->Height));

				fprintf(wctx->f, "%ld %ld\r\n", CRDX(x2), CRDY(y2));
			}
		}
		p = polylist_first(&ly->Polygon);
		if (p != NULL) {
			fprintf(wctx->f, "polys_lid_%ld    COPPER   0      0      %ld\r\n", (long)lid, (long)polylist_length(&ly->Polygon));
			for(; p != NULL; p = polylist_next(p)) {
				long n;

				fprintf(wctx->f, "COPCLS %ld   0 %d\r\n", (long)p->PointN+1, plid);
				for(n = 0; n < p->PointN; n++)
					fprintf(wctx->f, "%ld %ld\r\n", CRDX(p->Points[n].X), CRDY(p->Points[n].Y));
				fprintf(wctx->f, "%ld %ld\r\n", CRDX(p->Points[0].X), CRDY(p->Points[0].Y));
			}
		}
	}

	fprintf(wctx->f, "\r\n");
	return 0;
}


static int pads_write_pcb_(write_ctx_t *wctx)
{
	if (pads_write_blk_pcb(wctx) != 0) return -1;
	if (pads_write_blk_reuse(wctx) != 0) return -1;
	if (pads_write_blk_text(wctx) != 0) return -1;
	if (pads_write_blk_lines(wctx) != 0) return -1;

	if (pads_write_blk_misc_layers(wctx) != 0) return -1;
	return -1;
}

static int io_pads_write_pcb(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency, double ver)
{
	write_ctx_t wctx = {0};
	int res;

	wctx.f = f;
	wctx.pcb = PCB;
	wctx.ver = ver;

	fprintf(f, "!PADS-POWERPCB-V%.1f-BASIC! DESIGN DATABASE ASCII FILE 1.0\r\n", ver);

	pads_map_layers(&wctx);
	res = pads_write_pcb_(&wctx);
	pads_free_layers(&wctx);

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

