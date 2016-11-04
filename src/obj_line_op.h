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

/*** Standard operations on line ***/

#include "operation.h"

void *AddLineToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);

void *ChangeLineSize(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *ChangeLineClearSize(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *ChangeLineName(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *ChangeLineJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *SetLineJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *ClrLineJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);

void *MoveLineToBuffer(pcb_opctx_t *ctx, LayerType * layer, LineType * line);
void *CopyLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *MoveLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *MoveLinePoint(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point);
void *MoveLineToLayerLowLevel(pcb_opctx_t *ctx, LayerType * Source, LineType * line, LayerType * Destination);
void *MoveLineToLayer(pcb_opctx_t *ctx, LayerType * Layer, LineType * Line);
void *DestroyLine(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *RemoveLinePoint(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point);
void *RemoveLine_op(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line);
void *RotateLinePoint(pcb_opctx_t *ctx, LayerTypePtr Layer, LineTypePtr Line, PointTypePtr Point);



