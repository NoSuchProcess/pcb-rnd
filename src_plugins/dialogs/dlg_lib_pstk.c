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

/* Padstack library dialog */

#include "config.h"
#include "obj_subc.h"
#include "vtpadstack.h"
#include "hid_inlines.h"
#include "obj_text_draw.h"

htip_t pstk_libs; /* id -> pstk_lib_ctx_t */

typedef struct pstk_lib_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wlist, wprev, wgrid, wlayer[pcb_proto_num_layers];
	long subc_id;
	pcb_cardinal_t proto_id;
	pcb_cardinal_t *stat; /* temporary usage stat */
	pcb_box_t drawbox;
} pstk_lib_ctx_t;

static pcb_cardinal_t pstklib_last_proto_id; /* set on close to preserve the id after free'ing the context; useful only for modal windows because of blocking calls */

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
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	long id;

	if (data == NULL)
		return -1;

	attr = &ctx->dlg[ctx->wlist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);

	/* add all items */
	cell[3] = NULL;
	for(id = 0, proto = data->ps_protos.array; id < pcb_vtpadstack_proto_len(&data->ps_protos); proto++,id++) {
		if (!proto->in_use)
			continue;
		cell[0] = pcb_strdup_printf("%ld", id);
		cell[1] = pcb_strdup(proto->name == NULL ? "" : proto->name);
		if (ctx->stat != NULL)
			cell[2] = pcb_strdup_printf("%d", ctx->stat[id]);
		else
			cell[2] = pcb_strdup("");
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);
		free(cursor_path);
	}
	return 0;
}

static void pstklib_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pstk_lib_ctx_t *ctx = caller_data;
	htip_pop(&pstk_libs, ctx->subc_id);
	pstklib_last_proto_id = ctx->proto_id;
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void pstklib_setps(pcb_pstk_t *ps, pcb_data_t *data, pcb_cardinal_t proto_id)
{
	memset(ps, 0, sizeof(pcb_pstk_t));
	ps->parent_type = PCB_PARENT_DATA;
	ps->parent.data = data;
	ps->proto = proto_id;
	ps->ID = -1; /* disable undo and clipping */
}

static void pstklib_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pstk_lib_ctx_t *ctx = prv->user_ctx;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	pcb_pstk_t ps;
	char layers[pcb_proto_num_layers];
	int n;
	pcb_coord_t x1, y1, x2, y2, x, y, grid;

	if (data == NULL) {
		return;
	}

	if (ctx->proto_id == PCB_PADSTACK_INVALID)
		return;

	pstklib_setps(&ps, data, ctx->proto_id);

	/* draw the shapes */
	for(n = 0; n < pcb_proto_num_layers; n++)
		layers[n] = !!ctx->dlg[ctx->wlayer[n]].default_val.int_value;

	pcb_pstk_draw_preview(PCB, &ps, layers, 0, 0, &e->view);

	pcb_gui->set_color(gc, "#000000");
	pcb_hid_set_line_cap(gc, pcb_cap_round);
	pcb_hid_set_line_width(gc, -1);

	x1 = ctx->drawbox.X1;
	y1 = ctx->drawbox.Y1;
	x2 = ctx->drawbox.X2;
	y2 = ctx->drawbox.Y2;

	grid = ctx->dlg[ctx->wgrid].default_val.coord_value;
	for(x = 0; x < x2; x += grid)
		pcb_gui->draw_line(gc, x, y1, x, y2);
	for(x = -grid; x > x1; x -= grid)
		pcb_gui->draw_line(gc, x, y1, x, y2);
	for(y = 0; y < y2; y += grid)
		pcb_gui->draw_line(gc, x1, y, x2, y);
	for(y = -grid; y > y1; y -= grid)
		pcb_gui->draw_line(gc, x1, y, x2, y);

	/* draw the mark only */
	for(n = 0; n < pcb_proto_num_layers; n++)
		layers[n] = 0;
	pcb_pstk_draw_preview(PCB, &ps, layers, 1, 0, &e->view);
}

