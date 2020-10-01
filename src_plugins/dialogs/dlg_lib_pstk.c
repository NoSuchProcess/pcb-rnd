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

/* Padstack library dialog */

#include "config.h"
#include <librnd/core/actions.h>
#include "data.h"
#include "obj_subc.h"
#include "vtpadstack.h"
#include <librnd/core/hid_inlines.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>
#include <librnd/core/safe_fs.h>
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_text_draw.h"
#include "obj_pstk_draw.h"
#include "undo_old.h"
#include "plug_io.h"
#include "select.h"
#include "search.h"
#include "dlg_padstack.h"
#include "dlg_lib_pstk.h"

htip_t pstk_libs; /* id -> pstk_lib_ctx_t */

typedef struct pstk_lib_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wlist, wprev, wgrid;
	int wlayerv[pcb_proto_num_layers], wlayerc[pcb_proto_num_layers]; /* layer visibility/current */
	long subc_id;
	rnd_cardinal_t proto_id;
	rnd_cardinal_t *stat; /* temporary usage stat */
	rnd_box_t drawbox;
	rnd_bool modal;
} pstk_lib_ctx_t;

static rnd_cardinal_t pstklib_last_proto_id; /* set on close to preserve the id after free'ing the context; useful only for modal windows because of blocking calls */

static pcb_data_t *get_data(pstk_lib_ctx_t *ctx, long id, pcb_subc_t **sc_out)
{
	int type;
	void *r1, *r2, *r3;
	pcb_subc_t *sc;

	if (id < 0)
		return ctx->pcb->Data;

	type = pcb_search_obj_by_id_(ctx->pcb->Data, &r1, &r2, &r3, id, PCB_OBJ_SUBC);
	if (type != PCB_OBJ_SUBC)
		return NULL;

	sc = r2;

	if (sc_out != NULL)
		*sc_out = sc;

	return sc->data;
}

static int pstklib_data2dlg(pstk_lib_ctx_t *ctx)
{
	pcb_pstk_proto_t *proto;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	long id;

	if (data == NULL)
		return -1;

	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	/* add all items */
	cell[3] = NULL;
	for(id = 0, proto = data->ps_protos.array; id < pcb_vtpadstack_proto_len(&data->ps_protos); proto++,id++) {
		if (!proto->in_use)
			continue;
		cell[0] = rnd_strdup_printf("%ld", id);
		cell[1] = rnd_strdup(proto->name == NULL ? "" : proto->name);
		if (ctx->stat != NULL)
			cell[2] = rnd_strdup_printf("%d", ctx->stat[id]);
		else
			cell[2] = rnd_strdup("");
		rnd_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);
	}
	return 0;
}

static void pstklib_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	pstk_lib_ctx_t *ctx = caller_data;

	if (!ctx->modal)
		htip_pop(&pstk_libs, ctx->subc_id);
	pstklib_last_proto_id = ctx->proto_id;
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void pstklib_setps(pcb_pstk_t *ps, pcb_data_t *data, rnd_cardinal_t proto_id)
{
	memset(ps, 0, sizeof(pcb_pstk_t));
	ps->parent_type = PCB_PARENT_DATA;
	ps->parent.data = data;
	ps->proto = proto_id;
	ps->ID = -1; /* disable undo and clipping */
}

