/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

/* optional, generic toolbar docked DAD dialog that uses librnd tool infra
   (and in turn works from the menu file) */

#include <librnd/config.h>

#include <genvector/vti0.h>
#include <liblihata/tree.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/tool.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/conf_hid.h>

#include "toolbar.h"

typedef struct {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited, lock;
	vti0_t tid2wid; /* tool ID to widget ID conversion - value 0 means no widget */
} toolbar_ctx_t;

static toolbar_ctx_t toolbar;

static void toolbar_pcb2dlg()
{
	pcb_toolid_t tid;

	if (!toolbar.sub_inited)
		return;

	toolbar.lock = 1;

	for(tid = 0; tid < toolbar.tid2wid.used; tid++) {
		int st, wid = toolbar.tid2wid.array[tid];
		if (wid == 0)
			continue;
		st = (tid == pcbhl_conf.editor.mode) ? 2 : 1;
		pcb_gui->attr_dlg_widget_state(toolbar.sub.dlg_hid_ctx, wid, st);
	}
	toolbar.lock = 0;
}

static void toolbar_select_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ptrdiff_t tid;

	if (toolbar.lock)
		return;

	tid = (ptrdiff_t)attr->user_data;
	pcb_tool_select_by_id(pcb_gui->get_dad_hidlib(hid_ctx), tid);
}

static void toolbar_create_tool(pcb_toolid_t tid, pcb_tool_t *tool, const char *menufile_help)
{
	int wid;
	const char *help = tool->help;

	if (menufile_help != NULL)
		help = menufile_help;

	if (tool->icon != NULL)
		PCB_DAD_PICBUTTON(toolbar.sub.dlg, tool->icon);
	else
		PCB_DAD_BUTTON(toolbar.sub.dlg, tool->name);
	PCB_DAD_CHANGE_CB(toolbar.sub.dlg, toolbar_select_cb);
	PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_TIGHT | PCB_HATF_TOGGLE);
	if (help != NULL)
		PCB_DAD_HELP(toolbar.sub.dlg, help);
	wid = PCB_DAD_CURRENT(toolbar.sub.dlg);
	toolbar.sub.dlg[wid].user_data = (void *)(ptrdiff_t)tid;
	vti0_set(&toolbar.tid2wid, tid, wid);
}

static void toolbar_create_static(pcb_hid_cfg_t *cfg)
{
	const lht_node_t *t, *ts = pcb_hid_cfg_get_menu(cfg, "/toolbar_static");

	if ((ts != NULL) && (ts->type == LHT_LIST)) {
		for(t = ts->data.list.first; t != NULL; t = t->next) {
			pcb_toolid_t tid = pcb_tool_lookup(t->name);
			pcb_tool_t **tool;
			const char *mf_help;
			lht_node_t *nhelp;
			lht_err_t err;


			tool = (pcb_tool_t **)vtp0_get(&pcb_tools, tid, 0);
			if ((tid < 0) || (tool == NULL)) {
				pcb_message(PCB_MSG_ERROR, "toolbar: tool '%s' not found (referenced from the menu file %s:%d)\n", t->name, t->file_name, t->line);
				continue;
			}

			nhelp = lht_tree_path_(t->doc, t, "tip", 1, 0, &err);
			if ((nhelp != NULL) && (nhelp->type == LHT_TEXT))
				mf_help = nhelp->data.text.value;
			else
				mf_help = NULL;
			toolbar_create_tool(tid, *tool, mf_help);
		}
	}
	else {
		PCB_DAD_LABEL(toolbar.sub.dlg, "No toolbar found in the menu file.");
		PCB_DAD_HELP(toolbar.sub.dlg, "Check your menu file. If you use a locally modified or custom\nmenu file, make sure you merge upstream changes\n(such as the new toolbar subtree)");
	}
}

