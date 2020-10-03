/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#ifndef PCB_CROSSHAIR_H
#define PCB_CROSSHAIR_H

#include "config.h"
#include <librnd/core/hid.h>
#include <librnd/core/hidlib.h>
#include "obj_line.h"
#include "obj_poly.h"
#include "route.h"

typedef struct {              /* currently marked block */
	rnd_point_t Point1, Point2; /* start- and end-position */
	long int State;
	rnd_bool otherway;
} pcb_attached_box_t;

typedef struct {              /* currently attached object */
	rnd_coord_t X, Y;           /* saved position when tool is move */
	rnd_coord_t tx, ty;         /* target position when tool is move */
	rnd_box_t BoundingBox;
	long int Type;              /* object type */
	long int State;
	void *Ptr1, *Ptr2, *Ptr3;   /* three pointers to data, see search.c */
	rnd_angle_t start_angle, delta_angle;
	rnd_coord_t radius;
} pcb_attached_object_t;

typedef struct {                         /* holds crosshair, cursor and crosshair-attachment information */
	rnd_hid_gc_t GC;                       /* GC for cursor drawing */
	rnd_hid_gc_t AttachGC;                 /* and for displaying buffer contents */
	rnd_coord_t ptr_x, ptr_y;              /* last seen raw mouse pointer x;y coords */
	rnd_coord_t X, Y;                      /* (snapped) crosshair position */
	pcb_attached_line_t AttachedLine;      /* data of new lines... */
	pcb_attached_box_t AttachedBox;
	pcb_poly_t AttachedPolygon;
	int AttachedPolygon_pts;               /* number of valid points ever seen for this poly */
	pcb_attached_object_t AttachedObject;  /* data of attached objects */
	pcb_route_t Route;                     /* Calculated line route in LINE or MOVE(LINE) mode */ 
	pcb_any_obj_t *extobj_edit;            /* refers to the editobject (of an extobj) being edited */

	/* list of object IDs that could have been dragged so that they can be cycled */
	long int *drags;
	int drags_len, drags_current;
	rnd_coord_t dragx, dragy;              /* the point where drag started */

	/* tool-specific temporary storage */
	struct {
		int active, last_active;
		rnd_point_t *point[2];
		rnd_coord_t dx[2], dy[2];
	} edit_poly_point_extra;

	/* cached tool IDs */
	int tool_arrow, tool_line, tool_move, tool_arc, tool_poly, tool_poly_hole;
} pcb_crosshair_t;

typedef struct {
	rnd_cardinal_t Buffer; /* buffer number */
	rnd_bool Moving;       /* true if clicked on an object of PCB_SELECT_TYPES */
	void *ptr1, *ptr2, *ptr3;
} pcb_crosshair_note_t;

extern pcb_crosshair_note_t pcb_crosshair_note;


/*** all possible states of an attached object ***/
#define PCB_CH_STATE_FIRST  0  /* initial state */
#define PCB_CH_STATE_SECOND 1
#define PCB_CH_STATE_THIRD  2

extern pcb_crosshair_t pcb_crosshair;
extern rnd_mark_t pcb_marked;  /* the point the user explicitly marked, or in some operations where the operation originally started */

void pcb_notify_mark_change(rnd_bool changes_complete);
void pcb_crosshair_move_relative(rnd_coord_t, rnd_coord_t);
rnd_bool pcb_crosshair_move_absolute(pcb_board_t *pcb, rnd_coord_t, rnd_coord_t);
void pcb_crosshair_init(void);
void pcb_crosshair_pre_init(void);
void pcb_crosshair_uninit(void);
void pcb_crosshair_grid_fit(pcb_board_t *pcb, rnd_coord_t, rnd_coord_t);
void pcb_center_display(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y);

void pcb_crosshair_set_local_ref(rnd_coord_t X, rnd_coord_t Y, rnd_bool Showing);

/*** utility for plugins ***/
void pcb_xordraw_attached_line(rnd_coord_t, rnd_coord_t, rnd_coord_t, rnd_coord_t, rnd_coord_t);
void pcb_xordraw_poly(pcb_poly_t *polygon, rnd_coord_t dx, rnd_coord_t dy, int dash_last);
void pcb_xordraw_poly_subc(pcb_poly_t *polygon, rnd_coord_t dx, rnd_coord_t dy, rnd_coord_t w, rnd_coord_t h, int mirr);
void pcb_xordraw_attached_arc(rnd_coord_t thick);
void pcb_xordraw_buffer(pcb_buffer_t *Buffer);
void pcb_xordraw_movecopy(rnd_bool modifier);
void pcb_xordraw_insert_pt_obj(void);

/* Always call this before changing the attached object of the crosshair */
void pcb_crosshair_attached_clean(rnd_hidlib_t *hidlib);

#endif