static void pstklib_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	pstk_lib_ctx_t *ctx = prv->user_ctx;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	pcb_pstk_t ps;
	char layers[pcb_proto_num_layers];
	int n;
	rnd_coord_t x1, y1, x2, y2, x, y, grid;

	if (data == NULL) {
		return;
	}

	if (ctx->proto_id == PCB_PADSTACK_INVALID)
		return;

	pstklib_setps(&ps, data, ctx->proto_id);

	/* draw the shapes */
	for(n = 0; n < pcb_proto_num_layers; n++)
		layers[n] = !!ctx->dlg[ctx->wlayerv[n]].val.lng + !!ctx->dlg[ctx->wlayerc[n]].val.lng;

	pcb_pstk_draw_preview(PCB, &ps, layers, 0, 0, &e->view);

	rnd_render->set_color(gc, rnd_color_black);
	rnd_hid_set_line_cap(gc, rnd_cap_round);
	rnd_hid_set_line_width(gc, -1);

	x1 = ctx->drawbox.X1;
	y1 = ctx->drawbox.Y1;
	x2 = ctx->drawbox.X2;
	y2 = ctx->drawbox.Y2;

	grid = ctx->dlg[ctx->wgrid].val.crd;
	for(x = 0; x < x2; x += grid)
		rnd_render->draw_line(gc, x, y1, x, y2);
	for(x = -grid; x > x1; x -= grid)
		rnd_render->draw_line(gc, x, y1, x, y2);
	for(y = 0; y < y2; y += grid)
		rnd_render->draw_line(gc, x1, y, x2, y);
	for(y = -grid; y > y1; y -= grid)
		rnd_render->draw_line(gc, x1, y, x2, y);

	/* draw the mark only */
	for(n = 0; n < pcb_proto_num_layers; n++)
		layers[n] = 0;
	pcb_pstk_draw_preview(PCB, &ps, layers, 1, 0, &e->view);
}

static void pstklib_force_redraw(pstk_lib_ctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

	if (proto == NULL)
		return;

	pcb_pstk_bbox(ps);
	ps->BoundingBox.X1 -= RND_MM_TO_COORD(0.5);
	ps->BoundingBox.Y1 -= RND_MM_TO_COORD(0.5);
	ps->BoundingBox.X2 += RND_MM_TO_COORD(0.5);
	ps->BoundingBox.Y2 += RND_MM_TO_COORD(0.5);
	memcpy(&ctx->drawbox, &ps->BoundingBox, sizeof(rnd_box_t));
	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &ctx->drawbox);
}

static void pstklib_select(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_attr_val_t hv;
	rnd_hid_tree_t *tree = attrib->wdata;
	pstk_lib_ctx_t *ctx = tree->user_ctx;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	pcb_pstk_t ps;

	if ((row != NULL) && (data != NULL)) {
		ctx->proto_id = strtol(row->cell[0], NULL, 10);
		pstklib_setps(&ps, data, ctx->proto_id);
		pstklib_force_redraw(ctx, &ps);
	}
	else
		ctx->proto_id = PCB_PADSTACK_INVALID;

	hv.str = NULL;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}

static void pstklib_update_prv(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &ctx->drawbox);
}

static void pstklib_update_layerc(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	int n, idx = -1, widx = attr - ctx->dlg;
	rnd_hid_attr_val_t hv;

	for(n = 0; n < pcb_proto_num_layers; n++) {
		if (ctx->wlayerc[n] == widx) {
			hv.lng = 1;
			idx = n;
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlayerv[n], &hv); /* current must be visible as well */
		}
		else
			hv.lng = 0;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlayerc[n], &hv);
	}
	if (idx < 0)
		return;

	pstklib_update_prv(hid_ctx, caller_data, attr);
}


static void pstklib_filter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	const char *text;

	if (data == NULL)
		return;

	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;
	text = attr_inp->val.str;

	if ((text == NULL) || (*text == '\0')) {
		for(r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r))
			r->hide = 0;
	}
	else {
		for(r = gdl_first(&tree->rows); r != NULL; r = gdl_next(&tree->rows, r))
			r->hide = (strstr(r->cell[1], text) == NULL);
	}

	rnd_dad_tree_update_hide(attr);
}


static void pstklib_proto_edit_change_cb(pse_t *pse)
{
	pstklib_force_redraw(pse->user_data, pse->ps);
}

