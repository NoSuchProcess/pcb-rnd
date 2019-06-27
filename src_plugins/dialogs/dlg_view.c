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

#include <ctype.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include "actions.h"
#include "board.h"
#include "conf_core.h"
#include "hidlib_conf.h"
#include "view.h"
#include "draw.h"
#include "drc.h"
#include "event.h"
#include "hid_inlines.h"
#include "hid_dad.h"
#include "hid_dad_tree.h"
#include "undo.h"
#include "safe_fs.h"
#include "misc_util.h"

static const char *dlg_view_cookie = "dlg_drc";

typedef struct view_ctx_s view_ctx_t;
struct view_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
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

static void view_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	view_ctx_t *ctx = caller_data;

	PCB_DAD_FREE(ctx->dlg);
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
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[3], *cursor_path = NULL;

	attr = &ctx->dlg[ctx->wlist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	pcb_dad_tree_clear(tree);

	/* add all items */
	cell[2] = NULL;
	for(v = pcb_view_list_first(ctx->lst); v != NULL; v = pcb_view_list_next(v)) {
		pcb_hid_row_t *r, *rt;
		rt = htsp_get(&tree->paths, v->type);
		if (rt == NULL) {
			cell[0] = pcb_strdup(v->type);
			cell[1] = pcb_strdup("");
			rt = pcb_dad_tree_append(attr, NULL, cell);
			rt->user_data2.lng = 0;
		}

		cell[0] = pcb_strdup_printf("%lu", v->uid);
		cell[1] = pcb_strdup(v->title);
		r = pcb_dad_tree_append_under(attr, rt, cell);
		r->user_data2.lng = v->uid;
		pcb_dad_tree_expcoll(attr, rt, 1, 0);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
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
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wpos, str_value, pcb_strdup(tmp));
	}
	else
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wpos, str_value, pcb_strdup(""));
}

static void view2dlg_count(view_ctx_t *ctx)
{
	char tmp[32];

	sprintf(tmp, "%ld", (long)pcb_view_list_length(ctx->lst));
	PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wcount, str_value, pcb_strdup(tmp));
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
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdescription, str_value, pcb_text_wrap(pcb_strdup(v->description), 32, '\n', ' '));
		switch(v->data_type) {
			case PCB_VIEW_PLAIN:
				PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup(""));
				break;
			case PCB_VIEW_DRC:
				if (v->data.drc.have_measured)
					PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup_printf("DRC: %m+required: %$ms\nmeasured: %$ms\n", pcbhl_conf.editor.grid_unit->allow, v->data.drc.required_value, v->data.drc.measured_value));
				else
					PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup_printf("DRC: %m+required: %$ms\n", pcbhl_conf.editor.grid_unit->allow, v->data.drc.required_value));
				break;
		}
	}

	if (v == NULL) {
		ctx->selected = 0;
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wdescription, str_value, pcb_strdup(""));
		PCB_DAD_SET_VALUE(ctx->dlg_hid_ctx, ctx->wmeasure, str_value, pcb_strdup(""));
	}
	else
		pcb_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &v->bbox);
}

static void view_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	view_ctx_t *ctx = tree->user_ctx;

	if (row != NULL)
		ctx->selected = row->user_data2.lng;
	view_simple_show(ctx);
}

static vtp0_t view_color_save;

static void view_expose_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	view_ctx_t *ctx = prv->user_ctx;
	pcb_xform_t xform;
	int old_termlab, g;
	static const pcb_color_t *offend_color[2];
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);
	size_t n;
	void **p;

	if (v == NULL)
		return;

	offend_color[0] = pcb_color_red;
	offend_color[1] = pcb_color_blue;

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
				obj->override_color = offend_color[g];
			}
		}
	}

	/* draw the board */
	old_termlab = pcb_draw_force_termlab;
	pcb_draw_force_termlab = 1;
	memset(&xform, 0, sizeof(xform));
	xform.layer_faded = 1;
	pcbhl_expose_main(pcb_gui, e, &xform);
	pcb_draw_force_termlab = old_termlab;

	/* restore object color */
	for(n = 0, p = view_color_save.array; n < view_color_save.used; n+=2,p+=2) {
		pcb_any_obj_t *obj = p[0];
		pcb_color_t *s = p[1];
		obj->override_color = s;
	}
	vtp0_truncate(&view_color_save, 0);
}


