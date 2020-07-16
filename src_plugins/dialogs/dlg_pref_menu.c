/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Preferences dialog, menu tab */

#include "dlg_pref.h"
#include <librnd/core/hid_menu.h>

extern conf_dialogs_t dialogs_conf;
extern void pcb_wplc_save_to_role(rnd_conf_role_t role);
extern int pcb_wplc_save_to_file(const char *fn);

static void pref_menu_brd2dlg(pref_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cursor_path = NULL, *cell[5];
	long n;

	attr = &ctx->dlg[ctx->menu.wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all items */
	for(n = 0; n < rnd_menu_sys.patches.used; n++) {
		rnd_menu_patch_t *m = rnd_menu_sys.patches.array[n];
		rnd_hid_row_t *row;
		const char *fn = m->cfg.doc->root->file_name;

		cell[0] = rnd_strdup_printf("%ld", m->uid);
		cell[1] = rnd_strdup((n == 0 ? "base " : "addon"));
		cell[2] = rnd_strdup_printf("%d", m->prio);
		cell[3] = rnd_strdup_printf("%s", m->cookie);
		cell[4] = rnd_strdup_printf("%s", fn == NULL ? "" : fn);
		cell[5] = NULL;

		row = rnd_dad_tree_append(attr, NULL, cell);
		row->user_data = m;
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->menu.wlist, &hv);
		free(cursor_path);
	}
}

void pcb_dlg_pref_menu_open(pref_ctx_t *ctx)
{
	pref_menu_brd2dlg(ctx);
}

void pcb_dlg_pref_menu_close(pref_ctx_t *ctx)
{
}

static void menu_select(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_attr_val_t hv;
	rnd_hid_tree_t *tree = attrib->wdata;
	pref_ctx_t *ctx = tree->user_ctx;
	rnd_menu_patch_t *m;

	if ((row == NULL) || (row->user_data == NULL)) {
		hv.str = "";
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->menu.wdesc, &hv);
		return;
	}

	m = row->user_data;
	hv.str = m->desc == NULL ? "" : m->desc;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->menu.wdesc, &hv);


}


void pcb_dlg_pref_menu_create(pref_ctx_t *ctx)
{
	static const char *hdr[] = {"uid", "type", "prio", "cookie", "file name", NULL};

	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
	RND_DAD_TREE(ctx->dlg, 5, 0, hdr);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
		ctx->menu.wlist = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, menu_select);
		RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_LABEL(ctx->dlg, "Description:");
		RND_DAD_LABEL(ctx->dlg, "");
		ctx->menu.wdesc = RND_DAD_CURRENT(ctx->dlg);
	RND_DAD_END(ctx->dlg);
}
