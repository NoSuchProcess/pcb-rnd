/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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

#include <ctype.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include <librnd/core/actions.h>
#include "board.h"
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include "view.h"
#include "draw.h"
#include "drc.h"
#include "actions_pcb.h"
#include <librnd/core/event.h>
#include <librnd/core/hid_inlines.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>
#include "undo.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/misc_util.h>

static const char *dlg_view_cookie = "dlg_drc";

typedef struct view_ctx_s view_ctx_t;
struct view_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	pcb_view_list_t *lst;
	pcb_view_list_t lst_local;
	int alloced, active;

	void (*refresh)(view_ctx_t *ctx);

	unsigned long int selected;

	int wpos, wlist, wcount, wprev, wdescription, wmeasure;
	int wbtn_cut;

	unsigned list_alloced:1;
};

view_ctx_t view_ctx;

static void view_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	view_ctx_t *ctx = caller_data;

	RND_DAD_FREE(ctx->dlg);
	if (ctx->list_alloced) {
		pcb_view_list_free(ctx->lst);
		ctx->lst = NULL;
	}
	if (ctx->alloced)
		free(ctx);
	else
		ctx->active = 0;
}

static void view2dlg_list(view_ctx_t *ctx)
{
	pcb_view_t *v;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[3], *cursor_path = NULL;

	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all items */
	cell[2] = NULL;
	for(v = pcb_view_list_first(ctx->lst); v != NULL; v = pcb_view_list_next(v)) {
		rnd_hid_row_t *r, *rt;
		rt = htsp_get(&tree->paths, v->type);
		if (rt == NULL) {
			cell[0] = rnd_strdup(v->type);
			cell[1] = rnd_strdup("");
			rt = rnd_dad_tree_append(attr, NULL, cell);
			rt->user_data2.lng = 0;
		}

		cell[0] = rnd_strdup_printf("%lu", v->uid);
		cell[1] = rnd_strdup(v->title);
		r = rnd_dad_tree_append_under(attr, rt, cell);
		r->user_data2.lng = v->uid;
		rnd_dad_tree_expcoll(attr, rt, 1, 0);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);
	}
}

static void view2dlg_pos(view_ctx_t *ctx)
{
	long cnt;

	pcb_view_by_uid_cnt(ctx->lst, ctx->selected, &cnt);
	if (cnt >= 0) {
		char tmp[32];
		sprintf(tmp, "%ld", cnt+1);
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wpos, str, rnd_strdup(tmp));
	}
	else
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wpos, str, rnd_strdup(""));
}

static void view2dlg_count(view_ctx_t *ctx)
{
	char tmp[32];

	sprintf(tmp, "%ld", (long)pcb_view_list_length(ctx->lst));
	RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wcount, str, rnd_strdup(tmp));
}

static void view2dlg(view_ctx_t *ctx)
{
	view2dlg_count(ctx);

	if (ctx->wlist >= 0)
		view2dlg_list(ctx);

	if (ctx->wpos >= 0)
		view2dlg_pos(ctx);
}

void view_simple_show(view_ctx_t *ctx)
{
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);
	if (v != NULL) {
		pcb_view_goto(v);
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdescription, str, rnd_text_wrap(rnd_strdup(v->description), 32, '\n', ' '));
		switch(v->data_type) {
			case PCB_VIEW_PLAIN:
				RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str, rnd_strdup(""));
				break;
			case PCB_VIEW_DRC:
				if (v->data.drc.have_measured)
					RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str, rnd_strdup_printf("DRC: %m+required: %$mw\nmeasured: %$mw\n", rnd_conf.editor.grid_unit->allow, v->data.drc.required_value, v->data.drc.measured_value));
				else
					RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str, rnd_strdup_printf("DRC: %m+required: %$mw\n", rnd_conf.editor.grid_unit->allow, v->data.drc.required_value));
				break;
		}
	}

	if (v == NULL) {
		ctx->selected = 0;
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdescription, str, rnd_strdup(""));
		RND_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str, rnd_strdup(""));
	}
	else
		rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &v->bbox);
}