static pcb_bool view_mouse_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false; /* don't redraw */
}

void view_refresh(view_ctx_t *ctx)
{
	if (ctx->refresh != NULL)
		ctx->refresh(ctx);
	view2dlg(ctx);
}

static void view_preview_update(view_ctx_t *ctx)
{
	pcb_hid_attr_val_t hv;

	if ((ctx == NULL) || (!ctx->active) || (ctx->selected == 0))
		return;

	hv.str_value = NULL;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}


static void view_refresh_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	view_refresh((view_ctx_t *)caller_data);
}

static void view_close_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	view_close_cb(caller_data, 0);
}

static void view_stepped(view_ctx_t *ctx, pcb_view_t *v)
{
	if (v == NULL)
		return;
	ctx->selected = v->uid;
	view_simple_show(ctx);
	view2dlg_pos(ctx);
}

static void view_del_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
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
		pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
		pcb_hid_row_t *rc, *r = pcb_dad_tree_get_selected(attr);

		if (r == NULL)
			return;

		if (r->user_data2.lng == 0) {
			/* remove a whole category - assume a single level */
			for(rc = gdl_first(&r->children); rc != NULL; rc = gdl_next(&r->children, rc)) {
				v = pcb_view_by_uid(ctx->lst, rc->user_data2.lng);
				pcb_dad_tree_remove(attr, rc);
				if (v != NULL)
					pcb_view_free(v);
			}
			pcb_dad_tree_remove(attr, r);
		}
		else {
			/* remove a single item */
			v = pcb_view_by_uid(ctx->lst, r->user_data2.lng);
			pcb_dad_tree_remove(attr, r);
			if (v != NULL)
				pcb_view_free(v);
		}
	}
}

static void view_copy_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v;
	gds_t tmp;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
	pcb_hid_row_t *rc, *r = pcb_dad_tree_get_selected(attr);
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
	pcb_gui->clip_set(PCB_HID_CLIPFMT_TEXT, tmp.array, tmp.used+1);
	gds_uninit(&tmp);
	if (cut)
		view2dlg_list(ctx);
}

static void view_paste_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_hid_clipfmt_t cformat;
	void *cdata, *load_ctx;
	size_t clen;
	pcb_view_t *v, *vt = NULL;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
	pcb_hid_row_t *r = pcb_dad_tree_get_selected(attr);

	if (r != NULL) {
		/* if cursor is a category */
		if (r->user_data2.lng == 0) {
			r = gdl_first(&r->children);
			if (r == NULL)
				return;
		}
		vt = pcb_view_by_uid(ctx->lst, r->user_data2.lng);
	}

	if (pcb_gui->clip_get(&cformat, &cdata, &clen) != 0)
		return;

	if (cformat != PCB_HID_CLIPFMT_TEXT) {
		pcb_gui->clip_free(cformat, cdata, clen);
		return;
	}

	load_ctx = pcb_view_load_start_str(cdata);
	pcb_gui->clip_free(cformat, cdata, clen);
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

static void view_save_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	gds_t tmp;
	pcb_view_t *v;
	char *fn;
	FILE *f;

	fn = pcb_gui->fileselect("Save view list", "Save all views from the list", "view.lht", "lht", NULL, "view", 0, NULL);
	if (fn == NULL)
		return;

	f = pcb_fopen(&PCB->hidlib, fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open %s for write\n", fn);
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

static void view_load_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v;
	char *fn;
	FILE *f;
	void *load_ctx;

	fn = pcb_gui->fileselect("Load view list", "Load all views from the list", "view.lht", "lht", NULL, "view", PCB_HID_FSD_READ, NULL);
	if (fn == NULL)
		return;

	f = pcb_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open %s for read\n", fn);
		return;
	}

	load_ctx = pcb_view_load_start_file(f);
	if (load_ctx == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error parsing %s - is it a view list?\n", fn);
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
		pcb_board_set_changed_flag(pcb_true);
		view_preview_update(ctx);
	}
}

