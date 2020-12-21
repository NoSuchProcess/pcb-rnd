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
#include <librnd/core/safe_fs.h>

extern conf_dialogs_t dialogs_conf;

#define GET_ROW(ctx) \
	rnd_hid_attribute_t *lattr= &((pref_ctx_t *)ctx)->dlg[((pref_ctx_t *)ctx)->menu.wlist]; \
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(lattr); \

#define GET_ROW_AND_MENU(ctx) \
	GET_ROW(ctx) \
	rnd_menu_patch_t *m; \
	if (r == NULL) {rnd_message(RND_MSG_ERROR, "Select a menu file first\n"); return; } \
	m = r->user_data; \
	if (m == NULL) {rnd_message(RND_MSG_ERROR, "Invalid menu file selection\n"); return; } \

/* Update button states: disable or enable them depending on what kind of
   menu patch row is selected from the list */
static void pref_menu_btn_update(pref_ctx_t *ctx)
{
	rnd_menu_patch_t *m;
	GET_ROW(ctx);

	if ((r == NULL) || (r->user_data == NULL)) {
		rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->menu.wunload, 0);
		rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->menu.wreload, 0);
		rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->menu.wexport, 0);
		return;
	}

	m = r->user_data;
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->menu.wunload, 1);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->menu.wreload, m->has_file);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, ctx->menu.wexport, 1);
}

static void pref_menu_brd2dlg(pref_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cursor_path = NULL, *cell[6];
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
	pref_menu_btn_update(ctx);
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

	pref_menu_btn_update(ctx);
}



static void pref_menu_load(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char *fn = rnd_gui->fileselect(rnd_gui, "menu patch load", "Load a menu patch to file", "menu_patch.lht", "lht", NULL, "menu_patch_load", RND_HID_FSD_READ, NULL);
	if (fn == NULL)
		return;
	if (rnd_hid_menu_load(rnd_gui, NULL, "preferences", 300, fn, 1, NULL, "User reuqested load through the preferences dialog") == NULL)
		rnd_message(RND_MSG_ERROR, "Failed to load/parse menu file '%s' - menu file not loaded\n", fn);
	free(fn);
}

static void pref_menu_unload(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	GET_ROW_AND_MENU(caller_data);
	rnd_hid_menu_unload_patch(rnd_gui, m);
}

static void pref_menu_reload(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	const char *fn;
	GET_ROW_AND_MENU(caller_data);

	fn = m->cfg.doc->root->file_name;
	rnd_hid_menu_merge_inhibit_inc();
	if (rnd_hid_menu_load(rnd_gui, NULL, m->cookie, m->prio, fn, 1, NULL, m->desc) == NULL)
		rnd_message(RND_MSG_ERROR, "Failed to load/parse menu file '%s' - menu file not reloaded\n", fn);
	else
		rnd_hid_menu_unload_patch(rnd_gui, m);
	rnd_hid_menu_merge_inhibit_dec();
}

static void pref_menu_export(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	FILE *f;
	char *fn;
	GET_ROW_AND_MENU(caller_data);

	fn = rnd_gui->fileselect(rnd_gui, "menu patch export", "Export a menu patch to file for debugging", "menu_patch.lht", "lht", NULL, "menu_patch_export", RND_HID_FSD_MAY_NOT_EXIST, NULL);
	if (fn == NULL)
		return;

	f = rnd_fopen(NULL, fn, "w");
	lht_dom_export(m->cfg.doc->root, f, "");
	fclose(f);
	free(fn);
}

#define NONPERS "\nNon-persistent: the file not will be loaded automatically\nafter pcb-rnd is restarted"

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
	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Load...");
			ctx->menu.wload = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_CHANGE_CB(ctx->dlg, pref_menu_load);
			RND_DAD_HELP(ctx->dlg, "Load a new menu (patch) file" NONPERS);
		RND_DAD_BUTTON(ctx->dlg, "Unload");
			ctx->menu.wunload = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_CHANGE_CB(ctx->dlg, pref_menu_unload);
			RND_DAD_HELP(ctx->dlg, "Remove the selected menu (patch) from the menu system" NONPERS);
		RND_DAD_BUTTON(ctx->dlg, "Reload");
			ctx->menu.wreload = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_CHANGE_CB(ctx->dlg, pref_menu_reload);
			RND_DAD_HELP(ctx->dlg, "Reload the menu file from disk\nand re-merge the menu system");
		RND_DAD_BUTTON(ctx->dlg, "Export...");
			ctx->menu.wexport = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_CHANGE_CB(ctx->dlg, pref_menu_export);
			RND_DAD_HELP(ctx->dlg, "Export menu file to disk\n(useful for debuggin)");
	RND_DAD_END(ctx->dlg);
}
