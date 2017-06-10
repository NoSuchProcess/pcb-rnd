/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#ifndef PCB_OBJ_SUBC_OP_H
#define PCB_OBJ_SUBC_OP_H

#include "operation.h"

void *pcb_subcop_copy(pcb_opctx_t *ctx, pcb_subc_t *src);
void *pcb_subcop_move(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_rotate90(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_move_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_clear_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_1st_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_2nd_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_nonetlist(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_name(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_destroy(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_remove(pcb_opctx_t *ctx, pcb_subc_t *sc);

void *pcb_subcop_clear_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_set_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_clear_square(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_set_square(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_square(pcb_opctx_t *ctx, pcb_subc_t *sc);

void *pcb_subcop_change_flag(pcb_opctx_t *ctx, pcb_subc_t *sc);


#endif
