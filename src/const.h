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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* global source constants */

#ifndef	PCB_CONST_H
#define	PCB_CONST_H

/* ---------------------------------------------------------------------------
 * modes
 */
typedef enum {
	PCB_MODE_NO              = -1,  /* no mode selected - panning - TODO: remove this in favor of the default mode (it's the same as the arrow mode) */
	PCB_MODE_VIA             = 15,  /* draw vias */
	PCB_MODE_LINE            = 5,   /* draw lines */
	PCB_MODE_RECTANGLE       = 10,  /* create rectangles */
	PCB_MODE_POLYGON         = 8,   /* draw filled polygons */
	PCB_MODE_PASTE_BUFFER    = 2,   /* paste objects from buffer */
	PCB_MODE_TEXT            = 13,  /* create text objects */
	PCB_MODE_ROTATE          = 12,  /* rotate objects */
	PCB_MODE_REMOVE          = 11,  /* remove objects */
	PCB_MODE_MOVE            = 7,   /* move objects */
	PCB_MODE_COPY            = 3,   /* copy objects */
	PCB_MODE_INSERT_POINT    = 4,   /* insert point into line/polygon */
	PCB_MODE_RUBBERBAND_MOVE = 16,  /* move objects and attached lines */
	PCB_MODE_THERMAL         = 14,  /* toggle thermal layer flag */
	PCB_MODE_ARC             = 0,   /* draw arcs */
	PCB_MODE_ARROW           = 1,   /* selection with arrow mode */
	PCB_MODE_LOCK            = 6,   /* lock/unlock objects */
	PCB_MODE_POLYGON_HOLE    = 9    /* cut holes in filled polygons */
} pcb_mode_t;

/* ---------------------------------------------------------------------------
 * object types (bitfield)
 */
typedef enum {
	PCB_TYPE_NONE          = 0x00000, /* special: no object */
	PCB_TYPE_VIA           = 0x00001,
	PCB_TYPE_ELEMENT       = 0x00002,
	PCB_TYPE_LINE          = 0x00004,
	PCB_TYPE_POLY          = 0x00008,
	PCB_TYPE_TEXT          = 0x00010,
	PCB_TYPE_RATLINE       = 0x00020,
	PCB_TYPE_SUBC          = 0x00040, /* TODO: should be 0x00002 once PCB_TYPE_ELEMENT is removed */

	PCB_TYPE_PIN           = 0x00100, /* objects that are part */
	PCB_TYPE_PAD           = 0x00200, /* 'pin' of SMD element */
	PCB_TYPE_ELEMENT_NAME  = 0x00400, /* of others */
	PCB_TYPE_POLY_POINT    = 0x00800,
	PCB_TYPE_LINE_POINT    = 0x01000,
	PCB_TYPE_ELEMENT_LINE  = 0x02000,
	PCB_TYPE_ARC           = 0x04000,
	PCB_TYPE_ELEMENT_ARC   = 0x08000,

	PCB_TYPE_LOCKED        = 0x10000, /* used to tell search to include locked items. */
	PCB_TYPE_SUBC_PART     = 0x20000, /* used to tell search to include objects that are part of a subcircuit */
	PCB_TYPE_NET           = 0x40000, /* used to select whole net. */

	PCB_TYPE_ARC_POINT     = 0x80000,
	PCB_TYPE_PSTK          = 0x100000,

	/* groups/properties */
	PCB_TYPEMASK_PIN       = (PCB_TYPE_VIA | PCB_TYPE_PIN | PCB_TYPE_PSTK | PCB_TYPE_SUBC_PART),
	PCB_TYPEMASK_TERM      = (PCB_TYPEMASK_PIN | PCB_TYPE_SUBC_PART | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLY | PCB_TYPE_TEXT),
	PCB_TYPEMASK_LOCK      = (PCB_TYPE_PSTK | PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLY | PCB_TYPE_ELEMENT | PCB_TYPE_SUBC | PCB_TYPE_TEXT | PCB_TYPE_ELEMENT_NAME | PCB_TYPE_LOCKED),

	PCB_TYPEMASK_ALL       = (~0)   /* all bits set */
} pcb_obj_type_t;

#endif
