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

#ifndef PCB_OBJ_PADSTACK_H
#define PCB_OBJ_PADSTACK_H

#include "layer.h"

typedef struct pcb_padstack_poly_s {
	unsigned int len;             /* number of points in polygon */
	pcb_point_t *pt;              /* ordered list of points */
	pcb_polyarea_t *pa;           /* cache for the poly code */
} pcb_padstack_poly_t;


typedef struct pcb_padstack_line_s {
	pcb_coord_t length, thickness;
	unsigned square:1;
} pcb_padstack_line_t;

typedef struct pcb_padstack_shape_s {
	pcb_layer_type_t layer_mask;
	pcb_layer_combining_t comb;
	union {
		pcb_padstack_poly_t poly;
		pcb_padstack_line_t line;
		pcb_coord_t dia;             /* diameter of the filled circle */
	} data;
	enum {
		PCB_PSSH_POLY,
		PCB_PSSH_LINE,
		PCB_PSSH_FILLCIRCLE
	} shape;
	pcb_coord_t clearance;         /* per layer clearance: internal layer clearance is sometimes different for production or insulation reasons (IPC2221A) */
} pcb_padstack_shape_t;

typedef struct pcb_padstack_proto_s {
	pcb_coord_t hdia;              /* if > 0, diameter of the hole (else there's no hole) */
	int htop, hbottom;             /* if hdia > 0, determine the hole's span, counted in copper layers from the top or bottom copper layer */

	unsigned char len;             /* number of shapes */
	pcb_padstack_shape_t *shapes;  /* list of layer-shape pairs */
} pcb_padstack_proto_t;

#include "obj_common.h"

struct pcb_padstack_s {
	PCB_ANYOBJECTFIELDS;
	pcb_cardinal_t proto;          /* reference to a pcb_padstack_proto_t within pcb_data_t */
	pcb_coord_t x, y;
	struct {
		int used;
		char *shape;                 /* indexed by layer ID */
	} thermal;
};

#endif
