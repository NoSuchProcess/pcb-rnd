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
#include <librnd/core/compat_misc.h>
#include <librnd/core/conf_hid.h>

/*
 * Used by pcb_layervis_save_stack() and
 * pcb_layervis_restore_stack()
 */
static struct {
	rnd_bool ElementOn, InvisibleObjectsOn, pstk_on, RatOn;
	int pcb_layer_stack[PCB_MAX_LAYER];
	rnd_bool LayerOn[PCB_MAX_LAYER];
	int cnt;
} SavedStack;


/* ---------------------------------------------------------------------------
 * move layer (number is passed in) to top of layerstack
 */
static void PushOnTopOfLayerStack(pcb_board_t *pcb, int NewTop)
{
	int i;

	/* ignore silk layers */
	if (NewTop < pcb_max_layer(pcb)) {
		/* first find position of passed one */
		for (i = 0; i < pcb_max_layer(pcb); i++)
			if (pcb_layer_stack[i] == NewTop)
				break;

		/* bring this element to the top of the stack */
		for (; i; i--)
			pcb_layer_stack[i] = pcb_layer_stack[i - 1];
		pcb_layer_stack[0] = NewTop;
	}
}

int pcb_layervis_change_group_vis(rnd_hidlib_t *hl, rnd_layer_id_t Layer, int On, rnd_bool ChangeStackOrder)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;
	rnd_layergrp_id_t group;
	int i, changed = 1; /* at least the current layer changes */


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
		On = !pcb->Data->Layer[Layer].meta.real.vis;

	/* decrement 'i' to keep stack in order of layergroup */
	if ((group = pcb_layer_get_group(pcb, Layer)) >= 0) {
		for (i = pcb->LayerGroups.grp[group].len; i;) {
			rnd_layer_id_t layer = pcb->LayerGroups.grp[group].lid[--i];

			/* don't count the passed member of the group */
			if (layer != Layer && layer < pcb_max_layer(pcb)) {
				pcb->Data->Layer[layer].meta.real.vis = On;

				/* push layer on top of stack if switched on */
				if (On && ChangeStackOrder)
					PushOnTopOfLayerStack(pcb, layer);
				changed++;
			}
		}
		pcb->LayerGroups.grp[group].vis = On;
	}

	/* change at least the passed layer */
	pcb->Data->Layer[Layer].meta.real.vis = On;
	if (On && ChangeStackOrder)
		PushOnTopOfLayerStack(pcb, Layer);

	done:;
	/* update control panel and exit */
	if (ChangeStackOrder)
		pcb->RatDraw = 0; /* any layer selection here means we can not be in rat mode */
	rnd_event(hl, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	return changed;
}

void pcb_layervis_reset_stack(rnd_hidlib_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;
	rnd_layer_id_t comp;
	rnd_cardinal_t i;

	assert(pcb->Data->LayerN <= PCB_MAX_LAYER);
	for (i = 0; i < pcb_max_layer(pcb); i++) {
		pcb_layergrp_t *grp = pcb_get_layergrp(pcb, pcb->Data->Layer[i].meta.real.grp);

		if (!(pcb_layer_flags(pcb, i) & PCB_LYT_SILK))
			pcb_layer_stack[i] = i;
		if (grp != NULL)
			pcb->Data->Layer[i].meta.real.vis = !grp->init_invis;
		else
			pcb->Data->Layer[i].meta.real.vis = rnd_true;
	}
	pcb->InvisibleObjectsOn = rnd_true;
	pcb->pstk_on = rnd_true;
	pcb->SubcOn = rnd_true;
	pcb->SubcPartsOn = rnd_true;
	pcb->RatOn = rnd_true;
	pcb->padstack_mark_on = rnd_true;
	pcb->hole_on = rnd_true;

	/* Bring the top copper group to the front and make it active.  */
	if (pcb_layer_list(pcb, PCB_LYT_TOP | PCB_LYT_COPPER, &comp, 1) > 0)
		pcb_layervis_change_group_vis(hl, comp, 1, 1);
}

/* ---------------------------------------------------------------------------
 * saves the layerstack setting
 */
