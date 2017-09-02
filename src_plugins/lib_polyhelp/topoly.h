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

#ifndef PCB_TOPOLY_H
#define PCB_TOPOLY_H

#include "board.h"
#include "obj_common.h"

typedef enum pcb_topoly_e {
	PCB_TOPOLY_KEEP_ORIG = 1,   /* keep original objects */
	PCB_TOPOLY_FLOATING = 2     /* do not add the new polygon on any layer */
} pcb_topoly_t;

/* Convert a loop of connected objects into a polygon (with no holes); the first
   object is named in start. */
pcb_polygon_t *pcb_topoly_conn(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how);

/* Find the first line/arc on the outline layer from top-left */
pcb_any_obj_t *pcb_topoly_find_1st_outline(pcb_board_t *pcb);


extern const char pcb_acts_topoly[];
extern const char pcb_acth_topoly[];
int pcb_act_topoly(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

#endif
