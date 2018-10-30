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

/*** Standard operations on rat lines ***/

#include "operation.h"

void *pcb_ratop_add_to_buffer(pcb_opctx_t *ctx, pcb_rat_t *Rat);
void *pcb_ratop_move_buffer(pcb_opctx_t *ctx, pcb_rat_t * rat);
void *pcb_ratop_insert_point(pcb_opctx_t *ctx, pcb_rat_t *Rat);
void *pcb_ratop_move_to_layer(pcb_opctx_t *ctx, pcb_rat_t * Rat);
void *pcb_ratop_destroy(pcb_opctx_t *ctx, pcb_rat_t *Rat);
void *pcb_ratop_remove(pcb_opctx_t *ctx, pcb_rat_t *Rat);
