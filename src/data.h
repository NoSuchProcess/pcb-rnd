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

#include "obj_all_list.h"

/* Generic container object that can hold subcircuits with layer-global
   objects (e.g. vias and rats) and layer-locals (lines, arcs) */
struct pcb_data_s {
	int LayerN;                        /* number of layers in this board */
	pinlist_t Via;
	elementlist_t Element;
/**/
	pcb_rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
	 *rat_tree;
	pcb_board_t *pcb;
	pcb_layer_t Layer[MAX_LAYER + 2];    /* add 2 silkscreen layers */
	pcb_plug_io_t *loader;
	ratlist_t Rat;
};


#define pcb_max_group (PCB->Data->LayerN)
#define pcb_max_copper_layer (PCB->Data->LayerN)
#define pcb_solder_silk_layer (pcb_max_copper_layer + SOLDER_LAYER)
#define pcb_component_silk_layer (pcb_max_copper_layer + COMPONENT_LAYER)

extern pcb_buffer_t pcb_buffers[MAX_BUFFER];
extern int pcb_added_lines;
extern int pcb_layer_stack[MAX_LAYER];

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
typedef void (*pcb_poly_cb_t)(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly);

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
void pcb_loop_layers(void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb);

/* Loop over all elements and element primitives. Element is the outer loop. */
void pcb_loop_elements(void *ctx, pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb);

/* Loop over all vias. */
void pcb_loop_vias(void *ctx, pcb_via_cb_t vcb);

/* Loop over all design objects. (So all the above three in one call.) */
void pcb_loop_all(void *ctx,
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb,
	pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb,
	pcb_via_cb_t vcb
);

void pcb_data_free(pcb_data_t *);
pcb_bool pcb_data_is_empty(pcb_data_t *);
pcb_box_t *pcb_data_bbox(pcb_data_t *Data);

#endif
