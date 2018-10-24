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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* The preferences dialog, invoked by the Preferences() action */

#include <genvector/gds_char.h>
#include "build_run.h"
#include "pcb-printf.h"
#include "dlg_pref.h"
#include "error.h"
#include "conf.h"
#include "conf_hid.h"

#include "dlg_pref_sizes.c"

pref_ctx_t pref_ctx;
static const char *pref_cookie = "preferences dialog";
conf_hid_id_t pref_hid;

void pcb_pref_conf2dlg_item(conf_native_t *cn, pref_conflist_t *item)
{
pcb_trace("Conf change: '%s'\n", item->label);
}

void pcb_pref_dlg2conf_item(pref_ctx_t *ctx, pref_conflist_t *item, pcb_hid_attribute_t *attr)
{
	pref_conflist_t *old = ctx->conf_lock;
	conf_native_t *cn = conf_get_field(item->confpath);

	ctx->conf_lock = item;
	switch(cn->type) {
		case CFN_COORD:
			conf_setf(CFR_DESIGN, item->confpath, -1, "%.8$mm", attr->default_val.coord_value);
			break;
	}
	ctx->conf_lock = old;
}

void pcb_pref_dlg2conf_table(pref_ctx_t *ctx, pref_conflist_t *list, pcb_hid_attribute_t *attr)
{
	pref_conflist_t *c;
	int wid = attr - ctx->dlg;

	for(c = list; c->confpath != NULL; c++) {
		if (c->wid == wid) {
			pcb_pref_dlg2conf_item(ctx, c, attr);
			return;
		}
	}
	pcb_message(PCB_MSG_ERROR, "pcb_pref_dlg2conf_table(): widget not found\n");
}


void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_conflist_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr))
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
				PCB_DAD_DEFAULT(ctx->dlg, cn->val.coord[0]);
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

void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_conflist_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr))
{
	pref_conflist_t *c;
	for(c = list; c->confpath != NULL; c++)
		pcb_pref_create_conf_item(ctx, c, change_cb);
}


static void pref_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pref_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(pref_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}


static void pcb_dlg_pref(void)
{
	const char *tabs[] = { "General", "Window", "Sizes & DRC",  "Library", "Layers", "Colors", "Config tree", NULL };

	if (pref_ctx.active)
		return;

	PCB_DAD_BEGIN_TABBED(pref_ctx.dlg, tabs);
		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* General */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Window */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Sizes & DRC */
			DLG_PREF_SIZES_DAD;
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Library */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Layers */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Colors */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(pref_ctx.dlg); /* Config tree */
			PCB_DAD_LABEL(pref_ctx.dlg, "TODO");
		PCB_DAD_END(pref_ctx.dlg);

	PCB_DAD_END(pref_ctx.dlg);

	/* set up the context */
	pref_ctx.active = 1;

	PCB_DAD_NEW(pref_ctx.dlg, "pcb-rnd preferences", "", &pref_ctx, pcb_false, pref_close_cb);
}

static void pref_ev_board_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	pref_sizes_brd2dlg(ctx);
}

static void pref_ev_board_meta_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pref_ctx_t *ctx = user_data;
	if (!pref_ctx.active)
		return;

	pref_sizes_brd2dlg(ctx);
}

void pref_conf_changed(conf_native_t *cfg, int arr_idx)
{
	pref_conflist_t *i;

	for(i = conf_hid_get_data(cfg, pref_hid); i != NULL; i = i->cnext)
		if (i != pref_ctx.conf_lock)
			pcb_pref_conf2dlg_item(cfg, i);
}

static conf_hid_callbacks_t pref_conf_cb;
static void dlg_pref_init(void)
{
	pref_conf_cb.val_change_post = pref_conf_changed;
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, pref_ev_board_changed, &pref_ctx, pref_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, pref_ev_board_meta_changed, &pref_ctx, pref_cookie);
	pref_hid = conf_hid_reg(pref_cookie, &pref_conf_cb);
}

static void dlg_pref_uninit(void)
{
	pcb_event_unbind_allcookie(pref_cookie);
	conf_hid_unreg(pref_cookie);
}

static const char pcb_acts_Preferences[] = "Preferences()\n";
static const char pcb_acth_Preferences[] = "Present the preferences dialog";
static fgw_error_t pcb_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_pref();
	PCB_ACT_IRES(0);
	return 0;
}
