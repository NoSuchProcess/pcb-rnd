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

#ifndef PCB_OBJ_PSTK_STRUCT_DECLARED
#define PCB_OBJ_PSTK_STRUCT_DECLARED

#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include "obj_common.h"

/* The actual padstack is just a reference to a padstack proto within the same data */
struct pcb_pstk_s {
#define thermal thermal_dont_use
	PCB_ANY_PRIMITIVE_FIELDS;
#undef thermal
	pcb_cardinal_t proto;          /* reference to a pcb_pstk_proto_t within pcb_data_t */
	int protoi;                    /* index of the transformed proto; -1 means invalid; local cache, not saved */
	pcb_coord_t x, y;
	pcb_coord_t Clearance;
	double rot;                    /* rotation angle */
	char xmirror, smirror;
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
	char *name;                    /* optional user assigned name (or NULL) */
	pcb_coord_t hdia;              /* if > 0, diameter of the hole (else there's no hole) */
	int htop, hbottom;             /* if hdia > 0, determine the hole's span, counted in copper layer groups from the top or bottom copper layer group */

	pcb_vtpadstack_tshape_t tr;    /* [0] is the canonical prototype with rot=0, xmirror=0 and smirror=0; the rest is an unordered list of transformed entries */

	/* local cache - not saved */
	unsigned long hash;            /* optimization: linear search compare speeded up: go into detailed match only if hash matches */
	pcb_data_t *parent;
	int mech_idx;                  /* -1 or index to the first shape[] that is of PCB_LYT_MECH */
} pcb_pstk_proto_t;

/* Whether a proto cuts through board layers (has a hole or slot) */
#define PCB_PSTK_PROTO_CUTS(proto) (((proto)->hdia > 0) || ((proto)->mech_idx >= 0))

/* Whether a proto cuts through board layers (has a hole or slot) and connects
   layers with conductive material */
#define PCB_PSTK_PROTO_PLATES(proto) (((proto)->hplated) && (((proto)->hdia > 0) || ((proto)->mech_idx >= 0)))


pcb_pstk_t *pcb_pstk_alloc(pcb_data_t *data);
pcb_pstk_t *pcb_pstk_alloc_id(pcb_data_t *data, long int id);
void pcb_pstk_free(pcb_pstk_t *ps);
pcb_pstk_t *pcb_pstk_new(pcb_data_t *data, long int id, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags);
pcb_pstk_t *pcb_pstk_new_tr(pcb_data_t *data, long int id, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags, double rot, int xmirror, int smirror);
void pcb_pstk_add(pcb_data_t *data, pcb_pstk_t *ps);
void pcb_pstk_bbox(pcb_pstk_t *ps);
void pcb_pstk_copper_bbox(pcb_box_t *dst, pcb_pstk_t *ps);

void pcb_pstk_reg(pcb_data_t *data, pcb_pstk_t *pstk);
void pcb_pstk_unreg(pcb_pstk_t *pstk);


void pcb_pstk_pre(pcb_pstk_t *pstk);
void pcb_pstk_post(pcb_pstk_t *pstk);

/* hash and eq */
int pcb_pstk_eq(const pcb_host_trans_t *tr1, const pcb_pstk_t *p1, const pcb_host_trans_t *tr2, const pcb_pstk_t *p2);
unsigned int pcb_pstk_hash(const pcb_host_trans_t *tr, const pcb_pstk_t *p);


void pcb_pstk_set_thermal(pcb_pstk_t *ps, unsigned long lid, unsigned char shape);
unsigned char *pcb_pstk_get_thermal(pcb_pstk_t *ps, unsigned long lid, pcb_bool_t alloc);

pcb_pstk_t *pcb_pstk_by_id(pcb_data_t *base, long int ID);

/* Undoably change the instance parameters of a padstack ref */
int pcb_pstk_change_instance(pcb_pstk_t *ps, pcb_cardinal_t *proto, const pcb_coord_t *clearance, double *rot, int *xmirror, int *smirror);

/* Return whether a group is empty (free of padstack shapes) */
pcb_bool pcb_pstk_is_group_empty(pcb_board_t *pcb, pcb_layergrp_id_t gid);

/* Copy all metadata (attributes, thermals, etc.) */
pcb_pstk_t *pcb_pstk_copy_meta(pcb_pstk_t *dst, pcb_pstk_t *src);

/* Copy orientation information (rotatioin and mirror) of an instance */
pcb_pstk_t *pcb_pstk_copy_orient(pcb_pstk_t *dst, pcb_pstk_t *src);

/* Clearance of the padstack on a given layer */
pcb_coord_t obj_pstk_get_clearance(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layer_t *layer);

/*** proto ***/

/* Convert selection or current buffer to padstack; returns PCB_PADSTACK_INVALID
   on error; looks for existing matching protos to avoid adding redundant
   entries */
pcb_cardinal_t pcb_pstk_conv_selection(pcb_board_t *pcb, int quiet, pcb_coord_t ox, pcb_coord_t oy);
pcb_cardinal_t pcb_pstk_conv_buffer(int quiet);

