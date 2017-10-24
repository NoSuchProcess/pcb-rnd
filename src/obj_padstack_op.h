/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

/*** Standard operations on padstacks ***/

#include "operation.h"

/* TODO
void *pcb_padstackop_change_size(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_change_clear_size(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_change_name(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_change_num(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_change_square(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_set_square(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_clear_square(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_change_mask_size(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_change_flag(pcb_opctx_t *ctx, pcb_element_t *elem, pcb_padstack_t *ps);
*/

void *pcb_padstackop_move_to_buffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_line_t * line);
void *pcb_padstackop_copy(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_move(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_move_noclip(pcb_opctx_t *ctx, pcb_padstack_t *ps);
void *pcb_padstackop_clip(pcb_opctx_t *ctx, pcb_padstack_t *ps);



