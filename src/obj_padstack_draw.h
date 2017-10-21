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

#include "obj_padstack.h"

/* Include rtree.h for these */
#ifdef PCB_RTREE_H
pcb_r_dir_t pcb_padstack_draw_callback(const pcb_box_t *b, void *cl);
pcb_r_dir_t pcb_padstack_clear_callback(const pcb_box_t *b, void *cl);
#endif

void pcb_padstack_draw(pcb_pin_t *pin, pcb_bool draw_hole);
void pcb_padstack_invalidate_erase(pcb_padstack_t *ps);
void pcb_padstack_invalidate_draw(pcb_padstack_t *ps);
