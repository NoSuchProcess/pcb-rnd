/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018, 2020 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include "board.h"
#include "data.h"
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/event.h>
#include "layer.h"
#include "layer_ui.h"
#include "layer_grp.h"
#include <librnd/core/pcb-printf.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid.h>

#include "layer_menu.h"

/* How much to wait to batch menu refresh events */
#define MENU_REFRESH_TIME_MS 200

typedef struct {
	const char *anch;
	int view;
} ly_ctx_t;

static void layer_install_menu1(void *ctx_, pcb_hid_cfg_t *cfg, lht_node_t *node, char *path)
{
	ly_ctx_t *ctx = ctx_;
	int plen = strlen(path);
	int len_avail = 125;
	char *end = path + plen;
	pcb_menu_prop_t props;
	char act[256], chk[256];
	int idx, max_ml, sect;
	pcb_layergrp_id_t gid;
	const pcb_menu_layers_t *ml;

	memset(&props, 0, sizeof(props));
	props.action = act;
	props.update_on = "";
	props.cookie = ctx->anch;

	pcb_hid_cfg_del_anchor_menus(node, ctx->anch);

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	/* ui layers; have to go reverse to keep order because this will insert items */
	if ((ctx->view) && (vtp0_len(&pcb_uilayers) > 0)) {
		for(idx = vtp0_len(&pcb_uilayers)-1; idx >= 0; idx--) {
			pcb_layer_t *ly = pcb_uilayers.array[idx];
			if ((ly == NULL) || (ly->name == NULL))
				continue;

			props.checked = chk;
			sprintf(act, "ToggleView(ui:%d)", idx);
			sprintf(chk, "ChkView(ui:%d)", idx);

			pcb_snprintf(end, len_avail, "  %s", ly->name);
			pcb_gui->create_menu(pcb_gui, path, &props);
		}

		props.checked = NULL;
		pcb_snprintf(end, len_avail, "[UI]");
		pcb_gui->create_menu(pcb_gui, path, &props);
	}

	/* menu-only virtual layers; have to go reverse to keep order because this will insert items */
	for(ml = pcb_menu_layers, max_ml = 0; ml->name != NULL; ml++) max_ml++;
	for(idx = max_ml-1; idx >= 0; idx--) {
		ml = &pcb_menu_layers[idx];
		props.checked = chk;
		if (ctx->view) {
			sprintf(act, "ToggleView(%s)", ml->abbrev);
			sprintf(chk, "ChkView(%s)", ml->abbrev);
		}
		else {
			if (ml->sel_offs == 0) /* do not list layers that can not be selected */
				continue;
			sprintf(act, "SelectLayer(%s)", ml->abbrev);
			sprintf(chk, "ChkLayer(%s)", ml->abbrev);
		}
		pcb_snprintf(end, len_avail, "  %s", ml->name);
		pcb_gui->create_menu(pcb_gui, path, &props);
	}

	props.checked = NULL;
	pcb_snprintf(end, len_avail, "[virtual]");
	pcb_gui->create_menu(pcb_gui, path, &props);


	/* have to go reverse to keep order because this will insert items */
	for(sect = 0; sect < 2; sect++) {

		pcb_snprintf(end, len_avail, "-");
		props.foreground = NULL;
		props.background = NULL;
		props.checked = NULL;
		*act = '\0';
		*chk = '\0';
		pcb_gui->create_menu(pcb_gui, path, &props);

		for(gid = pcb_max_group(PCB)-1; gid >= 0; gid--) {
			pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
			int n;

			if (g->ltype & PCB_LYT_SUBSTRATE)
				continue;

			if (!!sect != !!(PCB_LAYER_IN_STACK(g->ltype)))
				continue;

			for(n = g->len-1; n >= 0; n--) {
				pcb_layer_id_t lid = g->lid[n];
				pcb_layer_t *l = pcb_get_layer(PCB->Data, lid);
				pcb_layer_type_t lyt = pcb_layer_flags_(l);

				props.background = &l->meta.real.color;
				props.foreground = &pcbhl_conf.appearance.color.background;
				props.checked = chk;
				if (ctx->view) {
					sprintf(act, "ToggleView(%ld)", lid+1);
					sprintf(chk, "ChkView(%ld)", lid+1);
				}
				else {
					sprintf(act, "SelectLayer(%ld)", lid+1);
					sprintf(chk, "ChkLayer(%ld)", lid+1);
				}
				pcb_snprintf(end, len_avail, "  %s", l->name);
				pcb_gui->create_menu(pcb_gui, path, &props);
			}

			props.foreground = NULL;
			props.background = NULL;
			props.checked = NULL;
			pcb_snprintf(end, len_avail, "[%s]", g->name);
			pcb_gui->create_menu(pcb_gui, path, &props);
		}
	}

	/* restore the path */
	end--;
	*end = '\0';
}