static void pstklib_proto_edit_common(pstk_lib_ctx_t *ctx, pcb_data_t *data, rnd_cardinal_t proto_id, int tab)
{
	pcb_pstk_t ps;
	pse_t pse;

	pstklib_setps(&ps, data, proto_id);
	memset(&pse, 0, sizeof(pse));
	pse.pcb = ctx->pcb;
	pse.data = data;
	pse.ps = &ps;
	pse.disable_instance_tab = 1;
	pse.user_data = ctx;
	pse.change_cb = pstklib_proto_edit_change_cb;
	pse.gen_shape_in_place = 1;

	pcb_pstkedit_dialog(&pse, tab);
}


static void pstklib_proto_edit(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wlist]);

	if ((row == NULL) || (data == NULL))
		return;

	pstklib_proto_edit_common(ctx, data, strtol(row->cell[0], NULL, 10), 1);
}

static void pstklib_proto_new_(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr, int dup)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	pcb_pstk_proto_t proto_, *proto;
	rnd_hid_attr_val_t hv;
	char tmp[64];
	int tab;

	if (data == NULL)
		return;

	if (dup) {
		rnd_hid_row_t *row = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wlist]);
		if (row == NULL)
			return;
		proto = pcb_pstk_get_proto_(data, strtol(row->cell[0], NULL, 10));
		ctx->proto_id = pcb_pstk_proto_insert_forcedup(data, proto, 0, (pcb_data_get_top(data) == ctx->pcb));
		tab = 1;
	}
	else {
		memset(&proto_, 0, sizeof(proto_));
		pcb_pstk_proto_update(&proto_);
		proto = &proto_;
		ctx->proto_id = pcb_pstk_proto_insert_dup(data, proto, 1, (pcb_data_get_top(data) == ctx->pcb));
		tab = 2;
	}

	/* make sure the new item appears in the list and is selected */
	pstklib_data2dlg(ctx);
	sprintf(tmp, "%u", ctx->proto_id);
	hv.str = tmp;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);

	pstklib_proto_edit_common(ctx, data, ctx->proto_id, tab);
}

static void pstklib_proto_new(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstklib_proto_new_(hid_ctx, caller_data, attr, 0);
}

static void pstklib_proto_dup(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstklib_proto_new_(hid_ctx, caller_data, attr, 1);
}

static char *proto_save_fn = NULL, *proto_load_fn = NULL;

static void pstklib_save(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wlist]);
	pcb_pstk_proto_t *proto;
	FILE *f;
	char *old_fn;

	if (data == NULL)
		return;

	if (row == NULL) {
		rnd_message(RND_MSG_ERROR, "First select a prototype to save\n");
		return;
	}


	proto = pcb_pstk_get_proto_(data, strtol(row->cell[0], NULL, 10));
	if (proto == NULL)
		return;

	if (proto_save_fn == NULL)
		proto_save_fn = rnd_strdup("padstack.lht");
	old_fn = proto_save_fn;
	proto_save_fn = rnd_gui->fileselect(rnd_gui, "Save padstack", "Select a file the padstack prototype is saved to", old_fn, ".lht", NULL, "padstack", 0, NULL);
	if (proto_save_fn == NULL)
		return; /* cancel */
	free(old_fn);

	f = rnd_fopen(&ctx->pcb->hidlib, proto_save_fn, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open %s for write.\n", proto_save_fn);
		return;
	}

	if (pcb_write_padstack(f, proto, "lihata") == 0)
		rnd_message(RND_MSG_INFO, "Padstack saved to %s.\n", proto_save_fn);
	else
		rnd_message(RND_MSG_ERROR, "Padstack not saved to %s.\n", proto_save_fn);
}

