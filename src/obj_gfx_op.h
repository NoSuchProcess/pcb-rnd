/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/*** Standard operations on gfx ***/

#include "operation.h"

void *pcb_gfxop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_gfx_t *gfx);
void *pcb_gfxop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_gfx_t * gfx, pcb_layer_t * Destination);
void *pcb_gfxop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_gfx_t * gfx);
void *pcb_gfxop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_gfx_t *gfx);
void *pcb_gfxop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_gfx_t *gfx);



