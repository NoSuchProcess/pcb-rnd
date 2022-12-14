/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2021 Tibor 'Igor2' Palinkas
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

/* Preferences dialog, library tab */

#include <liblihata/tree.h>
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/paths.h>
#include <librnd/hid/hid_dad_tree.h>
#include <librnd/plugins/lib_hid_common/dlg_pref.h>

typedef struct pref_libhelp_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} pref_libhelp_ctx_t;

typedef struct {
	int wlist, whsbutton, wmoveup, wmovedown, wedit, wremove;
	int lock; /* a change in on the dialog box causes a change on the board but this shouldn't in turn casue a changein the dialog */
	char *cursor_path;
	pref_libhelp_ctx_t help;
} pref_lib_t;

#undef DEF_TABDATA
#define DEF_TABDATA pref_lib_t *tabdata = PREF_TABDATA(ctx)

static const char *SRC_BRD = "<board file>";

static void libhelp_btn(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr);

static void pref_lib_update_buttons(rnd_design_t *hl)
{
	pref_ctx_t *ctx = rnd_pref_get_ctx(hl);
	DEF_TABDATA;
	rnd_hid_attribute_t *attr = &ctx->dlg[tabdata->wlist];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);
	int en = (r != NULL);

	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, tabdata->wedit, en);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, tabdata->wremove, en);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, tabdata->wmoveup, en);
	rnd_gui->attr_dlg_widget_state(ctx->dlg_hid_ctx, tabdata->wmovedown, en);
}

static void pref_lib_select_cb(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	pref_lib_update_buttons(rnd_gui->get_dad_design(hid_ctx));
}

/* Current libraries from config to dialog box: remove everything from
   the widget first */
static void pref_lib_conf2dlg_pre(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	pref_ctx_t *ctx = user_data;
	DEF_TABDATA;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;

	if ((tabdata->lock) || (!ctx->active))
		return;

	attr = &ctx->dlg[tabdata->wlist];
	tree = attr->wdata;

	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL) {
		free(tabdata->cursor_path);
		tabdata->cursor_path = rnd_strdup(r->cell[0]);
	}

	/* remove all existing entries */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows)) {
		rnd_dad_tree_remove(attr, r);
	}
}

static const char *pref_node_src(lht_node_t *nd)
{
	if (nd->file_name != NULL)
		return nd->file_name;
	return rnd_conf_role_name(rnd_conf_lookup_role(nd));
}

/* Current libraries from config to dialog box: after the change, fill
   in all widget rows from the conf */
static void pref_lib_conf2dlg_post(rnd_conf_native_t *cfg, int arr_idx, void *user_data)
{
	pref_ctx_t *ctx = user_data;
	rnd_design_t *hl;
	DEF_TABDATA;
	rnd_conf_listitem_t *i;
	int idx;
	const char *s;
	char *cell[4];
	rnd_hid_attribute_t *attr;
	rnd_hid_attr_val_t hv;

	if ((tabdata->lock) || (!ctx->active))
		return;

	hl = rnd_gui->get_dad_design(ctx->dlg_hid_ctx);
	attr = &ctx->dlg[tabdata->wlist];

	/* copy everything from the config tree to the dialog */
	rnd_conf_loop_list_str(&conf_core.rc.library_search_paths, i, s, idx) {
		char *tmp;
		cell[0] = rnd_strdup(i->payload);
		rnd_path_resolve(hl, cell[0], &tmp, 0, rnd_false);
		cell[1] = rnd_strdup(tmp == NULL ? "" : tmp);
		cell[2] = rnd_strdup(pref_node_src(i->prop.src));
		cell[3] = NULL;
		rnd_dad_tree_append(attr, NULL, cell);
	}

	hv.str = tabdata->cursor_path;
	if (rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, tabdata->wlist, &hv) == 0) {
		free(tabdata->cursor_path);
		tabdata->cursor_path = NULL;
	}
	pref_lib_update_buttons(hl);
}