static void pstklib_load(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wlist]);
	pcb_pstk_proto_t *proto;
	char *old_fn;
	rnd_hid_attr_val_t hv;

	if (data == NULL)
		return;

	if (row == NULL) {
		rnd_message(RND_MSG_ERROR, "First select a prototype to load into\n");
		return;
	}

	proto = pcb_pstk_get_proto_(data, strtol(row->cell[0], NULL, 10));
	if (proto == NULL)
		return;

	if (proto_load_fn == NULL)
		proto_load_fn = rnd_strdup("padstack.lht");
	old_fn = proto_load_fn;
	proto_load_fn = rnd_gui->fileselect(rnd_gui, "Save padstack", "Select a file the padstack prototype is loaded from", old_fn, ".lht", NULL, "padstack", RND_HID_FSD_READ, NULL);
	if (proto_load_fn == NULL)
		return; /* cancel */
	free(old_fn);

	if (pcb_load_padstack(&ctx->pcb->hidlib, proto, proto_load_fn, NULL) == 0)
		rnd_message(RND_MSG_INFO, "Padstack loaded from %s.\n", proto_load_fn);
	else
		rnd_message(RND_MSG_ERROR, "Padstack failed to load from %s.\n", proto_load_fn);

	proto->parent = data;

	/* redraw */
	hv.str = NULL;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}

static void pstklib_proto_switch(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_attribute_t *attr;
	rnd_hid_row_t *r;
	rnd_cardinal_t from_pid, to_pid;
	pcb_pstk_t *ps;

	if (data == NULL)
		return;

	attr = &ctx->dlg[ctx->wlist];
	r = rnd_dad_tree_get_selected(attr);
	if (r == NULL)
		return;

	from_pid = strtol(r->cell[0], NULL, 10);
	to_pid = pcb_dlg_pstklib(ctx->pcb, ctx->subc_id, rnd_true, "Select a prototype to switch to");
	if ((to_pid == PCB_PADSTACK_INVALID) || (to_pid == from_pid))
		return;

	for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
		if (ps->proto == from_pid)
			pcb_pstk_change_instance(ps, &to_pid, NULL, NULL, NULL, NULL);
	}

	rnd_gui->invalidate_all(rnd_gui);
}


static void pstklib_proto_select(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_btn)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	rnd_hid_attribute_t *attr;
	rnd_hid_row_t *r;
	long pid;
	rnd_box_t box;
	int changed = 0;
	pcb_pstk_t *ps;

	if (data == NULL)
		return;

	attr = &ctx->dlg[ctx->wlist];
	r = rnd_dad_tree_get_selected(attr);
	if (r == NULL)
		return;

	pid = strtol(r->cell[0], NULL, 10);

	/* unselect all */
	box.X1 = -RND_MAX_COORD;
	box.Y1 = -RND_MAX_COORD;
	box.X2 = RND_MAX_COORD;
	box.Y2 = RND_MAX_COORD;
	if (pcb_select_block(PCB, &box, rnd_false, rnd_false, rnd_false))
		changed = 1;

	for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
		if (ps->proto == pid) {
			changed = 1;
			pcb_undo_add_obj_to_flag(ps);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, ps);
		}
	}

	if (changed) {
		pcb_board_set_changed_flag(PCB, rnd_true);
		rnd_gui->invalidate_all(rnd_gui);
	}
}

static void pstklib_count_uses(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_cardinal_t len;
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);

	if (data == NULL)
		return;

	ctx->stat = pcb_pstk_proto_used_all(data, &len);
	pstklib_data2dlg(ctx);
	free(ctx->stat);
	ctx->stat = NULL;
}

static void pstklib_del_unused(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_cardinal_t len, n;
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);

	if (data == NULL)
		return;

	ctx->stat = pcb_pstk_proto_used_all(data, &len);
	for(n = 0; n < len; n++) {
		if (ctx->stat[n] == 0)
			pcb_pstk_proto_del(data, n);
	}
	pstklib_data2dlg(ctx);
	free(ctx->stat);
	ctx->stat = NULL;
}