static void view_select(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	view_ctx_t *ctx = tree->user_ctx;

	if (row != NULL)
		ctx->selected = row->user_data2.lng;
	view_simple_show(ctx);
}

static vtp0_t view_color_save;

static void view_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	view_ctx_t *ctx = prv->user_ctx;
	rnd_xform_t xform;
	int old_termlab, g;
	static const rnd_color_t *offend_color[2];
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);
	size_t n;
	void **p;

	if (v == NULL)
		return;

	offend_color[0] = rnd_color_red;
	offend_color[1] = rnd_color_blue;

	/* NOTE: zoom box was already set on select */

	/* save offending object colors */
	vtp0_truncate(&view_color_save, 0);
	for(g = 0; g < 2; g++) {
		pcb_idpath_t *i;
		for(i = pcb_idpath_list_first(&v->objs[g]); i != NULL; i = pcb_idpath_list_next(i)) {
			pcb_any_obj_t *obj = pcb_idpath2obj_in(ctx->pcb->Data, i);
			if ((obj != NULL) && (obj->type & PCB_OBJ_CLASS_REAL)) {
				vtp0_append(&view_color_save, obj);
				if (obj->override_color != NULL)
					vtp0_append(&view_color_save, (char *)obj->override_color);
				else
					vtp0_append(&view_color_save, NULL);
				obj->override_color = (rnd_color_t *)offend_color[g]; /* we are sure obj is not free'd before we restore this */
			}
		}
	}

	/* draw the board */
	old_termlab = pcb_draw_force_termlab;
	pcb_draw_force_termlab = 1;
	memset(&xform, 0, sizeof(xform));
	xform.layer_faded = 1;
	rnd_expose_main(rnd_gui, e, &xform);
	pcb_draw_force_termlab = old_termlab;

	/* restore object color */
	for(n = 0, p = view_color_save.array; n < view_color_save.used; n+=2,p+=2) {
		pcb_any_obj_t *obj = p[0];
		rnd_color_t *s = p[1];
		obj->override_color = s;
	}
	vtp0_truncate(&view_color_save, 0);
}


static rnd_bool view_mouse_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	return rnd_false; /* don't redraw */
}

void view_refresh(view_ctx_t *ctx)
{
	if (ctx->refresh != NULL)
		ctx->refresh(ctx);
	view2dlg(ctx);
}

static void view_preview_update(view_ctx_t *ctx)
{
	rnd_hid_attr_val_t hv;

	if ((ctx == NULL) || (!ctx->active) || (ctx->selected == 0))
		return;

	hv.str = NULL;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}


static void view_refresh_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	view_refresh((view_ctx_t *)caller_data);
}

static void view_close_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	view_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
}

static void view_stepped(view_ctx_t *ctx, pcb_view_t *v)
{
	if (v == NULL)
		return;
	ctx->selected = v->uid;
	view_simple_show(ctx);
	view2dlg_pos(ctx);
}

static void view_del_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v, *newv;

	if (ctx->wlist < 0) { /* simplified dialog, no list */
		v = pcb_view_by_uid(ctx->lst, ctx->selected);
		if (v == NULL)
			return;
		newv = pcb_view_list_next(v);
		if (newv == NULL)
			newv = pcb_view_list_first(ctx->lst);
		pcb_view_free(v);
		view_stepped(ctx, newv);
		view2dlg_count(ctx);
	}
	else { /* full dialog, go by the list */
		rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
		rnd_hid_row_t *rc, *r = rnd_dad_tree_get_selected(attr);

		if (r == NULL)
			return;

		if (r->user_data2.lng == 0) {
			/* remove a whole category - assume a single level */
			for(rc = gdl_first(&r->children); rc != NULL; rc = gdl_next(&r->children, rc)) {
				v = pcb_view_by_uid(ctx->lst, rc->user_data2.lng);
				rnd_dad_tree_remove(attr, rc);
				if (v != NULL)
					pcb_view_free(v);
			}
			rnd_dad_tree_remove(attr, r);
		}
		else {
			/* remove a single item */
			v = pcb_view_by_uid(ctx->lst, r->user_data2.lng);
			rnd_dad_tree_remove(attr, r);
			if (v != NULL)
				pcb_view_free(v);
		}
	}
}

