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


#define ARC_LOOP(element) do {                                      \
  ArcType *arc;                                                     \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Arc, &__it__, arc) {
