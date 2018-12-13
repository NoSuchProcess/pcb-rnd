/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

#ifndef PCB_GLOBALCONST_H
#define PCB_GLOBALCONST_H

#include "config.h"

#define PCB_LARGE_VALUE      (COORD_MAX / 2 - 1) /* maximum extent of board and elements */

#define PCB_MAX_LAYER        38    /* max number of layer, check source code for more changes, a *lot* more changes */
/* new array size that allows substrate layers */
#define PCB_MAX_LAYERGRP     ((PCB_MAX_LAYER+8)*2)    /* max number of layer groups, a.k.a. physical layers: a few extra outer layers per side, pluse one substrate per real layer */
#define PCB_MIN_LINESIZE     PCB_MIL_TO_COORD(0.01)	/* thickness of lines */
#define PCB_MAX_LINESIZE     ((pcb_coord_t)PCB_LARGE_VALUE)
#define PCB_MIN_ARCSIZE      PCB_MIL_TO_COORD(0.01)
#define PCB_MAX_ARCSIZE      ((pcb_coord_t)PCB_LARGE_VALUE)
#define PCB_MIN_TEXTSCALE    10 /* scaling of text objects in percent */
#define PCB_MAX_TEXTSCALE    10000
#define PCB_MIN_PINORVIASIZE PCB_MIL_TO_COORD(20)	/* size of a pin or via */
#define PCB_MIN_PINORVIAHOLE PCB_MIL_TO_COORD(4)	/* size of a pins or vias drilling hole */
#define PCB_MAX_PINORVIASIZE ((pcb_coord_t)PCB_LARGE_VALUE)
#define PCB_MIN_PINORVIACOPPER PCB_MIL_TO_COORD(4)	/* min difference outer-inner diameter */
#define PCB_MIN_GRID         1
#define PCB_MAX_GRID         PCB_MIL_TO_COORD(1000)
#define PCB_MAX_FONTPOSITION 255 /* upper limit of characters in my font */

#define PCB_MAX_COORD        ((pcb_coord_t)PCB_LARGE_VALUE) /* coordinate limits */
#define PCB_MIN_SIZE         PCB_MIL_TO_COORD(10) /* lowest width and height of the board */
#define PCB_MAX_BUFFER       5 /* number of pastebuffers additional changes in menu.c are also required to select more buffers */

#define PCB_DEFAULT_DRILLINGHOLE 40 /* default inner/outer ratio for pins/vias in percent */

#define PCB_MAX_SIZE ((pcb_coord_t)PCB_LARGE_VALUE)

#ifndef PCB_PATH_MAX   /* maximum path length */
#ifdef PATH_MAX
#define PCB_PATH_MAX PATH_MAX
#else
#define PCB_PATH_MAX 2048
#endif
#endif

/* number of dynamic flag bits that can be allocated at once; should be n*64 for
   memory efficiency */
#define PCB_DYNFLAG_BLEN 64

#define PCB_MAX_LINE_POINT_DISTANCE     0   /* maximum distance when searching line points; same for arc point */
#define PCB_MAX_POLYGON_POINT_DISTANCE  0   /* maximum distance when searching polygon points */
#define PCB_MAX_NETLIST_LINE_LENGTH     255 /* maximum line length for netlist files */
#define PCB_MAX_MODESTACK_DEPTH         16  /* maximum depth of mode stack */
#define PCB_MIN_GRID_DISTANCE           4   /* minimum distance between point to enable grid drawing */
#define PCB_EMARK_SIZE                  PCB_MIL_TO_COORD(10) 	/* size of diamond element mark */


/**** Font ***/
/* These are used in debug draw font rendering (e.g. pin names) and reverse
   scale calculations (e.g. when report is trying to figure how the font
   is scaled. Changing these values is not really supported. */
#define  PCB_FONT_CAPHEIGHT    PCB_MIL_TO_COORD (45)   /* (Approximate) capheight size of the default PCB font */
#define  PCB_DEFAULT_CELLSIZE  50                  /* default cell size for symbols */

#endif
