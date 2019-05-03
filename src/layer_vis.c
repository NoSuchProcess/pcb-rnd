/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
	pcb_bool ElementOn, InvisibleObjectsOn, pstk_on, RatOn;
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
int pcb_layervis_change_group_vis(pcb_layer_id_t Layer, int On, pcb_bool ChangeStackOrder)
{
	pcb_layergrp_id_t group;
	int i, changed = 1;		/* at least the current layer changes */


	if (Layer & PCB_LYT_UI) {
		pcb_layer_t *ly = pcb_uilayer_get(Layer);
		if (ly == NULL)
			return 0;
		if (On < 0)
			On = !ly->meta.real.vis;
		ly->meta.real.vis = On;
		changed = 1;
		goto done;
	}

	if (On < 0)
		On = !PCB->Data->Layer[Layer].meta.real.vis;

	/* decrement 'i' to keep stack in order of layergroup */
	if ((group = pcb_layer_get_group(PCB, Layer)) >= 0) {
		for (i = PCB->LayerGroups.grp[group].len; i;) {
			pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[--i];

			/* don't count the passed member of the group */
			if (layer != Layer && layer < pcb_max_layer) {
				PCB->Data->Layer[layer].meta.real.vis = On;

				/* push layer on top of stack if switched on */
				if (On && ChangeStackOrder)
					PushOnTopOfLayerStack(layer);
				changed++;
			}
		}
		PCB->LayerGroups.grp[group].vis = On;
	}

	/* change at least the passed layer */
	PCB->Data->Layer[Layer].meta.real.vis = On;
	if (On && ChangeStackOrder)
		PushOnTopOfLayerStack(Layer);

	done:;
	/* update control panel and exit */
	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	return changed;
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
		PCB->Data->Layer[i].meta.real.vis = pcb_true;
	}
	PCB->InvisibleObjectsOn = pcb_true;
	PCB->pstk_on = pcb_true;
	PCB->SubcOn = pcb_true;
	PCB->SubcPartsOn = pcb_true;
	PCB->RatOn = pcb_true;
	PCB->padstack_mark_on = pcb_true;
	PCB->hole_on = pcb_true;

	/* Bring the top copper group to the front and make it active.  */
	if (pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp, 1) > 0)
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

	if (SavedStack.cnt != 0)
		pcb_message(PCB_MSG_ERROR, "pcb_layervis_save_stack()  layerstack was already saved and not yet restored.  cnt = %d\n", SavedStack.cnt);

	for (i = 0; i < pcb_max_layer; i++) {
		if (!(pcb_layer_flags(PCB, i) & PCB_LYT_SILK))
			SavedStack.pcb_layer_stack[i] = pcb_layer_stack[i];
		SavedStack.LayerOn[i] = PCB->Data->Layer[i].meta.real.vis;
	}
	SavedStack.ElementOn = pcb_silk_on(PCB);
	SavedStack.InvisibleObjectsOn = PCB->InvisibleObjectsOn;
	SavedStack.pstk_on = PCB->pstk_on;
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
		pcb_message(PCB_MSG_ERROR, "pcb_layervis_restore_stack()  layerstack has not" " been saved.  cnt = %d\n", SavedStack.cnt);
		return;
	}
	else if (SavedStack.cnt != 1) {
		pcb_message(PCB_MSG_ERROR, "pcb_layervis_restore_stack()  layerstack save count is" " wrong.  cnt = %d\n", SavedStack.cnt);
	}

	for (i = 0; i < pcb_max_layer; i++) {
		if (!(pcb_layer_flags(PCB, i) & PCB_LYT_SILK))
			pcb_layer_stack[i] = SavedStack.pcb_layer_stack[i];
		PCB->Data->Layer[i].meta.real.vis = SavedStack.LayerOn[i];
	}
	PCB->InvisibleObjectsOn = SavedStack.InvisibleObjectsOn;
	PCB->pstk_on = SavedStack.pstk_on;
	PCB->RatOn = SavedStack.RatOn;

	SavedStack.cnt--;
}

static conf_hid_id_t layer_vis_conf_id;