static void pstklib_force_redraw(pstk_lib_ctx_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_bbox(ps);
	ps->BoundingBox.X1 -= PCB_MM_TO_COORD(0.5);
	ps->BoundingBox.Y1 -= PCB_MM_TO_COORD(0.5);
	ps->BoundingBox.X2 += PCB_MM_TO_COORD(0.5);
	ps->BoundingBox.Y2 += PCB_MM_TO_COORD(0.5);
	memcpy(&ctx->drawbox, &ps->BoundingBox, sizeof(pcb_box_t));
	pcb_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &ctx->drawbox);
}

static void pstklib_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_attr_val_t hv;
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
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

	hv.str_value = NULL;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}

static void pstklib_update_prv(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &ctx->drawbox);
}

static void pstklib_proto_edit_change_cb(pse_t *pse)
{
	pstklib_force_redraw(pse->user_data, pse->ps);
}

static void pstklib_proto_edit_common(pstk_lib_ctx_t *ctx, pcb_data_t *data, pcb_cardinal_t proto_id, int tab)
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


static void pstklib_proto_edit(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(&ctx->dlg[ctx->wlist]);

	if ((row == NULL) && (data == NULL))
		return;

	pstklib_proto_edit_common(ctx, data, strtol(row->cell[0], NULL, 10), 1);
}

static void pstklib_proto_new(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);
	pcb_pstk_proto_t proto;
	pcb_hid_attr_val_t hv;
	char tmp[64];

	if (data == NULL)
		return;

	memset(&proto, 0, sizeof(proto));
	pcb_pstk_proto_update(&proto);
	ctx->proto_id = pcb_pstk_proto_insert_dup(data, &proto, 1);

	/* make sure the new item appears in the list and is selected */
	pstklib_data2dlg(ctx);
	sprintf(tmp, "%lu", ctx->proto_id);
	hv.str_value = tmp;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wlist, &hv);

	pstklib_proto_edit_common(ctx, data, ctx->proto_id, 2);
}

