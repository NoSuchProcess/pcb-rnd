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
#include "layer_ui.h"
#include "layer_vis.h"
#include "event.h"
#include "compat_misc.h"
#include "conf_hid.h"

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


/* ---------------------------------------------------------------------------
 * move layer (number is passed in) to top of layerstack
 */
static void PushOnTopOfLayerStack(int NewTop)
{
	int i;

	/* ignore silk layers */
	if (NewTop < pcb_max_layer) {
		/* first find position of passed one */
		for (i = 0; i < pcb_max_layer; i++)
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
int pcb_layervis_change_group_vis(pcb_layer_id_t Layer, pcb_bool On, pcb_bool ChangeStackOrder)
{
	unsigned long flg;
	pcb_layergrp_id_t group;
	int i, changed = 1;		/* at least the current layer changes */

	if (Layer & PCB_LYT_UI) {
		Layer &= ~(PCB_LYT_UI | PCB_LYT_VIRTUAL);
		if (Layer >= vtlayer_len(&pcb_uilayer))
			return 0;
		pcb_uilayer.array[Layer].On = On;
		changed = 1;
		goto done;
	}

	/* Warning: these special case values must agree with what gui-top-window.c
	   |  thinks the are.
	 */

	if (conf_core.rc.verbose)
		printf("pcb_layervis_change_group_vis(Layer=%d, On=%d, ChangeStackOrder=%d)\n", Layer, On, ChangeStackOrder);

	/* special case: some layer groups are controlled by a config setting */
	flg = pcb_layer_flags(PCB, Layer);
	if (flg & PCB_LYT_MASK)
		conf_set_editor(show_mask, On);
	if (flg & PCB_LYT_PASTE)
		conf_set_editor(show_paste, On);

	/* decrement 'i' to keep stack in order of layergroup */
	if ((group = pcb_layer_get_group(PCB, Layer)) >= 0) {
		for (i = PCB->LayerGroups.grp[group].len; i;) {
			pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[--i];

			/* don't count the passed member of the group */
			if (layer != Layer && layer < pcb_max_layer) {
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

	done:;
	/* update control panel and exit */
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return (changed);
}

/* ---------------------------------------------------------------------------
 * resets the layerstack setting
 */
void pcb_layervis_reset_stack(void)
{
	pcb_layer_id_t comp;
	pcb_cardinal_t i;

	assert(PCB->Data->LayerN <= PCB_MAX_LAYER);
	for (i = 0; i < pcb_max_layer; i++) {
		if (!(pcb_layer_flags(PCB, i) & PCB_LYT_SILK))
			pcb_layer_stack[i] = i;
		PCB->Data->Layer[i].On = pcb_true;
	}
	PCB->ElementOn = pcb_true;
	PCB->InvisibleObjectsOn = pcb_true;
	PCB->PinOn = pcb_true;
	PCB->ViaOn = pcb_true;
	PCB->RatOn = pcb_true;

	/* Bring the top copper group to the front and make it active.  */
	if (pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, &comp, 1) > 0)
		pcb_layervis_change_group_vis(comp, 1, 1);
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

	for (i = 0; i < pcb_max_layer; i++) {
		if (!(pcb_layer_flags(PCB, i) & PCB_LYT_SILK))
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

	for (i = 0; i < pcb_max_layer; i++) {
		if (!(pcb_layer_flags(PCB, i) & PCB_LYT_SILK))
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

static conf_hid_id_t layer_vis_conf_id;

void layer_vis_chg_mask(conf_native_t *cfg)
{
	pcb_layer_id_t n;
	int chg = 0;
	static int in = 0; /* don't run when called from the PCB_EVENT_LAYERS_CHANGED triggered from this function */

	if ((PCB == NULL) || (in))
		return;

	in = 1;
	for(n = 0; n < pcb_max_layer; n++) {
		if (pcb_layer_flags(PCB, n) & PCB_LYT_MASK) {
			if (PCB->Data->Layer[n].On != *cfg->val.boolean) {
				chg = 1;
				PCB->Data->Layer[n].On = *cfg->val.boolean;
			}
		}
	}
	if (chg)
		pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	in = 0;
}

void layer_vis_sync(void)
{
	conf_native_t *n_mask = conf_get_field("editor/show_mask");
	layer_vis_chg_mask(n_mask);
}

static void layer_vis_sync_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	layer_vis_sync();
}

static const char *layer_vis_cookie = "core_layer_vis";

void layer_vis_init(void)
{
	conf_native_t *n_mask = conf_get_field("editor/show_mask");
	static conf_hid_callbacks_t cbs_mask;

	layer_vis_conf_id = conf_hid_reg(layer_vis_cookie, NULL);

	if (n_mask != NULL) {
		memset(&cbs_mask, 0, sizeof(conf_hid_callbacks_t));
		cbs_mask.val_change_post = layer_vis_chg_mask;
		conf_hid_set_cb(n_mask, layer_vis_conf_id, &cbs_mask);
	}

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, layer_vis_sync_ev, NULL, layer_vis_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, layer_vis_sync_ev, NULL, layer_vis_cookie);
}

void layer_vis_uninit(void)
{
	pcb_event_unbind_allcookie(layer_vis_cookie);
}
