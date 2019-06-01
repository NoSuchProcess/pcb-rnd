/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* common identifiers */

#ifndef	PCB_DATA_H
#define	PCB_DATA_H

#include "globalconst.h"
#include "global_typedefs.h"
#include "layer.h"
#include "crosshair.h"
#include "buffer.h"
#include "data_parent.h"
#include "obj_rat_list.h"
#include "obj_subc_list.h"
#include "obj_pstk_list.h"
#include "vtpadstack.h"
#include <genht/htip.h>

/* Generic container object that can hold subcircuits with layer-global
   objects (e.g. vias and rats) and layer-locals (lines, arcs) */
struct pcb_data_s {
	htip_t id2obj;                     /* long object ID -> (pcb_any_obj_t *) */
	int LayerN;                        /* number of layers in this board */

	pcb_vtpadstack_proto_t ps_protos;

	padstacklist_t padstack;
	pcb_subclist_t subc;
/**/
	pcb_rtree_t *padstack_tree, *subc_tree, *rat_tree;
	pcb_layer_t Layer[PCB_MAX_LAYER]; /* layer TODO: make this dynamic */
	pcb_plug_io_t *loader;
	ratlist_t Rat;

/**/
	pcb_parenttype_t parent_type;
	pcb_parent_t parent;

/* poly clip inhibit */
	int clip_inhibit; /* counter: >0 means we are in inhibit mode */
};

#define pcb_max_group(pcb) ((pcb)->LayerGroups.len)

/* DO NOT USE this macro, it is not PCB-safe */
#define pcb_max_layer (PCB->Data->LayerN)



extern pcb_buffer_t pcb_buffers[PCB_MAX_BUFFER];
extern int pcb_added_lines;
extern int pcb_layer_stack[PCB_MAX_LAYER];

extern pcb_bool pcb_bumped;

/****** callback based loops *****/

/* The functions returning int are called once when processing of a new layer
   or element starts, with enter=1. If they return non-zero, the current layer
   or element is skipped. If it is not skipped, the function is called once
   at the end of processing the given layer or element, with enter=0 (and
   return value ignored).

   Any of the callbacks for any loop function can be NULL.
   */

/* layer object callbacks */
typedef int (*pcb_layer_cb_t)(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, int enter);
typedef void (*pcb_line_cb_t)(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line);
typedef void (*pcb_arc_cb_t)(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc);
typedef void (*pcb_text_cb_t)(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_text_t *text);
typedef void (*pcb_poly_cb_t)(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_poly_t *poly);

/* subc callbacks */
typedef int (*pcb_subc_cb_t)(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter);

/* via and padstack callbacks */
typedef void (*pcb_pstk_cb_t)(void *ctx, pcb_board_t *pcb, pcb_pstk_t *ps);

/* Loop over all layer objects on each layer. Layer is the outer loop. */
void pcb_loop_layers(pcb_board_t *pcb, void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb);

/* Loop over all subcircuits. */
void pcb_loop_subc(pcb_board_t *pcb, void *ctx, pcb_subc_cb_t scb);

/* Loop over all design objects. (So all the above three in one call.) */
void pcb_loop_all(pcb_board_t *pcb, void *ctx,
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb,
	pcb_subc_cb_t scb,
	pcb_pstk_cb_t pscb
);

/* Initialize the fields of data */
void pcb_data_init(pcb_data_t *data);

/* Allocate new data and initialize all fields */
pcb_data_t *pcb_data_new(pcb_board_t *parent);

/* Uninitialize and free the fields of data (doesn't free data) */
void pcb_data_uninit(pcb_data_t *data);

/* Calls pcb_data_uninit() and free data */
void pcb_data_free(pcb_data_t *data);

pcb_bool pcb_data_is_empty(pcb_data_t *);

/* gets minimum and maximum coordinates
 * returns NULL if layout is empty */
pcb_box_t *pcb_data_bbox(pcb_box_t *out, pcb_data_t *Data, pcb_bool ignore_floaters);
pcb_box_t *pcb_data_bbox_naked(pcb_box_t *out, pcb_data_t *Data, pcb_bool ignore_floaters);

/* Make sure all layers of data has their .parent field pointing to the data */
void pcb_data_set_layer_parents(pcb_data_t *data);

/* Set up all data layers as bound layers to pcb's Data */
void pcb_data_bind_board_layers(pcb_board_t *pcb, pcb_data_t *data, int share_rtrees);

/* redo all layers of data to be bound layers (layer recipes) using the stackup
   from pcb4layer_groups. The new bound layers are not bound to any real layer. */
void pcb_data_make_layers_bound(pcb_board_t *pcb4layer_groups, pcb_data_t *data);

