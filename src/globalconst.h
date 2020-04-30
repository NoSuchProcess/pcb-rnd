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

#include <librnd/core/globalconst.h>

#define PCB_MAX_LAYER        38    /* max number of layer, check source code for more changes, a *lot* more changes */
/* new array size that allows substrate layers */
#define PCB_MAX_LAYERGRP     ((PCB_MAX_LAYER+8)*2)    /* max number of layer groups, a.k.a. physical layers: a few extra outer layers per side, pluse one substrate per real layer */
#define PCB_MIN_THICKNESS    RND_MIN_SIZE
#define PCB_MAX_THICKNESS    RND_MAX_COORD
#define PCB_MIN_ARCRADIUS    RND_MIN_SIZE
#define PCB_MAX_ARCRADIUS    RND_MAX_COORD
#define PCB_MIN_TEXTSCALE    1 /* scaling of text objects in percent (must be an integer greater than 0) */
#define PCB_MAX_TEXTSCALE    10000
#define PCB_MIN_PINORVIASIZE PCB_MIL_TO_COORD(20)	/* size of a pin or via */
#define PCB_MIN_PINORVIAHOLE PCB_MIL_TO_COORD(4)	/* size of a pins or vias drilling hole */
#define PCB_MAX_PINORVIASIZE ((rnd_coord_t)RND_LARGE_VALUE)
#define PCB_MIN_PINORVIACOPPER PCB_MIL_TO_COORD(4)	/* min difference outer-inner diameter */
#define PCB_MIN_GRID         1
#define PCB_MAX_FONTPOSITION 255 /* upper limit of characters in my font */

#define PCB_MAX_BUFFER       5 /* number of pastebuffers additional changes in menu.c are also required to select more buffers */

#define PCB_EMARK_SIZE                  PCB_MIL_TO_COORD(10) 	/* size of diamond element mark */
#define PCB_SUBC_AUX_UNIT               PCB_MM_TO_COORD(1) /* size of the unit vector for the subc-aux layer */

/**** Font ***/
/* These are used in debug draw font rendering (e.g. pin names) and reverse
   scale calculations (e.g. when report is trying to figure how the font
   is scaled. Changing these values is not really supported. */
#define  PCB_FONT_CAPHEIGHT    PCB_MIL_TO_COORD (45)   /* (Approximate) capheight size of the default PCB font */
#define  PCB_DEFAULT_CELLSIZE  50                  /* default cell size for symbols */

#endif
