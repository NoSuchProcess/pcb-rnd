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
#ifndef PCB_SHAPE_H
#define PCB_SHAPE_H

#include "board.h"
#include "data.h"
#include "layer.h"
#include <librnd/core/pcb_bool.h>

/* special layer: when used, the shape is always placed on the current layer */
extern pcb_layer_t *pcb_shape_current_layer;

void pcb_shape_dialog(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_t *layer, pcb_bool modal);

typedef enum {
	PCB_CORN_ROUND,
	PCB_CORN_CHAMF,
	PCB_CORN_SQUARE
} pcb_shape_corner_t;
extern const char *pcb_shape_corner_name[];

pcb_poly_t *pcb_genpoly_roundrect(pcb_layer_t *layer, pcb_coord_t w, pcb_coord_t h, pcb_coord_t rx, pcb_coord_t ry, double rot_deg, pcb_coord_t cx, pcb_coord_t cy, pcb_shape_corner_t corner[4], double roundres);

/* shorthand rounded rectangle: rounding radius is roundness * smaller_size */
void pcb_shape_roundrect(pcb_pstk_shape_t *shape, pcb_coord_t width, pcb_coord_t height, double roundness);

#endif