static void view_copy_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v;
	gds_t tmp;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
	rnd_hid_row_t *rc, *r = rnd_dad_tree_get_selected(attr);
	int btn_idx = attr_btn - ctx->dlg;
	int cut = (ctx->wbtn_cut == btn_idx);

	if (r == NULL)
		return;

	/* only full dialog, go by the list */

	gds_init(&tmp);

	pcb_view_save_list_begin(&tmp, NULL);
	if (r->user_data2.lng == 0) {
		/* dump a whole category */
		for(rc = gdl_first(&r->children); rc != NULL; rc = gdl_next(&r->children, rc)) {
			v = pcb_view_by_uid(ctx->lst, rc->user_data2.lng);
			if (v != NULL) {
				pcb_view_save(v, &tmp, "  ");
				if (cut)
					pcb_view_free(v);
			}
		}
	}
	else {
		/* dump a single item */
		v = pcb_view_by_uid(ctx->lst, r->user_data2.lng);
		if (v != NULL) {
			pcb_view_save(v, &tmp, "  ");
			if (cut)
				pcb_view_free(v);
		}
	}
	pcb_view_save_list_end(&tmp, NULL);
	rnd_gui->clip_set(rnd_gui, RND_HID_CLIPFMT_TEXT, tmp.array, tmp.used+1);
	gds_uninit(&tmp);
	if (cut)
		view2dlg_list(ctx);
}

static void view_paste_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	rnd_hid_clipfmt_t cformat;
	void *cdata, *load_ctx;
	size_t clen;
	pcb_view_t *v, *vt = NULL;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(attr);

	if (r != NULL) {
		/* if cursor is a category */
		if (r->user_data2.lng == 0) {
			r = gdl_first(&r->children);
			if (r == NULL)
				return;
		}
		vt = pcb_view_by_uid(ctx->lst, r->user_data2.lng);
	}

	if (rnd_gui->clip_get(rnd_gui, &cformat, &cdata, &clen) != 0)
		return;

	if (cformat != RND_HID_CLIPFMT_TEXT) {
		rnd_gui->clip_free(rnd_gui, cformat, cdata, clen);
		return;
	}

	load_ctx = pcb_view_load_start_str(cdata);
	rnd_gui->clip_free(rnd_gui, cformat, cdata, clen);
	if (load_ctx == NULL)
		return;

	for(;;) {
		v = pcb_view_load_next(load_ctx, NULL);
		if (v == NULL)
			break;
		pcb_view_list_insert_before(ctx->lst, vt, v);
		vt = v;
	}

	pcb_view_load_end(load_ctx);
	view2dlg_list(ctx);
}

static void view_save_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	gds_t tmp;
	pcb_view_t *v;
	char *fn;
	FILE *f;

	fn = rnd_gui->fileselect(rnd_gui, "Save view list", "Save all views from the list", "view.lht", "lht", NULL, "view", 0, NULL);
	if (fn == NULL)
		return;

	f = rnd_fopen(&PCB->hidlib, fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't open %s for write\n", fn);
		return;
	}

	gds_init(&tmp);
	pcb_view_save_list_begin(&tmp, NULL);
	for(v = pcb_view_list_first(ctx->lst); v != NULL; v = pcb_view_list_next(v))
		pcb_view_save(v, &tmp, "  ");
	pcb_view_save_list_end(&tmp, NULL);

	fprintf(f, "%s", tmp.array);
	fclose(f);
	gds_uninit(&tmp);
}