/* Dialog box to current libraries in config */
static void pref_lib_dlg2conf(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	rnd_hid_tree_t *tree = attr->wdata;
	lht_node_t *m, *lst, *nd;
	rnd_hid_row_t *r;


	m = rnd_pref_dlg2conf_pre(hl, ctx);
	if (m == NULL)
		return;

	tabdata->lock++;
	/* get the list and clean it */

	lst = lht_tree_path_(m->doc, m, "rc/library_search_paths", 1, 0, NULL);
	if (lst == NULL)
		rnd_conf_set(ctx->role, "rc/library_search_paths", 0, "", RND_POL_OVERWRITE);
	lst = lht_tree_path_(m->doc, m, "rc/library_search_paths", 1, 0, NULL);
	assert(lst != NULL);
	lht_tree_list_clean(lst);

	/* append items from the widget */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r)) {
		nd = lht_dom_node_alloc(LHT_TEXT, "");
		nd->data.text.value = rnd_strdup(r->cell[0]);
		nd->doc = m->doc;
		lht_dom_list_append(lst, nd);
		rnd_dad_tree_modify_cell(attr, r, 2, rnd_strdup(pref_node_src(nd)));
	}

	rnd_conf_update("rc/library_search_paths", -1);
	rnd_conf_makedirty(ctx->role); /* low level lht_dom_node_alloc() wouldn't make user config to be saved! */

	rnd_pref_dlg2conf_post(hl, ctx);

	tabdata->lock--;
}

static void lib_btn_remove(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	rnd_hid_attribute_t *attr = &ctx->dlg[tabdata->wlist];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);

	if (r == NULL)
		return;

	if (rnd_dad_tree_remove(attr, r) == 0) {
		pref_lib_dlg2conf(hid_ctx, caller_data, attr);
		pref_lib_update_buttons(rnd_gui->get_dad_design(hid_ctx));
	}
}

static void lib_btn_up(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	rnd_hid_attribute_t *attr = &ctx->dlg[tabdata->wlist];
	rnd_hid_row_t *prev, *r = rnd_dad_tree_get_selected(attr);
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[4];

	if (r == NULL)
		return;

	prev = gdl_prev(&tree->rows, r);
	if (prev == NULL)
		return;

	cell[0] = rnd_strdup(r->cell[0]); /* have to copy because this is also the primary key (path for the hash) */
	cell[1] = r->cell[1]; r->cell[1] = NULL;
	cell[2] = r->cell[2]; r->cell[2] = NULL;
	cell[3] = NULL;
	if (rnd_dad_tree_remove(attr, r) == 0) {
		rnd_hid_attr_val_t hv;
		rnd_dad_tree_insert(attr, prev, cell);
		pref_lib_dlg2conf(hid_ctx, caller_data, attr);
		hv.str = cell[0];
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, tabdata->wlist, &hv);
	}
}

static void lib_btn_down(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	rnd_hid_attribute_t *attr = &ctx->dlg[tabdata->wlist];
	rnd_hid_row_t *next, *r = rnd_dad_tree_get_selected(attr);
	rnd_hid_tree_t *tree = attr->wdata;
	char *cell[4];

	if (r == NULL)
		return;

	next = gdl_next(&tree->rows, r);
	if (next == NULL)
		return;

	cell[0] = rnd_strdup(r->cell[0]); /* have to copy because this is also the primary key (path for the hash) */
	cell[1] = r->cell[1]; r->cell[1] = NULL;
	cell[2] = r->cell[2]; r->cell[2] = NULL;
	cell[3] = NULL;
	if (rnd_dad_tree_remove(attr, r) == 0) {
		rnd_hid_attr_val_t hv;
		rnd_dad_tree_append(attr, next, cell);
		pref_lib_dlg2conf(hid_ctx, caller_data, attr);
		hv.str = cell[0];
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, tabdata->wlist, &hv);
	}
}

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int wpath, wexp;
	pref_ctx_t *pctx;
} cell_edit_ctx_t;

static void lib_cell_edit_update(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	rnd_design_t *hl = rnd_gui->get_dad_design(hid_ctx);
	cell_edit_ctx_t *ctx = caller_data;
	char *tmp;

	rnd_path_resolve(hl, ctx->dlg[ctx->wpath].val.str, &tmp, 0, rnd_true);
	if (tmp != NULL)
		RND_DAD_SET_VALUE(hid_ctx, ctx->wexp, str, tmp);
}