/* Low level converter: take an array of (pcb_any_obj_t *) objs and convert
   them into shapes of the dst proto. Does not remove input objects. */
int pcb_pstk_proto_conv(pcb_data_t *data, pcb_pstk_proto_t *dst, int quiet, vtp0_t *objs, pcb_coord_t ox, pcb_coord_t oy);

/* Break up src into discrete non-pstk objects placed in dst, one object
   per layer type. The hole is ignored. If remove_src is true, also remove
   src padstack. */
int pcb_pstk_proto_breakup(pcb_data_t *dst, pcb_pstk_t *src, pcb_bool remove_src);

/* free fields of a proto (not freeing the proto itself, not removing it from lists */
void pcb_pstk_proto_free_fields(pcb_pstk_proto_t *dst);

/* low level: allocate a new, uninitialized shape at the end of a transformed shape
   WARNING: this should be done on all transformed shapes in parallel! */
pcb_pstk_shape_t *pcb_pstk_alloc_append_shape(pcb_pstk_tshape_t *ts);

/* allocate/free the point array of a poly shape (single allocation for x and y) */
void pcb_pstk_shape_alloc_poly(pcb_pstk_poly_t *poly, int len);
void pcb_pstk_shape_free_poly(pcb_pstk_poly_t *poly);

/* geometry for select.c and search.c; if layer is NULL, consider all shapes */
int pcb_pstk_near_box(pcb_pstk_t *ps, pcb_box_t *box, pcb_layer_t *layer);
int pcb_is_point_in_pstk(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_pstk_t *ps, pcb_layer_t *layer);

/* Check if padstack has the proper clearance against polygon; returns 0 if everything's fine */
int pcb_pstk_drc_check_clearance(pcb_pstk_t *ps, pcb_poly_t *polygon, pcb_coord_t min_clr);

/* Check all possible local drc errors regarding to pad stacks - errors that
   depend only on the padstack, not on other objects. load err_minring and
   err_minhole with the relevant data for the report when ring or hole rules
   are violated */
void pcb_pstk_drc_check_and_warn(pcb_pstk_t *ps, pcb_coord_t *err_minring, pcb_coord_t *err_minhole);

/* Generate poly->pa (which should be NULL at the time of call) */
void pcb_pstk_shape_update_pa(pcb_pstk_poly_t *poly);

/* Insert proto into the cache of data; if it's already in, return the existing
   ID, else dup it and insert it. The forcedup variant always dups.
   WARNING: make sure pcb_pstk_proto_update() was called on proto some point
   in time before this call, esle the hash is invalid. */
pcb_cardinal_t pcb_pstk_proto_insert_dup(pcb_data_t *data, const pcb_pstk_proto_t *proto, int quiet);
pcb_cardinal_t pcb_pstk_proto_insert_forcedup(pcb_data_t *data, const pcb_pstk_proto_t *proto, int quiet);

/* Change the non-NULL hole properties of a padstack proto; undoable.
   Returns 0 on success. */
int pcb_pstk_proto_change_hole(pcb_pstk_proto_t *proto, const int *hplated, const pcb_coord_t *hdia, const int *htop, const int *hbottom);

/* Change the name of a padstack proto; not yet undoable.
   Returns 0 on success. */
int pcb_pstk_proto_change_name(pcb_pstk_proto_t *proto, const char *new_name);

/* Find or create a new transformed version of an existing proto */
pcb_pstk_tshape_t *pcb_pstk_make_tshape(pcb_data_t *data, pcb_pstk_proto_t *proto, double rot, int xmirror, int smirror, int *out_protoi);

/* Deep copy a prototype or shape */
void pcb_pstk_proto_copy(pcb_pstk_proto_t *dst, const pcb_pstk_proto_t *src);
void pcb_pstk_shape_copy(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *src);

/* free all fields of a shape */
void pcb_pstk_shape_free(pcb_pstk_shape_t *s);

/* grow (or shrink) a prototype to or by val - change the proto in place */
void pcb_pstk_proto_grow(pcb_pstk_proto_t *proto, pcb_bool is_absolute, pcb_coord_t val);
void pcb_pstk_shape_grow(pcb_pstk_shape_t *shp, pcb_bool is_absolute, pcb_coord_t val);
void pcb_pstk_shape_clr_grow(pcb_pstk_shape_t *shp, pcb_bool is_absolute, pcb_coord_t val);
void pcb_pstk_shape_scale(pcb_pstk_shape_t *shp, double sx, double sy);

/* Derive (copy and bloat) the shape at dst_idx from src_idx; set the mask and comb
   for the new shape. If dst_idx is -1, allocate the new shape */
void pcb_pstk_shape_derive(pcb_pstk_proto_t *proto, int dst_idx, int src_idx, pcb_coord_t bloat, pcb_layer_type_t mask, pcb_layer_combining_t comb);

/* Swap the layer definition of two shapes within a prototype; returns 0 on success */
int pcb_pstk_shape_swap_layer(pcb_pstk_proto_t *proto, int idx1, int idx2);

