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

#include "global_typedefs.h"
#include "operation.h"

/*** Memory ***/
ArcTypePtr GetArcMemory(LayerTypePtr);
ArcType *GetElementArcMemory(ElementType *Element);
void RemoveFreeArc(ArcType *data);

/*** Utility ***/
void SetArcBoundingBox(ArcTypePtr Arc);
BoxTypePtr GetArcEnds(ArcTypePtr Arc);
void ChangeArcAngles(LayerTypePtr Layer, ArcTypePtr a, Angle new_sa, Angle new_da);
void ChangeArcRadii(LayerTypePtr Layer, ArcTypePtr a, Coord new_width, Coord new_height);
void *RemoveArc(LayerTypePtr Layer, ArcTypePtr Arc);
ArcType *CreateNewArcOnLayer(LayerTypePtr Layer, Coord X1, Coord Y1, Coord width, Coord height, Angle sa, Angle dir, Coord Thickness, Coord Clearance, FlagType Flags);
void pcb_add_arc_on_layer(LayerType *Layer, ArcType *Arc);
void RotateArcLowLevel(ArcTypePtr Arc, Coord X, Coord Y, unsigned Number);

/*** Operations ***/
void *AddArcToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *MoveArcToBuffer(pcb_opctx_t *ctx, LayerType *layer, ArcType *arc);
void *ChangeArcSize(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *ChangeArcClearSize(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *ChangeArcRadius(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *ChangeArcAngle(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *ChangeArcJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *SetArcJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *ClrArcJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *CopyArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *MoveArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *MoveArcToLayerLowLevel(pcb_opctx_t *ctx, LayerType * Source, ArcType * arc, LayerType * Destination);
void *MoveArcToLayer(pcb_opctx_t *ctx, LayerType * Layer, ArcType * Arc);
void *DestroyArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *RemoveArc_op(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);
void *RotateArc(pcb_opctx_t *ctx, LayerTypePtr Layer, ArcTypePtr Arc);

#define	MOVE_ARC_LOWLEVEL(a,dx,dy) \
	{ \
		MOVE((a)->X,(a)->Y,(dx),(dy)) \
		MOVE_BOX_LOWLEVEL(&((a)->BoundingBox),(dx),(dy));		\
	}

#define ARC_LOOP(element) do {                                      \
  ArcType *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {

#define ALLARC_LOOP(top) do {		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer + 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define COPPERARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l =0; l < max_copper_layer; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define SILKARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	layer += max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		ARC_LOOP(layer)

#define	VISIBLEARC_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	LayerTypePtr	layer = (top)->Layer;		\
	for (l = 0; l < max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			ARC_LOOP(layer)