void pcb_layervis_save_stack(void)
{
	rnd_cardinal_t i;
	static rnd_bool run = rnd_false;

	if (run == rnd_false) {
		SavedStack.cnt = 0;
		run = rnd_true;
	}

	if (SavedStack.cnt != 0)
		rnd_message(RND_MSG_ERROR, "pcb_layervis_save_stack()  layerstack was already saved and not yet restored.  cnt = %d\n", SavedStack.cnt);

	for (i = 0; i < pcb_max_layer(PCB); i++) {
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
	rnd_cardinal_t i;

	if (SavedStack.cnt == 0) {
		rnd_message(RND_MSG_ERROR, "pcb_layervis_restore_stack()  layerstack has not" " been saved.  cnt = %d\n", SavedStack.cnt);
		return;
	}
	else if (SavedStack.cnt != 1) {
		rnd_message(RND_MSG_ERROR, "pcb_layervis_restore_stack()  layerstack save count is" " wrong.  cnt = %d\n", SavedStack.cnt);
	}

	for (i = 0; i < pcb_max_layer(PCB); i++) {
		if (!(pcb_layer_flags(PCB, i) & PCB_LYT_SILK))
			pcb_layer_stack[i] = SavedStack.pcb_layer_stack[i];
		PCB->Data->Layer[i].meta.real.vis = SavedStack.LayerOn[i];
	}
	PCB->InvisibleObjectsOn = SavedStack.InvisibleObjectsOn;
	PCB->pstk_on = SavedStack.pstk_on;
	PCB->RatOn = SavedStack.RatOn;

	SavedStack.cnt--;
}

static rnd_conf_hid_id_t layer_vis_conf_id;

void layer_vis_chg_mask(rnd_conf_native_t *cfg, int arr_idx)
{
	rnd_layer_id_t n;
	int chg = 0;
	static int in = 0; /* don't run when called from the PCB_EVENT_LAYERS_CHANGED triggered from this function */

	if ((PCB == NULL) || (in))
		return;

	in = 1;
	for(n = 0; n < pcb_max_layer(PCB); n++) {
		if (pcb_layer_flags(PCB, n) & PCB_LYT_MASK) {
			if (PCB->Data->Layer[n].meta.real.vis != *cfg->val.boolean) {
				chg = 1;
				PCB->Data->Layer[n].meta.real.vis = *cfg->val.boolean;
			}
		}
	}
	if (chg)
		rnd_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	in = 0;
}

void pcb_layer_vis_change_all(pcb_board_t *pcb, rnd_bool_op_t open, rnd_bool_op_t vis)
{
	rnd_layergrp_id_t gid;
	int n;

	for(gid = 0; gid < pcb_max_group(pcb); gid++) {
		pcb_layergrp_t *g = &pcb->LayerGroups.grp[gid];

		rnd_bool_op(g->open, open);

		if (vis == RND_BOOL_PRESERVE) {
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
			rnd_bool_op(g->vis, vis);
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *l = pcb_get_layer(pcb->Data, g->lid[n]);
				rnd_bool_op(l->meta.real.vis, vis);
			}
		}
	}
	/* do NOT generate an event: some callers want to handle that */
}

void pcb_layer_vis_historical_hides(pcb_board_t *pcb)
{
	rnd_layergrp_id_t gid;

	/* do not show paste and mask by default - they are distractive */
	for(gid = 0; gid < pcb_max_group(pcb); gid++) {
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
		if ((g->ltype & PCB_LYT_MASK) || (g->ltype & PCB_LYT_PASTE)) {
			int n;
			g->vis = rnd_false;
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *l = pcb_get_layer(PCB->Data, g->lid[n]);
				if (l == NULL)
					rnd_message(RND_MSG_ERROR, "broken layer groups; layer group references to non-existing layer\n");
				else
					l->meta.real.vis = 0;
			}
		}
	}
}

static void layer_vis_grp_defaults(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	pcb_layer_vis_change_all(pcb, RND_BOOL_SET, RND_BOOL_PRESERVE);
	pcb_layer_vis_historical_hides(pcb);

	rnd_event(hidlib, PCB_EVENT_LAYERS_CHANGED, NULL); /* Can't send LAYERVIS_CHANGED here: it's a race condition, the layer selector could still have the old widgets */
}

rnd_layer_id_t pcb_layer_vis_last_lyt(pcb_layer_type_t target)
{
	int n;
	/* find the last used match on stack first */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		rnd_layer_id_t lid = pcb_layer_stack[n];
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
	rnd_layer_id_t i;
	for (i = 0; i < pcb_max_layer(PCB); i++) {
		save_array[i] = PCB->Data->Layer[i].meta.real.vis;
		PCB->Data->Layer[i].meta.real.vis = 1;
	}
}

void pcb_hid_save_and_show_layergrp_ons(int *save_array)
{
	rnd_layergrp_id_t i;
	for (i = 0; i < PCB->LayerGroups.len; i++)
		save_array[i] = PCB->LayerGroups.grp[i].vis;
}

void pcb_hid_restore_layer_ons(int *save_array)
{
	rnd_layer_id_t i;
	for (i = 0; i < pcb_max_layer(PCB); i++)
		PCB->Data->Layer[i].meta.real.vis = save_array[i];
}

void pcb_hid_restore_layergrp_ons(int *save_array)
{
	rnd_layergrp_id_t i;
	for (i = 0; i < PCB->LayerGroups.len; i++)
		PCB->LayerGroups.grp[i].vis = save_array[i];
}

static const char *layer_vis_cookie = "core_layer_vis";

void pcb_layer_vis_init(void)
{
	rnd_conf_native_t *n_mask = rnd_conf_get_field("editor/show_mask");
	static rnd_conf_hid_callbacks_t cbs_mask;

	layer_vis_conf_id = rnd_conf_hid_reg(layer_vis_cookie, NULL);

	if (n_mask != NULL) {
		memset(&cbs_mask, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs_mask.val_change_post = layer_vis_chg_mask;
		rnd_conf_hid_set_cb(n_mask, layer_vis_conf_id, &cbs_mask);
	}

	rnd_event_bind(RND_EVENT_BOARD_CHANGED, layer_vis_grp_defaults, NULL, layer_vis_cookie);
	rnd_event_bind(RND_EVENT_GUI_INIT, layer_vis_grp_defaults, NULL, layer_vis_cookie);
}

void pcb_layer_vis_uninit(void)
{
	rnd_event_unbind_allcookie(layer_vis_cookie);
	rnd_conf_hid_unreg(layer_vis_cookie);
}
