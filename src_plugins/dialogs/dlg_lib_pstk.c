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

htip_t pstk_libs; /* id -> pstk_lib_ctx_t */

typedef struct pstk_lib_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
	int wlist, wprev;
	long id;
} pstk_lib_ctx_t;

static pcb_data_t *get_data(long id, pcb_subc_t **sc_out)
{
	int type;
	void *r1, *r2, *r3;
	pcb_subc_t *sc;

	if (id < 0)
		return PCB->Data;

	type = pcb_search_obj_by_id_(PCB->Data, &r1, &r2, &r3, id, PCB_OBJ_SUBC);
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
	pcb_data_t *data = get_data(ctx->id, NULL);
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
	for(id = 0, proto = data->ps_protos.array; id < pcb_vtpadstack_proto_len(data->ps_protos.array); proto++,id++) {
		if (!proto->in_use)
			continue;
		cell[0] = pcb_strdup_printf("%ld", id);
		cell[1] = pcb_strdup(proto->name == NULL ? "" : proto->name);
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
	htip_pop(&pstk_libs, ctx->id);
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void pstklib_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pstk_lib_ctx_t *ctx = prv->user_ctx;
	pcb_data_t *data = get_data(ctx->id, NULL);
	pcb_pstk_t ps;

	if (data == NULL) {
		return;
	}

	memset(&ps, 0, sizeof(ps));
	ps.parent.data = data;
	ps.proto = ctx->id;

	pcb_pstk_draw_preview(PCB, ps, &e->view);
}

static void pstklib_select(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_attr_val_t hv;
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	pstk_lib_ctx_t *ctx = tree->user_ctx;

	printf("Select!\n");
	hv.str_value = NULL;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wprev, &hv);
}

static int pcb_dlg_pstklib(long id)
{
	static const char *hdr[] = {"ID", "name", "used", NULL};
	pcb_subc_t *sc;
	pcb_data_t *data;
	pstk_lib_ctx_t *ctx;
	int n;
	char *name;

	if (id <= 0)
		id = -1;

	data = get_data(id, &sc);
	if (data == NULL)
		return -1;

	if (htip_get(&pstk_libs, id) != NULL)
		return 0; /* already open - have only one per id */

	ctx = calloc(sizeof(pstk_lib_ctx_t), 1);
	ctx->id = id;
	htip_set(&pstk_libs, id, ctx);

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
				PCB_DAD_BUTTON(ctx->dlg, "Count uses");
					PCB_DAD_HELP(ctx->dlg, "Count how many times each prototype\nis used and update the \"used\"\ncolumn of the table");
				PCB_DAD_BUTTON(ctx->dlg, "Del unused");
					PCB_DAD_HELP(ctx->dlg, "Update prototype usage stats and\nremove prototypes that are not\nused by any padstack");
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		/* right */
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_PREVIEW(ctx->dlg, pstklib_expose, NULL, NULL, NULL, ctx);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				ctx->wprev = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
				for(n = 0; n < pcb_proto_num_layers; n++) {
					PCB_DAD_LABEL(ctx->dlg, pcb_proto_layers[n].name);
					PCB_DAD_BOOL(ctx->dlg, "");
				}
			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	if (id > 0) {
		if (sc->refdes != NULL)
			name = pcb_strdup_printf("pcb-rnd padstacks - subcircuit #%ld (%s)", id, sc->refdes);
		else
			name = pcb_strdup_printf("pcb-rnd padstacks - subcircuit #%ld", id);
	}
	else
		name = pcb_strdup("pcb-rnd padstacks - board");

	PCB_DAD_NEW(ctx->dlg, name, "", ctx, pcb_false, pstklib_close_cb);

	free(name);
	pstklib_data2dlg(ctx);
	return 0;
}

static const char pcb_acts_pstklib[] = "pstklib([board|subcid])\n";
static const char pcb_acth_pstklib[] = "Present the padstack library dialog on board padstacks or the padstacks of a subcircuit";
static fgw_error_t pcb_act_pstklib(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long id = -1;
	PCB_ACT_MAY_CONVARG(1, FGW_LONG, pstklib, id = argv[1].val.nat_long);
	PCB_ACT_IRES(pcb_dlg_pstklib(id));
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
