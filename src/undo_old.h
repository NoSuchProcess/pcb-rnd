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

/* old undo infra: should be replaced by per operation local undo */

#ifndef	PCB_UNDO_OLD_H
#define	PCB_UNDO_OLD_H

#include "global_typedefs.h"

#define DRAW_FLAGS	(PCB_FLAG_RAT | PCB_FLAG_SELECTED \
			| PCB_FLAG_HIDENAME | PCB_FLAG_HOLE | PCB_FLAG_OCTAGON | PCB_FLAG_FOUND | PCB_FLAG_CLEARLINE)

/* different layers */

void pcb_undo_move_obj_to_remove(int, void *, void *, void *);
void pcb_undo_add_obj_to_remove_point(int, void *, void *, pcb_cardinal_t);
void pcb_undo_add_obj_to_insert_point(int, void *, void *, void *);
void pcb_undo_add_obj_to_remove_contour(int, pcb_layer_t *, pcb_poly_t *);
void pcb_undo_add_obj_to_insert_contour(int, pcb_layer_t *, pcb_poly_t *);
void pcb_undo_add_obj_to_move(int, void *, void *, void *, pcb_coord_t, pcb_coord_t);
void pcb_undo_add_obj_to_change_name(int, void *, void *, void *, char *);
void pcb_undo_add_obj_to_change_pinnum(int, void *, void *, void *, char *);
void pcb_undo_add_obj_to_rotate90(int, void *, void *, void *, pcb_coord_t, pcb_coord_t, unsigned int);
void pcb_undo_add_obj_to_rotate(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t CenterX, pcb_coord_t CenterY, pcb_angle_t angle);
void pcb_undo_add_obj_to_create(int, void *, void *, void *);
void pcb_undo_add_subc_to_otherside(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t yoff);
void pcb_undo_add_obj_to_move_to_layer(int, void *, void *, void *);
void pcb_undo_add_obj_to_flag(void *obj);
void pcb_undo_add_obj_to_size(int, void *, void *, void *);
void pcb_undo_add_obj_to_2nd_size(int Type, void *ptr1, void *ptr2, void *ptr3);
void pcb_undo_add_obj_to_rot(int Type, void *ptr1, void *ptr2, void *ptr3);
void pcb_undo_add_obj_to_clear_size(int, void *, void *, void *);
void pcb_undo_add_obj_to_change_angles(int, void *, void *, void *);
void pcb_undo_add_obj_to_change_radii(int, void *, void *, void *);
void pcb_undo_add_obj_to_clear_poly(int, void *, void *, void *, pcb_bool);
void pcb_undo_add_layer_move(int, int, int);

/* ---------------------------------------------------------------------------
 * define supported types of undo operations
 * note these must be separate bits now
 */
typedef enum {
	PCB_UNDO_CHANGENAME        = 0x000001, /* change of names */
	PCB_UNDO_MOVE              = 0x000002, /* moving objects */
	PCB_UNDO_REMOVE            = 0x000004, /* removing objects */
	PCB_UNDO_REMOVE_POINT      = 0x000008, /* removing polygon/... points */
	PCB_UNDO_INSERT_POINT      = 0x000010, /* inserting polygon/... points */
	PCB_UNDO_REMOVE_CONTOUR    = 0x000020, /* removing a contour from a polygon */
	PCB_UNDO_INSERT_CONTOUR    = 0x000040, /* inserting a contour from a polygon */
	PCB_UNDO_ROTATE90          = 0x000080, /* rotations by 90 deg steps */
	PCB_UNDO_CREATE            = 0x000100, /* creation of objects */
	PCB_UNDO_MOVETOLAYER       = 0x000200, /* moving objects to */
	PCB_UNDO_FLAG              = 0x000400, /* toggling SELECTED flag */
	PCB_UNDO_CHANGESIZE        = 0x000800, /* change size of object */
	PCB_UNDO_CHANGECLEARSIZE   = 0x004000, /* change clearance size */
	PCB_UNDO_CHANGEANGLES      = 0x010000, /* change arc angles */
	PCB_UNDO_LAYERMOVE         = 0x020000, /* layer new/delete/move */
	PCB_UNDO_CLEAR             = 0x040000, /* clear/restore to polygons */
	PCB_UNDO_CHANGERADII       = 0x200000, /* change arc radii */
	PCB_UNDO_OTHERSIDE         = 0x400000, /* change side of board (subcircuit) */
	PCB_UNDO_ROTATE            = 0x800000, /* rotations at arbitrary angle */
	PCB_UNDO_CHANGEROT         = 0x1000000, /* change internal/self rotation of an object */
	PCB_UNDO_CHANGE2SIZE       = 0x2000000 /* change 2nd size of object */
} pcb_undo_op_t;

const char *undo_type2str(int type);

#endif