static void toolbar_create_dyn_all(void)
{
	pcb_tool_t **t;
	pcb_toolid_t tid;
	for(tid = 0, t = (pcb_tool_t **)pcb_tools.array; tid < pcb_tools.used; tid++,t++) {
		int *wid = vti0_get(&toolbar.tid2wid, tid, 0);
		if (((*t)->flags & PCB_TLF_AUTO_TOOLBAR) == 0)
			continue; /* static or inivisible */
		if ((wid != NULL) && (*wid != 0))
			continue; /* already has an icon */
		toolbar_create_tool(tid, *t, NULL);
	}
}

static void toolbar_docked_create(pcb_hid_cfg_t *cfg)
{
	toolbar.tid2wid.used = 0;

	PCB_DAD_BEGIN_HBOX(toolbar.sub.dlg);
		PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_EXPFILL | PCB_HATF_TIGHT);

		toolbar_create_static(cfg);
		toolbar_create_dyn_all();

	/* eat up remaining space in the middle before displaying the dynamic tools */
		PCB_DAD_BEGIN_HBOX(toolbar.sub.dlg);
			PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_END(toolbar.sub.dlg);

	/* later on dynamic tools would be displayed here */

	PCB_DAD_END(toolbar.sub.dlg);
}

static void toolbar_create(void)
{
	pcb_hid_cfg_t *cfg = pcb_gui->get_menu_cfg(pcb_gui);
	if (cfg == NULL)
		return;
	toolbar_docked_create(cfg);
	if (pcb_hid_dock_enter(&toolbar.sub, PCB_HID_DOCK_TOP_LEFT, "Toolbar") == 0) {
		toolbar.sub_inited = 1;
		toolbar_pcb2dlg();
	}
}

void pcb_toolbar_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL))
		toolbar_create();
}

void pcb_toolbar_reg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((toolbar.sub_inited) && (argv[1].type == PCB_EVARG_PTR)) {
		pcb_tool_t *tool = argv[1].d.p;
		pcb_toolid_t tid = pcb_tool_lookup(tool->name);
		if ((tool->flags & PCB_TLF_AUTO_TOOLBAR) != 0) {
			int *wid = vti0_get(&toolbar.tid2wid, tid, 0);
			if ((wid != NULL) && (*wid != 0))
				return;
			pcb_hid_dock_leave(&toolbar.sub);
			toolbar.sub_inited = 0;
			toolbar_create();
		}
	}
}

void pcb_toolbar_update_conf(conf_native_t *cfg, int arr_idx)
{
	toolbar_pcb2dlg();
}



static const char *toolbar_cookie = "lib_hid_pcbui/toolbar";

static conf_hid_id_t install_events(const char *cookie, const char *paths[], conf_hid_callbacks_t cb[], void (*update_cb)(conf_native_t*,int))
{
	const char **rp;
	conf_native_t *nat;
	int n;
	conf_hid_id_t conf_id;

	conf_id = pcb_conf_hid_reg(cookie, NULL);
	for(rp = paths, n = 0; *rp != NULL; rp++, n++) {
		memset(&cb[n], 0, sizeof(cb[0]));
		cb[n].val_change_post = update_cb;
		nat = pcb_conf_get_field(*rp);
		if (nat != NULL)
			pcb_conf_hid_set_cb(nat, conf_id, &cb[n]);
	}

	return conf_id;
}

void rnd_toolbar_uninit(void)
{
	pcb_event_unbind_allcookie(toolbar_cookie);
	pcb_conf_hid_unreg(toolbar_cookie);
}

void rnd_toolbar_init(void)
{
	const char *tpaths[] = {"editor/mode",  NULL};
	static conf_hid_callbacks_t tcb[sizeof(tpaths)/sizeof(tpaths[0])];

	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_toolbar_gui_init_ev, NULL, toolbar_cookie);
	pcb_event_bind(PCB_EVENT_TOOL_REG, pcb_toolbar_reg_ev, NULL, toolbar_cookie);
	install_events(toolbar_cookie, tpaths, tcb, pcb_toolbar_update_conf);
}
