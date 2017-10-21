/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
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
#include "obj_subc_list.h"
#include "vtpadstack.h"

#include "obj_all_list.h"

/* Generic container object that can hold subcircuits with layer-global
   objects (e.g. vias and rats) and layer-locals (lines, arcs) */
struct pcb_data_s {
	int LayerN;                        /* number of layers in this board */

	pcb_vtpadstack_proto_t ps_protos;
	unsigned long ps_next_grp;         /* next group number to allocate; if 0, have to figure */

	padstacklist_t padstack;
	pinlist_t Via;
	pcb_subclist_t subc;
	elementlist_t Element;
/**/
	pcb_rtree_t *via_tree, *padstack_tree, *subc_tree, *rat_tree;
	pcb_rtree_t *element_tree, *pin_tree, *pad_tree, *name_tree[3]; /* old element support */
	pcb_layer_t Layer[PCB_MAX_LAYER]; /* layer TODO: make this dynamic */
	pcb_plug_io_t *loader;
	ratlist_t Rat;

/**/
	pcb_parenttype_t parent_type;
	pcb_parent_t parent;
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

/* element callbacks */
typedef int (*pcb_element_cb_t)(void *ctx, pcb_board_t *pcb, pcb_element_t *element, int enter);
typedef void (*pcb_eline_cb_t)(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_line_t *line);
typedef void (*pcb_earc_cb_t)(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_arc_t *arc);
typedef void (*pcb_etext_cb_t)(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_text_t *text);
typedef void (*pcb_epin_cb_t)(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pin_t *pin);
typedef void (*pcb_epad_cb_t)(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pad_t *pad);

/* via callbacks */
typedef void (*pcb_via_cb_t)(void *ctx, pcb_board_t *pcb, pcb_pin_t *via);

/* Loop over all layer objects on each layer. Layer is the outer loop. */
void pcb_loop_layers(pcb_board_t *pcb, void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb);

/* Loop over all elements and element primitives. Element is the outer loop. */
void pcb_loop_elements(pcb_board_t *pcb, void *ctx, pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb);

/* Loop over all vias. */
void pcb_loop_vias(pcb_board_t *pcb, void *ctx, pcb_via_cb_t vcb);

/* Loop over all design objects. (So all the above three in one call.) */
void pcb_loop_all(pcb_board_t *pcb, void *ctx,
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb,
	pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb,
	pcb_via_cb_t vcb
);

pcb_data_t *pcb_data_new(pcb_board_t *parent);
void pcb_data_free(pcb_data_t *);
pcb_bool pcb_data_is_empty(pcb_data_t *);

/* gets minimum and maximum coordinates
 * returns NULL if layout is empty */
pcb_box_t *pcb_data_bbox(pcb_box_t *out, pcb_data_t *Data, pcb_bool ignore_floaters);

/* Make sure all layers of data has their .parent field pointing to the data */
void pcb_data_set_layer_parents(pcb_data_t *data);

/* Set up all data layers as bound layers to pcb's Data */
void pcb_data_bind_board_layers(pcb_board_t *pcb, pcb_data_t *data, int share_rtrees);

/* Recalculate the layer bindings updating meta.bound.real to new board layers */
void pcb_data_binding_update(pcb_board_t *pcb, pcb_data_t *data);

/* Make sure there are no negative coords in data, knowing the bbox of the data */
int pcb_data_normalize_(pcb_data_t *data, pcb_box_t *data_bbox);

/* Make sure there are no negative coords in data (calculates the bbox of the data) */
int pcb_data_normalize(pcb_data_t *data);

/* Returns the top level pcb related to a data, or NULL if the data is floating
   (e.g. is a global buffer) */
pcb_board_t *pcb_data_get_top(pcb_data_t *data);


void pcb_data_mirror(pcb_data_t *data, pcb_coord_t y_offs);

void pcb_data_move(pcb_data_t *data, pcb_coord_t dx, pcb_coord_t dy);

/* rsearch on all trees matching types of data */
pcb_r_dir_t pcb_data_r_search(pcb_data_t *data, pcb_objtype_t types, const pcb_box_t *starting_region,
						 pcb_r_dir_t (*region_in_search) (const pcb_box_t *region, void *cl),
						 pcb_r_dir_t (*rectangle_in_region) (const pcb_box_t *box, void *cl),
						 void *closure, int *num_found);

#endif
