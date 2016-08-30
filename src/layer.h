/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas (pcb-rnd extensions)
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

/* prototypes for layer manipulation */

/* Returns true if the given layer is empty (there are no objects on the layer) */
bool IsLayerEmpty(LayerTypePtr);
bool IsLayerNumEmpty(int);

/* Returns true if all layers in a group are empty */
bool IsLayerGroupEmpty(int);


/************ OLD API - new code should not use these **************/

int ParseGroupString(char *, LayerGroupTypePtr, int /* LayerN */ );

int GetLayerNumber(DataTypePtr, LayerTypePtr);
int GetLayerGroupNumberByPointer(LayerTypePtr);
int GetLayerGroupNumberByNumber(Cardinal);
int GetGroupOfLayer(int);
int ChangeGroupVisibility(int, bool, bool);
void LayerStringToLayerStack(char *);

/* Layer Group Functions */

/* Returns group actually moved to (i.e. either group or previous) */
int MoveLayerToGroup(int layer, int group);

/* Returns pointer to private buffer */
char *LayerGroupsToString(LayerGroupTypePtr);

#define	LAYER_ON_STACK(n)	(&PCB->Data->Layer[LayerStack[(n)]])
#define LAYER_PTR(n)            (&PCB->Data->Layer[(n)])
#define	CURRENT			(PCB->SilkActive ? &PCB->Data->Layer[ \
				(conf_core.editor.show_solder_side ? solder_silk_layer : component_silk_layer)] \
				: LAYER_ON_STACK(0))
#define	INDEXOFCURRENT		(PCB->SilkActive ? \
				(conf_core.editor.show_solder_side ? solder_silk_layer : component_silk_layer) \
				: LayerStack[0])
#define SILKLAYER		Layer[ \
				(conf_core.editor.show_solder_side ? solder_silk_layer : component_silk_layer)]
#define BACKSILKLAYER		Layer[ \
				(conf_core.editor.show_solder_side ? component_silk_layer : solder_silk_layer)]

#define TEST_SILK_LAYER(layer)	(GetLayerNumber (PCB->Data, layer) >= max_copper_layer)



/************ NEW API - new code should use these **************/

/* Layer type bitfield */
typedef {
	/* Stack-up (vertical position): */
	PCB_LYT_TOP      = 0x0001, /* layer is on the top side of the board */
	PCB_LYT_BOTTOM   = 0x0002, /* layer is on the bottom side of the board */
	PCB_LYT_INTERN   = 0x0004, /* layer is internal */
	PCB_LYT_ANYWHERE = 0x00FF, /* MASK: layer is anywhere on the stack */

	/* What the layer consists of */
	PCB_LYT_COPPER   = 0x0100, /* copper objects */
	PCB_LYT_SILK     = 0x0200, /* silk objects */
	PCB_LYT_MASK     = 0x0400, /* solder mask */
	PCB_LYT_PASTE    = 0x0800, /* paste */
	PCB_LYT_OUTLINE  = 0x1000, /* outline (contour of the board) */
	PCB_LYT_ANYTHING = 0xFF00, /* MASK */
} pcb_layer_type_t;

/* List layer IDs that matches mask - write the first res_len items in res,
   if res is not NULL. Returns:
    - the total number of layers matching the query, if res is NULL
    - the number of layers copied into res if res is not NULL
*/
int pcb_layer_list(pcb_layer_type_t mask, int *res, int res_len);

/* Same as pcb_layer_list but lists layer groups. A group is matching
   if any layer in that group matches mask. */
int pcb_layer_group_list(pcb_layer_type_t mask, int *res, int res_len);
