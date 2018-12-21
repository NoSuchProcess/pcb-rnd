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

#include "config.h"

#include <genht/hash.h>

#include "board.h"
#include "actions.h"
#include "hid_dad.h"

#include "props.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_propedit_t pe;
	int wtree, wfilter, wtype;
} propdlg_t;

static void propdlgclose_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	propdlg_t *ctx = caller_data;
	pcb_props_uninit(&ctx->pe);
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void prop_prv_expose_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	
}


static pcb_bool prop_prv_mouse_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false; /* don't redraw */
}


static void build_propval(propdlg_t *ctx)
{
	static const char *type_tabs[] = {"string", "coord", "angle", "int", NULL};

	PCB_DAD_BEGIN_TABBED(ctx->dlg, type_tabs);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_HIDE_TABLAB);
		ctx->wtype = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: string");
			PCB_DAD_STRING(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: coord");
			PCB_DAD_COORD(ctx->dlg, "");
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: angle");
			PCB_DAD_REAL(ctx->dlg, "");
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: int");
			PCB_DAD_INTEGER(ctx->dlg, "");
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}

static void pcb_dlg_propdlg(propdlg_t *ctx)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static pcb_box_t prvbb = {0, 0, PCB_MM_TO_COORD(10), PCB_MM_TO_COORD(10)};

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HPANE(ctx->dlg);

			/* left: property tree and filter */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_TREE(ctx->dlg, 1, 1, NULL);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					ctx->wtree = PCB_DAD_CURRENT(ctx->dlg);
/*					PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, dlg_conf_select_node_cb);*/
/*					PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);*/
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_STRING(ctx->dlg);
						PCB_DAD_HELP(ctx->dlg, "Filter text:\nlist properties with\nmatching name only");
/*						PCB_DAD_CHANGE_CB(ctx->dlg, pcb_pref_dlg_conf_filter_cb);*/
						ctx->wfilter = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_BUTTON(ctx->dlg, "add");
						PCB_DAD_HELP(ctx->dlg, "Create a new attribute\n(in the a/ subtree)");
					PCB_DAD_BUTTON(ctx->dlg, "del");
						PCB_DAD_HELP(ctx->dlg, "Remove the selected attribute\n(from the a/ subtree)");
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

			/* right: preview and per type edit */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_PREVIEW(ctx->dlg, prop_prv_expose_cb, prop_prv_mouse_cb, NULL, &prvbb, 200, 200, ctx);
				build_propval(ctx);
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW("propedit", ctx->dlg, "Property editor", ctx, pcb_true, propdlgclose_cb);
}

extern int prop_scope_add(pcb_propedit_t *pe, const char *cmd, int quiet);

const char pcb_acts_propedit[] = "propedit(object[:id]|layer[:id]|layergrp[:id]|pcb|selection|selected)\n";
const char pcb_acth_propedit[] = "";
fgw_error_t pcb_act_propedit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	propdlg_t *ctx = calloc(sizeof(propdlg_t), 1);
	int a, r;

	pcb_props_init(&ctx->pe, PCB);

	for(a = 1; a < argc; a++) {
		const char *cmd;
		PCB_ACT_CONVARG(a, FGW_STR, propedit, cmd = argv[a].val.str);
		r = prop_scope_add(&ctx->pe, cmd, 0);
		if (r != 0)
			return r;
	}

	pcb_dlg_propdlg(ctx);
	return 0;
}
