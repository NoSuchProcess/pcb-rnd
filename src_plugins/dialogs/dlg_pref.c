/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

/* The preferences dialog, invoked by the Preferences() action */

#include "config.h"

#include <genvector/gds_char.h>
#include <limits.h>
#include "actions.h"
#include "build_run.h"
#include "pcb-printf.h"
#include "error.h"
#include "event.h"
#include "conf.h"
#include "conf_hid.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"

#include "dlg_pref.h"

#include "dlg_pref_sizes.c"
#include "dlg_pref_board.c"
#include "dlg_pref_general.c"
#include "dlg_pref_lib.c"
#include "dlg_pref_layer.c"
#include "dlg_pref_color.c"
#include "dlg_pref_win.c"
#include "dlg_pref_conf.c"

pref_ctx_t pref_ctx;
static const char *pref_cookie = "preferences dialog";
conf_hid_id_t pref_hid;

static const char *role_names[] =  { "user",   "project",   "design",   "cli", NULL };
static const conf_role_t roles[] = { CFR_USER, CFR_PROJECT, CFR_DESIGN, CFR_CLI, 0 };

void pcb_pref_conf2dlg_item(conf_native_t *cn, pref_confitem_t *item)
{
	switch(cn->type) {
		case CFN_COORD:
			PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, coord_value, cn->val.coord[0]);
			break;
		case CFN_BOOLEAN:
		case CFN_INTEGER:
			PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, int_value, cn->val.integer[0]);
			break;
		case CFN_REAL:
			PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, real_value, cn->val.real[0]);
			break;
		case CFN_STRING:
			PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, item->wid, str_value, cn->val.string[0]);
			break;
		default: pcb_message(PCB_MSG_ERROR, "pcb_pref_conf2dlg_item(): widget type not handled\n");
	}
}

void pcb_pref_dlg2conf_item(pref_ctx_t *ctx, pref_confitem_t *item, pcb_hid_attribute_t *attr)
{
	pref_confitem_t *old = ctx->conf_lock;
	conf_native_t *cn = conf_get_field(item->confpath);

	if (cn == NULL)
		return;

	ctx->conf_lock = item;
	switch(cn->type) {
		case CFN_COORD:
			if (cn->val.coord[0] != attr->val.coord_value)
				conf_setf(ctx->role, item->confpath, -1, "%.8$mm", attr->val.coord_value);
			break;
		case CFN_BOOLEAN:
		case CFN_INTEGER:
			if (cn->val.integer[0] != attr->val.int_value)
				conf_setf(ctx->role, item->confpath, -1, "%d", attr->val.int_value);
			break;
		case CFN_REAL:
			if (cn->val.real[0] != attr->val.real_value)
				conf_setf(ctx->role, item->confpath, -1, "%f", attr->val.real_value);
			break;
		case CFN_STRING:
			if (strcmp(cn->val.string[0], attr->val.str_value) != 0)
				conf_set(ctx->role, item->confpath, -1, attr->val.str_value, POL_OVERWRITE);
			break;
		default: pcb_message(PCB_MSG_ERROR, "pcb_pref_dlg2conf_item(): widget type not handled\n");
	}
	ctx->conf_lock = old;
}

pcb_bool pcb_pref_dlg2conf_table(pref_ctx_t *ctx, pref_confitem_t *list, pcb_hid_attribute_t *attr)
{
	pref_confitem_t *c;
	int wid = attr - ctx->dlg;

	for(c = list; c->confpath != NULL; c++) {
		if (c->wid == wid) {
			pcb_pref_dlg2conf_item(ctx, c, attr);
			return 1;
		}
	}
	return 0;
}


