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

#ifndef	PCB_CROSSHAIR_H
#define	PCB_CROSSHAIR_H

#include "config.h"
#include "vtonpoint.h"
#include "hid.h"
#include "obj_line.h"
#include "obj_poly.h"
#include "route.h"

typedef struct {								/* currently marked block */
	pcb_point_t Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool otherway;
} pcb_attached_box_t;

typedef struct {								/* currently attached object */
	pcb_coord_t X, Y;										/* saved position when PCB_MODE_MOVE */
	pcb_box_t BoundingBox;
	long int Type,								/* object type */
	  State;
	void *Ptr1,										/* three pointers to data, see */
	 *Ptr2,												/* search.c */
	 *Ptr3;
	pcb_angle_t start_angle, delta_angle;
	pcb_coord_t radius;
} pcb_attached_object_t;

typedef struct {
	pcb_bool status;
	pcb_coord_t X, Y;
} pcb_mark_t;

enum pcb_crosshair_shape_e {
	pcb_ch_shape_basic = 0,		/*  4-ray */
	pcb_ch_shape_union_jack,		/*  8-ray */
	pcb_ch_shape_dozen,				/* 12-ray */
	pcb_ch_shape_NUM
};

typedef struct {								/* holds cursor information */
	pcb_hid_gc_t GC,											/* GC for cursor drawing */
	  AttachGC;										/* and for displaying buffer contents */
	pcb_coord_t X, Y,										/* position in PCB coordinates */
	  MinX, MinY,									/* lowest and highest coordinates */
	  MaxX, MaxY;
	pcb_attached_line_t AttachedLine;	/* data of new lines... */
	pcb_attached_box_t AttachedBox;
	pcb_poly_t AttachedPolygon;
	int AttachedPolygon_pts;              /* number of valid points ever seen for this poly */
	pcb_attached_object_t AttachedObject;	/* data of attached objects */
	pcb_route_t Route;										/* Calculated line route in LINE or MOVE(LINE) mode */ 
	enum pcb_crosshair_shape_e shape;			/* shape of crosshair */
	vtop_t onpoint_objs;
	vtop_t old_onpoint_objs;
	pcb_pstk_t *snapped_pstk;


	/* list of object IDs that could have been dragged so that they can be cycled */
	long int *drags;
	int drags_len, drags_current;
	pcb_coord_t dragx, dragy;						/* the point where drag started */
} pcb_crosshair_t;


/* ---------------------------------------------------------------------------
 * all possible states of an attached object
 */
#define PCB_CH_STATE_FIRST  0  /* initial state */
#define PCB_CH_STATE_SECOND 1
#define PCB_CH_STATE_THIRD  2

extern pcb_crosshair_t pcb_crosshair;
extern pcb_mark_t pcb_marked;

void pcb_notify_crosshair_change(pcb_bool changes_complete);
void pcb_notify_mark_change(pcb_bool changes_complete);
void pcb_draw_attached(void);
void pcb_draw_mark(void);
void pcb_crosshair_move_relative(pcb_coord_t, pcb_coord_t);
pcb_bool pcb_crosshair_move_absolute(pcb_coord_t, pcb_coord_t);
void pcb_crosshair_set_range(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t);
void pcb_crosshair_init(void);
void pcb_crosshair_uninit(void);
void pcb_crosshair_grid_fit(pcb_coord_t, pcb_coord_t);
void pcb_center_display(pcb_coord_t X, pcb_coord_t Y);

/*** cursor/crosshair mode ***/

/* sets the crosshair range to the current buffer extents */
void pcb_crosshair_range_to_buffer(void);
void pcb_crosshair_save_mode(void);
void pcb_crosshair_restore_mode(void);
void pcb_crosshair_set_mode(int Mode);
void pcb_crosshair_set_local_ref(pcb_coord_t X, pcb_coord_t Y, pcb_bool Showing);


/*** utility for plugins ***/
void pcb_xordraw_attached_line(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t);
void pcb_xordraw_poly(pcb_poly_t *polygon, pcb_coord_t dx, pcb_coord_t dy, int dash_last);
void pcb_xordraw_poly_subc(pcb_poly_t *polygon, pcb_coord_t dx, pcb_coord_t dy, pcb_coord_t w, pcb_coord_t h, int mirr);
void pcb_xordraw_attached_arc(pcb_coord_t thick);
void pcb_xordraw_buffer(pcb_buffer_t *Buffer);
void pcb_xordraw_movecopy(void);
void pcb_xordraw_insert_pt_obj(void);



#endif
