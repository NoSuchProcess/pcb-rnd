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

/* Drawing primitive: (elliptical) arc */

#ifndef PCB_OBJ_ARC_H
#define PCB_OBJ_ARC_H

#include "obj_common.h"

struct pcb_arc_s {       /* holds information about arcs */
	ANYOBJECTFIELDS;
	Coord Thickness, Clearance;
	Coord Width, Height,					/* length of axis */
	  X, Y;												/* center coordinates */
	Angle StartAngle, Delta;			/* the two limiting angles in degrees */
	gdl_elem_t link;              /* an arc is in a list: either on a layer or in an element */
};


/*** Memory ***/
pcb_arc_t *GetArcMemory(pcb_layer_t *);
pcb_arc_t *GetElementArcMemory(ElementType *Element);
void RemoveFreeArc(pcb_arc_t *data);

/*** Utility ***/
void SetArcBoundingBox(pcb_arc_t *Arc);
pcb_box_t *GetArcEnds(pcb_arc_t *Arc);
void ChangeArcAngles(pcb_layer_t *Layer, pcb_arc_t *a, Angle new_sa, Angle new_da);
void ChangeArcRadii(pcb_layer_t *Layer, pcb_arc_t *a, Coord new_width, Coord new_height);
void *RemoveArc(pcb_layer_t *Layer, pcb_arc_t *Arc);
pcb_arc_t *CreateNewArcOnLayer(pcb_layer_t *Layer, Coord X1, Coord Y1, Coord width, Coord height, Angle sa, Angle dir, Coord Thickness, Coord Clearance, FlagType Flags);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_arc_on_layer(pcb_layer_t *Layer, pcb_arc_t *Arc);

void RotateArcLowLevel(pcb_arc_t *Arc, Coord X, Coord Y, unsigned Number);

#define	MOVE_ARC_LOWLEVEL(a,dx,dy) \
	{ \
		MOVE((a)->X,(a)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((a)->BoundingBox),(dx),(dy));		\
	}

#define ARC_LOOP(element) do {                                      \
  pcb_arc_t *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#define ALLARC_LOOP(top) do {		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer + 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define COPPERARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define SILKARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	VISIBLEARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			ARC_LOOP(layer)

#endif