void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_confitem_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr))
{
	conf_native_t *cn = conf_get_field(item->confpath);

	if (cn == NULL) {
		pcb_message(PCB_MSG_ERROR, "Internal error: pcb_pref_create_conf_item(): invalid conf node %s\n", item->confpath);
		item->wid = -1;
		return;
	}

	PCB_DAD_LABEL(ctx->dlg, item->label);
		PCB_DAD_HELP(ctx->dlg, cn->description);

	switch(cn->type) {
		case CFN_COORD:
			PCB_DAD_COORD(ctx->dlg, "");
				item->wid = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, 0, PCB_MAX_COORD);
				PCB_DAD_DEFAULT_NUM(ctx->dlg, cn->val.coord[0]);
				PCB_DAD_HELP(ctx->dlg, cn->description);
				PCB_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case CFN_BOOLEAN:
			PCB_DAD_BOOL(ctx->dlg, "");
				item->wid = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_DEFAULT_NUM(ctx->dlg, cn->val.integer[0]);
				PCB_DAD_HELP(ctx->dlg, cn->description);
				PCB_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case CFN_INTEGER:
			PCB_DAD_INTEGER(ctx->dlg, "");
				item->wid = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, 0, INT_MAX);
				PCB_DAD_DEFAULT_NUM(ctx->dlg, cn->val.integer[0]);
				PCB_DAD_HELP(ctx->dlg, cn->description);
				PCB_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case CFN_REAL:
			PCB_DAD_REAL(ctx->dlg, "");
				item->wid = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_MINMAX(ctx->dlg, 0, INT_MAX);
				ctx->dlg[item->wid].val.real_value = cn->val.real[0];
				PCB_DAD_HELP(ctx->dlg, cn->description);
				PCB_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		case CFN_STRING:
			PCB_DAD_STRING(ctx->dlg);
				item->wid = PCB_DAD_CURRENT(ctx->dlg);
				ctx->dlg[item->wid].val.str_value = pcb_strdup(cn->val.string[0]);
				PCB_DAD_HELP(ctx->dlg, cn->description);
				PCB_DAD_CHANGE_CB(ctx->dlg, change_cb);
			break;
		default:
			PCB_DAD_LABEL(ctx->dlg, "Internal error: pcb_pref_create_conf_item(): unhandled type");
			item->wid = -1;
			return;
	}

	item->cnext = conf_hid_get_data(cn, pref_hid);
	conf_hid_set_data(cn, pref_hid, item);
}

void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_confitem_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr))
{
	pref_confitem_t *c;
	for(c = list; c->confpath != NULL; c++)
		pcb_pref_create_conf_item(ctx, c, change_cb);
}

void pcb_pref_conflist_remove(pref_ctx_t *ctx, pref_confitem_t *list)
{
	pref_confitem_t *c;
	for(c = list; c->confpath != NULL; c++) {
		conf_native_t *cn = conf_get_field(c->confpath);
		c->cnext = NULL;
		if (cn != NULL)
			conf_hid_set_data(cn, pref_hid, NULL);
	}
}

static void pref_role_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	ctx->role = roles[attr->val.int_value];
}

