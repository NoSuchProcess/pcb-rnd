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

/* prototypes for undo routines */

#ifndef	PCB_UNDO_H
#define	PCB_UNDO_H

#include "library.h"

#define DRAW_FLAGS	(PCB_FLAG_RAT | PCB_FLAG_SELECTED \
			| PCB_FLAG_HIDENAME | PCB_FLAG_HOLE | PCB_FLAG_OCTAGON | PCB_FLAG_FOUND | PCB_FLAG_CLEARLINE)

/* different layers */

int pcb_undo(pcb_bool);
int pcb_redo(pcb_bool);
void pcb_undo_inc_serial(void);
void pcb_undo_save_serial(void);
void pcb_undo_restore_serial(void);
void pcb_undo_clear_list(pcb_bool);

void pcb_undo_move_obj_to_remove(int, void *, void *, void *);
void pcb_undo_add_obj_to_remove_point(int, void *, void *, pcb_cardinal_t);
void pcb_undo_add_obj_to_insert_point(int, void *, void *, void *);
void pcb_undo_add_obj_to_remove_contour(int, pcb_layer_t *, pcb_polygon_t *);
void pcb_undo_add_obj_to_insert_contour(int, pcb_layer_t *, pcb_polygon_t *);
void pcb_undo_add_obj_to_move(int, void *, void *, void *, pcb_coord_t, pcb_coord_t);
void pcb_undo_add_obj_to_change_name(int, void *, void *, void *, char *);
void pcb_undo_add_obj_to_change_pinnum(int, void *, void *, void *, char *);
void pcb_undo_add_obj_to_rotate(int, void *, void *, void *, pcb_coord_t, pcb_coord_t, pcb_uint8_t);
void pcb_undo_add_obj_to_create(int, void *, void *, void *);
void pcb_undo_add_obj_to_mirror(int, void *, void *, void *, pcb_coord_t);
void pcb_undo_add_obj_to_move_to_layer(int, void *, void *, void *);
void pcb_undo_add_obj_to_flag(int, void *, void *, void *);
void pcb_undo_add_obj_to_size(int, void *, void *, void *);
void pcb_undo_add_obj_to_2nd_size(int, void *, void *, void *);
void pcb_undo_add_obj_to_clear_size(int, void *, void *, void *);
void pcb_undo_add_obj_to_mask_size(int, void *, void *, void *);
void pcb_undo_add_obj_to_change_angles(int, void *, void *, void *);
void pcb_undo_add_obj_to_change_radii(int, void *, void *, void *);
void pcb_undo_add_obj_to_clear_poly(int, void *, void *, void *, pcb_bool);
void pcb_undo_add_layer_change(int, int);
void pcb_undo_add_netlist_lib(pcb_lib_t *);

void pcb_undo_lock(void);
void pcb_undo_unlock(void);
pcb_bool pcb_undoing(void);

/* Publish actions - these may be useful for other actions */
int ActionUndo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
int ActionRedo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
int ActionAtomic(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

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
	PCB_UNDO_ROTATE            = 0x000080, /* rotations */
	PCB_UNDO_CREATE            = 0x000100, /* creation of objects */
	PCB_UNDO_MOVETOLAYER       = 0x000200, /* moving objects to */
	PCB_UNDO_FLAG              = 0x000400, /* toggling SELECTED flag */
	PCB_UNDO_CHANGESIZE        = 0x000800, /* change size of object */
	PCB_UNDO_CHANGE2NDSIZE     = 0x001000, /* change 2ndSize of object */
	PCB_UNDO_MIRROR            = 0x002000, /* change side of board */
	PCB_UNDO_CHANGECLEARSIZE   = 0x004000, /* change clearance size */
	PCB_UNDO_CHANGEMASKSIZE    = 0x008000, /* change mask size */
	PCB_UNDO_CHANGEANGLES      = 0x010000, /* change arc angles */
	PCB_UNDO_LAYERCHANGE       = 0x020000, /* layer new/delete/move */
	PCB_UNDO_CLEAR             = 0x040000, /* clear/restore to polygons */
	PCB_UNDO_NETLISTCHANGE     = 0x080000, /* netlist change */
	PCB_UNDO_CHANGEPINNUM      = 0x100000, /* change of pin number */
	PCB_UNDO_CHANGERADII       = 0x200000  /* change arc radii */
} pcb_undo_op_t;

#endif
