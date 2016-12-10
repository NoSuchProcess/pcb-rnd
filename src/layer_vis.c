/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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

/* Layer visibility logics (affects the GUI and some exporters) */

#include "config.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "layer.h"
#include "layer_vis.h"
#include "event.h"
#include "compat_misc.h"

/*
 * Used by pcb_layervis_save_stack() and
 * pcb_layervis_restore_stack()
 */
static struct {
	pcb_bool ElementOn, InvisibleObjectsOn, PinOn, ViaOn, RatOn;
	int pcb_layer_stack[PCB_MAX_LAYER];
	pcb_bool LayerOn[PCB_MAX_LAYER];
	int cnt;
} SavedStack;


/* ----------------------------------------------------------------------
 * Given a string description of a layer stack, adjust the layer stack
 * to correspond.
*/

void pcb_layervis_parse_string(const char *layer_string)
{
	static int listed_layers = 0;
	int l = strlen(layer_string);
	char **args;
	int i, argn, lno;
	int prev_sep = 1;
	char *s;

	s = pcb_strdup(layer_string);
	args = (char **) malloc(l * sizeof(char *));
	argn = 0;

	for (i = 0; i < l; i++) {
		switch (s[i]) {
		case ' ':
		case '\t':
		case ',':
		case ';':
		case ':':
			prev_sep = 1;
			s[i] = '\0';
			break;
		default:
			if (prev_sep)
				args[argn++] = s + i;
			prev_sep = 0;
			break;
		}
	}

	for (i = 0; i < pcb_max_copper_layer + 2; i++) {
		if (i < pcb_max_copper_layer)
			pcb_layer_stack[i] = i;
		PCB->Data->Layer[i].On = pcb_false;
	}
	PCB->ElementOn = pcb_false;
	PCB->InvisibleObjectsOn = pcb_false;
	PCB->PinOn = pcb_false;
	PCB->ViaOn = pcb_false;
	PCB->RatOn = pcb_false;

	conf_set_editor(show_mask, 0);
	conf_set_editor(show_solder_side, 0);

	for (i = argn - 1; i >= 0; i--) {
		if (pcb_strcasecmp(args[i], "rats") == 0)
			PCB->RatOn = pcb_true;
		else if (pcb_strcasecmp(args[i], "invisible") == 0)
			PCB->InvisibleObjectsOn = pcb_true;
		else if (pcb_strcasecmp(args[i], "pins") == 0)
			PCB->PinOn = pcb_true;
		else if (pcb_strcasecmp(args[i], "vias") == 0)
			PCB->ViaOn = pcb_true;
		else if (pcb_strcasecmp(args[i], "elements") == 0 || pcb_strcasecmp(args[i], "silk") == 0)
			PCB->ElementOn = pcb_true;
		else if (pcb_strcasecmp(args[i], "mask") == 0)
			conf_set_editor(show_mask, 1);
		else if (pcb_strcasecmp(args[i], "solderside") == 0)
			conf_set_editor(show_solder_side, 1);
		else if (isdigit((int) args[i][0])) {
			lno = atoi(args[i]);
			pcb_layervis_change_group_vis(lno, pcb_true, pcb_true);
		}
		else {
			int found = 0;
			for (lno = 0; lno < pcb_max_copper_layer; lno++)
				if (pcb_strcasecmp(args[i], PCB->Data->Layer[lno].Name) == 0) {
					pcb_layervis_change_group_vis(lno, pcb_true, pcb_true);
					found = 1;
					break;
				}
			if (!found) {
				fprintf(stderr, "Warning: layer \"%s\" not known\n", args[i]);
				if (!listed_layers) {
					fprintf(stderr, "Named layers in this board are:\n");
					listed_layers = 1;
					for (lno = 0; lno < pcb_max_copper_layer; lno++)
						fprintf(stderr, "\t%s\n", PCB->Data->Layer[lno].Name);
					fprintf(stderr, "Also: component, solder, rats, invisible, pins, vias, elements or silk, mask, solderside.\n");
				}
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * move layer (number is passed in) to top of layerstack
 */
static void PushOnTopOfLayerStack(int NewTop)
{
	int i;

	/* ignore silk layers */
	if (NewTop < pcb_max_copper_layer) {
		/* first find position of passed one */
		for (i = 0; i < pcb_max_copper_layer; i++)
			if (pcb_layer_stack[i] == NewTop)
				break;

		/* bring this element to the top of the stack */
		for (; i; i--)
			pcb_layer_stack[i] = pcb_layer_stack[i - 1];
		pcb_layer_stack[0] = NewTop;
	}
}


/* ----------------------------------------------------------------------
 * changes the visibility of all layers in a group
 * returns the number of changed layers
 */
int pcb_layervis_change_group_vis(int Layer, pcb_bool On, pcb_bool ChangeStackOrder)
{
	pcb_layergrp_id_t group;
	int i, changed = 1;		/* at least the current layer changes */

	/* Warning: these special case values must agree with what gui-top-window.c
	   |  thinks the are.
	 */

	if (conf_core.rc.verbose)
		printf("pcb_layervis_change_group_vis(Layer=%d, On=%d, ChangeStackOrder=%d)\n", Layer, On, ChangeStackOrder);

	/* decrement 'i' to keep stack in order of layergroup */
	if ((group = pcb_layer_get_group(Layer)) >= 0) {
		for (i = PCB->LayerGroups.Number[group]; i;) {
			int layer = PCB->LayerGroups.Entries[group][--i];

			/* don't count the passed member of the group */
			if (layer != Layer && layer < pcb_max_copper_layer) {
				PCB->Data->Layer[layer].On = On;

				/* push layer on top of stack if switched on */
				if (On && ChangeStackOrder)
					PushOnTopOfLayerStack(layer);
				changed++;
			}
		}
	}

	/* change at least the passed layer */
	PCB->Data->Layer[Layer].On = On;
	if (On && ChangeStackOrder)
		PushOnTopOfLayerStack(Layer);

	/* update control panel and exit */
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return (changed);
}

/* ---------------------------------------------------------------------------
 * resets the layerstack setting
 */
void pcb_layervis_reset_stack(void)
{
	pcb_layergrp_id_t comp_group;
	pcb_cardinal_t i;

	assert(PCB->Data->LayerN <= PCB_MAX_LAYER);
	for (i = 0; i < pcb_max_copper_layer + 2; i++) {
		if (i < pcb_max_copper_layer)
			pcb_layer_stack[i] = i;
		PCB->Data->Layer[i].On = pcb_true;
	}
	PCB->ElementOn = pcb_true;
	PCB->InvisibleObjectsOn = pcb_true;
	PCB->PinOn = pcb_true;
	PCB->ViaOn = pcb_true;
	PCB->RatOn = pcb_true;

	/* Bring the component group to the front and make it active.  */
	comp_group = pcb_layer_get_group(pcb_component_silk_layer);
	pcb_layervis_change_group_vis(PCB->LayerGroups.Entries[comp_group][0], 1, 1);
}

/* ---------------------------------------------------------------------------
 * saves the layerstack setting
 */
void pcb_layervis_save_stack(void)
{
	pcb_cardinal_t i;
	static pcb_bool run = pcb_false;

	if (run == pcb_false) {
		SavedStack.cnt = 0;
		run = pcb_true;
	}

	if (SavedStack.cnt != 0) {
		fprintf(stderr,
						"pcb_layervis_save_stack()  layerstack was already saved and not" "yet restored.  cnt = %d\n", SavedStack.cnt);
	}

	for (i = 0; i < pcb_max_copper_layer + 2; i++) {
		if (i < pcb_max_copper_layer)
			SavedStack.pcb_layer_stack[i] = pcb_layer_stack[i];
		SavedStack.LayerOn[i] = PCB->Data->Layer[i].On;
	}
	SavedStack.ElementOn = PCB->ElementOn;
	SavedStack.InvisibleObjectsOn = PCB->InvisibleObjectsOn;
	SavedStack.PinOn = PCB->PinOn;
	SavedStack.ViaOn = PCB->ViaOn;
	SavedStack.RatOn = PCB->RatOn;
	SavedStack.cnt++;
}

/* ---------------------------------------------------------------------------
 * restores the layerstack setting
 */
void pcb_layervis_restore_stack(void)
{
	pcb_cardinal_t i;

	if (SavedStack.cnt == 0) {
		fprintf(stderr, "pcb_layervis_restore_stack()  layerstack has not" " been saved.  cnt = %d\n", SavedStack.cnt);
		return;
	}
	else if (SavedStack.cnt != 1) {
		fprintf(stderr, "pcb_layervis_restore_stack()  layerstack save count is" " wrong.  cnt = %d\n", SavedStack.cnt);
	}

	for (i = 0; i < pcb_max_copper_layer + 2; i++) {
		if (i < pcb_max_copper_layer)
			pcb_layer_stack[i] = SavedStack.pcb_layer_stack[i];
		PCB->Data->Layer[i].On = SavedStack.LayerOn[i];
	}
	PCB->ElementOn = SavedStack.ElementOn;
	PCB->InvisibleObjectsOn = SavedStack.InvisibleObjectsOn;
	PCB->PinOn = SavedStack.PinOn;
	PCB->ViaOn = SavedStack.ViaOn;
	PCB->RatOn = SavedStack.RatOn;

	SavedStack.cnt--;
}
