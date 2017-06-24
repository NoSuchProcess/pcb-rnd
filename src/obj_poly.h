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

/* Drawing primitive: polygons */

#ifndef PCB_OBJ_POLY_H
#define PCB_OBJ_POLY_H

#include "obj_common.h"
#include "polyarea.h"

struct pcb_polygon_s  {           /* holds information about a polygon */
	PCB_ANYOBJECTFIELDS;
	pcb_cardinal_t PointN;       /* number of points in polygon */
	pcb_cardinal_t PointMax;     /* max number from malloc() */
	pcb_polyarea_t *Clipped;           /* the clipped region of this polygon */
	pcb_pline_t *NoHoles;              /* the polygon broken into hole-less regions */
	int NoHolesValid;            /* Is the NoHoles polygon up to date? */
	pcb_point_t *Points;         /* data */
	pcb_cardinal_t *HoleIndex;   /* Index of hole data within the Points array */
	pcb_cardinal_t HoleIndexN;   /* number of holes in polygon */
	pcb_cardinal_t HoleIndexMax; /* max number from malloc() */
	gdl_elem_t link;             /* a poly is in a list of a layer */
};



pcb_polygon_t *pcb_poly_alloc(pcb_layer_t * layer);
void pcb_poly_free(pcb_polygon_t * data);
pcb_point_t *pcb_poly_point_alloc(pcb_polygon_t *Polygon);
pcb_cardinal_t *pcb_poly_holeidx_new(pcb_polygon_t *Polygon);
void pcb_poly_free_fields(pcb_polygon_t * polygon);

void pcb_poly_bbox(pcb_polygon_t *Polygon);
pcb_polygon_t *pcb_poly_new_from_rectangle(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_flag_t Flags);
pcb_polygon_t *pcb_poly_new(pcb_layer_t *Layer, pcb_flag_t Flags);
pcb_polygon_t *pcb_poly_dup(pcb_layer_t *dst, pcb_polygon_t *src);
pcb_polygon_t *pcb_poly_dup_at(pcb_layer_t *dst, pcb_polygon_t *src, pcb_coord_t dx, pcb_coord_t dy);
pcb_point_t *pcb_poly_point_new(pcb_polygon_t *Polygon, pcb_coord_t X, pcb_coord_t Y);
pcb_polygon_t *pcb_poly_hole_new(pcb_polygon_t * Polygon);
void *pcb_poly_remove(pcb_layer_t *Layer, pcb_polygon_t *Polygon);

void pcb_poly_rotate90(pcb_polygon_t *Polygon, pcb_coord_t X, pcb_coord_t Y, unsigned Number);
void pcb_poly_rotate(pcb_layer_t *layer, pcb_polygon_t *poly, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina);
void pcb_poly_mirror(pcb_layer_t *layer, pcb_polygon_t *polygon, pcb_coord_t y_offs);
void pcb_poly_flip_side(pcb_layer_t *layer, pcb_polygon_t *polygon);

void pcb_poly_move(pcb_polygon_t *Polygon, pcb_coord_t DX, pcb_coord_t DY);
pcb_polygon_t *pcb_poly_copy(pcb_polygon_t *Dest, pcb_polygon_t *Src, pcb_coord_t dx, pcb_coord_t dy);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_polygon_on_layer(pcb_layer_t *Layer, pcb_polygon_t *polygon);

double pcb_poly_area(const pcb_polygon_t *poly);

#define PCB_POLY_LOOP(layer) do {                                    \
  pcb_polygon_t *polygon;                                             \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Polygon, &__it__, polygon) {

#define	PCB_POLY_POINT_LOOP(polygon) do	{	\
	pcb_cardinal_t			n;		\
	pcb_point_t *point;				\
	for (n = (polygon)->PointN-1; n != -1; n--)	\
	{						\
		point = &(polygon)->Points[n]

#define	PCB_POLY_ALL_LOOP(top)	do {		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN) ; l++, layer++)	\
	{ \
		PCB_POLY_LOOP(layer)

#define	PCB_POLY_COPPER_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER)) continue; \
		PCB_POLY_LOOP(layer)

#define	PCB_POLY_SILK_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK)) continue; \
		PCB_POLY_LOOP(layer)

#define	PCB_POLY_VISIBLE_LOOP(top) do	{	\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (layer->meta.real.vis) \
			PCB_POLY_LOOP(layer)


#endif
