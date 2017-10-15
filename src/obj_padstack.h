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

#include "obj_padstack_list.h"
#include "layer.h"

typedef struct pcb_padstack_poly_s {
	unsigned int len;             /* number of points in polygon */
	pcb_point_t *pt;              /* ordered list of points */
	pcb_polyarea_t *pa;           /* cache for the poly code */
} pcb_padstack_poly_t;


typedef struct pcb_padstack_line_s {
	pcb_coord_t x1, y1, x2, y2, thickness;
	unsigned square:1;
} pcb_padstack_line_t;

typedef struct pcb_padstack_circ_s {
	pcb_coord_t dia;             /* diameter of the filled circle */
	pcb_coord_t x, y;            /* assymetric pads */
} pcb_padstack_circ_t;

typedef struct pcb_padstack_shape_s {
	pcb_layer_type_t layer_mask;
	pcb_layer_combining_t comb;
	union {
		pcb_padstack_poly_t poly;
		pcb_padstack_line_t line;
		pcb_padstack_circ_t circ;
	} data;
	enum {
		PCB_PSSH_POLY,
		PCB_PSSH_LINE,
		PCB_PSSH_CIRC                /* filled circle */
	} shape;
	pcb_coord_t clearance;         /* per layer clearance: internal layer clearance is sometimes different for production or insulation reasons (IPC2221A) */
} pcb_padstack_shape_t;

typedef struct pcb_padstack_proto_s {
	pcb_coord_t hdia;              /* if > 0, diameter of the hole (else there's no hole) */
	int htop, hbottom;             /* if hdia > 0, determine the hole's span, counted in copper layer groups from the top or bottom copper layer group */

	unsigned char len;             /* number of shapes (PCB_PADSTACK_MAX_SHAPES) */
	pcb_padstack_shape_t *shape;   /* list of layer-shape pairs */

	unsigned long hash;            /* optimization: linear search compare speeded up: go into detailed match only if hash matches */

	/* a proto can be derived from another proto via rotation/mirroring;
	   the above fields store the current state, after the transformations,
	   but we need to link the original we forked from; this is done by unique
	   group IDs. A brand new proto will get a new group ID, larger than any
	   existing group ID, a forked one will copy its parents group ID. */
	unsigned long group;
} pcb_padstack_proto_t;

/* allocate and return the next available group ID */
unsigned long pcb_padstack_alloc_group(pcb_data_t *data);

/*** hash ***/
unsigned int pcb_padstack_hash(const pcb_padstack_proto_t *p);
int pcb_padstack_eq(const pcb_padstack_proto_t *p1, const pcb_padstack_proto_t *p2);

#endif
