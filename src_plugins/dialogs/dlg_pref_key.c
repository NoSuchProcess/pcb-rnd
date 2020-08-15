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
#include <librnd/core/hid_cfg_input.h>

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

	if ((pref_ctx.key.lock) || (!pref_ctx.active))
		return;

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

static lht_node_t *pref_key_mod_pre(pref_ctx_t *ctx)
{
/*	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->key.wlist];
	rnd_hid_tree_t *tree = attr->wdata;*/
	lht_node_t *lst, *m;
	rnd_conf_role_t save;

	save = ctx->role;
	ctx->role = RND_CFR_USER;
	m = pref_dlg2conf_pre(ctx);
	if (m == NULL) {
		ctx->role = save;
		return NULL;
	}

	ctx->key.lock++;
	/* get the list and clean it */

	lst = lht_tree_path_(m->doc, m, "editor/translate_key", 1, 0, NULL);
	if (lst == NULL)
		rnd_conf_set(RND_CFR_USER, "editor/translate_key", 0, "", RND_POL_OVERWRITE);
	lst = lht_tree_path_(m->doc, m, "editor/translate_key", 1, 0, NULL);
	ctx->role = save;

	assert(lst != NULL);
	return lst;
}

static void pref_key_mod_post(pref_ctx_t *ctx)
{
/*	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->key.wlist];
	rnd_hid_tree_t *tree = attr->wdata;*/
	rnd_conf_role_t save;

	save = ctx->role;
	ctx->role = RND_CFR_USER;

	rnd_conf_update("editor/translate_key", -1);
	rnd_conf_makedirty(ctx->role); /* low level lht_dom_node_alloc() wouldn't make user config to be saved! */

	pref_dlg2conf_post(ctx);

	ctx->role = save;
	ctx->key.lock--;
}


void pcb_dlg_pref_key_open(pref_ctx_t *ctx)
{
	pref_key_brd2dlg(ctx);
}

static void pref_key_remove(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *battr)
{
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.key.wlist];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *r, *row = rnd_dad_tree_get_selected(attr);
	lht_node_t *nd, *lst = pref_key_mod_pre(&pref_ctx);

	if ((row == NULL) || (lst == NULL))
		return;

	for(nd = lst->data.list.first, r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r), nd = nd->next) {
		if (r == row) {
			rnd_dad_tree_remove(attr, r);
			lht_tree_del(nd);
			break;
		}
	}

	pref_key_mod_post(&pref_ctx);
}

/*** key detector GUI ***/

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int wkdesc, wtdesc;
} kd_ctx_t;

static kd_ctx_t kd;

void dummy_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
}

rnd_bool key_press_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_bool release, rnd_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr)
{
	rnd_hid_attr_val_t hv;
	char *desc;

	if (release)
		return 0;

	desc = rnd_hid_cfg_keys_gen_desc(mods, key_raw, 0);
	if (desc == NULL) {
/* Can't do this yet because mods are passed on too:
		rnd_message(RND_MSG_ERROR, "Failed to recognize that key (%d %d)\n", key_raw, key_tr);*/
		return 0;
	}
	hv.str = desc;
	rnd_gui->attr_dlg_set_value(kd.dlg_hid_ctx, kd.wkdesc, &hv);

	free(desc);
	return 0;
}

static void pref_key_append(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *battr)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"add", 0}, {NULL, 0}};
	rnd_hid_attribute_t *attr = &pref_ctx.dlg[pref_ctx.key.wlist];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(attr);
	lht_node_t *nd, *lst = pref_key_mod_pre(&pref_ctx);
	rnd_box_t vbox = {0, 0, RND_MM_TO_COORD(55), RND_MM_TO_COORD(55)};
	int res;

	if (lst == NULL)
		return;

	RND_DAD_BEGIN_VBOX(kd.dlg);
		RND_DAD_COMPFLAG(kd.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HBOX(kd.dlg);
			RND_DAD_LABEL(kd.dlg, "Key pressed:");
			RND_DAD_STRING(kd.dlg);
				kd.wkdesc = RND_DAD_CURRENT(kd.dlg);
				RND_DAD_HELP(kd.dlg, "Enter a key description here\nTypical examples: <key>t or Alt<key>w\nOr use the key detection box on the right");
			RND_DAD_PREVIEW(kd.dlg,  dummy_expose_cb, NULL, key_press_cb, NULL, &vbox, 20, 20, NULL);
				RND_DAD_COMPFLAG(kd.dlg, RND_HATF_FRAME);
				RND_DAD_HELP(kd.dlg, "Click here then press a key and it will be filled in automatically");
		RND_DAD_END(kd.dlg);
		RND_DAD_BEGIN_HBOX(kd.dlg);
			RND_DAD_LABEL(kd.dlg, "Translated to:");
			RND_DAD_STRING(kd.dlg);
				kd.wtdesc = RND_DAD_CURRENT(kd.dlg);
			RND_DAD_HELP(kd.dlg, "Enter a key description here\nTypical examples: <key>t or <char>|");
		RND_DAD_END(kd.dlg);
		RND_DAD_BUTTON_CLOSES(kd.dlg, clbtn);
	RND_DAD_END(kd.dlg);

	RND_DAD_NEW("pref_key_set", kd.dlg, "set key translation", /**/NULL, rnd_true, NULL);
	res = RND_DAD_RUN(kd.dlg);

	if (res == 0) {
			const char *k1 = kd.dlg[kd.wkdesc].val.str, *k2 = kd.dlg[kd.wtdesc].val.str;
			char *cell[3];

			cell[0] = rnd_strdup(k1);
			cell[1] = rnd_strdup(k2);
			cell[2] = NULL;
			row = rnd_dad_tree_append(attr, NULL, cell);

			nd = lht_dom_node_alloc(LHT_TEXT, k1);
			nd->data.text.value = rnd_strdup(k2);
			lht_dom_list_append(lst, nd);
	}

	pref_key_mod_post(&pref_ctx);

	RND_DAD_FREE(kd.dlg);
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
			RND_DAD_CHANGE_CB(ctx->dlg, pref_key_remove);
		RND_DAD_BUTTON(ctx->dlg, "Add new...");
			RND_DAD_CHANGE_CB(ctx->dlg, pref_key_append);
	RND_DAD_END(ctx->dlg);
}