rnd_cardinal_t pcb_dlg_pstklib(pcb_board_t *pcb, long subc_id, rnd_bool modal, const char *hint)
{
	static const char *hdr[] = {"ID", "name", "used", NULL};
	pcb_subc_t *sc;
	pcb_data_t *data;
	pstk_lib_ctx_t *ctx;
	int n;
	char *name;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	rnd_hid_dad_buttons_t clbtn_modal[] = {{"Cancel", -1}, {"Use selected", 0}, {NULL, 0}};

	if (subc_id <= 0)
		subc_id = -1;

	if ((!modal) && (htip_get(&pstk_libs, subc_id) != NULL))
		return 0; /* already open - have only one per id */

	ctx = calloc(sizeof(pstk_lib_ctx_t), 1);
	ctx->pcb = pcb;
	ctx->subc_id = subc_id;
	ctx->proto_id = PCB_PADSTACK_INVALID;
	ctx->modal = modal;

	data = get_data(ctx, subc_id, &sc);
	if (data == NULL) {
		free(ctx);
		return PCB_PADSTACK_INVALID;
	}

	if (!modal)
		htip_set(&pstk_libs, subc_id, ctx);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			if (hint != NULL)
				RND_DAD_LABEL(ctx->dlg, hint);

		/* create the dialog box */
		RND_DAD_BEGIN_HPANE(ctx->dlg);
			/* left */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 3, 0, hdr);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, pstklib_select);
					RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
					ctx->wlist = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_STRING(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, "Filter text:\nlist padstacks with matching name only");
					RND_DAD_CHANGE_CB(ctx->dlg, pstklib_filter_cb);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "Edit...");
						RND_DAD_HELP(ctx->dlg, "Edit the selected prototype\nusing the padstack editor");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_edit);
					RND_DAD_BUTTON(ctx->dlg, "New...");
						RND_DAD_HELP(ctx->dlg, "Create a new prototype and edit it\nusing the padstack editor");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_new);
					RND_DAD_BUTTON(ctx->dlg, "Switch");
						RND_DAD_HELP(ctx->dlg, "Find all padstacks using this prototype\nand modify them to use a different prototype\nmaking this prototype unused");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_switch);
					RND_DAD_BUTTON(ctx->dlg, "Select");
						RND_DAD_HELP(ctx->dlg, "Select all padstack ref. objects that\nreference (use) this prototype");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_select);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "Dup...");
						RND_DAD_HELP(ctx->dlg, "Create a new prototype by duplicating\nthe currently selected on and\n edit it using the padstack editor");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_dup);
					RND_DAD_BUTTON(ctx->dlg, "Count uses");
						RND_DAD_HELP(ctx->dlg, "Count how many times each prototype\nis used and update the \"used\"\ncolumn of the table");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_count_uses);
					RND_DAD_BUTTON(ctx->dlg, "Del unused");
						RND_DAD_HELP(ctx->dlg, "Update prototype usage stats and\nremove prototypes that are not\nused by any padstack");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_del_unused);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "Load...");
						RND_DAD_HELP(ctx->dlg, "Replace the selected padstack with one loaded from a file");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_load);
					RND_DAD_BUTTON(ctx->dlg, "Save...");
						RND_DAD_HELP(ctx->dlg, "Save the selected padstack to a file");
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_save);
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);

			/* right */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_PREVIEW(ctx->dlg, pstklib_expose, NULL, NULL, NULL, NULL, 200, 200, ctx);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
					ctx->wprev = RND_DAD_CURRENT(ctx->dlg);

				RND_DAD_BEGIN_TABLE(ctx->dlg, 2);
					RND_DAD_LABEL(ctx->dlg, "Grid:");
					RND_DAD_COORD(ctx->dlg);
						ctx->wgrid = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_MINMAX(ctx->dlg, RND_MM_TO_COORD(0.01), RND_MM_TO_COORD(10));
						RND_DAD_DEFAULT_NUM(ctx->dlg, (rnd_coord_t)RND_MM_TO_COORD(1));
						RND_DAD_CHANGE_CB(ctx->dlg, pstklib_update_prv);

					RND_DAD_LABEL(ctx->dlg, "");
					RND_DAD_BEGIN_HBOX(ctx->dlg);
						RND_DAD_LABEL(ctx->dlg, "Vis");
							RND_DAD_HELP(ctx->dlg, "layer is visible");
						RND_DAD_LABEL(ctx->dlg, "Curr");
							RND_DAD_HELP(ctx->dlg, "layer is set to current/primary\nfor display (color emphasis)");
					RND_DAD_END(ctx->dlg);
					for(n = 0; n < pcb_proto_num_layers; n++) {
						RND_DAD_LABEL(ctx->dlg, pcb_proto_layers[n].name);
						RND_DAD_BEGIN_HBOX(ctx->dlg);
							RND_DAD_BOOL(ctx->dlg);
								RND_DAD_DEFAULT_NUM(ctx->dlg, 1);
								RND_DAD_CHANGE_CB(ctx->dlg, pstklib_update_prv);
								ctx->wlayerv[n] = RND_DAD_CURRENT(ctx->dlg);
							RND_DAD_BOOL(ctx->dlg);
								RND_DAD_DEFAULT_NUM(ctx->dlg, (pcb_proto_layers[n].mask == (PCB_LYT_TOP | PCB_LYT_COPPER)));
								RND_DAD_CHANGE_CB(ctx->dlg, pstklib_update_layerc);
								ctx->wlayerc[n] = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_END(ctx->dlg);
					}
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BUTTON_CLOSES(ctx->dlg, (modal ? clbtn_modal : clbtn));
	RND_DAD_END(ctx->dlg);

	if (subc_id > 0) {
		if (sc->refdes != NULL)
			name = rnd_strdup_printf("pcb-rnd padstacks - subcircuit #%ld (%s)", subc_id, sc->refdes);
		else
			name = rnd_strdup_printf("pcb-rnd padstacks - subcircuit #%ld", subc_id);
	}
	else
		name = rnd_strdup("pcb-rnd padstacks - board");

	RND_DAD_NEW("pstk_lib", ctx->dlg, name, ctx, modal, pstklib_close_cb);

	pstklib_data2dlg(ctx);
	free(name);

	if (modal) {
		if (RND_DAD_RUN(ctx->dlg) != 0)
			return PCB_PADSTACK_INVALID;
		return pstklib_last_proto_id;
	}

	return 0;
}