static void view_select_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_btn)
{
	view_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->wlist];
	pcb_hid_row_t *rc, *r = pcb_dad_tree_get_selected(attr);

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

static void view_prev_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);

	if (v == NULL)
		v = pcb_view_list_first(ctx->lst);
	else
		v = pcb_view_list_prev(v);
	view_stepped(ctx, v);
}

static void view_next_btn_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	view_ctx_t *ctx = caller_data;
	pcb_view_t *v = pcb_view_by_uid(ctx->lst, ctx->selected);

	if (v == NULL)
		v = pcb_view_list_first(ctx->lst);
	else
		v = pcb_view_list_next(v);
	view_stepped(ctx, v);
}


static void pcb_dlg_view_full(const char *id, view_ctx_t *ctx, const char *title)
{
	const char *hdr[] = { "ID", "title", NULL };

	ctx->wpos = -1;

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

		PCB_DAD_BEGIN_HPANE(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

			/* left */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_LABEL(ctx->dlg, "Number of violations:");
					PCB_DAD_LABEL(ctx->dlg, "n/a");
					ctx->wcount = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_END(ctx->dlg);

				PCB_DAD_TREE(ctx->dlg, 2, 1, hdr);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_SCROLL | PCB_HATF_EXPFILL);
					PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, view_select);
					PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
					ctx->wlist = PCB_DAD_CURRENT(ctx->dlg);

				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_BUTTON(ctx->dlg, "Copy");
						PCB_DAD_CHANGE_CB(ctx->dlg, view_copy_btn_cb);
					PCB_DAD_BUTTON(ctx->dlg, "Cut");
						ctx->wbtn_cut = PCB_DAD_CURRENT(ctx->dlg);
						PCB_DAD_CHANGE_CB(ctx->dlg, view_copy_btn_cb);
					PCB_DAD_BUTTON(ctx->dlg, "Paste");
						PCB_DAD_CHANGE_CB(ctx->dlg, view_paste_btn_cb);
					PCB_DAD_BUTTON(ctx->dlg, "Del");
						PCB_DAD_CHANGE_CB(ctx->dlg, view_del_btn_cb);
					PCB_DAD_BUTTON(ctx->dlg, "Select");
						PCB_DAD_CHANGE_CB(ctx->dlg, view_select_btn_cb);
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

			/* right */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_PREVIEW(ctx->dlg, view_expose_cb, view_mouse_cb, NULL, NULL, 100, 100, ctx);
					ctx->wprev = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_PRV_BOARD);
				PCB_DAD_LABEL(ctx->dlg, "(description)");
					ctx->wdescription = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "(measure)");
					ctx->wmeasure = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Save all");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_save_btn_cb);
			PCB_DAD_BUTTON(ctx->dlg, "Load all");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_load_btn_cb);
			if (ctx->refresh != NULL) {
				PCB_DAD_BUTTON(ctx->dlg, "Refresh");
					PCB_DAD_CHANGE_CB(ctx->dlg, view_refresh_btn_cb);
			}
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Close");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_close_btn_cb);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW(id, ctx->dlg, title, ctx, pcb_false, view_close_cb);

	ctx->active = 1;
}