static void pref_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pref_ctx_t *ctx = caller_data;

	pcb_dlg_pref_sizes_close(ctx);
	pcb_dlg_pref_board_close(ctx);
	pcb_dlg_pref_general_close(ctx);
	pcb_dlg_pref_lib_close(ctx);
	pcb_dlg_pref_color_close(ctx);
	pcb_dlg_pref_win_close(ctx);

	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(pref_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

/* Compare inp to fixed in case insensitive manner; accept full length match
   or if inp contains the first word of fixed. Returns 0 on accept. */
static int pref_strcmp(const char *fixed, const char *inp)
{
	for(;;) {
		if ((*inp == '\0') && ((*fixed == '\0') || (*fixed == ' ')))
			return 0;
		if (tolower(*fixed) != tolower(*inp))
			return -1;
		fixed++;
		inp++;
	}
}

static void pcb_dlg_pref(const char *target_tab_str, const char *tabarg)
{
	const char *tabs[] = { "General", "Board meta", "Sizes & DRC",  "Library", "Layers", "Colors", "Window", "Config tree", NULL };
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	int target_tab = -1;

	if ((target_tab_str != NULL) && (*target_tab_str != '\0')) {
		const char **t;
		int tt;

		for(tt = 0, t = tabs; *t != NULL; t++,tt++) {
			if (pref_strcmp(*t, target_tab_str) == 0) {
				target_tab = tt;
				break;
			}
		}
	}

	if (pref_ctx.active) {
		if (target_tab >= 0)
			PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.wtab, int_value, target_tab);
		return;
	}

	PCB_DAD_BEGIN_VBOX(pref_ctx.dlg);
		PCB_DAD_COMPFLAG(pref_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABBED(pref_ctx.dlg, tabs);
			PCB_DAD_COMPFLAG(pref_ctx.dlg, PCB_HATF_EXPFILL);
			pref_ctx.wtab = PCB_DAD_CURRENT(pref_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* General */
				pcb_dlg_pref_general_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Board meta */
				pcb_dlg_pref_board_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Sizes & DRC */
				pcb_dlg_pref_sizes_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Library */
				pcb_dlg_pref_lib_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Layers */
				pcb_dlg_pref_layer_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Colors */
				pcb_dlg_pref_color_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Window */
				pcb_dlg_pref_win_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Config tree */
				pcb_dlg_pref_conf_create(&pref_ctx);
			PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_END(pref_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(pref_ctx.dlg);
			PCB_DAD_BEGIN_HBOX(pref_ctx.dlg);
				PCB_DAD_COMPFLAG(pref_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(pref_ctx.dlg, "All changes are made to role:");
				PCB_DAD_ENUM(pref_ctx.dlg, role_names);
					pref_ctx.wrole = PCB_DAD_CURRENT(pref_ctx.dlg);
					PCB_DAD_CHANGE_CB(pref_ctx.dlg, pref_role_cb);
				PCB_DAD_BUTTON_CLOSES_NAKED(pref_ctx.dlg, clbtn);
			PCB_DAD_END(pref_ctx.dlg);
		PCB_DAD_END(pref_ctx.dlg);
	PCB_DAD_END(pref_ctx.dlg);

	/* set up the context */
	pref_ctx.active = 1;

	PCB_DAD_NEW("preferences", pref_ctx.dlg, "pcb-rnd preferences", &pref_ctx, pcb_false, pref_close_cb);

	PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.wrole, int_value, 2);
	pref_ctx.role = CFR_DESIGN;

	pcb_dlg_pref_lib_open(&pref_ctx);
	pcb_dlg_pref_color_open(&pref_ctx);
	pcb_dlg_pref_win_open(&pref_ctx);
	pcb_dlg_pref_conf_open(&pref_ctx, (target_tab == sizeof(tabs)/sizeof(tabs[0]) - 2) ? tabarg : NULL);
	if (target_tab >= 0)
		PCB_DAD_SET_VALUE(pref_ctx.dlg_hid_ctx, pref_ctx.wtab, int_value, target_tab);
}

static void pref_ev_board_changed(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	pref_sizes_brd2dlg(ctx);
	pref_board_brd2dlg(ctx);
	pref_color_brd2dlg(ctx);
	pref_win_brd2dlg(ctx);
}

static void pref_ev_board_meta_changed(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	pref_sizes_brd2dlg(ctx);
	pref_board_brd2dlg(ctx);
	pref_color_brd2dlg(ctx);
	pref_win_brd2dlg(ctx);
}

void pref_conf_changed(conf_native_t *cfg, int arr_idx)
{
	pref_confitem_t *i;

	for(i = conf_hid_get_data(cfg, pref_hid); i != NULL; i = i->cnext)
		if (i != pref_ctx.conf_lock)
			pcb_pref_conf2dlg_item(cfg, i);

	pcb_pref_dlg_conf_changed_cb(&pref_ctx, cfg, arr_idx);
}

static conf_hid_callbacks_t pref_conf_cb;
void pcb_dlg_pref_init(void)
{
	pref_conf_cb.val_change_post = pref_conf_changed;
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pref_ev_board_changed, &pref_ctx, pref_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, pref_ev_board_meta_changed, &pref_ctx, pref_cookie);
	pref_hid = conf_hid_reg(pref_cookie, &pref_conf_cb);
	pcb_dlg_pref_sizes_init(&pref_ctx);
	pcb_dlg_pref_lib_init(&pref_ctx);
}

void pcb_dlg_pref_uninit(void)
{
	pcb_event_unbind_allcookie(pref_cookie);
	conf_hid_unreg(pref_cookie);
}

const char pcb_acts_Preferences[] = "Preferences([tabname])\n";
const char pcb_acth_Preferences[] = "Present the preferences dialog, optionally opening the tab requested.";
fgw_error_t pcb_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *tab = NULL, *tabarg = NULL;
	PCB_ACT_MAY_CONVARG(1, FGW_STR, Preferences, tab = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Preferences, tabarg = argv[2].val.str);
	pcb_dlg_pref(tab, tabarg);
	PCB_ACT_IRES(0);
	return 0;
}
