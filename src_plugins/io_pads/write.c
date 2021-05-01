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

#include <librnd/core/unit.h>
#include <librnd/core/hidlib_conf.h>

#include "board.h"
#include "conf_core.h"

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	double ver;
} write_ctx_t;

#define CRD(c)   ((long)rnd_round(RND_COORD_TO_MM(c) * 10000))

static int pads_write_pcb_block(write_ctx_t *wctx)
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

return 0;
}

static int pads_write_pcb_(write_ctx_t *wctx)
{
	if (pads_write_pcb_block(wctx) != 0) return -1;

	return -1;
}

static int io_pads_write_pcb(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency, double ver)
{
	write_ctx_t wctx;

	wctx.f = f;
	wctx.pcb = PCB;
	wctx.ver = ver;

	fprintf(f, "!PADS-POWERPCB-V%.1f-METRIC! DESIGN DATABASE ASCII FILE 1.0\r\n", ver);

	return pads_write_pcb_(&wctx);
}

int io_pads_write_pcb_2005(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency)
{
	return io_pads_write_pcb(ctx, f, old_filename, new_filename, emergency, 2005);
}

int io_pads_write_pcb_9_4(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency)
{
	return io_pads_write_pcb(ctx, f, old_filename, new_filename, emergency, 9.4);
}

