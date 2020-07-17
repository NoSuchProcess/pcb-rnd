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

#include <genvector/gds_char.h>
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/event.h>
#include "layer.h"
#include "layer_ui.h"
#include "layer_grp.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid.h>

#include "layer_menu.h"

static const char *menu_cookie = "lib_hid_pcbui layer menus";

static void layer_install_menu1(const char *anch, int view)
{
	int plen = strlen(anch);
	rnd_menu_prop_t props;
	char act[256], chk[256];
	int idx, max_ml, sect;
	rnd_layergrp_id_t gid;
	const pcb_menu_layers_t *ml;
	gds_t path = {0};

	memset(&props, 0, sizeof(props));
	props.action = act;
	props.update_on = "";
	props.cookie = menu_cookie;

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	gds_append_str(&path, anch);
	gds_append(&path, '/');
	plen++;

	/* ui layers; have to go reverse to keep order because this will insert items */
	if ((view) && (vtp0_len(&pcb_uilayers) > 0)) {
		for(idx = vtp0_len(&pcb_uilayers)-1; idx >= 0; idx--) {
			pcb_layer_t *ly = pcb_uilayers.array[idx];
			if ((ly == NULL) || (ly->name == NULL))
				continue;

			props.checked = chk;
			sprintf(act, "ToggleView(ui:%d)", idx);
			sprintf(chk, "ChkView(ui:%d)", idx);

			gds_truncate(&path, plen);
			gds_append_str(&path, "  ");
			gds_append_str(&path, ly->name);
			rnd_hid_menu_create(path.array, &props);
		}

		props.checked = NULL;
		gds_truncate(&path, plen);
		gds_append_str(&path, "[UI]");

		rnd_hid_menu_create(path.array, &props);
	}

	/* menu-only virtual layers; have to go reverse to keep order because this will insert items */
	for(ml = pcb_menu_layers, max_ml = 0; ml->name != NULL; ml++) max_ml++;
	for(idx = max_ml-1; idx >= 0; idx--) {
		ml = &pcb_menu_layers[idx];
		props.checked = chk;
		if (view) {
			sprintf(act, "ToggleView(%s)", ml->abbrev);
			sprintf(chk, "ChkView(%s)", ml->abbrev);
		}
		else {
			if (ml->sel_offs == 0) /* do not list layers that can not be selected */
				continue;
			sprintf(act, "SelectLayer(%s)", ml->abbrev);
			sprintf(chk, "ChkLayer(%s)", ml->abbrev);
		}
		gds_truncate(&path, plen);
		gds_append_str(&path, "  ");
		gds_append_str(&path, ml->name);
		rnd_hid_menu_create(path.array, &props);
	}

	props.checked = NULL;
	gds_truncate(&path, plen);
	gds_append_str(&path, "[virtual]");
	rnd_hid_menu_create(path.array, &props);


	/* have to go reverse to keep order because this will insert items */
	for(sect = 0; sect < 2; sect++) {
		gds_truncate(&path, plen);
		gds_append(&path, '-');
		props.foreground = NULL;
		props.background = NULL;
		props.checked = NULL;
		*act = '\0';
		*chk = '\0';
		rnd_hid_menu_create(path.array, &props);

		for(gid = pcb_max_group(PCB)-1; gid >= 0; gid--) {
			pcb_layergrp_t *g = &PCB->LayerGroups.grp[gid];
			int n;

			if (g->ltype & PCB_LYT_SUBSTRATE)
				continue;

			if (!!sect != !!(PCB_LAYER_IN_STACK(g->ltype)))
				continue;

			for(n = g->len-1; n >= 0; n--) {
				rnd_layer_id_t lid = g->lid[n];
				pcb_layer_t *l = pcb_get_layer(PCB->Data, lid);
				pcb_layer_type_t lyt = pcb_layer_flags_(l);

				props.background = &l->meta.real.color;
				props.foreground = &rnd_conf.appearance.color.background;
				props.checked = chk;
				if (view) {
					sprintf(act, "ToggleView(%ld)", lid+1);
					sprintf(chk, "ChkView(%ld)", lid+1);
				}
				else {
					sprintf(act, "SelectLayer(%ld)", lid+1);
					sprintf(chk, "ChkLayer(%ld)", lid+1);
				}
				gds_truncate(&path, plen);
				gds_append_str(&path, "  ");
				gds_append_str(&path, l->name);
				rnd_hid_menu_create(path.array, &props);
			}

			props.foreground = NULL;
			props.background = NULL;
			props.checked = NULL;
			gds_truncate(&path, plen);
			gds_append(&path, '[');
			gds_append_str(&path, g->name);
			gds_append(&path, ']');
			rnd_hid_menu_create(path.array, &props);
		}
	}

	gds_uninit(&path);
}

