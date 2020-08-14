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

/* Preferences dialog, key translation tab */

#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>

static void pref_key_brd2dlg(pref_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *row, *r;
	rnd_conf_native_t *nat = rnd_conf_get_field("editor/translate_key");
	char *cursor_path = NULL, *cell[3];
	long n;
	gdl_iterator_t it;
	rnd_conf_listitem_t *kt;

	attr = &ctx->dlg[ctx->key.wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if ((r != NULL) && (nat != NULL))
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	if (nat == NULL)
		return;

	/* add all items */
	rnd_conflist_foreach(nat->val.list, &it, kt) {
		cell[0] = rnd_strdup(kt->name);
		cell[1] = rnd_strdup(kt->payload);
		row = rnd_dad_tree_append(attr, NULL, cell);
		row->user_data = kt;
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->menu.wlist, &hv);
		free(cursor_path);
	}
}

void pcb_dlg_pref_key_open(pref_ctx_t *ctx)
{
	pref_key_brd2dlg(ctx);
}


void pcb_dlg_pref_key_create(pref_ctx_t *ctx)
{
	static const char *hdr[] = {"pressed", "translated to", NULL};

	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME | RND_HATF_SCROLL | RND_HATF_EXPFILL);
		RND_DAD_TREE(ctx->dlg, 3, 0, hdr);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			ctx->key.wlist = RND_DAD_CURRENT(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Remove");
		RND_DAD_BUTTON(ctx->dlg, "Add new...");
	RND_DAD_END(ctx->dlg);
}
