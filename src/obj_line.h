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

/* Drawing primitive: line segment */

#ifndef PCB_OBJ_LINE_H
#define PCB_OBJ_LINE_H

#include "global_objs.h"
#include "obj_common.h"

struct pcb_line_s {            /* holds information about one line */
	ANYLINEFIELDS;
	char *Number;
	gdl_elem_t link;             /* a line is in a list: either on a layer or in an element */
};

/* crosshair: */
typedef struct {								/* current marked line */
	PointType Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool draw;
} AttachedLineType, *AttachedLineTypePtr;


LineType *GetLineMemory(LayerType * layer);
void RemoveFreeLine(LineType * data);

LineTypePtr CreateDrawnLineOnLayer(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, FlagType Flags);
LineTypePtr CreateNewLineOnLayer(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, FlagType Flags);
void pcb_add_line_on_layer(LayerType *Layer, LineType *Line);
void SetLineBoundingBox(LineTypePtr Line);
void *RemoveLine(LayerTypePtr Layer, LineTypePtr Line);
void RotateLineLowLevel(LineTypePtr Line, Coord X, Coord Y, unsigned Number);

/*** DRC enforcement (obj_line_drcenf.c) ***/
void AdjustAttachedLine(void);
void AdjustTwoLine(pcb_bool);
void FortyFiveLine(AttachedLineTypePtr);
void EnforceLineDRC(void);

/* Rather than mode the line bounding box, we set it so the point bounding
 * boxes are updated too.
 */
#define	MOVE_LINE_LOWLEVEL(l,dx,dy)							\
	{									\
		MOVE((l)->Point1.X,(l)->Point1.Y,(dx),(dy))			\
		MOVE((l)->Point2.X,(l)->Point2.Y,(dx),(dy))			\
		SetLineBoundingBox ((l)); \
	}


#define LINE_LOOP(layer) do {                                       \
  LineType *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Line, &__it__, line) {

#define	ALLLINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define	COPPERLINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		LINE_LOOP(layer)

#define	SILKLINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		LINE_LOOP(layer)

#define	VISIBLELINE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			LINE_LOOP(layer)

#endif
