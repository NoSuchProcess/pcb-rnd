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

/* global source constants */

#ifndef	PCB_CONST_H
#define	PCB_CONST_H

/* These need to be carefully written to avoid overflows, and return
   a Coord type.  */
#define PCB_SCALE_TEXT(COORD,TEXTSCALE) ((pcb_coord_t)((COORD) * ((double)(TEXTSCALE) / 100.0)))
#define PCB_UNPCB_SCALE_TEXT(COORD,TEXTSCALE) ((pcb_coord_t)((COORD) * (100.0 / (double)(TEXTSCALE))))

/* ---------------------------------------------------------------------------
 * modes
 */
typedef enum {
	PCB_MODE_NO              = 0,   /* no mode selected - panning */
	PCB_MODE_VIA             = 1,   /* draw vias */
	PCB_MODE_LINE            = 2,   /* draw lines */
	PCB_MODE_RECTANGLE       = 3,   /* create rectangles */
	PCB_MODE_POLYGON         = 4,   /* draw filled polygons */
	PCB_MODE_PASTE_BUFFER    = 5,   /* paste objects from buffer */
	PCB_MODE_TEXT            = 6,   /* create text objects */
	PCB_MODE_ROTATE          = 102, /* rotate objects */
	PCB_MODE_REMOVE          = 103, /* remove objects */
	PCB_MODE_MOVE            = 104, /* move objects */
	PCB_MODE_COPY            = 105, /* copy objects */
	PCB_MODE_INSERT_POINT    = 106, /* insert point into line/polygon */
	PCB_MODE_RUBBERBAND_MOVE = 107, /* move objects and attached lines */
	PCB_MODE_THERMAL         = 108, /* toggle thermal layer flag */
	PCB_MODE_ARC             = 109, /* draw arcs */
	PCB_MODE_ARROW           = 110, /* selection with arrow mode */
	PCB_MODE_LOCK            = 111, /* lock/unlock objects */
	PCB_MODE_POLYGON_HOLE    = 112  /* cut holes in filled polygons */
} pcb_mode_t;

/* ---------------------------------------------------------------------------
 * object types (bitfield)
 */
typedef enum {
	PCB_TYPE_NONE          = 0x00000, /* special: no object */
	PCB_TYPE_VIA           = 0x00001,
	PCB_TYPE_ELEMENT       = 0x00002,
	PCB_TYPE_LINE          = 0x00004,
	PCB_TYPE_POLYGON       = 0x00008,
	PCB_TYPE_TEXT          = 0x00010,
	PCB_TYPE_RATLINE       = 0x00020,

	PCB_TYPE_PIN           = 0x00100, /* objects that are part */
	PCB_TYPE_PAD           = 0x00200, /* 'pin' of SMD element */
	PCB_TYPE_ELEMENT_NAME  = 0x00400, /* of others */
	PCB_TYPE_POLYGON_POINT = 0x00800,
	PCB_TYPE_LINE_POINT    = 0x01000,
	PCB_TYPE_ELEMENT_LINE  = 0x02000,
	PCB_TYPE_ARC           = 0x04000,
	PCB_TYPE_ELEMENT_ARC   = 0x08000,

	PCB_TYPE_LOCKED        = 0x10000, /* used to tell search to include locked items. */
	PCB_TYPE_NET           = 0x20000, /* used to select whole net. */

	/* groups/properties */
	PCB_TYPEMASK_PIN       = (PCB_TYPE_VIA | PCB_TYPE_PIN),
	PCB_TYPEMASK_LOCK      = (PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLYGON | PCB_TYPE_ELEMENT | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_LOCKED),

	PCB_TYPEMASK_ALL       = (~0)   /* all bits set */
} pcb_obj_type_t;

#endif
