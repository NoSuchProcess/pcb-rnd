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

#ifndef PCB_THERMAL_H
#define PCB_THERMAL_H

#include "obj_common.h"
#include "layer.h"

typedef enum pcb_thermal_e {
	/* bit 0 and 1: shape */
	PCB_THERMAL_NOSHAPE = 0,  /* padstack: no shape shall be drawn, omit copper, no connection */
	PCB_THERMAL_ROUND = 1,
	PCB_THERMAL_SHARP = 2,
	PCB_THERMAL_SOLID = 3,

	/* bit 2: orientation */
	PCB_THERMAL_DIAGONAL = 4
} pcb_thermal_t;

pcb_polyarea_t *pcb_thermal_area(pcb_board_t *p, pcb_any_obj_t *obj, pcb_layer_id_t lid);
pcb_polyarea_t *pcb_thermal_area_pin(pcb_board_t *pcb, pcb_pin_t *pin, pcb_layer_id_t lid);


#endif
