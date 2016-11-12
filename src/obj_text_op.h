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

/*** Standard operations on text ***/

#include "operation.h"

void *AddTextToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *MoveTextToBuffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_text_t * text);
void *ChangeTextSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *ChangeTextName(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *ChangeTextJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *SetTextJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *ClrTextJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *CopyText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *MoveText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *MoveTextToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_text_t * text, pcb_layer_t * Destination);
void *MoveTextToLayer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_text_t * text);
void *DestroyText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *RemoveText_op(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
void *RotateText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text);
