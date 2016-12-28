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

/*** Standard operations on arc ***/

#include "operation.h"

void *AddArcToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *MoveArcToBuffer(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_arc_t *arc);
void *ChangeArcSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *ChangeArcClearSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *ChangeArcRadius(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *ChangeArcAngle(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *ChangeArcJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *SetArcJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *ClrArcJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *CopyArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *MoveArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *MoveArcToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_arc_t * arc, pcb_layer_t * Destination);
void *MoveArcToLayer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_arc_t * Arc);
void *DestroyArc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *RemoveArc_op(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *RemoveArcPoint_op(pcb_opctx_t *ctx, pcb_layer_t *l, pcb_arc_t *a, int *end_id);
void *Rotate90Arc(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);

void *pcb_arc_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *arc);


