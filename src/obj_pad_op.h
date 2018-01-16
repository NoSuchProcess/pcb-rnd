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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/*** Standard operations on pads ***/

#include "operation.h"

void *pcb_padop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_change_num(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_change_mask_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad);
void *pcb_padop_change_flag(pcb_opctx_t *ctx, pcb_element_t *elem, pcb_pad_t *pad);





