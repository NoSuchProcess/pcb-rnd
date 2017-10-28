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

#include "compat_misc.h"
#include "obj_pinvia_therm.h"
#include "polygon.h"

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
	return ThermPoly(pcb, pin, lid);
}

static pcb_polyarea_t *pa_line_at(double x1, double y1, double x2, double y2, pcb_coord_t clr)
{
	pcb_line_t ltmp;

	ltmp.Flags = pcb_no_flags();
	ltmp.Point1.X = pcb_round(x1); ltmp.Point1.Y = pcb_round(y1);
	ltmp.Point2.X = pcb_round(x2); ltmp.Point2.Y = pcb_round(y2);
	return pcb_poly_from_line(&ltmp, clr);
}

static pcb_polyarea_t *pa_arc_at(double cx, double cy, double r, double e1x, double e1y, double e2x, double e2y, pcb_coord_t clr, double max_span_angle)
{
	double sa, ea, da;
	pcb_arc_t atmp;

	sa = atan2(-(e1y - cy), e1x - cx) * PCB_RAD_TO_DEG + 180.0;
	ea = atan2(-(e2y - cy), e2x - cx) * PCB_RAD_TO_DEG + 180.0;

/*	pcb_trace("sa=%f ea=%f diff=%f\n", sa, ea, ea-sa);*/

	atmp.Flags = pcb_no_flags();
	atmp.X = pcb_round(cx);
	atmp.Y = pcb_round(cy);

	da = ea-sa;
	if ((da < max_span_angle) && (da > -max_span_angle)) {
		atmp.StartAngle = sa;
		atmp.Delta = ea-sa;
	}
	else {
		atmp.StartAngle = ea;
		atmp.Delta = 360-ea+sa;
	}
	atmp.Width = atmp.Height = r;
	return pcb_poly_from_arc(&atmp, clr);
}

pcb_polyarea_t *pcb_thermal_area_line(pcb_board_t *pcb, pcb_line_t *line, pcb_layer_id_t lid)
{
	pcb_polyarea_t *pa, *pb, *pc;
	double dx, dy, len, vx, vy, nx, ny, clr, clrth, x1, y1, x2, y2, mx, my;

	if ((x1 == line->Point2.X) && (line->Point1.Y == line->Point2.Y)) {
		/* conrer case zero-long line is a circle: do the same as for vias */
#warning thermal TODO
		abort();
	}

	x1 = line->Point1.X;
	y1 = line->Point1.Y;
	x2 = line->Point2.X;
	y2 = line->Point2.Y;
	mx = (x1+x2)/2.0;
	my = (y1+y2)/2.0;
	dx = x1 - x2;
	dy = y1 - y2;

	len = sqrt(dx*dx + dy*dy);
	vx = dx / len;
	vy = dy / len;
	nx = -vy;
	ny = vx;

	clr = line->Clearance / 2;
	clrth = (line->Clearance/2 + line->Thickness) / 2;

	assert(line->thermal & PCB_THERMAL_ON); /* caller should have checked this */
	switch(line->thermal & 3) {
		case PCB_THERMAL_NOSHAPE:
		case PCB_THERMAL_SOLID: return 0;

		case PCB_THERMAL_ROUND:
			if (line->thermal & PCB_THERMAL_DIAGONAL) {
				/* side clear lines */
				pa = pa_line_at(
					x1 - clrth * nx - clr * vx * 0.75, y1 - clrth * ny - clr * vy * 0.75,
					x2 - clrth * nx + clr * vx * 0.75, y2 - clrth * ny + clr * vy * 0.75,
					clr);
				pb = pa_line_at(
					x1 + clrth * nx - clr * vx * 0.75, y1 + clrth * ny - clr * vy * 0.75,
					x2 + clrth * nx + clr * vx * 0.75, y2 + clrth * ny + clr * vy * 0.75,
					clr);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				/* x1;y1 cap arc */
				pa = pc; pc = NULL;
				pb = pa_arc_at(x1, y1, clrth,
					x1 - clrth * nx + clr * vx * 2.0, y1 - clrth * ny + clr * vy * 2.0,
					x1 + clrth * nx + clr * vx * 2.0, y1 + clrth * ny + clr * vy * 2.0,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				/* x2;y2 cap arc */
				pa = pc; pc = NULL;
				pb = pa_arc_at(x2, y2, clrth,
					x2 - clrth * nx - clr * vx * 2.0, y2 - clrth * ny - clr * vy * 2.0,
					x2 + clrth * nx - clr * vx * 2.0, y2 + clrth * ny - clr * vy * 2.0,
					clr, 180.0);
				pcb_polyarea_boolean_free(pa, pb, &pc, PCB_PBO_UNITE);

				return pc;
			}
			
		case PCB_THERMAL_SHARP:
			break;
	}
	return NULL;
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

