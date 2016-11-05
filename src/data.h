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
#include "global_objs.h"
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
	rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
	 *rat_tree;
	PCBType *pcb;
	LayerType Layer[MAX_LAYER + 2];    /* add 2 silkscreen layers */
	plug_io_t *loader;
	ratlist_t Rat;
};


/* ---------------------------------------------------------------------------
 * some shared identifiers
 */
extern CrosshairType Crosshair;

extern MarkType Marked;

#define max_group (PCB->Data->LayerN)
#define max_copper_layer (PCB->Data->LayerN)
#define solder_silk_layer (max_copper_layer + SOLDER_LAYER)
#define component_silk_layer (max_copper_layer + COMPONENT_LAYER)

extern BufferType Buffers[MAX_BUFFER];

/*extern	DeviceInfoType	PrintingDevice[];*/

extern char *InputTranslations;

extern int addedLines;
extern int LayerStack[MAX_LAYER];

extern pcb_bool Bumped;

/****** callback based loops *****/

/* The functions returning int are called once when processing of a new layer
   or element starts, with enter=1. If they return non-zero, the current layer
   or element is skipped. If it is not skipped, the function is called once
   at the end of processing the given layer or element, with enter=0 (and
   return value ignored).

   Any of the callbacks for any loop function can be NULL.
   */

/* layer object callbacks */
typedef int (*pcb_layer_cb_t)(void *ctx, PCBType *pcb, LayerType *layer, int enter);
typedef void (*pcb_line_cb_t)(void *ctx, PCBType *pcb, LayerType *layer, LineType *line);
typedef void (*pcb_arc_cb_t)(void *ctx, PCBType *pcb, LayerType *layer, ArcType *arc);
typedef void (*pcb_text_cb_t)(void *ctx, PCBType *pcb, LayerType *layer, TextType *text);
typedef void (*pcb_poly_cb_t)(void *ctx, PCBType *pcb, LayerType *layer, PolygonType *poly);

/* element callbacks */
typedef int (*pcb_element_cb_t)(void *ctx, PCBType *pcb, ElementType *element, int enter);
typedef void (*pcb_eline_cb_t)(void *ctx, PCBType *pcb, ElementType *element, LineType *line);
typedef void (*pcb_earc_cb_t)(void *ctx, PCBType *pcb, ElementType *element, ArcType *arc);
typedef void (*pcb_etext_cb_t)(void *ctx, PCBType *pcb, ElementType *element, TextType *text);
typedef void (*pcb_epin_cb_t)(void *ctx, PCBType *pcb, ElementType *element, PinType *pin);
typedef void (*pcb_epad_cb_t)(void *ctx, PCBType *pcb, ElementType *element, PadType *pad);

/* via callbacks */
typedef void (*pcb_via_cb_t)(void *ctx, PCBType *pcb, PinType *via);

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

void FreeDataMemory(DataTypePtr);
pcb_bool IsDataEmpty(DataTypePtr);
BoxTypePtr GetDataBoundingBox(DataTypePtr Data);

#endif
