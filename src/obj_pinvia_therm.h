/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
 *
 *  this file was written by and is
 *  (C) Copyright 2006, harry eaton
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

/* prototypes for thermal routines
 *
 * Thermals are normal lines on the layout. The only thing unique
 * about them is that they have the PCB_FLAG_USETHERMAL set so that they
 * can be identified as thermals. It is handy for pcb to automatically
 * make adjustments to the thermals when the user performs certain
 * operations, and the functions in thermal.h help implement that.
 */

#ifndef	PCB_PINVIA_THERMAL_H
#define	PCB_PINVIA_THERMAL_H

#include <stdlib.h>
#include "config.h"
#include "polyarea.h"
#include "polygon.h"

pcb_polyarea_t *ThermPoly(pcb_board_t *, pcb_pin_t *, pcb_cardinal_t);

static inline PCB_FUNC_UNUSED pcb_polyarea_t *pcb_pa_diag_line(pcb_coord_t X, pcb_coord_t Y, pcb_coord_t l, pcb_coord_t w, pcb_bool rt)
{
	pcb_pline_t *c;
	pcb_vector_t v;
	pcb_coord_t x1, x2, y1, y2;

	if (rt) {
		x1 = (l - w) * M_SQRT1_2;
		x2 = (l + w) * M_SQRT1_2;
		y1 = x1;
		y2 = x2;
	}
	else {
		x2 = -(l - w) * M_SQRT1_2;
		x1 = -(l + w) * M_SQRT1_2;
		y1 = -x1;
		y2 = -x2;
	}

	v[0] = X + x1;
	v[1] = Y + y2;
	if ((c = pcb_poly_contour_new(v)) == NULL)
		return NULL;
	v[0] = X - x2;
	v[1] = Y - y1;
	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	v[0] = X - x1;
	v[1] = Y - y2;
	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	v[0] = X + x2;
	v[1] = Y + y1;
	pcb_poly_vertex_include(c->head.prev, pcb_poly_node_create(v));
	return pcb_poly_from_contour(c);
}


#endif
