/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/conf.h>
#include "conf_core.h"
#include "route_style.h"
#include "event.h"
#include <librnd/core/hid.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_menu.h>

#include "routest.h"

#define ANCH "/anchored/@routestyles"

#define MAX_STYLES 64

typedef struct {
	rnd_hid_dad_subdialog_t sub;
	int sub_inited, last_len;
	int whbox[MAX_STYLES], wchk[MAX_STYLES], wlab[MAX_STYLES];
} rst_ctx_t;

static rst_ctx_t rst;

#include "routest_dlg.c"

static void rst_install_menu(void)
{
	rnd_menu_prop_t props;
	char act[256], chk[256], *path, *end;
	int idx;
	size_t len = 0;

	for(idx = vtroutestyle_len(&PCB->RouteStyle)-1; idx >= 0; idx--) {
		size_t l = strlen(PCB->RouteStyle.array[idx].name);
		if (l > len) len = l;
	}

	path = malloc(len + 32);
	strcpy(path, ANCH);
	end = path + strlen(ANCH);

	memset(&props, 0,sizeof(props));
	props.action = act;
	props.checked = chk;
	props.update_on = "";
	props.cookie = "lib_hid_pcbui route styles";

	/* prepare for appending the strings at the end of the path, "under" the anchor */
	*end = '/';
	end++;

	rnd_hid_menu_merge_inhibit_inc();
	rnd_hid_menu_unload(rnd_gui, props.cookie);
	/* have to go reverse to keep order because this will insert items */
	for(idx = vtroutestyle_len(&PCB->RouteStyle)-1; idx >= 0; idx--) {
		sprintf(act, "RouteStyle(%d)", idx+1); /* for historical reasons this action counts from 1 */
		sprintf(chk, "ChkRst(%d)", idx);
		strcpy(end, PCB->RouteStyle.array[idx].name);
		rnd_hid_menu_create(path, &props);
	}
	rnd_hid_menu_merge_inhibit_dec();
	free(path);
}

/* Update the edit dialog and all checkboxes, but nothing else on the sub */
static void rst_force_update_chk_and_dlg()
{
	int n, target = pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL);
	rnd_hid_attr_val_t hv;

	idx_changed();

	for(n = 0; n < vtroutestyle_len(&PCB->RouteStyle); n++) {
		hv.lng = (n == target);
		rnd_gui->attr_dlg_set_value(rst.sub.dlg_hid_ctx, rst.wchk[n], &hv);
	}
	rstdlg_pcb2dlg(target);
}

static int need_rst_menu = 0;

void pcb_rst_menu_batch_timer_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (need_rst_menu) {
/*		rnd_trace("layer key update timer!\n");*/
		rst_install_menu();
		need_rst_menu = 0;
	}
}


static int rst_lock = 0;
static void rst_update(rnd_hidlib_t *hidlib)
{
	if (rst_lock) return;
	rst_lock++;

	need_rst_menu = 1;
	rnd_hid_gui_batch_timer(hidlib);

	if (rst.sub_inited) {
		int n, target;
		target = pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL);
		for(n = 0; n < vtroutestyle_len(&PCB->RouteStyle); n++) {
			rnd_hid_attr_val_t hv;

			hv.lng = (n == target);
			if (rst.sub.dlg[rst.wlab[n]].val.lng != hv.lng)
				rnd_gui->attr_dlg_set_value(rst.sub.dlg_hid_ctx, rst.wchk[n], &hv);

			hv.str = PCB->RouteStyle.array[n].name;
			if (strcmp(rst.sub.dlg[rst.wlab[n]].name, hv.str) != 0)
				rnd_gui->attr_dlg_set_value(rst.sub.dlg_hid_ctx, rst.wlab[n], &hv);
		}
		if (vtroutestyle_len(&PCB->RouteStyle) != rst.last_len) {
			rst.last_len = vtroutestyle_len(&PCB->RouteStyle);
			for(n = 0; n < MAX_STYLES; n++)
				rnd_gui->attr_dlg_widget_hide(rst.sub.dlg_hid_ctx, rst.whbox[n], n >= rst.last_len);
		}
		rstdlg_pcb2dlg(target);
	}
	rst_lock--;
}

static void rst_select_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int n, ridx = -1;
	for(n = 0; n < vtroutestyle_len(&PCB->RouteStyle); n++) {
		if ((attr == &rst.sub.dlg[rst.wlab[n]]) || (attr == &rst.sub.dlg[rst.wchk[n]])) {
			ridx = n;
			break;
		}
	}
	if (ridx < 0)
		return;
	pcb_use_route_style(&(PCB->RouteStyle.array[ridx]));
	rst_force_update_chk_and_dlg();
}

static void rst_edit_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int target = pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL);
	if (target >= 0)
		pcb_dlg_rstdlg(target);
}

static void rst_new_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int target = pcb_route_style_new(PCB, "new style", 1);
	pcb_use_route_style(&(PCB->RouteStyle.array[target]));
	pcb_dlg_rstdlg(target);
}

static void rst_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int target = pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL);
	if (target >= 0) {
		pcb_route_style_del(PCB, target, 1);
		rst_updated(NULL);
		rst_force_update_chk_and_dlg();
	}
}

static void rst_docked_create()
{
	int n;
	RND_DAD_BEGIN_VBOX(rst.sub.dlg);
		for(n = 0; n < MAX_STYLES; n++) {
			RND_DAD_BEGIN_HBOX(rst.sub.dlg);
				RND_DAD_COMPFLAG(rst.sub.dlg, RND_HATF_HIDE);
					rst.whbox[n] = RND_DAD_CURRENT(rst.sub.dlg);
				RND_DAD_BOOL(rst.sub.dlg);
					rst.wchk[n] = RND_DAD_CURRENT(rst.sub.dlg);
					RND_DAD_CHANGE_CB(rst.sub.dlg, rst_select_cb);
				RND_DAD_LABEL(rst.sub.dlg, "unused");
					rst.wlab[n] = RND_DAD_CURRENT(rst.sub.dlg);
					RND_DAD_CHANGE_CB(rst.sub.dlg, rst_select_cb);
			RND_DAD_END(rst.sub.dlg);
		}

		RND_DAD_BEGIN_HBOX(rst.sub.dlg);
			RND_DAD_BUTTON(rst.sub.dlg, "New");
				RND_DAD_CHANGE_CB(rst.sub.dlg, rst_new_cb);
			RND_DAD_BUTTON(rst.sub.dlg, "Edit");
				RND_DAD_CHANGE_CB(rst.sub.dlg, rst_edit_cb);
			RND_DAD_BUTTON(rst.sub.dlg, "Del");
				RND_DAD_CHANGE_CB(rst.sub.dlg, rst_del_cb);
		RND_DAD_END(rst.sub.dlg);
	RND_DAD_END(rst.sub.dlg);
}


void pcb_rst_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	rst_update(hidlib);
}

void pcb_rst_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (RND_HAVE_GUI_ATTR_DLG) {
		rst_docked_create();
		if (rnd_hid_dock_enter(&rst.sub, RND_HID_DOCK_LEFT, "Route styles") == 0)
			rst.sub_inited = 1;
	}
	rst_update(hidlib);
}

void pcb_rst_update_conf(rnd_conf_native_t *cfg, int arr_idx)
{
	if ((PCB != NULL) && (rnd_gui != NULL)) {
		if (rnd_gui->update_menu_checkbox != NULL)
			rnd_gui->update_menu_checkbox(rnd_gui, NULL);
		if (rst.sub_inited)
			rst_force_update_chk_and_dlg();
	}
}
