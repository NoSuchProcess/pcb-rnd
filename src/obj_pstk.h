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

#ifndef PCB_OBJ_PSTK_STRUCT_DECLARED
#define PCB_OBJ_PSTK_STRUCT_DECLARED

#include "obj_common.h"

/* The actual padstack is just a reference to a padstack proto within the same data */
struct pcb_pstk_s {
#define thermal thermal_dont_use
	PCB_ANYOBJECTFIELDS;
#undef thermal
	pcb_cardinal_t proto;          /* reference to a pcb_pstk_proto_t within pcb_data_t */
	int protoi;                    /* index of the transformed proto; -1 means invalid; local cache, not saved */
	pcb_coord_t x, y;
	pcb_coord_t Clearance;
	double rot;                    /* rotation angle */
	int xmirror;
	struct {
		unsigned long used;
		unsigned char *shape;        /* indexed by layer ID */
	} thermals;
	gdl_elem_t link;               /* a padstack is in a list in pcb_data_t as a global object */
};
#endif

#ifndef PCB_PADSTACK_STRUCT_ONLY
#ifndef PCB_OBJ_PSTK_H
#define PCB_OBJ_PSTK_H

#define PCB_PADSTACK_MAX_SHAPES 31

#define PCB_PADSTACK_INVALID ((pcb_cardinal_t)(-1))

#include "layer.h"
#include "obj_common.h"
#include "obj_pstk_shape.h"
#include "vtpadstack_t.h"

typedef struct pcb_pstk_proto_s {
	unsigned in_use:1;             /* 1 if the slot is in use */

	unsigned hplated:1;            /* if > 0, whether the hole is plated */
	pcb_coord_t hdia;              /* if > 0, diameter of the hole (else there's no hole) */
	int htop, hbottom;             /* if hdia > 0, determine the hole's span, counted in copper layer groups from the top or bottom copper layer group */

	pcb_vtpadstack_tshape_t tr;    /* [0] is the canonical prototype with rot=0 and xmirror=0; the rest is an unordered list of transformed entries */

	/* local cache - not saved */
	unsigned long hash;            /* optimization: linear search compare speeded up: go into detailed match only if hash matches */
	pcb_data_t *parent;
} pcb_pstk_proto_t;



pcb_pstk_t *pcb_pstk_alloc(pcb_data_t *data);
void pcb_pstk_free(pcb_pstk_t *ps);
pcb_pstk_t *pcb_pstk_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags);
void pcb_pstk_add(pcb_data_t *data, pcb_pstk_t *ps);
void pcb_pstk_bbox(pcb_pstk_t *ps);

void pcb_pstk_set_thermal(pcb_pstk_t *ps, unsigned long lid, unsigned char shape);
unsigned char *pcb_pstk_get_thermal(pcb_pstk_t *ps, unsigned long lid, pcb_bool_t alloc);

pcb_pstk_t *pcb_pstk_by_id(pcb_data_t *base, long int ID);

/* Undoably change the instance parameters of a padstack ref */
int pcb_pstk_change_instance(pcb_pstk_t *ps, pcb_cardinal_t *proto, const pcb_coord_t *clearance, double *rot, int *xmirror);

/* Return whether a group is empty (free of padstack shapes) */
pcb_bool pcb_pstk_is_group_empty(pcb_board_t *pcb, pcb_layergrp_id_t gid);


/*** proto ***/

/* Convert selection or current buffer to padstack; returns PCB_PADSTACK_INVALID
   on error; looks for existing matching protos to avoid adding redundant
   entries */
pcb_cardinal_t pcb_pstk_conv_selection(pcb_board_t *pcb, int quiet, pcb_coord_t ox, pcb_coord_t oy);
pcb_cardinal_t pcb_pstk_conv_buffer(int quiet);

/* free fields of a proto (not freeing the proto itself, not removing it from lists */
void pcb_pstk_proto_free_fields(pcb_pstk_proto_t *dst);

/* allocate/free the point array of a poly shape (single allocation for x and y) */
void pcb_pstk_shape_alloc_poly(pcb_pstk_poly_t *poly, int len);
void pcb_pstk_shape_free_poly(pcb_pstk_poly_t *poly);

/* geometry for select.c and search.c; if layer is NULL, consider all shapes */
int pcb_pstk_near_box(pcb_pstk_t *ps, pcb_box_t *box, pcb_layer_t *layer);
int pcb_is_point_in_pstk(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_pstk_t *ps, pcb_layer_t *layer);

/* Check if padstack has the proper clearance against polygon; returns 0 if everything's fine */
int pcb_pstk_drc_check_clearance(pcb_pstk_t *ps, pcb_poly_t *polygon, pcb_coord_t min_clr);

/* Check all possible local drc errors regarding to pad stacks - errors that
   depend only on the padstack, not on other objects. Return 0 if everything
   was fine */
int pcb_pstk_drc_check_and_warn(pcb_pstk_t *ps);

/* Generate poly->pa (which should be NULL at the time of call) */
void pcb_pstk_shape_update_pa(pcb_pstk_poly_t *poly);

/* Insert proto into the cache of data; if it's already in, return the existing
   ID, else dup it and insert it. */
pcb_cardinal_t pcb_pstk_proto_insert_dup(pcb_data_t *data, const pcb_pstk_proto_t *proto, int quiet);

/* Change the non-NULL hole properties of a padstack proto; undoable.
   Returns 0 on success. */
int pcb_pstk_proto_change_hole(pcb_pstk_proto_t *proto, const int *hplated, const pcb_coord_t *hdia, const int *htop, const int *hbottom);

/* Find or create a new transformed version of an existing proto */
pcb_pstk_tshape_t *pcb_pstk_make_tshape(pcb_data_t *data, pcb_pstk_proto_t *proto, double rot, int xmirror, int *out_protoi);

/* Deep copy a prototype or shape */
void pcb_pstk_proto_copy(pcb_pstk_proto_t *dst, const pcb_pstk_proto_t *src);
void pcb_pstk_shape_copy(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *src);

/* free all fields of a shape */
void pcb_pstk_shape_free(pcb_pstk_shape_t *s);

/* grow (or shrink) a prototype to or by val - change the proto in place */
void pcb_pstk_proto_grow(pcb_pstk_proto_t *proto, pcb_bool is_absolute, pcb_coord_t val);

/* Look up the shape index corresponding to a lty/comb; returns -1 if not found/empty */
int pcb_pstk_get_shape_idx(pcb_pstk_tshape_t *ts, pcb_layer_type_t lyt, pcb_layer_combining_t comb);

/* Remove a shape from the proto (either by layer or by idx) */
void pcb_pstk_proto_del_shape(pcb_pstk_proto_t *proto, pcb_layer_type_t lyt, pcb_layer_combining_t comb);
void pcb_pstk_proto_del_shape_idx(pcb_pstk_proto_t *proto, int idx);


/*** hash ***/
unsigned int pcb_pstk_hash(const pcb_pstk_proto_t *p);
int pcb_pstk_eq(const pcb_pstk_proto_t *p1, const pcb_pstk_proto_t *p2);

/*** loops ***/
#define PCB_PADSTACK_LOOP(top) do {                                \
  pcb_pstk_t *padstack;                                        \
  gdl_iterator_t __it__;                                           \
  padstacklist_foreach(&(top)->padstack, &__it__, padstack) {

#endif
#endif
