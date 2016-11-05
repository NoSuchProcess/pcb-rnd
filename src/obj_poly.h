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

#include "global_objs.h"
#include "obj_common.h"

PolygonType *GetPolygonMemory(LayerType * layer);
void RemoveFreePolygon(PolygonType * data);
PointTypePtr GetPointMemoryInPolygon(PolygonTypePtr Polygon);
pcb_cardinal_t *GetHoleIndexMemoryInPolygon(PolygonTypePtr Polygon);
void FreePolygonMemory(PolygonType * polygon);

void SetPolygonBoundingBox(PolygonTypePtr Polygon);
PolygonTypePtr CreateNewPolygonFromRectangle(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, FlagType Flags);
PolygonTypePtr CreateNewPolygon(LayerTypePtr Layer, FlagType Flags);
PointTypePtr CreateNewPointInPolygon(PolygonTypePtr Polygon, Coord X, Coord Y);
PolygonType *CreateNewHoleInPolygon(PolygonType * Polygon);
void *RemovePolygon(LayerTypePtr Layer, PolygonTypePtr Polygon);

void MovePolygonLowLevel(PolygonTypePtr Polygon, Coord DX, Coord DY);
void RotatePolygonLowLevel(PolygonTypePtr Polygon, Coord X, Coord Y, unsigned Number);
PolygonTypePtr CopyPolygonLowLevel(PolygonTypePtr Dest, PolygonTypePtr Src);

void pcb_add_polygon_on_layer(LayerType *Layer, PolygonType *polygon);

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
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	COPPERPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer; l++, layer++)	\
	{ \
		POLYGON_LOOP(layer)

#define	SILKPOLYGON_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		POLYGON_LOOP(layer)

#define	VISIBLEPOLYGON_LOOP(top) do	{	\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			POLYGON_LOOP(layer)


#endif