static void pstklib_count_uses(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{

	pcb_cardinal_t len, n;
	pstk_lib_ctx_t *ctx = caller_data;
	pcb_data_t *data = get_data(ctx, ctx->subc_id, NULL);

	if (data == NULL)
		return;

	ctx->stat = pcb_pstk_proto_used_all(data, &len);
	pstklib_data2dlg(ctx);
	free(ctx->stat);
	ctx->stat = NULL;
}

static void pstklib_del_unused(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_cardinal_t len, n;
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

pcb_cardinal_t pcb_dlg_pstklib(pcb_board_t *pcb, long subc_id, pcb_bool modal)
{
	static const char *hdr[] = {"ID", "name", "used", NULL};
	pcb_subc_t *sc;
	pcb_data_t *data;
	pstk_lib_ctx_t *ctx;
	int n;
	char *name;

	if (subc_id <= 0)
		subc_id = -1;

	if (htip_get(&pstk_libs, subc_id) != NULL)
		return 0; /* already open - have only one per id */

	ctx = calloc(sizeof(pstk_lib_ctx_t), 1);
	ctx->pcb = pcb;
	ctx->subc_id = subc_id;
	ctx->proto_id = PCB_PADSTACK_INVALID;

	data = get_data(ctx, subc_id, &sc);
	if (data == NULL) {
		free(ctx);
		return PCB_PADSTACK_INVALID;
	}

	htip_set(&pstk_libs, subc_id, ctx);

	/* create the dialog box */
	PCB_DAD_BEGIN_HPANE(ctx->dlg);
		/* left */
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(ctx->dlg, 3, 0, hdr);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, pstklib_select);
				PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
				ctx->wlist = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_HELP(ctx->dlg, "Filter text:\nlist padstacks with matching name only");
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "Edit...");
					PCB_DAD_HELP(ctx->dlg, "Edit the selected prototype\nusing the padstack editor");
					PCB_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_edit);
				PCB_DAD_BUTTON(ctx->dlg, "New...");
					PCB_DAD_HELP(ctx->dlg, "Create a new prototype and edit it\nusing the padstack editor");
					PCB_DAD_CHANGE_CB(ctx->dlg, pstklib_proto_new);
				PCB_DAD_BUTTON(ctx->dlg, "Switch");
					PCB_DAD_HELP(ctx->dlg, "Find all padstacks using this prototype\nand modify them to use a different prototype\nmaking this prototype unused");
			PCB_DAD_END(ctx->dlg);
			PCB_DAD_BEGIN_HBOX(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "Count uses");
					PCB_DAD_HELP(ctx->dlg, "Count how many times each prototype\nis used and update the \"used\"\ncolumn of the table");
					PCB_DAD_CHANGE_CB(ctx->dlg, pstklib_count_uses);
				PCB_DAD_BUTTON(ctx->dlg, "Del unused");
					PCB_DAD_HELP(ctx->dlg, "Update prototype usage stats and\nremove prototypes that are not\nused by any padstack");
					PCB_DAD_CHANGE_CB(ctx->dlg, pstklib_del_unused);
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		/* right */
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_PREVIEW(ctx->dlg, pstklib_expose, NULL, NULL, NULL, ctx);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				ctx->wprev = PCB_DAD_CURRENT(ctx->dlg);

			PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
				PCB_DAD_LABEL(ctx->dlg, "Grid:");
				PCB_DAD_COORD(ctx->dlg, "");
					ctx->wgrid = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_MINMAX(ctx->dlg, PCB_MM_TO_COORD(0.01), PCB_MM_TO_COORD(10));
					PCB_DAD_DEFAULT(ctx->dlg, (pcb_coord_t)PCB_MM_TO_COORD(1));
					PCB_DAD_CHANGE_CB(ctx->dlg, pstklib_update_prv);
				for(n = 0; n < pcb_proto_num_layers; n++) {
					PCB_DAD_LABEL(ctx->dlg, pcb_proto_layers[n].name);
					PCB_DAD_BOOL(ctx->dlg, "");
						PCB_DAD_DEFAULT(ctx->dlg, 1);
						PCB_DAD_CHANGE_CB(ctx->dlg, pstklib_update_prv);
						ctx->wlayer[n] = PCB_DAD_CURRENT(ctx->dlg);
				}
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	if (subc_id > 0) {
		if (sc->refdes != NULL)
			name = pcb_strdup_printf("pcb-rnd padstacks - subcircuit #%ld (%s)", subc_id, sc->refdes);
		else
			name = pcb_strdup_printf("pcb-rnd padstacks - subcircuit #%ld", subc_id);
	}
	else
		name = pcb_strdup("pcb-rnd padstacks - board");

	PCB_DAD_NEW(ctx->dlg, name, "", ctx, modal, pstklib_close_cb);

	pstklib_data2dlg(ctx);
	free(name);

	if (modal) {
		if (PCB_DAD_RUN(ctx->dlg) != 0)
			return PCB_PADSTACK_INVALID;
		return pstklib_last_proto_id;
	}

	return 0;
}

static const char pcb_acts_pstklib[] = "pstklib([board|subcid])\n";
static const char pcb_acth_pstklib[] = "Present the padstack library dialog on board padstacks or the padstacks of a subcircuit";
static fgw_error_t pcb_act_pstklib(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long id = -1;
	PCB_ACT_MAY_CONVARG(1, FGW_LONG, pstklib, id = argv[1].val.nat_long);
	if (pcb_dlg_pstklib(PCB, id, pcb_false+1) == PCB_PADSTACK_INVALID)
		PCB_ACT_IRES(-1);
	else
		PCB_ACT_IRES(0);
	return 0;
}

static void dlg_pstklib_init(void)
{
	htip_init(&pstk_libs, longhash, longkeyeq);
}

static void dlg_pstklib_uninit(void)
{
	htip_entry_t *e;
	for(e = htip_first(&pstk_libs); e != NULL; e = htip_next(&pstk_libs, e))
		pstklib_close_cb(e->value, 0);
	htip_uninit(&pstk_libs);
}