static int lib_cell_edit(pref_ctx_t *pctx, char **cell)
{
	cell_edit_ctx_t ctx;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};

	memset(&ctx, 0, sizeof(ctx));
	ctx.pctx = pctx;

	RND_DAD_BEGIN_VBOX(ctx.dlg);
		RND_DAD_BEGIN_TABLE(ctx.dlg, 2);
			RND_DAD_LABEL(ctx.dlg, "Path:");
			RND_DAD_STRING(ctx.dlg);
				ctx.wpath = RND_DAD_CURRENT(ctx.dlg);
				ctx.dlg[ctx.wpath].val.str = rnd_strdup(cell[0]);
				RND_DAD_CHANGE_CB(ctx.dlg, lib_cell_edit_update);

			RND_DAD_LABEL(ctx.dlg, "Expanded\nversion:");
			RND_DAD_LABEL(ctx.dlg, rnd_strdup(cell[1]));
				ctx.wexp = RND_DAD_CURRENT(ctx.dlg);
				ctx.dlg[ctx.wexp].val.str = rnd_strdup(cell[1]);

			RND_DAD_LABEL(ctx.dlg, "");
			RND_DAD_BUTTON(ctx.dlg, "Help...");
				RND_DAD_CHANGE_CB(ctx.dlg, libhelp_btn);
		RND_DAD_END(ctx.dlg);
		RND_DAD_BUTTON_CLOSES(ctx.dlg, clbtn);
	RND_DAD_END(ctx.dlg);

	RND_DAD_NEW("pref_lib_path", ctx.dlg, "Edit library path", &ctx, rnd_true, NULL);
	if (RND_DAD_RUN(ctx.dlg) != 0) {
		RND_DAD_FREE(ctx.dlg);
		return -1;
	}

	free(cell[0]);
	cell[0] = rnd_strdup(RND_EMPTY(ctx.dlg[ctx.wpath].val.str));
	free(cell[1]);
	cell[1] = rnd_strdup(RND_EMPTY(ctx.dlg[ctx.wexp].val.str));

	RND_DAD_FREE(ctx.dlg);
	return 0;
}

static void lib_btn_insert(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr, int pos)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	rnd_hid_attribute_t *attr = &ctx->dlg[tabdata->wlist];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);
	char *cell[4];

	if (r == NULL) {
		rnd_hid_tree_t *tree = attr->wdata;

		switch(pos) {
			case 0: /* replace */
				rnd_message(RND_MSG_ERROR, "need to select a library path row first\n");
				return;

			case -1: /* before */
				r = gdl_first(&tree->rows);
				break;
			case +1: /* after */
				r = gdl_last(&tree->rows);
				break;
		}
	}

	if (pos != 0) {
		cell[0] = rnd_strdup("");
		cell[1] = rnd_strdup("");
		cell[2] = rnd_strdup(SRC_BRD);
	}
	else {
		cell[0] = rnd_strdup(r->cell[0]);
		cell[1] = rnd_strdup(r->cell[1]);
		cell[2] = rnd_strdup(r->cell[2]);
	}
	cell[3] = NULL;

	if (lib_cell_edit(ctx, cell) != 0) {
		free(cell[0]);
		free(cell[1]);
		free(cell[2]);
		return;
	}

	switch(pos) {
		case -1: /* before */
			rnd_dad_tree_insert(attr, r, cell);
			break;
		case +1: /* after */
			rnd_dad_tree_append(attr, r, cell);
			break;
		case 0: /* replace */
			rnd_dad_tree_modify_cell(attr, r, 0, cell[0]);
			rnd_dad_tree_modify_cell(attr, r, 1, cell[1]);
			rnd_dad_tree_modify_cell(attr, r, 2, cell[2]);
			break;
	}
	pref_lib_dlg2conf(hid_ctx, caller_data, attr);
}

static void lib_btn_insert_before(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	lib_btn_insert(hid_ctx, caller_data, btn_attr, -1);
}

static void lib_btn_insert_after(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	lib_btn_insert(hid_ctx, caller_data, btn_attr, +1);
}

static void lib_btn_edit(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *btn_attr)
{
	lib_btn_insert(hid_ctx, caller_data, btn_attr, 0);
}

void pcb_dlg_pref_lib_close(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	DEF_TABDATA;
	if (tabdata->help.active)
		RND_DAD_FREE(tabdata->help.dlg);
}

static void pref_libhelp_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
/*	pref_libhelp_ctx_t *ctx = caller_data;*/
}