static void view_load_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v;
	char *fn;
	FILE *f;
	void *load_ctx;

	fn = rnd_gui->fileselect(rnd_gui, "Load view list", "Load all views from the list", "view.lht", "lht", NULL, "view", RND_HID_FSD_READ, NULL);
	if (fn == NULL)
		return;

	f = rnd_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't open %s for read\n", fn);
		return;
	}

	load_ctx = pcb_view_load_start_file(f);
	if (load_ctx == NULL) {
		rnd_message(RND_MSG_ERROR, "Error parsing %s - is it a view list?\n", fn);
		fclose(f);
		return;
	}
	fclose(f);

	pcb_view_list_free_fields(ctx->lst);
	for(;;) {
		v = pcb_view_load_next(load_ctx, NULL);
		if (v == NULL)
			break;
		pcb_view_list_append(ctx->lst, v);
	}

	pcb_view_load_end(load_ctx);
	view2dlg_list(ctx);
}

static void view_select_obj(view_ctx_t *ctx, pcb_view_t *v)
{
	pcb_idpath_t *i;
	int chg = 0;

	if (v == NULL)
		return;

	for(i = pcb_idpath_list_first(&v->objs[0]); i != NULL; i = pcb_idpath_list_next(i)) {
		pcb_any_obj_t *obj = pcb_idpath2obj_in(ctx->pcb->Data, i);
		if ((obj != NULL) && (obj->type & PCB_OBJ_CLASS_REAL)) {
			pcb_undo_add_obj_to_flag((void *)obj);
			pcb_draw_obj(obj);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, 1, obj);
			chg = 1;
		}
	}

	if (chg) {
		pcb_board_set_changed_flag(PCB, rnd_true);
		view_preview_update(ctx);
	}
}

static void view_select_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
	rnd_hid_row_t *rc, *r = rnd_dad_tree_get_selected(attr);

	if (r == NULL)
		return;

	if (r->user_data2.lng == 0) {
		/* select a whole category - assume a single level */
		for(rc = gdl_first(&r->children); rc != NULL; rc = gdl_next(&r->children, rc)) {
			view_select_obj(ctx, pcb_view_by_uid(ctx->lst, rc->user_data2.lng));
		}
	}
	else {
		/* select a single item */
		view_select_obj(ctx, pcb_view_by_uid(ctx->lst, r->user_data2.lng));
	}
}

static void simple_rewind(view_ctx_t *ctx)
{
	pcb_view_t *v = pcb_view_list_first(ctx->lst);
	if (v != NULL) {
		ctx->selected = v->uid;
		view_stepped(ctx, v);
	}
	else
		ctx->selected = 0;
}

static void view_prev_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);

	if (v == NULL)
		v = pcb_view_list_first(ctx->lst);
	else
		v = pcb_view_list_prev(v);
	view_stepped(ctx, v);
}

static void view_next_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);

	if (v == NULL)
		v = pcb_view_list_first(ctx->lst);
	else
		v = pcb_view_list_next(v);
	view_stepped(ctx, v);
}


static void pcb_dlg_view_full(const char *id, view_ctx_t *ctx, const char *title, void (*extra_buttons)(view_ctx_t *), unsigned long preview_flipflags)
{
	const char *hdr[] = { "ID", "title", NULL };

	ctx->wpos = -1;

	preview_flipflags &= (RND_HATF_PRV_GFLIP | RND_HATF_PRV_LFLIP);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

		RND_DAD_BEGIN_HPANE(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

			/* left */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_LABEL(ctx->dlg, "Number of violations:");
					RND_DAD_LABEL(ctx->dlg, "n/a");
					ctx->wcount = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_END(ctx->dlg);

				RND_DAD_TREE(ctx->dlg, 2, 1, hdr);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_SCROLL | RND_HATF_EXPFILL);
					RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, view_select);
					RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
					ctx->wlist = RND_DAD_CURRENT(ctx->dlg);

				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "Copy");
						RND_DAD_CHANGE_CB(ctx->dlg, view_copy_btn_cb);
					RND_DAD_BUTTON(ctx->dlg, "Cut");
						ctx->wbtn_cut = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, view_copy_btn_cb);
					RND_DAD_BUTTON(ctx->dlg, "Paste");
						RND_DAD_CHANGE_CB(ctx->dlg, view_paste_btn_cb);
					RND_DAD_BUTTON(ctx->dlg, "Del");
						RND_DAD_CHANGE_CB(ctx->dlg, view_del_btn_cb);
					RND_DAD_BUTTON(ctx->dlg, "Select");
						RND_DAD_CHANGE_CB(ctx->dlg, view_select_btn_cb);
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);

			/* right */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_PREVIEW(ctx->dlg, view_expose_cb, view_mouse_cb, NULL, NULL, NULL, 100, 100, ctx);
					ctx->wprev = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_PRV_BOARD | preview_flipflags);
				RND_DAD_LABEL(ctx->dlg, "(description)");
					ctx->wdescription = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "(measure)");
					ctx->wmeasure = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Save all");
				RND_DAD_CHANGE_CB(ctx->dlg, view_save_btn_cb);
			RND_DAD_BUTTON(ctx->dlg, "Load all");
				RND_DAD_CHANGE_CB(ctx->dlg, view_load_btn_cb);
			if (ctx->refresh != NULL) {
				RND_DAD_BUTTON(ctx->dlg, "Refresh");
					RND_DAD_CHANGE_CB(ctx->dlg, view_refresh_btn_cb);
			}
			if (extra_buttons != NULL)
				extra_buttons(ctx);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Close");
				RND_DAD_CHANGE_CB(ctx->dlg, view_close_btn_cb);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_NEW(id, ctx->dlg, title, ctx, rnd_false, view_close_cb);

	ctx->active = 1;
}

