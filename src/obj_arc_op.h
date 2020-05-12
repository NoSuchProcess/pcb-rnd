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

/*** Standard operations on arc ***/

#include "operation.h"

void *pcb_arcop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_arc_t *arc);
void *pcb_arcop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_change_radius(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_change_angle(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_arc_t * arc, pcb_layer_t * Destination);
void *pcb_arcop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_arc_t * Arc);
void *pcb_arcop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_remove_point(pcb_opctx_t *ctx, pcb_layer_t *l, pcb_arc_t *a, int *end_id);
void *pcb_arcop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *Arc);
void *pcb_arcop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_arc_t *arc);
void *pcb_arcop_change_thermal(pcb_opctx_t *ctx, pcb_layer_t *ly, pcb_arc_t *arc);


void *pcb_arc_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_arc_t *arc);


