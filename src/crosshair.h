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

#include "global.h"
#include "rubberband.h"

typedef struct {								/* current marked line */
	PointType Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool draw;
} AttachedLineType, *AttachedLineTypePtr;

typedef struct {								/* currently marked block */
	PointType Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool otherway;
} AttachedBoxType, *AttachedBoxTypePtr;

typedef struct {								/* currently attached object */
	Coord X, Y;										/* saved position when PCB_MODE_MOVE */
	BoxType BoundingBox;
	long int Type,								/* object type */
	  State;
	void *Ptr1,										/* three pointers to data, see */
	 *Ptr2,												/* search.c */
	 *Ptr3;
	pcb_cardinal_t RubberbandN,					/* number of lines in array */
	  RubberbandMax;
	RubberbandTypePtr Rubberband;
} AttachedObjectType, *AttachedObjectTypePtr;

typedef struct {
	pcb_bool status;
	Coord X, Y;
} MarkType, *MarkTypePtr;

enum crosshair_shape {
	Basic_Crosshair_Shape = 0,		/*  4-ray */
	Union_Jack_Crosshair_Shape,		/*  8-ray */
	Dozen_Crosshair_Shape,				/* 12-ray */
	Crosshair_Shapes_Number
};

typedef struct {								/* holds cursor information */
	hidGC GC,											/* GC for cursor drawing */
	  AttachGC;										/* and for displaying buffer contents */
	Coord X, Y,										/* position in PCB coordinates */
	  MinX, MinY,									/* lowest and highest coordinates */
	  MaxX, MaxY;
	AttachedLineType AttachedLine;	/* data of new lines... */
	AttachedBoxType AttachedBox;
	PolygonType AttachedPolygon;
	AttachedObjectType AttachedObject;	/* data of attached objects */
	enum crosshair_shape shape;		/* shape of crosshair */
	vtop_t onpoint_objs;
	vtop_t old_onpoint_objs;

	/* list of object IDs that could have been dragged so that they can be cycled */
	long int *drags;
	int drags_len, drags_current;
	Coord dragx, dragy;						/* the point where drag started */
} CrosshairType, *CrosshairTypePtr;


/* ---------------------------------------------------------------------------
 * all possible states of an attached object
 */
#define	STATE_FIRST		0					/* initial state */
#define	STATE_SECOND	1
#define	STATE_THIRD		2

Coord GridFit(Coord x, Coord grid_spacing, Coord grid_offset);
void notify_crosshair_change(pcb_bool changes_complete);
void notify_mark_change(pcb_bool changes_complete);
void HideCrosshair(void);
void RestoreCrosshair(void);
void DrawAttached(void);
void DrawMark(void);
void MoveCrosshairRelative(Coord, Coord);
pcb_bool MoveCrosshairAbsolute(Coord, Coord);
void SetCrosshairRange(Coord, Coord, Coord, Coord);
void InitCrosshair(void);
void DestroyCrosshair(void);
void FitCrosshairIntoGrid(Coord, Coord);

#endif