static void pcb_dlg_view_simplified(const char *id, view_ctx_t *ctx, const char *title, unsigned long preview_flipflags)
{
	pcb_view_t *v;

	preview_flipflags &= (RND_HATF_PRV_GFLIP | RND_HATF_PRV_LFLIP);

	ctx->wlist = -1;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);

		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_PREVIEW(ctx->dlg, view_expose_cb, view_mouse_cb, NULL, NULL, NULL, 100, 100, ctx);
				ctx->wprev = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_PRV_BOARD | preview_flipflags);
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "(description)");
					ctx->wdescription = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "(measure)");
					ctx->wmeasure = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Previous");
				RND_DAD_CHANGE_CB(ctx->dlg, view_prev_btn_cb);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_LABEL(ctx->dlg, "na");
					ctx->wpos = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "/");
				RND_DAD_LABEL(ctx->dlg, "na");
					ctx->wcount = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_END(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Del");
				RND_DAD_CHANGE_CB(ctx->dlg, view_del_btn_cb);
			RND_DAD_BUTTON(ctx->dlg, "Next");
				RND_DAD_CHANGE_CB(ctx->dlg, view_next_btn_cb);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_HBOX(ctx->dlg);
			if (ctx->refresh != NULL) {
				RND_DAD_BUTTON(ctx->dlg, "Refresh");
					RND_DAD_CHANGE_CB(ctx->dlg, view_refresh_btn_cb);
			}
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ctx->dlg);
			RND_DAD_BUTTON(ctx->dlg, "Close");
				RND_DAD_CHANGE_CB(ctx->dlg, view_close_btn_cb);
		RND_DAD_END(ctx->dlg);

	RND_DAD_END(ctx->dlg);

	RND_DAD_NEW(id, ctx->dlg, title, ctx, rnd_false, view_close_cb);

	ctx->active = 1;

	v = pcb_view_list_first(ctx->lst);
	if (v != NULL)
		ctx->selected = v->uid;
	else
		ctx->selected = 0;
	simple_rewind(ctx);
}

static void drc_refresh(view_ctx_t *ctx)
{
	pcb_drc_all();

	if (ctx->wlist < 0)
		simple_rewind(ctx);
}

static void drc_config_btn_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(&PCB->hidlib, "preferences", "Sizes", NULL);
}

void drc_extra_buttons(view_ctx_t *ctx)
{
	RND_DAD_BUTTON(ctx->dlg, "Config...");
		RND_DAD_CHANGE_CB(ctx->dlg, drc_config_btn_cb);
}

