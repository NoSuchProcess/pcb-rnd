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

/*** Standard operations on pins and vias ***/

#include "operation.h"

void *pcb_viaop_add_to_buffer(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_move_to_buffer(pcb_opctx_t *ctx, pcb_pin_t * via);
void *pcb_viaop_change_thermal(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_pinop_change_thermal(pcb_opctx_t *ctx, pcb_element_t *element, pcb_pin_t *Pin);
void *pcb_viaop_change_size(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_change_2nd_size(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_pinop_change_2nd_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_viaop_change_clear_size(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_pinop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_pinop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_viaop_change_name(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_pinop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_pinop_change_num(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_viaop_change_square(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_pinop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_pinop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_pinop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_viaop_change_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_set_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_clear_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_pinop_change_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_pinop_set_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_pinop_clear_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
pcb_bool pcb_pin_change_hole(pcb_pin_t *Via);
void *pcb_pinop_change_mask_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin);
void *pcb_viaop_change_mask_size(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_copy(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_move(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_destroy(pcb_opctx_t *ctx, pcb_pin_t *Via);
void *pcb_viaop_remove(pcb_opctx_t *ctx, pcb_pin_t *Via);
