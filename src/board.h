/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* definition of types */
#ifndef PCB_BOARD_H
#define PCB_BOARD_H

#include "config.h"

#include "const.h"
#include "macro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include "global_typedefs.h"
#include "global_objs.h"
#include "layer.h"
#include "library.h"
#include "attrib.h"
#include "rats_patch.h"
#include "list_common.h"
#include "list_line.h"
#include "list_arc.h"
#include "list_text.h"
#include "list_poly.h"
#include "list_pad.h"
#include "list_pin.h"
#include "list_rat.h"
#include "vtonpoint.h"
#include "hid.h"
#include "polyarea.h"
#include "vtroutestyle.h"

	/* The PCBType struct holds information about board layout most of which is
	   |  saved with the layout.  A new PCB layout struct is first initialized
	   |  with values from the user configurable Settings struct and then reset
	   |  to the saved layout values when a layout is loaded.
	   |  This struct is also used for the remove list and for buffer handling
	 */
struct pcb_board_s {
	long ID;											/* see macro.h */
	char *Name,										/* name of board */
	 *Filename,										/* name of file (from load) */
	 *PrintFilename,							/* from print dialog */
	 *Netlistname,								/* name of netlist file */
	  ThermStyle;									/* type of thermal to place with thermal tool */
	pcb_bool Changed,									/* layout has been changed */
	  ViaOn,											/* visibility flags */
	  ElementOn, RatOn, InvisibleObjectsOn, PinOn, SilkActive,	/* active layer is actually silk */
	  RatDraw;										/* we're drawing rats */
	char *ViaColor,								/* some colors */
	 *ViaSelectedColor,
		*PinColor,
		*PinSelectedColor,
		*PinNameColor,
		*ElementColor,
		*ElementColor_nonetlist,
		*RatColor,
		*InvisibleObjectsColor,
		*InvisibleMarkColor, *ElementSelectedColor, *RatSelectedColor, *ConnectedColor, *WarnColor, *MaskColor;
	Coord CursorX,									/* cursor position as saved with layout */
	  CursorY;
	Coord Bloat,									/* drc sizes saved with layout */
	  Shrink, minWid, minSlk, minDrill, minRing;
	Coord GridOffsetX,						/* as saved with layout */
	  GridOffsetY, MaxWidth,			/* allowed size */
	  MaxHeight;

	Coord Grid;										/* used grid with offsets */
	double Zoom,									/* zoom factor */
	  IsleArea,										/* minimum poly island to retain */
	  ThermScale;									/* scale factor used with thermals */
	FontType Font;
	LayerGroupType LayerGroups;
	vtroutestyle_t RouteStyle;
	LibraryType NetlistLib[NUM_NETLISTS];
	rats_patch_line_t *NetlistPatches, *NetlistPatchLast;
	AttributeListType Attributes;
	DataTypePtr Data;							/* entire database */

	pcb_bool is_footprint;						/* If set, the user has loaded a footprint, not a pcb. */

/* netlist states */
	int netlist_frozen;                /* counter */
	unsigned netlist_needs_update:1;
};

void FreePCBMemory(PCBTypePtr);

#endif