static void custom_layer_attr_key(pcb_layer_t *l, rnd_layer_id_t lid, const char *attrname, const char *menu_prefix, const char *action_prefix, rnd_menu_prop_t *keyprops, gds_t *path, int plen)
{
	char *key = pcb_attribute_get(&l->Attributes, attrname);
	if (key != NULL) {
		keyprops->accel = key;
		gds_truncate(path, plen);
		rnd_append_printf(path, "%s %ld:%s", menu_prefix, lid+1, l->name);
		sprintf((char *)keyprops->action, "%s(%ld)", action_prefix, lid+1);
		rnd_hid_menu_create(path->array, keyprops);
	}
}


static void layer_install_menu_key(const char *anch, int view)
{
	int plen = strlen(anch);
	char act[256];
	rnd_layer_id_t lid;
	pcb_layer_t *l;
	rnd_menu_prop_t keyprops;
	gds_t path = {0};

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	gds_append_str(&path, anch);
	gds_append(&path, '/');
	plen++;

	memset(&keyprops, 0, sizeof(keyprops));
	keyprops.action = act;
	keyprops.update_on = "";
	keyprops.cookie = menu_cookie;
	

	for(lid = 0, l = PCB->Data->Layer; lid < PCB->Data->LayerN; lid++,l++) {
		custom_layer_attr_key(l, lid, "pcb-rnd::key::select", "select", "SelectLayer", &keyprops, &path, plen);
		custom_layer_attr_key(l, lid, "pcb-rnd::key::vis",    "vis",    "ToggleView", &keyprops, &path, plen);
	}

	gds_uninit(&path);
}

static int need_layer_menu_update = 0, need_layer_key_update = 0;
void pcb_layer_menu_batch_timer_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	int keys_done = 0;

	if (need_layer_menu_update) {
/*rnd_trace("layer menu update.\n");*/
		rnd_hid_menu_unload(rnd_gui, menu_cookie);

		layer_install_menu1("/anchored/@layerview", 1);
		layer_install_menu1("/anchored/@layerpick", 0);
		layer_install_menu_key("/anchored/@layerkeys", 0);

		need_layer_menu_update = 0;
		keys_done = 1;
	}

	if (need_layer_key_update) {
/*rnd_trace("layer key update timer!\n");*/
		if (!keys_done)
			layer_install_menu_key("/anchored/@layerkeys", 0);
		need_layer_key_update = 0;
	}
}

static void layer_install_menu(rnd_hidlib_t *hidlib)
{
	need_layer_menu_update = 1;
	rnd_hid_gui_batch_timer(hidlib);
}

void pcb_layer_menu_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	layer_install_menu(hidlib);
	if ((rnd_gui != NULL) && (rnd_gui->update_menu_checkbox != NULL))
		rnd_gui->update_menu_checkbox(rnd_gui, NULL);
}

void pcb_layer_menu_vis_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((rnd_gui != NULL) && (rnd_gui->update_menu_checkbox != NULL))
		rnd_gui->update_menu_checkbox(rnd_gui, NULL);
}

void pcb_layer_menu_key_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	need_layer_key_update = 1;
	rnd_hid_gui_batch_timer(hidlib);
}
