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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* global source constants */

#ifndef	PCB_CONST_H
#define	PCB_CONST_H

/* ---------------------------------------------------------------------------
 * object types (bitfield)
 */
typedef enum {
	PCB_TYPE_NONE          = 0x00000, /* special: no object */


	PCB_TYPE_ARC           = 0x04000,
	PCB_TYPE_ARC_POINT     = 0x80000,
	PCB_TYPE_LINE          = 0x00004,
	PCB_TYPE_LINE_POINT    = 0x01000,
	PCB_TYPE_POLY          = 0x00008,
	PCB_TYPE_POLY_POINT    = 0x00800,
	PCB_TYPE_TEXT          = 0x00010,
	PCB_TYPE_RATLINE       = 0x00020,
	PCB_TYPE_SUBC          = 0x00040, /* TODO: should be 0x00002 once PCB_TYPE_ELEMENT is removed */
	PCB_TYPE_PSTK          = 0x100000,



	PCB_TYPE_LOCKED        = 0x10000, /* used to tell search to include locked items. */
	PCB_TYPE_SUBC_PART     = 0x20000, /* used to tell search to include objects that are part of a subcircuit */
	PCB_TYPE_NET           = 0x40000, /* used to select whole net. */

	PCB_TYPE_SUBC_FLOATER  = 0x200000, /* prefer subc floaters in search by location; TODO: should be 2 slots up */

	/* groups/properties */
	PCB_TYPEMASK_PIN       = (PCB_TYPE_PSTK | PCB_TYPE_SUBC_PART),
	PCB_TYPEMASK_TERM      = (PCB_TYPEMASK_PIN | PCB_TYPE_SUBC_PART | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLY | PCB_TYPE_TEXT),
	PCB_TYPEMASK_LOCK      = (PCB_TYPE_PSTK | PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLY | PCB_TYPE_SUBC | PCB_TYPE_TEXT | PCB_TYPE_LOCKED),

	PCB_TYPEMASK_ALL       = (~0)   /* all bits set */
} pcb_obj_type_t;

#endif