static void pcb_dlg_view_simplified(const char *id, view_ctx_t *ctx, const char *title)
{
	pcb_view_t *v;

	ctx->wlist = -1;

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_PREVIEW(ctx->dlg, view_expose_cb, view_mouse_cb, NULL, NULL, 100, 100, ctx);
				ctx->wprev = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_PRV_BOARD);
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "(description)");
					ctx->wdescription = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "(measure)");
					ctx->wmeasure = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Previous");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_prev_btn_cb);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(ctx->dlg, "na");
					ctx->wpos = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "/");
				PCB_DAD_LABEL(ctx->dlg, "na");
					ctx->wcount = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Del");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_del_btn_cb);
			PCB_DAD_BUTTON(ctx->dlg, "Next");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_next_btn_cb);
		PCB_DAD_END(ctx->dlg);

		PCB_DAD_BEGIN_HBOX(ctx->dlg);
			if (ctx->refresh != NULL) {
				PCB_DAD_BUTTON(ctx->dlg, "Refresh");
					PCB_DAD_CHANGE_CB(ctx->dlg, view_refresh_btn_cb);
			}
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BUTTON(ctx->dlg, "Close");
				PCB_DAD_CHANGE_CB(ctx->dlg, view_close_btn_cb);
		PCB_DAD_END(ctx->dlg);

	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW(id, ctx->dlg, title, ctx, pcb_false, view_close_cb);

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

static view_ctx_t drc_gui_ctx = {0};
const char pcb_acts_DrcDialog[] = "DrcDialog([list|simple])\n";
const char pcb_acth_DrcDialog[] = "Execute drc checks and invoke a view list dialog box for presenting the results";
fgw_error_t pcb_act_DrcDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *dlg_type = "list";
	PCB_ACT_MAY_CONVARG(1, FGW_STR, DrcDialog, dlg_type = argv[1].val.str);

	if (!drc_gui_ctx.active) {
		drc_gui_ctx.pcb = PCB;
		drc_gui_ctx.lst = &pcb_drc_lst;
		drc_gui_ctx.refresh = drc_refresh;
		pcb_drc_all();
		if (pcb_strcasecmp(dlg_type, "simple") == 0)
			pcb_dlg_view_simplified("drc_simple", &drc_gui_ctx, "DRC violations");
		else
			pcb_dlg_view_full("drc_full", &drc_gui_ctx, "DRC violations");
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
	PCB_ACT_MAY_CONVARG(1, FGW_STR, IOIncompatListDialog, dlg_type = argv[1].val.str);

	if (!io_gui_ctx.active) {
		io_gui_ctx.pcb = PCB;
		io_gui_ctx.lst = &pcb_io_incompat_lst;
		io_gui_ctx.refresh = NULL;
		if (pcb_strcasecmp(dlg_type, "simple") == 0)
			pcb_dlg_view_simplified("io_incompat_simple", &io_gui_ctx, "IO incompatibilities in last save");
		else
			pcb_dlg_view_full("io_incompat_full", &io_gui_ctx, "IO incompatibilities in last save");
	}

	view2dlg(&io_gui_ctx);

	return 0;
}

const char pcb_acts_ViewList[] = "viewlist([name, [winid]])\n";
const char pcb_acth_ViewList[] = "Present a new empty view list";
fgw_error_t pcb_act_ViewList(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	view_ctx_t *ctx = calloc(sizeof(view_ctx_t), 1);
	const char *name = "view list", *winid = "viewlist";
	PCB_ACT_MAY_CONVARG(1, FGW_STR, ViewList, name = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, ViewList, winid = argv[2].val.str);

	ctx->pcb = PCB;
	ctx->lst = calloc(sizeof(pcb_view_list_t), 1);
	ctx->refresh = NULL;
	pcb_dlg_view_full(winid, ctx, name);
	view2dlg(ctx);
	return 0;
}

static void view_preview_update_cb(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (drc_gui_ctx.active)
		view_preview_update(&drc_gui_ctx);
	if (io_gui_ctx.active)
	view_preview_update(&io_gui_ctx);
}

void pcb_view_dlg_uninit(void)
{
	pcb_event_unbind_allcookie(dlg_view_cookie);
	vtp0_uninit(&view_color_save);
}

void pcb_view_dlg_init(void)
{
	pcb_event_bind(PCB_EVENT_USER_INPUT_POST, view_preview_update_cb, NULL, dlg_view_cookie);
}