static void pref_libhelp_open(pref_libhelp_ctx_t *ctx)
{
	htsp_entry_t *e;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (ctx->active)
		return;

	RND_DAD_LABEL(ctx->dlg, "The following $(variables) can be used in the path:");
		RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
			rnd_conf_fields_foreach(e) {
				rnd_conf_native_t *nat = e->value;
				char tmp[256];

				if (strncmp(e->key, "rc/path/", 8) != 0)
					continue;

				rnd_snprintf(tmp, sizeof(tmp), "$(rc.path.%s)", e->key + 8);
				RND_DAD_LABEL(ctx->dlg, tmp);
				RND_DAD_LABEL(ctx->dlg, nat->val.string[0] == NULL ? "" : nat->val.string[0]);
			}
		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	ctx->active = 1;
	RND_DAD_NEW("pref_lib_path_help", ctx->dlg, "pcb-rnd preferences: library help", ctx, rnd_true, pref_libhelp_close_cb);

	RND_DAD_RUN(ctx->dlg);
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(pref_libhelp_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void libhelp_btn(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	DEF_TABDATA;
	pref_libhelp_open(&tabdata->help);
}

void pcb_dlg_pref_lib_create(pref_ctx_t *ctx, rnd_design_t *dsg)
{
	DEF_TABDATA;
	static const char *hdr[] = {"configured path", "actual path on the filesystem", "config source", NULL};
	rnd_hid_tree_t *tree;

	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL); /* get the parent vbox, which is the tab's page vbox, to expand and fill */

	RND_DAD_LABEL(ctx->dlg, "Ordered list of footprint library search directories.");

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME | RND_HATF_EXPFILL);
		RND_DAD_TREE(ctx->dlg, 3, 0, hdr);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
			tabdata->wlist = RND_DAD_CURRENT(ctx->dlg);
			tree = ctx->dlg[tabdata->wlist].wdata;
			RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, pref_lib_select_cb);
	RND_DAD_END(ctx->dlg);

	RND_DAD_BEGIN_HBOX(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Move up");
			RND_DAD_CHANGE_CB(ctx->dlg, lib_btn_up);
			tabdata->wmoveup = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Move down");
			RND_DAD_CHANGE_CB(ctx->dlg, lib_btn_down);
			tabdata->wmovedown = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Insert before");
			RND_DAD_CHANGE_CB(ctx->dlg, lib_btn_insert_before);
		RND_DAD_BUTTON(ctx->dlg, "Insert after");
			RND_DAD_CHANGE_CB(ctx->dlg, lib_btn_insert_after);
		RND_DAD_BUTTON(ctx->dlg, "Remove");
			RND_DAD_CHANGE_CB(ctx->dlg, lib_btn_remove);
			tabdata->wremove = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Edit...");
			RND_DAD_CHANGE_CB(ctx->dlg, lib_btn_edit);
			tabdata->wedit = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BUTTON(ctx->dlg, "Help...");
			tabdata->whsbutton = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_CHANGE_CB(ctx->dlg, libhelp_btn);
	RND_DAD_END(ctx->dlg);

}

void pcb_dlg_pref_lib_open(pref_ctx_t *ctx, rnd_design_t *dsg, const char *tabdatareq)
{
	rnd_conf_native_t *cn = rnd_conf_get_field("rc/library_search_paths");
	pref_lib_conf2dlg_post(cn, -1, ctx);
}

static const rnd_pref_tab_hook_t pref_lib = {
	"Library", RND_PREFTAB_AUTO_FREE_DATA | RND_PREFTAB_NEEDS_ROLE,
	pcb_dlg_pref_lib_open, pcb_dlg_pref_lib_close,
	pcb_dlg_pref_lib_create,
	NULL, NULL
};

void pcb_dlg_pref_lib_init(pref_ctx_t *ctx, int tab)
{
	static rnd_conf_hid_callbacks_t cbs_spth;
	rnd_conf_native_t *cn = rnd_conf_get_field("rc/library_search_paths");

	PREF_INIT(ctx, &pref_lib);
	PREF_TABDATA(ctx) = calloc(sizeof(pref_lib_t), 1);

	if (cn != NULL) {
		memset(&cbs_spth, 0, sizeof(rnd_conf_hid_callbacks_t));
		cbs_spth.val_change_pre = pref_lib_conf2dlg_pre;
		cbs_spth.val_change_post = pref_lib_conf2dlg_post;
		cbs_spth.user_data = ctx;
		rnd_conf_hid_set_cb(cn, pref_hid, &cbs_spth);
	}
}

#undef PREF_INIT_FUNC
#define PREF_INIT_FUNC pcb_dlg_pref_lib_init
