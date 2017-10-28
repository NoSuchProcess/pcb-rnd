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

#include "config.h"

#include "thermal.h"
#include "obj_pinvia_therm.h"

pcb_thermal_t pcb_thermal_str2bits(const char *str)
{
	/* shape */
	if (strcmp(str, "noshape") == 0) return PCB_THERMAL_NOSHAPE;
	if (strcmp(str, "round") == 0)   return PCB_THERMAL_ROUND;
	if (strcmp(str, "sharp") == 0)   return PCB_THERMAL_SHARP;
	if (strcmp(str, "solid") == 0)   return PCB_THERMAL_SOLID;

	/* orientation */
	if (strcmp(str, "diag") == 0)   return PCB_THERMAL_DIAGONAL;

	if (strcmp(str, "on") == 0)     return PCB_THERMAL_ON;

	return 0;
}

const char *pcb_thermal_bits2str(pcb_thermal_t *bit)
{
	if ((*bit) & PCB_THERMAL_ON) {
		*bit &= ~PCB_THERMAL_ON;
		return "on";
	}
	if ((*bit) & PCB_THERMAL_DIAGONAL) {
		*bit &= ~PCB_THERMAL_ON;
		return "diag";
	}
	if ((*bit) & 3) {
		switch((*bit) & 3) {
			*bit &= ~3;
			case PCB_THERMAL_NOSHAPE: return "noshape";
			case PCB_THERMAL_ROUND: return "round";
			case PCB_THERMAL_SHARP: return "sharp";
			case PCB_THERMAL_SOLID: return "solid";
			default: return NULL;
		}
	}
	return NULL;
}


pcb_polyarea_t *pcb_thermal_area_pin(pcb_board_t *pcb, pcb_pin_t *pin, pcb_layer_id_t lid)
{
	ThermPoly(pcb, pin, lid);
}

pcb_polyarea_t *pcb_thermal_area_line(pcb_board_t *pcb, pcb_line_t *line, pcb_layer_id_t lid)
{

}

pcb_polyarea_t *pcb_thermal_area(pcb_board_t *pcb, pcb_any_obj_t *obj, pcb_layer_id_t lid)
{
	switch(obj->type) {
		case PCB_OBJ_PIN:
		case PCB_OBJ_VIA:
			return pcb_thermal_area_pin(pcb, (pcb_pin_t *)obj, lid);

		case PCB_OBJ_LINE:
			return pcb_thermal_area_line(pcb, (pcb_line_t *)obj, lid);

		case PCB_OBJ_POLYGON:
		case PCB_OBJ_ARC:

		case PCB_OBJ_PADSTACK:
			break;

		default: break;
	}

	return NULL;
}

