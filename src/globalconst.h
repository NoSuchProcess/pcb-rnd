/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* global constants
 * most of these values are also required by files outside the source tree
 * (manuals...)
 */

#ifndef	__GLOBALCONST_INCLUDED__
#define	__GLOBALCONST_INCLUDED__

#include "config.h"


/* frame between the groundplane and the copper or mask - noone seems
   to remember what these two are for; changing them may have unforeseen
   side effects. */
#define	GROUNDPLANEFRAME        PCB_MIL_TO_COORD(15)
#define MASKFRAME               PCB_MIL_TO_COORD(3)

/* ---------------------------------------------------------------------------
 * some limit specifications
 */
#define LARGE_VALUE		(COORD_MAX / 2 - 1) /* maximum extent of board and elements */
 
#define	MAX_LAYER		16	/* max number of layer, check source */
					/* code for more changes, a *lot* more changes */
#define	MIN_LINESIZE		PCB_MIL_TO_COORD(0.01)	/* thickness of lines */
#define	MAX_LINESIZE		LARGE_VALUE
#define MIN_ARCSIZE		PCB_MIL_TO_COORD(0.01)
#define MAX_ARCSIZE		LARGE_VALUE
#define	MIN_TEXTSCALE		10	/* scaling of text objects in percent */
#define	MAX_TEXTSCALE		10000
#define	MIN_PINORVIASIZE	PCB_MIL_TO_COORD(20)	/* size of a pin or via */
#define	MIN_PINORVIAHOLE	PCB_MIL_TO_COORD(4)	/* size of a pins or vias drilling hole */
#define	MAX_PINORVIASIZE	LARGE_VALUE
#define	MIN_PINORVIACOPPER	PCB_MIL_TO_COORD(4)	/* min difference outer-inner diameter */
#define	MIN_PADSIZE		PCB_MIL_TO_COORD(1)	/* min size of a pad */
#define	MAX_PADSIZE		LARGE_VALUE   /* max size of a pad */
#define	MIN_DRC_VALUE		PCB_MIL_TO_COORD(0.1)
#define	MAX_DRC_VALUE		PCB_MIL_TO_COORD(500)
#define	MIN_DRC_SILK		PCB_MIL_TO_COORD(1)
#define	MAX_DRC_SILK		PCB_MIL_TO_COORD(30)
#define	MIN_DRC_DRILL		PCB_MIL_TO_COORD(1)
#define	MAX_DRC_DRILL		PCB_MIL_TO_COORD(50)
#define	MIN_DRC_RING		0
#define	MAX_DRC_RING		PCB_MIL_TO_COORD(100)
#define	MIN_GRID		1
#define	MAX_GRID		PCB_MIL_TO_COORD(1000)
#define	MAX_FONTPOSITION	255	/* upper limit of characters in my font */

#define	MAX_COORD		LARGE_VALUE	/* coordinate limits */
#define	MIN_SIZE		PCB_MIL_TO_COORD(10)	/* lowest width and height of the board */
#define	MAX_BUFFER		5	/* number of pastebuffers */
					/* additional changes in menu.c are */
					/* also required to select more buffers */

#define	DEFAULT_DRILLINGHOLE	40	/* default inner/outer ratio for */
					/* pins/vias in percent */

#if MAX_LINESIZE > MAX_PINORVIASIZE	/* maximum size value */
#define	MAX_SIZE	MAX_LINESIZE
#else
#define	MAX_SIZE	MAX_PINORVIASIZE
#endif

#ifndef	MAXPATHLEN			/* maximum path length */
#ifdef	PATH_MAX
#define	MAXPATHLEN	PATH_MAX
#else
#define	MAXPATHLEN	2048
#endif
#endif

#define	MAX_LINE_POINT_DISTANCE		0	/* maximum distance when searching */
						/* line points */
#define	MAX_POLYGON_POINT_DISTANCE	0	/* maximum distance when searching */
						/* polygon points */
#define	MAX_ELEMENTNAMES		3	/* number of supported names of */
						/* an element */
#define MAX_NETLIST_LINE_LENGTH		255	/* maximum line length for netlist files */
#define	MAX_MODESTACK_DEPTH		16	/* maximum depth of mode stack */
#define	MIN_GRID_DISTANCE		4	/* minimum distance between point */
						/* to enable grid drawing */
	/* size of diamond element mark */
#define EMARK_SIZE	PCB_MIL_TO_COORD (10)


/**** Font ***/
/* These are used in debug draw font rendering (e.g. pin names) and reverse
   scale calculations (e.g. when report is trying to figure how the font
   is scaled. Changing these values is not really supported. */
#define  FONT_CAPHEIGHT    PCB_MIL_TO_COORD (45)   /* (Approximate) capheight size of the default PCB font */
#define  DEFAULT_CELLSIZE  50                  /* default cell size for symbols */

#endif
