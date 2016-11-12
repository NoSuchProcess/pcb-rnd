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

/*** Standard operations on polygons ***/

#include "operation.h"

void *AddPolygonToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);
void *MovePolygonToBuffer(pcb_opctx_t *ctx, pcb_layer_t * layer, PolygonType * polygon);
void *ChangePolygonClearSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr poly);
void *ChangePolyClear(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);
void *InsertPointIntoPolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);
void *MovePolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);
void *MovePolygonPoint(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon, pcb_point_t *Point);
void *MovePolygonToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, PolygonType * polygon, pcb_layer_t * Destination);
void *MovePolygonToLayer(pcb_opctx_t *ctx, pcb_layer_t * Layer, PolygonType * Polygon);
void *DestroyPolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);
void *DestroyPolygonPoint(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon, pcb_point_t *Point);
void *RemovePolygon_op(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);
void *RemovePolygonContour(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon, pcb_cardinal_t contour);
void *RemovePolygonPoint(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon, pcb_point_t *Point);
void *CopyPolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, PolygonTypePtr Polygon);


