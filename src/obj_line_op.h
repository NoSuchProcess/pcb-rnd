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

/*** Standard operations on line segments ***/

#include "operation.h"

void *pcb_lineop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_name(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move_to_buffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_line_t * line);
void *pcb_lineop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point);
void *pcb_lineop_move_point_with_route(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point);
void *pcb_lineop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_line_t * line, pcb_layer_t * Destination);
void *pcb_lineop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_line_t * Line);
void *pcb_lineop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_remove_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point);
void *pcb_lineop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_rotate90_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point);
void *pcb_lineop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_line_t *line);



