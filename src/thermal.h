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

#ifndef PCB_THERMAL_H
#define PCB_THERMAL_H

#include "obj_common.h"
#include "layer.h"
#include "polygon1_gen.h"

typedef enum pcb_thermal_e {
	/* bit 0 and 1: shape */
	PCB_THERMAL_NOSHAPE = 0,  /* padstack: no shape shall be drawn, omit copper, no connection */
	PCB_THERMAL_ROUND = 1,
	PCB_THERMAL_SHARP = 2,
	PCB_THERMAL_SOLID = 3,

	/* bit 2: orientation */
	PCB_THERMAL_DIAGONAL = 4,

	/* bit 3: do we have a thermal at all? */
	PCB_THERMAL_ON = 8
} pcb_thermal_t;

/* convert the textual version of thermal to a bitmask */
pcb_thermal_t pcb_thermal_str2bits(const char *str);

/* convert a bitmask to a single word of thermal description and remove the
   affected bits from bits. Call this repeatedly until bits == 0 to get
   all words. */
const char *pcb_thermal_bits2str(pcb_thermal_t *bits);


pcb_polyarea_t *pcb_thermal_area(pcb_board_t *p, pcb_any_obj_t *obj, pcb_layer_id_t lid);
pcb_polyarea_t *pcb_thermal_area_line(pcb_board_t *pcb, pcb_line_t *line, pcb_layer_id_t lid);
pcb_polyarea_t *pcb_thermal_area_poly(pcb_board_t *pcb, pcb_poly_t *poly, pcb_layer_id_t lid);
pcb_polyarea_t *pcb_thermal_area_pstk(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layer_id_t lid);

unsigned char pcb_themal_style_old2new(pcb_cardinal_t t);
pcb_cardinal_t pcb_themal_style_new2old(unsigned char t);


#endif
