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

/* Drawing primitive: (elliptical) arc */

#ifndef PCB_OBJ_ARC_H
#define PCB_OBJ_ARC_H

#include <genlist/gendlist.h>
#include "obj_common.h"

struct pcb_arc_s {       /* holds information about arcs */
	PCB_ANY_PRIMITIVE_FIELDS;
	rnd_coord_t Thickness, Clearance;
	rnd_coord_t Width, Height;     /* length of axis */
	rnd_coord_t X, Y;              /* center coordinates */
	rnd_angle_t StartAngle, Delta; /* the two limiting angles in degrees */
	gdl_elem_t link;               /* an arc is in a list: either on a layer or in an element or in a font */
};

/*** Memory ***/
pcb_arc_t *pcb_arc_alloc(pcb_layer_t *);
pcb_arc_t *pcb_arc_alloc_id(pcb_layer_t *layer, long int id);
void pcb_arc_free(pcb_arc_t *data);

pcb_arc_t *pcb_arc_new(pcb_layer_t *Layer, rnd_coord_t center_x, rnd_coord_t center_y, rnd_coord_t width_r, rnd_coord_t height_r, rnd_angle_t start_angle, rnd_angle_t delta_angle, rnd_coord_t Thickness, rnd_coord_t Clearance, pcb_flag_t Flags, rnd_bool prevent_dups);
pcb_arc_t *pcb_arc_dup(pcb_layer_t *dst, pcb_arc_t *src);
pcb_arc_t *pcb_arc_dup_at(pcb_layer_t *dst, pcb_arc_t *src, rnd_coord_t dx, rnd_coord_t dy);
void *pcb_arc_destroy(pcb_layer_t *Layer, pcb_arc_t *Arc);

void pcb_arc_reg(pcb_layer_t *layer, pcb_arc_t *arc);
void pcb_arc_unreg(pcb_arc_t *arc);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_arc_on_layer(pcb_layer_t *Layer, pcb_arc_t *Arc);


/*** Utility ***/
void pcb_arc_bbox(pcb_arc_t *Arc);
void pcb_arc_rotate90(pcb_arc_t *Arc, rnd_coord_t X, rnd_coord_t Y, unsigned Number);
void pcb_arc_rotate(pcb_layer_t *layer, pcb_arc_t *arc, rnd_coord_t X, rnd_coord_t Y, double cosa, double sina, rnd_angle_t angle);
void pcb_arc_mirror(pcb_arc_t *arc, rnd_coord_t y_offs, rnd_bool undoable);
void pcb_arc_flip_side(pcb_layer_t *layer, pcb_arc_t *arc);
void pcb_arc_scale(pcb_arc_t *arc, double sx, double sy, double sth);
rnd_box_t pcb_arc_mini_bbox(const pcb_arc_t *arc);


/*** hash and eq ***/
int pcb_arc_eq(const pcb_host_trans_t *tr1, const pcb_arc_t *a1, const pcb_host_trans_t *tr2, const pcb_arc_t *a2);
unsigned int pcb_arc_hash(const pcb_host_trans_t *tr, const pcb_arc_t *a);


/* Return the x;y coordinate of the endpoint of an arc; if which is 0, return
   the endpoint that corresponds to StartAngle, else return the end angle's. */
void pcb_arc_get_end(pcb_arc_t *Arc, int which, rnd_coord_t *x, rnd_coord_t *y);
void pcb_arc_middle(const pcb_arc_t *arc, rnd_coord_t *x, rnd_coord_t *y);

/* Call cb() with coords of approximation for an arc from start to end, or
   end to start (if reverse is true). Resolution is set by res: if it is positive,
   it is an angle in degrees; if negative, it's an edge length in rnd_coord_t; if
   zero, the default 1 mm resolution is used. If cb returns non-zero, the loop
   quits immediately. Even stepping is not guaranteed, but visiting the exact
   arc start and end point is. */
void pcb_arc_approx(const pcb_arc_t *arc, double res, int reverse, void *uctx, int (*cb)(void *uctx, rnd_coord_t x, rnd_coord_t y));


void pcb_arc_set_angles(pcb_layer_t *Layer, pcb_arc_t *a, rnd_angle_t new_sa, rnd_angle_t new_da);
void pcb_arc_set_radii(pcb_layer_t *Layer, pcb_arc_t *a, rnd_coord_t new_width, rnd_coord_t new_height);

rnd_coord_t pcb_arc_length(const pcb_arc_t *arc);
double pcb_arc_area(const pcb_arc_t *arc);

/* ptr3 values for start and end point */
extern int *pcb_arc_start_ptr, *pcb_arc_end_ptr;

/* un-administrate a arc; call before changing geometry */
void pcb_arc_pre(pcb_arc_t *arc);

/* re-administrate a arc; call after changing geometry */
void pcb_arc_post(pcb_arc_t *arc);

#define	pcb_arc_move(a,dx,dy)                                     \
	do {                                                            \
		rnd_coord_t __dx__ = (dx), __dy__ = (dy);                     \
		pcb_arc_t *__a__ = (a);                                       \
		RND_MOVE_POINT((__a__)->X,(__a__)->Y,__dx__,__dy__);          \
		RND_BOX_MOVE_LOWLEVEL(&((__a__)->BoundingBox),__dx__,__dy__); \
		RND_BOX_MOVE_LOWLEVEL(&((__a__)->bbox_naked),__dx__,__dy__); \
	} while(0)

#define PCB_ARC_LOOP(element) do {                                      \
  pcb_arc_t *arc;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#define PCB_ARC_ALL_LOOP(top) do {		\
	rnd_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l =0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)		\
	{ \
		PCB_ARC_LOOP(layer)

#define PCB_ARC_COPPER_LOOP(top) do	{		\
	rnd_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l =0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)		\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER)) continue; \
		PCB_ARC_LOOP(layer)

#define PCB_ARC_SILK_LOOP(top) do	{		\
	rnd_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK)) continue; \
		PCB_ARC_LOOP(layer)

#define PCB_ARC_VISIBLE_LOOP(top) do	{		\
	rnd_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (layer->meta.real.vis) \
			PCB_ARC_LOOP(layer)

#endif
