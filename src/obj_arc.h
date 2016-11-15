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
	PCB_ANYOBJECTFIELDS;
	pcb_coord_t Thickness, Clearance;
	pcb_coord_t Width, Height,					/* length of axis */
	  X, Y;												/* center coordinates */
	pcb_angle_t StartAngle, Delta;			/* the two limiting angles in degrees */
	gdl_elem_t link;              /* an arc is in a list: either on a layer or in an element */
};

/*** Memory ***/
pcb_arc_t *pcb_arc_alloc(pcb_layer_t *);
pcb_arc_t *pcb_element_arc_alloc(pcb_element_t *Element);
void pcb_arc_free(pcb_arc_t *data);

pcb_arc_t *pcb_arc_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t width, pcb_coord_t height, pcb_angle_t sa, pcb_angle_t dir, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags);
void *pcb_arc_destroy(pcb_layer_t *Layer, pcb_arc_t *Arc);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_arc_on_layer(pcb_layer_t *Layer, pcb_arc_t *Arc);



/*** Utility ***/
void pcb_arc_bbox(pcb_arc_t *Arc);
void pcb_arc_rotate90(pcb_arc_t *Arc, pcb_coord_t X, pcb_coord_t Y, unsigned Number);

pcb_box_t *pcb_arc_get_ends(pcb_arc_t *Arc);
void pcb_arc_set_angles(pcb_layer_t *Layer, pcb_arc_t *a, pcb_angle_t new_sa, pcb_angle_t new_da);
void pcb_arc_set_radii(pcb_layer_t *Layer, pcb_arc_t *a, pcb_coord_t new_width, pcb_coord_t new_height);

#define	pcb_arc_move(a,dx,dy)                                     \
	do {                                                            \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy);                     \
		pcb_arc_t *__a__ = (a);                                       \
		PCB_MOVE((__a__)->X,(__a__)->Y,__dx__,__dy__)                 \
		PCB_BOX_MOVE_LOWLEVEL(&((__a__)->BoundingBox),__dx__,__dy__); \
	} while(0)

#define PCB_ARC_LOOP(element) do {                                      \
  pcb_arc_t *arc;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#define PCB_ARC_ALL_LOOP(top) do {		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer + 2; l++, layer++)		\
	{ \
		PCB_ARC_LOOP(layer)

#define PCB_ARC_COPPER_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer; l++, layer++)		\
	{ \
		PCB_ARC_LOOP(layer)

#define PCB_ARC_SILK_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		PCB_ARC_LOOP(layer)

#define PCB_ARC_VISIBLE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			PCB_ARC_LOOP(layer)

#endif