/* Create a new hshadow shape (in all transformed shape as well) */
void pcb_pstk_shape_add_hshadow(pcb_pstk_proto_t *proto, pcb_layer_type_t mask, pcb_layer_combining_t comb);


/* Look up the shape index corresponding to a lty/comb; returns -1 if not found/empty */
int pcb_pstk_get_shape_idx(pcb_pstk_tshape_t *ts, pcb_layer_type_t lyt, pcb_layer_combining_t comb);

/* Remove a shape from the proto (either by layer or by idx) */
void pcb_pstk_proto_del_shape(pcb_pstk_proto_t *proto, pcb_layer_type_t lyt, pcb_layer_combining_t comb);
void pcb_pstk_proto_del_shape_idx(pcb_pstk_proto_t *proto, int idx);

#define PCB_PSTK_DONT_MIRROR_COORDS PCB_MAX_COORD
/* Mirror a padstack (useful for sending to the other side - set swap_side to 1 in that case)
   Disabling xmirror is useful if side needs to be swapped but coordinates
   are already mirrored so they represent the other-side geometry (e.g. when
   importing from old pcb formats). If y_offs is PCB_PSTK_DONT_MIRROR_COORDS,
   do not change the y coord */
void pcb_pstk_mirror(pcb_pstk_t *ps, pcb_coord_t y_offs, int swap_side, int disable_xmirror);

/* Create a new proto and scale it for the padstack; center x and y are scaled too */
void pcb_pstk_scale(pcb_pstk_t *ps, double sx, double sy);


/* Rotate in place (op wrapper) */
void pcb_pstk_rotate90(pcb_pstk_t *pstk, pcb_coord_t cx, pcb_coord_t cy, int steps);
void pcb_pstk_rotate(pcb_pstk_t *pstk, pcb_coord_t cx, pcb_coord_t cy, double cosa, double sina, double angle);

/* High level move (op wrapper; no undo) */
void pcb_pstk_move(pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy, pcb_bool more_to_come);

/* Low level move - updates only the coordinates and the bbox */
void pcb_pstk_move_(pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy);

/* Temporary hack until we have a refcounted objects and ID->pcb_any_obj_t hash */
extern pcb_data_t *pcb_pstk_data_hack;

/* Insert a proto in data and return the proto-id. If the proto is already
   in data, the fields of the caller's version are free'd, else they are
   copied into data. In any case, the caller should not free proto. */
pcb_cardinal_t pcb_pstk_proto_insert_or_free(pcb_data_t *data, pcb_pstk_proto_t *proto, int quiet);

/* Update caches and hash - must be called after any change to the prototype */
void pcb_pstk_proto_update(pcb_pstk_proto_t *dst);

/* Overwrite all fields of a proto in-place; returns the id or INVALID on error */
pcb_cardinal_t pcb_pstk_proto_replace(pcb_data_t *data, pcb_cardinal_t proto_id, const pcb_pstk_proto_t *src);

/* Cycle through all (first level) padstacks of data and count how many times
   each prototype is referenced by them. The result is returned as an array
   of counts per prototype ID; the array is as large as data's prototype array.
   len_out is always filled with the length of the array. If the length is 0,
   NULL is returned. The caller needs to call free() on the returned array. */
pcb_cardinal_t *pcb_pstk_proto_used_all(pcb_data_t *data, pcb_cardinal_t *len_out);

/* Remove a prototype: free all fields and mark it unused */
void pcb_pstk_proto_del(pcb_data_t *data, pcb_cardinal_t proto_id);

/*** layer info ***/
typedef struct pcb_proto_layer_s {
	const char *name;
	pcb_layer_type_t mask;
	pcb_layer_combining_t comb;

	int auto_from[2];
	pcb_coord_t auto_bloat;
} pcb_proto_layer_t;

#define PCB_PROTO_MASK_BLOAT PCB_MIL_TO_COORD(2*3)

#define pcb_proto_num_layers 8
const pcb_proto_layer_t pcb_proto_layers[pcb_proto_num_layers];


/* Return the id of a board layer that matches a mask:comb pair or invalid if
   nothing matched */
pcb_layer_id_t pcb_proto_board_layer_for(const pcb_data_t *data, pcb_layer_type_t mask, pcb_layer_combining_t comb);

/*** hash ***/
unsigned int pcb_pstk_proto_hash(const pcb_pstk_proto_t *p);
int pcb_pstk_proto_eq(const pcb_pstk_proto_t *p1, const pcb_pstk_proto_t *p2);

int pcb_pstk_shape_eq(const pcb_pstk_shape_t *sh1, const pcb_pstk_shape_t *sh2);

/*** loops ***/
#define PCB_PADSTACK_LOOP(top) do {                                \
  pcb_pstk_t *padstack;                                        \
  gdl_iterator_t __it__;                                           \
  padstacklist_foreach(&(top)->padstack, &__it__, padstack) {

#endif
#endif
