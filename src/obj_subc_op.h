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

#ifndef PCB_OBJ_SUBC_OP_H
#define PCB_OBJ_SUBC_OP_H

#include "operation.h"

void *pcb_subcop_copy(pcb_opctx_t *ctx, pcb_subc_t *src);
void *pcb_subcop_move(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_rotate90(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_rotate(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_move_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_clear_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_1st_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_2nd_size(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_nonetlist(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_change_name(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_destroy(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *pcb_subcop_remove(pcb_opctx_t *ctx, pcb_subc_t *sc);

void *pcb_subcop_change_flag(pcb_opctx_t *ctx, pcb_subc_t *sc);

typedef enum pcb_subc_op_undo_e {
	PCB_SUBCOP_UNDO_NORMAL,  /* each part operation is undone separately */
	PCB_SUBCOP_UNDO_SUBC,    /* the undo for the parent subcircuit will handle each part's undo */
	PCB_SUBCOP_UNDO_BATCH    /* each part operation has its own undo, but with the same serial */
} pcb_subc_op_undo_t;

void *pcb_subc_op(pcb_data_t *Data, pcb_subc_t *sc, pcb_opfunc_t *opfunc, pcb_opctx_t *ctx, pcb_subc_op_undo_t batch_undo);

#endif
