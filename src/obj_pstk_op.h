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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/*** Standard operations on padstacks ***/

#include "operation.h"

void *pcb_pstkop_add_to_buffer(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_move_buffer(pcb_opctx_t *ctx, pcb_pstk_t *ps);

void *pcb_pstkop_copy(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_move(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_move_noclip(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_clip(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_remove(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_destroy(pcb_opctx_t *ctx, pcb_pstk_t *ps);

void *pcb_pstkop_change_thermal(pcb_opctx_t *ctx, pcb_pstk_t *ps);

void *pcb_pstkop_change_flag(pcb_opctx_t *ctx, pcb_pstk_t *ps);

void *pcb_pstkop_rotate90(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_rotate(pcb_opctx_t *ctx, pcb_pstk_t *ps);

void *pcb_pstkop_change_size(pcb_opctx_t *ctx, pcb_pstk_t *ps);
void *pcb_pstkop_change_clear_size(pcb_opctx_t *ctx, pcb_pstk_t *ps); /* changes the global clearance */
void *pcb_pstkop_change_2nd_size(pcb_opctx_t *ctx, pcb_pstk_t *ps);

/*** TODO: unimplemented ones ***/

void *pcb_pstkop_change_clear_size(pcb_opctx_t *ctx, pcb_pstk_t *ps);