static view_ctx_t drc_gui_ctx = {0};
const char pcb_acts_DrcDialog[] = "DrcDialog([list|simple]\n";
const char pcb_acth_DrcDialog[] = "Execute drc checks and invoke a view list dialog box for presenting the results";
fgw_error_t pcb_act_DrcDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	unsigned long flip_flags = RND_HATF_PRV_GFLIP;

	RND_ACT_MAY_CONVARG(1, FGW_STR, DrcDialog, dlg_type = argv[1].val.str);

	if (!drc_gui_ctx.active) {
		drc_gui_ctx.pcb = PCB;
		drc_gui_ctx.lst = &pcb_drc_lst;
		drc_gui_ctx.refresh = drc_refresh;
		pcb_drc_all();
		if (rnd_strcasecmp(dlg_type, "simple") == 0)
			pcb_dlg_view_simplified("drc_simple", &drc_gui_ctx, "DRC violations", flip_flags);
		else
			pcb_dlg_view_full("drc_full", &drc_gui_ctx, "DRC violations", drc_extra_buttons, flip_flags);
	}

	view2dlg(&drc_gui_ctx);

	return 0;
}

extern pcb_view_list_t pcb_io_incompat_lst;
static view_ctx_t io_gui_ctx = {0};
const char pcb_acts_IOIncompatListDialog[] = "IOIncompatListDialog([list|simple])\n";
const char pcb_acth_IOIncompatListDialog[] = "Present the format incompatibilities of the last save to file operation in a GUI dialog.";
fgw_error_t pcb_act_IOIncompatListDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	RND_ACT_MAY_CONVARG(1, FGW_STR, IOIncompatListDialog, dlg_type = argv[1].val.str);

	if (!io_gui_ctx.active) {
		io_gui_ctx.pcb = PCB;
		io_gui_ctx.lst = &pcb_io_incompat_lst;
		io_gui_ctx.refresh = NULL;
		if (rnd_strcasecmp(dlg_type, "simple") == 0)
			pcb_dlg_view_simplified("io_incompat_simple", &io_gui_ctx, "IO incompatibilities in last save", 0);
		else
			pcb_dlg_view_full("io_incompat_full", &io_gui_ctx, "IO incompatibilities in last save", NULL, 0);
	}

	view2dlg(&io_gui_ctx);

	return 0;
}

const char pcb_acts_ViewList[] = "viewlist([name, [winid, [listptr]]])\n";
const char pcb_acth_ViewList[] = "Present a new empty view list";
fgw_error_t pcb_act_ViewList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	view_ctx_t *ctx;
	void *lst = NULL;
	const char *name = "view list", *winid = "viewlist";
	RND_ACT_MAY_CONVARG(1, FGW_STR, ViewList, name = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, ViewList, winid = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_PTR, ViewList, lst = argv[3].val.ptr_void);

	if ((lst != NULL) && (!fgw_ptr_in_domain(&rnd_fgw, &argv[3], PCB_PTR_DOMAIN_VIEWLIST))) {
		rnd_message(RND_MSG_ERROR, "invalid list pointer");
		RND_ACT_IRES(1);
		return 0;
	}

	RND_ACT_IRES(0);
	ctx = calloc(sizeof(view_ctx_t), 1);
	ctx->pcb = PCB;
	if (lst != NULL)
		ctx->lst = lst;
	else
		ctx->lst = calloc(sizeof(pcb_view_list_t), 1);
	ctx->list_alloced = 1;
	ctx->refresh = NULL;
	pcb_dlg_view_full(winid, ctx, name, NULL, 0);
	view2dlg(ctx);
	return 0;
}

static void view_preview_update_cb(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (drc_gui_ctx.active)
		view_preview_update(&drc_gui_ctx);
	if (io_gui_ctx.active)
	view_preview_update(&io_gui_ctx);
}

void pcb_view_dlg_uninit(void)
{
	rnd_event_unbind_allcookie(dlg_view_cookie);
	vtp0_uninit(&view_color_save);
}

void pcb_view_dlg_init(void)
{
	rnd_event_bind(RND_EVENT_USER_INPUT_POST, view_preview_update_cb, NULL, dlg_view_cookie);
}