/* Recalculate the layer bindings updating meta.bound.real to new board layers */
void pcb_data_binding_update(pcb_board_t *pcb, pcb_data_t *data);

/* Break the binding in data bound layers (set htem all to NULL) */
void pcb_data_unbind_layers(pcb_data_t *data);

/* Make sure there are no negative coords in data, knowing the bbox of the data */
int pcb_data_normalize_(pcb_data_t *data, pcb_box_t *data_bbox);

/* Make sure there are no negative coords in data (calculates the bbox of the data) */
int pcb_data_normalize(pcb_data_t *data);

/* Returns the top level pcb related to a data, or NULL if the data is floating
   (e.g. is a global buffer) */
pcb_board_t *pcb_data_get_top(pcb_data_t *data);

/* Force-set the parent.data field of each global object in data (but not
   the layers!) */
void pcb_data_set_parent_globals(pcb_data_t *data, pcb_data_t *new_parent);

typedef enum pcb_data_mirror_text_e {
	PCB_TXM_NONE = 0, /* do not mirror text */
	PCB_TXM_SIDE = 1, /* mirror text, changing side */
	PCB_TXM_COORD    /* mirror text base coords only */
} pcb_data_mirror_text_t;
void pcb_data_mirror(pcb_data_t *data, pcb_coord_t y_offs, pcb_data_mirror_text_t mtxt, pcb_bool pstk_smirror);

void pcb_data_move(pcb_data_t *data, pcb_coord_t dx, pcb_coord_t dy);

/* Multiply x and y coords by sx and sy and thickness by sth (where applicable).
   If recurse is non-zero, also modify subcircuits */
void pcb_data_scale(pcb_data_t *data, double sx, double sy, double sth, int recurse);

/* run pcb_poly_init_clip() on all polygons in data */
void pcb_data_clip_polys(pcb_data_t *data);


/* rsearch on all trees matching types of data */
pcb_r_dir_t pcb_data_r_search(pcb_data_t *data, pcb_objtype_t types, const pcb_box_t *starting_region,
						 pcb_r_dir_t (*region_in_search) (const pcb_box_t *region, void *cl),
						 pcb_r_dir_t (*rectangle_in_region) (const pcb_box_t *box, void *cl),
						 void *closure, int *num_found, pcb_bool vis_only);

/* Either pcb->data or the subcircuit's data if PCB is a subc (footprint edit mode) */
#define PCB_REAL_DATA(pcb) \
	((pcb)->is_footprint ? (pcb_subclist_first(&(pcb)->Data->subc)->data) : ((pcb)->Data))


/*** Polygon clipping inhibit ***/

/* increase the inhibit counter (stop clipping polygons) */
void pcb_data_clip_inhibit_inc(pcb_data_t *data);

/* decrease the inhibit counter - if it's zero, reclip all dirty polygons;
   enable_progbar controls whether a progress bar is drawn for reclips
   that take longer than a few seconds. */
void pcb_data_clip_inhibit_dec(pcb_data_t *data, pcb_bool enable_progbar);

/* attempt to reclip all dirty polygons; normally called by
   pcb_data_clip_inhibit_dec(). */
void pcb_data_clip_dirty(pcb_data_t *data, pcb_bool enable_progbar);

/* Force reclip all polygons; useful after a move-everything kind
   of operation (e.g. autocrop()) */
void pcb_data_clip_all(pcb_data_t *data, pcb_bool enable_progbar);


/* Recursively change flags of data; how is one of pcb_change_flag_t */
void pcb_data_flag_change(pcb_data_t *data, pcb_objtype_t mask, int how, unsigned long flags);

/* Clear specific static flag from all objects, optionally with matching
   object types only. If redraw is not 0, issue an object redraw on change.
   If undoable is not 0, add flag changes to the undo list. Returns number
   of object changed. */
unsigned long pcb_data_clear_obj_flag(pcb_data_t *data, pcb_objtype_t tmask, unsigned long flag, int redraw, int undoable);
unsigned long pcb_data_clear_flag(pcb_data_t *data, unsigned long flag, int redraw, int undoable);

/* Clear the given dyflag bit from all objects under data */
void pcb_data_dynflag_clear(pcb_data_t *data, pcb_dynf_t dynf);

/* Convert name to data, return NULL if not found */
pcb_data_t *pcb_data_by_name(pcb_board_t *pcb, const char **name);

/* Convert data to name; either returns a static srting or fills in
   buf with dynamic data and returns buf. Buf needs to be at least 16
   bytes long. Returns NULL if data is not the pcb or a buffer. */
const char *pcb_data_to_name(pcb_board_t *pcb, pcb_data_t *data, char *buf, int buf_len);

#endif