void layer_vis_chg_mask(conf_native_t *cfg, int arr_idx)
{
	pcb_layer_id_t n;
	int chg = 0;
	static int in = 0; /* don't run when called from the PCB_EVENT_LAYERS_CHANGED triggered from this function */

	if ((PCB == NULL) || (in))
		return;

	in = 1;
	for(n = 0; n < pcb_max_layer; n++) {
		if (pcb_layer_flags(PCB, n) & PCB_LYT_MASK) {
			if (PCB->Data->Layer[n].meta.real.vis != *cfg->val.boolean) {
				chg = 1;
				PCB->Data->Layer[n].meta.real.vis = *cfg->val.boolean;
			}
		}
	}
	if (chg)
		pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	in = 0;
}

void pcb_layer_vis_change_all(pcb_board_t *pcb, pcb_bool_op_t open, pcb_bool_op_t vis)
{
	pcb_layergrp_id_t gid;
	int n;

	for(gid = 0; gid < pcb_max_group(pcb); gid++) {
		pcb_layergrp_t *g = &pcb->LayerGroups.grp[gid];

		pcb_bool_op(g->open, open);

		if (vis == PCB_BOOL_PRESERVE) {
			/* the group is visible exactly if if any layer is visible */
			g->vis = 0;
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *l = pcb_get_layer(pcb->Data, g->lid[n]);
				if ((l != NULL) && (l->meta.real.vis)) {
					g->vis = 1;
					break;
				}
			}
		}
		else {
			pcb_bool_op(g->vis, vis);
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *l = pcb_get_layer(pcb->Data, g->lid[n]);
				pcb_bool_op(l->meta.real.vis, vis);
			}
		}
	}
	/* do NOT generate an event: some callers want to handle that */
}

static void layer_vis_grp_defaults(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_layergrp_id_t gid;

	pcb_layer_vis_change_all(PCB, PCB_BOOL_SET, PCB_BOOL_PRESERVE);

	/* do not show paste and mask by default - they are distractive */
	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
		if ((g->ltype & PCB_LYT_MASK) || (g->ltype & PCB_LYT_PASTE)) {
			int n;
			g->vis = pcb_false;
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *l = pcb_get_layer(PCB->Data, g->lid[n]);
				if (l == NULL)
					pcb_message(PCB_MSG_ERROR, "broken layer groups; layer group references to non-existing layer\n");
				else
					l->meta.real.vis = 0;
			}
		}
	}

	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL); /* Can't send LAYERVIS_CHANGED here: it's a race condition, the layer selector could still have the old widgets */
}

pcb_layer_id_t pcb_layer_vis_last_lyt(pcb_layer_type_t target)
{
	int n;
	/* find the last used match on stack first */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_id_t lid = pcb_layer_stack[n];
		pcb_layer_type_t lyt = pcb_layer_flags(PCB, lid);
		if ((lyt & target) == target)
			return lid;
	}

	/* if no match, find any matching layer */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_type_t lyt = pcb_layer_flags(PCB, n);
		if ((lyt & target) == target)
			return n;
	}

	return -1;
}

void pcb_hid_save_and_show_layer_ons(int *save_array)
{
	int i;
	for (i = 0; i < pcb_max_layer; i++) {
		save_array[i] = PCB->Data->Layer[i].meta.real.vis;
		PCB->Data->Layer[i].meta.real.vis = 1;
	}
}

void pcb_hid_restore_layer_ons(int *save_array)
{
	int i;
	for (i = 0; i < pcb_max_layer; i++)
		PCB->Data->Layer[i].meta.real.vis = save_array[i];
}

static const char *layer_vis_cookie = "core_layer_vis";

void pcb_layer_vis_init(void)
{
	conf_native_t *n_mask = conf_get_field("editor/show_mask");
	static conf_hid_callbacks_t cbs_mask;

	layer_vis_conf_id = conf_hid_reg(layer_vis_cookie, NULL);

	if (n_mask != NULL) {
		memset(&cbs_mask, 0, sizeof(conf_hid_callbacks_t));
		cbs_mask.val_change_post = layer_vis_chg_mask;
		conf_hid_set_cb(n_mask, layer_vis_conf_id, &cbs_mask);
	}

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, layer_vis_grp_defaults, NULL, layer_vis_cookie);
}

void pcb_layer_vis_uninit(void)
{
	pcb_event_unbind_allcookie(layer_vis_cookie);
	conf_hid_unreg(layer_vis_cookie);
}