const char pcb_acts_pstklib[] = "pstklib([board|subcid|object])\n";
const char pcb_acth_pstklib[] = "Present the padstack library dialog on board padstacks or the padstacks of a subcircuit";
fgw_error_t pcb_act_pstklib(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long id = -1;
	const char *cmd = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, pstklib, cmd = argv[1].val.str);
	if ((cmd != NULL) && (strcmp(cmd, "object") == 0)) {
		rnd_coord_t x, y;
		void *r1, *r2, *r3;
		pcb_subc_t *sc;
		int type;
		rnd_hid_get_coords("Pick a subcircuit for padstack lib editing", &x, &y, 0);
		type = pcb_search_obj_by_location(PCB_OBJ_SUBC, &r1, &r2, &r3, x, y, PCB_SLOP * rnd_pixel_slop);
		if (type != PCB_OBJ_SUBC) {
			RND_ACT_IRES(-1);
			return 0;
		}
		sc = r2;
		id = sc->ID;
	}
	else
		RND_ACT_MAY_CONVARG(1, FGW_LONG, pstklib, id = argv[1].val.nat_long);
	if (pcb_dlg_pstklib(PCB, id, rnd_false, NULL) == PCB_PADSTACK_INVALID)
		RND_ACT_IRES(-1);
	else
		RND_ACT_IRES(0);
	return 0;
}

void pcb_dlg_pstklib_init(void)
{
	htip_init(&pstk_libs, longhash, longkeyeq);
}

void pcb_dlg_pstklib_uninit(void)
{
	htip_entry_t *e;
	for(e = htip_first(&pstk_libs); e != NULL; e = htip_next(&pstk_libs, e))
		pstklib_close_cb(e->value, 0);
	htip_uninit(&pstk_libs);
	free(proto_save_fn);
	free(proto_load_fn);
	proto_save_fn = proto_load_fn = NULL;
}
