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

#include "board.h"
#include <librnd/core/unit.h>
#include <librnd/core/hidlib_conf.h>

typedef struct {
	FILE *f;
	double ver;
} write_ctx_t;

static int pads_write_pcb_block(write_ctx_t *wctx)
{
	int gridu = 1; /* default to metric - it's 2021 after all... */
	static const rnd_unit_t *unit_mm = NULL, *unit_mil;

	if (unit_mm == NULL) { /* cache mm and mil units to save on the lookups */
		unit_mm  = rnd_get_unit_struct("mm");
		unit_mil = rnd_get_unit_struct("mil");
	}

	if (rnd_conf.editor.grid_unit == unit_mm)        gridu = 1;
	else if (rnd_conf.editor.grid_unit == unit_mil)  gridu = 0;
	/* we don't ever set 2 for Inches */


	fprintf(wctx->f, "*PCB*        GENERAL PARAMETERS OF THE PCB DESIGN\r\n\r\n");
	fprintf(wctx->f, "UNITS        %d              2=Inches 1=Metric 0=Mils\r\n", gridu);
#if 0
USERGRID     1      1       Space between USER grid points
MAXIMUMLAYER 8              Maximum routing layer 
WORKLEVEL    0              Level items will be created on
DISPLAYLEVEL 1              toggle for displaying working level last
LAYERPAIR    1      2       Layer pair used to route connection
VIAMODE      T              Type of via to use when routing between layers
LINEWIDTH    12             Width items will be created with
TEXTSIZE     100    10      Height and LineWidth text will be created with
JOBTIME      58051          Amount of time spent on this PCB design
DOTGRID      100    100     Space between graphic dots
SCALE        19.506          Scale of window expansion
ORIGIN       15800  10000   User defined origin location
WINDOWCENTER 17983.65 11239.26  Point defining the center of the window
BACKUPTIME   20             Number of minutes between database backups
REAL WIDTH   2              Widths greater then this are displayed real size
ALLSIGONOFF  1              All signal nets displayed on/off
REFNAMESIZE  100    10      Height and LineWidth used by part ref. names
HIGHLIGHT    0              Highlight nets flag
JOBNAME      Main_tss_VRN.pcb
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

