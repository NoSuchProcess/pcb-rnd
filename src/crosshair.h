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

/* prototypes for crosshair routines */

#ifndef	PCB_CROSSHAIR_H
#define	PCB_CROSSHAIR_H

#include "config.h"
#include "rubberband.h"
#include "vtonpoint.h"
#include "hid.h"
#include "obj_line.h"
#include "obj_poly.h"

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
	pcb_cardinal_t RubberbandN,					/* number of lines in array */
	  RubberbandMax;
	pcb_rubberband_t *Rubberband;
} pcb_attached_object_t;

typedef struct {
	pcb_bool status;
	pcb_coord_t X, Y;
} pcb_mark_t;

enum pcb_crosshair_shape_e {
	Basic_Crosshair_Shape = 0,		/*  4-ray */
	Union_Jack_Crosshair_Shape,		/*  8-ray */
	Dozen_Crosshair_Shape,				/* 12-ray */
	Crosshair_Shapes_Number
};

typedef struct {								/* holds cursor information */
	pcb_hid_gc_t GC,											/* GC for cursor drawing */
	  AttachGC;										/* and for displaying buffer contents */
	pcb_coord_t X, Y,										/* position in PCB coordinates */
	  MinX, MinY,									/* lowest and highest coordinates */
	  MaxX, MaxY;
	pcb_attached_line_t AttachedLine;	/* data of new lines... */
	pcb_attached_box_t AttachedBox;
	pcb_polygon_t AttachedPolygon;
	pcb_attached_object_t AttachedObject;	/* data of attached objects */
	enum pcb_crosshair_shape_e shape;		/* shape of crosshair */
	vtop_t onpoint_objs;
	vtop_t old_onpoint_objs;

	/* list of object IDs that could have been dragged so that they can be cycled */
	long int *drags;
	int drags_len, drags_current;
	pcb_coord_t dragx, dragy;						/* the point where drag started */
} pcb_crosshair_t;


/* ---------------------------------------------------------------------------
 * all possible states of an attached object
 */
#define	STATE_FIRST		0					/* initial state */
#define	STATE_SECOND	1
#define	STATE_THIRD		2

pcb_coord_t GridFit(pcb_coord_t x, pcb_coord_t grid_spacing, pcb_coord_t grid_offset);
void notify_crosshair_change(pcb_bool changes_complete);
void notify_mark_change(pcb_bool changes_complete);
void HideCrosshair(void);
void RestoreCrosshair(void);
void DrawAttached(void);
void DrawMark(void);
void MoveCrosshairRelative(pcb_coord_t, pcb_coord_t);
pcb_bool MoveCrosshairAbsolute(pcb_coord_t, pcb_coord_t);
void SetCrosshairRange(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t);
void InitCrosshair(void);
void DestroyCrosshair(void);
void FitCrosshairIntoGrid(pcb_coord_t, pcb_coord_t);
void CenterDisplay(pcb_coord_t X, pcb_coord_t Y);

#endif
