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

#ifndef PCB_OBJ_PADSTACK_STRUCT_DECLARED
#define PCB_OBJ_PADSTACK_STRUCT_DECLARED

#include "obj_common.h"

/* The actual padstack is just a reference to a padstack proto within the same data */
struct pcb_padstack_s {
	PCB_ANYOBJECTFIELDS;
	pcb_cardinal_t proto;          /* reference to a pcb_padstack_proto_t within pcb_data_t */
	pcb_coord_t x, y;
	struct {
		unsigned long used;
		unsigned char *shape;        /* indexed by layer ID */
	} thermal;
	gdl_elem_t link;               /* a padstack is in a list in pcb_data_t as a global object */
};
#endif

#ifndef PCB_PADSTACK_STRUCT_ONLY
#ifndef PCB_OBJ_PADSTACK_H
#define PCB_OBJ_PADSTACK_H

#define PCB_PADSTACK_MAX_SHAPES 31

#define PCB_PADSTACK_INVALID ((pcb_cardinal_t)(-1))

#include "obj_common.h"

#include "layer.h"

typedef struct pcb_padstack_poly_s {
	unsigned int len;             /* number of points in polygon */
	pcb_coord_t *x;               /* ordered list of points, X coord */
	pcb_coord_t *y;               /* ordered list of points, X coord */
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
	unsigned in_use:1;             /* 1 if the slot is in use */

	unsigned hplated:1;            /* if > 0, whether the hole is plated */
	pcb_coord_t hdia;              /* if > 0, diameter of the hole (else there's no hole) */
	int htop, hbottom;             /* if hdia > 0, determine the hole's span, counted in copper layer groups from the top or bottom copper layer group */

	unsigned char len;             /* number of shapes (PCB_PADSTACK_MAX_SHAPES) */
	pcb_padstack_shape_t *shape;   /* list of layer-shape pairs */

	/* a proto can be derived from another proto via rotation/mirroring;
	   the above fields store the current state, after the transformations,
	   but we need to link the original we forked from; this is done by unique
	   group IDs. A brand new proto will get a new group ID, larger than any
	   existing group ID, a forked one will copy its parents group ID. */
	unsigned long group;

	/* local cache - not saved */
	unsigned long hash;            /* optimization: linear search compare speeded up: go into detailed match only if hash matches */
	pcb_data_t *parent;
} pcb_padstack_proto_t;

pcb_padstack_t *pcb_padstack_alloc(pcb_data_t *data);
void pcb_padstack_free(pcb_padstack_t *ps);
pcb_padstack_t *pcb_padstack_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_flag_t Flags);
void pcb_padstack_add(pcb_data_t *data, pcb_padstack_t *ps);
void pcb_padstack_bbox(pcb_padstack_t *ps);

void pcb_padstack_set_thermal(pcb_padstack_t *ps, unsigned long lid, unsigned char shape);

/*** proto ***/

/* allocate and return the next available group ID */
unsigned long pcb_padstack_alloc_group(pcb_data_t *data);

/* Convert selection or current buffer to padstack; returns PCB_PADSTACK_INVALID
   on error; looks for existing matching protos to avoid adding redundant
   entries */
pcb_cardinal_t pcb_padstack_conv_selection(pcb_board_t *pcb, int quiet, pcb_coord_t ox, pcb_coord_t oy);
pcb_cardinal_t pcb_padstack_conv_buffer(int quiet);

/* free fields of a proto (not freeing the proto itself, not removing it from lists */
void pcb_padstack_proto_free_fields(pcb_padstack_proto_t *dst);

/* allocate the point array of a poly shape (single allocation for x and y) */
void pcb_padstack_shape_alloc_poly(pcb_padstack_poly_t *poly, int len);

/* geometry for select.c and search.c */
int pcb_padstack_near_box(pcb_padstack_t *ps, pcb_box_t *box);
int pcb_is_point_in_padstack(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_padstack_t *ps);

/* Check if padstack has the proper clearance against polygon; returns 0 if everything's fine */
int pcb_padstack_drc_check_clearance(pcb_padstack_t *ps, pcb_poly_t *polygon, pcb_coord_t min_clr);

/* Check all possible local drc errors regarding to pad stacks - errors that
   depend only on the padstack, not on other objects. Return 0 if everything
   was fine */
int pcb_padstack_drc_check_and_warn(pcb_padstack_t *ps);

/* Generate poly->pa (which should be NULL at the time of call) */
void pcb_padstack_shape_update_pline(pcb_padstack_poly_t *poly);

/* Insert proto into the cache of data; if it's already in, return the existing
   ID, else dup it and insert it. */
pcb_cardinal_t pcb_padstack_proto_insert_dup(pcb_data_t *data, const pcb_padstack_proto_t *proto, int quiet);

/*** hash ***/
unsigned int pcb_padstack_hash(const pcb_padstack_proto_t *p);
int pcb_padstack_eq(const pcb_padstack_proto_t *p1, const pcb_padstack_proto_t *p2);

/*** loops ***/
#define PCB_PADSTACK_LOOP(top) do {                                \
  pcb_padstack_t *padstack;                                        \
  gdl_iterator_t __it__;                                           \
  padstacklist_foreach(&(top)->padstack, &__it__, padstack) {

#endif
#endif
