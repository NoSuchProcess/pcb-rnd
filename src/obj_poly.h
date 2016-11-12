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

struct polygon_st  {           /* holds information about a polygon */
	ANYOBJECTFIELDS;
	pcb_cardinal_t PointN;       /* number of points in polygon */
	pcb_cardinal_t PointMax;     /* max number from malloc() */
	POLYAREA *Clipped;           /* the clipped region of this polygon */
	PLINE *NoHoles;              /* the polygon broken into hole-less regions */
	int NoHolesValid;            /* Is the NoHoles polygon up to date? */
	PointTypePtr Points;         /* data */
	pcb_cardinal_t *HoleIndex;   /* Index of hole data within the Points array */
	pcb_cardinal_t HoleIndexN;   /* number of holes in polygon */
	pcb_cardinal_t HoleIndexMax; /* max number from malloc() */
	gdl_elem_t link;             /* a text is in a list of a layer */
};



PolygonType *GetPolygonMemory(pcb_layer_t * layer);
void RemoveFreePolygon(PolygonType * data);
PointTypePtr GetPointMemoryInPolygon(PolygonTypePtr Polygon);
pcb_cardinal_t *GetHoleIndexMemoryInPolygon(PolygonTypePtr Polygon);
void FreePolygonMemory(PolygonType * polygon);

void SetPolygonBoundingBox(PolygonTypePtr Polygon);
PolygonTypePtr CreateNewPolygonFromRectangle(pcb_layer_t *Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, FlagType Flags);
PolygonTypePtr CreateNewPolygon(pcb_layer_t *Layer, FlagType Flags);
PointTypePtr CreateNewPointInPolygon(PolygonTypePtr Polygon, Coord X, Coord Y);
PolygonType *CreateNewHoleInPolygon(PolygonType * Polygon);
void *RemovePolygon(pcb_layer_t *Layer, PolygonTypePtr Polygon);

void MovePolygonLowLevel(PolygonTypePtr Polygon, Coord DX, Coord DY);
void RotatePolygonLowLevel(PolygonTypePtr Polygon, Coord X, Coord Y, unsigned Number);
PolygonTypePtr CopyPolygonLowLevel(PolygonTypePtr Dest, PolygonTypePtr Src);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_polygon_on_layer(pcb_layer_t *Layer, PolygonType *polygon);

#define POLYGON_LOOP(layer) do {                                    \
  PolygonType *polygon;                                             \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Polygon, &__it__, polygon) {

#define	POLYGONPOINT_LOOP(polygon) do	{	\
	pcb_cardinal_t			n;		\
	PointTypePtr	point;				\
	for (n = (polygon)->PointN-1; n != -1; n--)	\
	{						\
		point = &(polygon)->Points[n]

#define	ALLPOLYGON_LOOP(top)	do {		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	COPPERPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	SILKPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		POLYGON_LOOP(layer)

#define	VISIBLEPOLYGON_LOOP(top) do	{	\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			POLYGON_LOOP(layer)


#endif
