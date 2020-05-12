/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/*** Standard operations on line segments ***/

#include "operation.h"

void *pcb_lineop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_line_t * line);
void *pcb_lineop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_move_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, rnd_point_t *Point);
void *pcb_lineop_move_point_with_route(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, rnd_point_t *Point);
void *pcb_lineop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_line_t * line, pcb_layer_t * Destination);
void *pcb_lineop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_line_t * Line);
void *pcb_lineop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_remove_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, rnd_point_t *Point);
void *pcb_lineop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_rotate90_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, rnd_point_t *Point);
void *pcb_lineop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line);
void *pcb_lineop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_line_t *line);
void *pcb_lineop_change_thermal(pcb_opctx_t *ctx, pcb_layer_t *ly, pcb_line_t *line);




