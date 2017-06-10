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

/*** Standard operations on elements ***/

#include "operation.h"

void *pcb_elemop_add_to_buffer(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_move_to_buffer(pcb_opctx_t *ctx, pcb_element_t * element);
void *pcb_elemop_clear_octagon(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_set_octagon(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_octagon(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_nonetlist(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_name_size(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_1st_size(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_change_2nd_size(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_copy(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_move_name(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_move(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_destroy(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_remove(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_rotate90(pcb_opctx_t *ctx, pcb_element_t *Element);
void *pcb_elemop_rotate90_name(pcb_opctx_t *ctx, pcb_element_t *Element);