static void custom_layer_attr_key(pcb_layer_t *l, pcb_layer_id_t lid, const char *attrname, const char *menu_prefix, const char *action_prefix, pcb_menu_prop_t *keyprops, char *path, char *end, int len_avail)
{
	char *key = rnd_attribute_get(&l->Attributes, attrname);
	if (key != NULL) {
		keyprops->accel = key;
		pcb_snprintf(end, len_avail, "%s %ld:%s", menu_prefix, lid+1, l->name);
		sprintf((char *)keyprops->action, "%s(%ld)", action_prefix, lid+1);
		pcb_gui->create_menu(pcb_gui, path, keyprops);
	}
}


static void layer_install_menu_key(void *ctx_, pcb_hid_cfg_t *cfg, lht_node_t *node, char *path)
{
	ly_ctx_t *ctx = ctx_;
	int plen = strlen(path);
	int len_avail = 125;
	char *end = path + plen;
	char act[256];
	pcb_layer_id_t lid;
	pcb_layer_t *l;
	pcb_menu_prop_t keyprops;

	pcb_hid_cfg_del_anchor_menus(node, ctx->anch);

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	memset(&keyprops, 0, sizeof(keyprops));
	keyprops.action = act;
	keyprops.update_on = "";
	keyprops.cookie = ctx->anch;
	

	for(lid = 0, l = PCB->Data->Layer; lid < PCB->Data->LayerN; lid++,l++) {
		custom_layer_attr_key(l, lid, "pcb-rnd::key::select", "select", "SelectLayer", &keyprops, path, end, len_avail);
		custom_layer_attr_key(l, lid, "pcb-rnd::key::vis",    "vis",    "ToggleView", &keyprops, path, end, len_avail);
	}

}

static void layer_install_menu_keys(void)
{
	ly_ctx_t ctx;

	ctx.view = 0;
	ctx.anch = "@layerkeys";
	pcb_hid_cfg_map_anchor_menus(ctx.anch, layer_install_menu_key, &ctx);
}

static int layer_menu_install_timer_active = 0;
static pcb_hidval_t layer_menu_install_timer;

static void layer_install_menu_cb(pcb_hidval_t user_data)
{
	ly_ctx_t ctx;

	ctx.view = 1;
	ctx.anch = "@layerview";
	pcb_hid_cfg_map_anchor_menus(ctx.anch, layer_install_menu1, &ctx);

	ctx.view = 0;
	ctx.anch = "@layerpick";
	pcb_hid_cfg_map_anchor_menus(ctx.anch, layer_install_menu1, &ctx);

	layer_install_menu_keys();
	layer_menu_install_timer_active = 0;
}

static void layer_install_menu(void)
{
	pcb_hidval_t timerdata;

	if ((pcb_gui == NULL) || (!pcb_gui->gui))
		return;

	if (layer_menu_install_timer_active) {
		pcb_gui->stop_timer(pcb_gui, layer_menu_install_timer);
		layer_menu_install_timer_active = 0;
	}

	timerdata.ptr = NULL;
	layer_menu_install_timer = pcb_gui->add_timer(pcb_gui, layer_install_menu_cb, MENU_REFRESH_TIME_MS, timerdata);
	layer_menu_install_timer_active = 1;

}

void pcb_layer_menu_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	layer_install_menu();
	if ((pcb_gui != NULL) && (pcb_gui->update_menu_checkbox != NULL))
		pcb_gui->update_menu_checkbox(pcb_gui, NULL);
}

void pcb_layer_menu_vis_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((pcb_gui != NULL) && (pcb_gui->update_menu_checkbox != NULL))
		pcb_gui->update_menu_checkbox(pcb_gui, NULL);
}


static int layer_menu_key_timer_active = 0;
static pcb_hidval_t layer_menu_key_timer;

static void timed_layer_menu_key_update_cb(pcb_hidval_t user_data)
{
/*	pcb_trace("************ layer key update timer!\n");*/
	layer_install_menu_keys();
	layer_menu_key_timer_active = 0;
}


void pcb_layer_menu_key_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_hidval_t timerdata;

/*	pcb_trace("************ layer key update ev!\n");*/

	if ((pcb_gui == NULL) || (!pcb_gui->gui))
		return;

	if (layer_menu_key_timer_active) {
		pcb_gui->stop_timer(pcb_gui, layer_menu_key_timer);
		layer_menu_key_timer_active = 0;
	}

	timerdata.ptr = NULL;
	layer_menu_key_timer = pcb_gui->add_timer(pcb_gui, timed_layer_menu_key_update_cb, MENU_REFRESH_TIME_MS, timerdata);
	layer_menu_key_timer_active = 1;
}
