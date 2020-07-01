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

#include <genht/hash.h>
#include <genlist/gendlist.h>
#include <genvector/gds_char.h>

#include "board.h"
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/hid_dad_tree.h>
#include <librnd/core/conf_hid.h>
#include "netlist.h"

#include "props.h"
#include "propsel.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	pcb_propedit_t pe;
	int wtree, wfilter, wtype, wvals, wscope;
	int wabs[PCB_PROPT_max], wedit[PCB_PROPT_max];
	gdl_elem_t link;
} propdlg_t;

gdl_list_t propdlgs;

static void prop_refresh(propdlg_t *ctx);

static void propdlgclose_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	propdlg_t *ctx = caller_data;
	gdl_remove(&propdlgs, ctx, link);
	pcb_props_uninit(&ctx->pe);
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void prop_filter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_ign)
{
	propdlg_t *ctx = caller_data;
	rnd_hid_attribute_t *attr, *attr_inp;
	rnd_hid_tree_t *tree;
	const char *text;
	int have_filter_text;

	attr = &ctx->dlg[ctx->wtree];
	attr_inp = &ctx->dlg[ctx->wfilter];
	tree = attr->wdata;
	text = attr_inp->val.str;
	have_filter_text = (text != NULL) && (*text != '\0');

	/* hide or unhide everything */
	rnd_dad_tree_hide_all(tree, &tree->rows, have_filter_text);

	if (have_filter_text) /* unhide hits and all their parents */
		rnd_dad_tree_unhide_filter(tree, &tree->rows, 0, text);

	rnd_dad_tree_update_hide(attr);
}

static void prop_pcb2dlg(propdlg_t *ctx)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wtree];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *r;
	htsp_entry_t *sorted, *e;
	char *cursor_path = NULL;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->path);

	rnd_dad_tree_clear(tree);

	pcb_props_clear(&ctx->pe);
	pcb_propsel_map_core(&ctx->pe);
	sorted = pcb_props_sort(&ctx->pe);
	for(e = sorted; e->key != NULL; e++) {
		char *cell[6] = {NULL};
		pcb_props_t *p = e->value;
		pcb_propval_t com, min, max, avg;

		if (p->type == PCB_PROPT_STRING) {
			pcb_props_stat(&ctx->pe, p, &com, NULL, NULL, NULL);
		}
		else {
			pcb_props_stat(&ctx->pe, p, &com, &min, &max, &avg);
			cell[2] = pcb_propsel_printval(p->type, &min);
			cell[3] = pcb_propsel_printval(p->type, &max);
			cell[4] = pcb_propsel_printval(p->type, &avg);
		}

		cell[0] = rnd_strdup(e->key);
		cell[1] = pcb_propsel_printval(p->type, &com);

		r = rnd_dad_tree_mkdirp(tree, cell[0], cell);
		r->user_data = e->key;
	}
	free(sorted);
	prop_filter_cb(ctx->dlg_hid_ctx, ctx, NULL);

	rnd_dad_tree_expcoll(attr, NULL, 1, 1);

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtree, &hv);
		free(cursor_path);
	}

	/* set scope */
	{
		rnd_hid_attr_val_t hv;
		gds_t scope;
		int n, inv;
		long *l;
		pcb_idpath_t *idp;

		gds_init(&scope);
		if (ctx->pe.board)
			gds_append_str(&scope, "board, ");
		if (ctx->pe.selection)
			gds_append_str(&scope, "selected objects, ");
		inv = 0;
		for(n = 0, l = ctx->pe.layers.array; n < vtl0_len(&ctx->pe.layers); n++, l++) {
			const char *name = pcb_layer_name(ctx->pe.data, *l);
			if (name != 0) {
				gds_append_str(&scope, "layer: ");
				gds_append_str(&scope, name);
				gds_append_str(&scope, ", ");
			}
			else
				inv++;
		}
		if (inv > 0)
			rnd_append_printf(&scope, "%d invalid layers, ", inv);
		inv = 0;
		for(n = 0, l = ctx->pe.layergrps.array; n < vtl0_len(&ctx->pe.layergrps); n++, l++) {
			const char *name = pcb_layergrp_name(ctx->pe.pcb, *l);
			if (name != 0) {
				gds_append_str(&scope, "layergrp: ");
				gds_append_str(&scope, name);
				gds_append_str(&scope, ", ");
			}
			else
				inv++;
		}
		if (inv > 0)
			rnd_append_printf(&scope, "%d invalid layer groups, ", inv);

		if (ctx->pe.nets_inited) {
			htsp_entry_t *e;
			inv = 0;
			for(e = htsp_first(&ctx->pe.nets); e != NULL; e = htsp_next(&ctx->pe.nets, e)) {
				if (pcb_net_get(ctx->pe.pcb, &ctx->pe.pcb->netlist[PCB_NETLIST_EDITED], e->key, 0) != NULL) {
					gds_append_str(&scope, "net: ");
					gds_append_str(&scope, e->key);
					gds_append_str(&scope, ", ");
				}
				else
					inv++;
			}
			if (inv > 0)
				rnd_append_printf(&scope, "%d invalid nets, ", inv);
		}

		inv = 0;
		for(idp = pcb_idpath_list_first(&ctx->pe.objs); idp != NULL; idp = pcb_idpath_list_next(idp)) {
			pcb_any_obj_t *o = pcb_idpath2obj_in(ctx->pe.data, idp);
			if (o != NULL)
				rnd_append_printf(&scope, "%s #%ld, ", pcb_obj_type_name(o->type), o->ID);
			else
				inv++;
		}
		if (inv > 0)
			rnd_append_printf(&scope, "%d invalid objects, ", inv);

		gds_truncate(&scope, gds_len(&scope)-2);

		hv.str = scope.array;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wscope, &hv);
	}
}

static void prop_prv_expose_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	
}


static rnd_bool prop_prv_mouse_cb(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	return rnd_false; /* don't redraw */
}

static void prop_valedit_update(propdlg_t *ctx, pcb_props_t *p, pcb_propval_t *pv)
{
	rnd_hid_attr_val_t hv;

	/* do not update the value if widget is numeric and the user wants a relative value */
	switch(p->type) {
		case PCB_PROPT_COORD:
		case PCB_PROPT_ANGLE:
		case PCB_PROPT_DOUBLE:
		case PCB_PROPT_INT:
			if (!ctx->dlg[ctx->wabs[p->type]].val.lng)
				return;
		default: break;
	}


	memset(&hv, 0, sizeof(hv));
	switch(p->type) {
		case PCB_PROPT_STRING: hv.str = rnd_strdup(pv->string == NULL ? "" : pv->string); break;
		case PCB_PROPT_COORD:  hv.crd = pv->coord; break;
		case PCB_PROPT_ANGLE:  hv.dbl = pv->angle; break;
		case PCB_PROPT_DOUBLE: hv.dbl = pv->d; break;
		case PCB_PROPT_BOOL:
		case PCB_PROPT_INT:    hv.lng = pv->i; break;
		case PCB_PROPT_COLOR:  hv.clr = pv->clr; break;
		case PCB_PROPT_invalid:
		case PCB_PROPT_max: return;
	}
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wedit[p->type], &hv);
}

typedef struct {
	pcb_propval_t *val;
	unsigned int cnt;
} pvsort_t;

static int sort_pv(const void *pv1_, const void *pv2_)
{
	const pvsort_t *pv1 = pv1_, *pv2 = pv2_;
	return pv1->cnt > pv2->cnt ? -1 : +1;
}

static void prop_vals_update(propdlg_t *ctx, pcb_props_t *p)
{
	rnd_hid_attr_val_t hv;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wvals];
	rnd_hid_tree_t *tree = attr->wdata;
	htprop_entry_t *e;
	pvsort_t *pvs;
	char *cell[3] = {NULL, NULL, NULL};
	int n;

	rnd_dad_tree_clear(tree);

	if (p == NULL) { /* deselect or not found */
		hv.lng = 0;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtype, &hv);
		return;
	}

	/* get a sorted list of all values, by frequency */
	pvs = malloc(sizeof(pvsort_t) * p->values.used);
	for (e = htprop_first(&p->values), n = 0; e; e = htprop_next(&p->values, e), n++) {
		pvs[n].val = &e->key;
		pvs[n].cnt = e->value;
	}
	qsort(pvs, p->values.used, sizeof(pvsort_t), sort_pv);

	for(n = 0; n < p->values.used; n++) {
		rnd_hid_row_t *r;
		cell[0] = rnd_strdup_printf("%ld", pvs[n].cnt);
		cell[1] = pcb_propsel_printval(p->type, pvs[n].val);
		r = rnd_dad_tree_append(attr, NULL, cell);
		r->user_data = pvs[n].val;
	}

	hv.lng = p->type;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtype, &hv);

	prop_valedit_update(ctx, p, pvs[0].val);

	free(pvs);
}

static void prop_select_node_cb(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	propdlg_t *ctx = tree->user_ctx;
	pcb_props_t *p = NULL;

	if (row != NULL)
		p = pcb_props_get(&ctx->pe, row->user_data);

	prop_vals_update(ctx, p);
}


static void prop_data_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr, int force_update)
{
	propdlg_t *ctx = caller_data;
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wtree]);
	pcb_props_t *p = NULL;
	pcb_propset_ctx_t sctx;

	if (r == NULL)
		return;
	p = pcb_props_get(&ctx->pe, r->user_data);
	if (p == NULL)
		return;

	memset(&sctx, 0, sizeof(sctx));
	switch(p->type) {
		case PCB_PROPT_invalid:
		case PCB_PROPT_max:
			return;
		case PCB_PROPT_STRING:
			sctx.s = ctx->dlg[ctx->wedit[p->type]].val.str;
			break;
		case PCB_PROPT_COORD:
			sctx.c = ctx->dlg[ctx->wedit[p->type]].val.crd;
			sctx.c_absolute = ctx->dlg[ctx->wabs[p->type]].val.lng;
			sctx.c_valid = 1;
			break;
		case PCB_PROPT_ANGLE:
			sctx.d = ctx->dlg[ctx->wedit[p->type]].val.dbl;
			sctx.d_absolute = ctx->dlg[ctx->wabs[p->type]].val.lng;
			sctx.d_valid = 1;
			break;
		case PCB_PROPT_DOUBLE:
			sctx.d = ctx->dlg[ctx->wedit[p->type]].val.dbl;
			sctx.d_absolute = ctx->dlg[ctx->wabs[p->type]].val.lng;
			sctx.d_valid = 1;
			break;
		case PCB_PROPT_INT:
			sctx.c = ctx->dlg[ctx->wedit[p->type]].val.lng;
			sctx.c_absolute = ctx->dlg[ctx->wabs[p->type]].val.lng;
			sctx.c_valid = 1;
			break;
		case PCB_PROPT_BOOL:
			sctx.c = ctx->dlg[ctx->wedit[p->type]].val.lng;
			sctx.c_absolute = ctx->dlg[ctx->wabs[p->type]].val.lng;
			sctx.c_valid = 1;
			break;
		case PCB_PROPT_COLOR:
			sctx.color = ctx->dlg[ctx->wedit[p->type]].val.clr;
			sctx.clr_valid = 1;
			break;
	}

	if (force_update || ctx->dlg[ctx->wabs[p->type]].val.lng) {
		pcb_propsel_set(&ctx->pe, r->user_data, &sctx);
		prop_refresh(ctx);
		rnd_gui->invalidate_all(rnd_gui);
	}
}

static void prop_data_auto_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	prop_data_cb(hid_ctx, caller_data, attr, 0);
}

static void prop_data_force_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	prop_data_cb(hid_ctx, caller_data, attr, 1);
}


static void prop_add_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	RND_DAD_DECL(dlg)
	propdlg_t *ctx = caller_data;
	const char *key;
	int wkey, wval, failed;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"OK", 0}, {NULL, 0}};

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_BEGIN_TABLE(dlg, 2);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
			RND_DAD_LABEL(dlg, "Attribute key:");
			RND_DAD_STRING(dlg);
				wkey = RND_DAD_CURRENT(dlg);
			RND_DAD_LABEL(dlg, "Attribute value:");
			RND_DAD_STRING(dlg);
				wval = RND_DAD_CURRENT(dlg);
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);
	RND_DAD_AUTORUN("propedit_add", dlg, "Propedit: add new attribute", NULL, failed);

	key = dlg[wkey].val.str;
	if (key == NULL) key = "";
	while(isspace(*key)) key++;

	if ((failed == 0) && (*key != '\0')) {
		char *path = rnd_strdup_printf("a/%s", key);
		pcb_propsel_set_str(&ctx->pe, path, dlg[wval].val.str);
		free(path);
		prop_refresh(ctx);
	}
	RND_DAD_FREE(dlg);
}

static void prop_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	propdlg_t *ctx = caller_data;
	rnd_hid_row_t *r = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wtree]);
	if (r == NULL) {
		rnd_message(RND_MSG_ERROR, "can not delete: no attribute selected\n");
		return;
	}
	if (r->path[0] != 'a') {
		rnd_message(RND_MSG_ERROR, "Only atributes (a/ subtree) can be deleted.\n");
		return;
	}
	if (pcb_propsel_del(&ctx->pe, r->path) < 1) {
		rnd_message(RND_MSG_ERROR, "Failed to remove the attribute from any object.\n");
		return;
	}
	prop_refresh(ctx);
}

static void prop_preset_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	propdlg_t *ctx = caller_data;
	rnd_hid_row_t *rv = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wvals]);
	rnd_hid_row_t *rp = rnd_dad_tree_get_selected(&ctx->dlg[ctx->wtree]);
	pcb_props_t *p;

	if ((rv == NULL) || (rv->user_data == NULL) || (rp == NULL))
		return;

	p = pcb_props_get(&ctx->pe, rp->user_data);
	if (p != NULL)
		prop_valedit_update(ctx, p, rv->user_data);
}


static void prop_refresh_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	prop_refresh((propdlg_t *)caller_data);
}


static void prop_refresh(propdlg_t *ctx)
{
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->wtree];
	prop_pcb2dlg(ctx);
	prop_select_node_cb(attr, ctx->dlg_hid_ctx, rnd_dad_tree_get_selected(attr));
}


static void build_propval(propdlg_t *ctx)
{
	static const char *type_tabs[] = {"none", "string", "coord", "angle", "double", "int", NULL};
	static const char *abshelp = "When unticked each apply is a relative change added to\nthe current value of each object";

	RND_DAD_BEGIN_TABBED(ctx->dlg, type_tabs);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_HIDE_TABLAB);
		ctx->wtype = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(nothing to edit)");
				ctx->wabs[0] = 0;
				ctx->wedit[0] = 0;
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: string");
			RND_DAD_STRING(ctx->dlg);
				ctx->wedit[1] = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_ENTER_CB(ctx->dlg, prop_data_auto_cb);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				ctx->wabs[1] = 0;
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: coord");
			RND_DAD_COORD(ctx->dlg);
				ctx->wedit[2] = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, -RND_MAX_COORD, RND_MAX_COORD);
				RND_DAD_ENTER_CB(ctx->dlg, prop_data_auto_cb);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "abs");
				RND_DAD_BOOL(ctx->dlg);
					ctx->wabs[2] = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, abshelp);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: angle");
			RND_DAD_REAL(ctx->dlg);
				ctx->wedit[3] = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, -360.0, +360.0);
				RND_DAD_ENTER_CB(ctx->dlg, prop_data_auto_cb);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "abs");
				RND_DAD_BOOL(ctx->dlg);
					ctx->wabs[3] = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, abshelp);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: double");
			RND_DAD_REAL(ctx->dlg);
				ctx->wedit[4] = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_MINMAX(ctx->dlg, -1000, +1000);
				RND_DAD_ENTER_CB(ctx->dlg, prop_data_auto_cb);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "abs");
				RND_DAD_BOOL(ctx->dlg);
					ctx->wabs[4] = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, abshelp);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);


		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: int");
			RND_DAD_INTEGER(ctx->dlg);
				ctx->wedit[5] = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_ENTER_CB(ctx->dlg, prop_data_auto_cb);
			RND_DAD_MINMAX(ctx->dlg, -(1<<30), 1<<30);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "abs");
				RND_DAD_BOOL(ctx->dlg);
					ctx->wabs[5] = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, abshelp);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: boolean");
			RND_DAD_BOOL(ctx->dlg);
				ctx->wedit[6] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				ctx->wabs[6] = 0;
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: color");
			RND_DAD_COLOR(ctx->dlg);
				ctx->wedit[7] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_BEGIN_HBOX(ctx->dlg);
				ctx->wabs[7] = 0;
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON(ctx->dlg, "apply");
					RND_DAD_CHANGE_CB(ctx->dlg, prop_data_force_cb);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);

	RND_DAD_END(ctx->dlg);
}

static void pcb_dlg_propdlg(propdlg_t *ctx)
{
	const char *hdr[] = {"property", "common", "min", "max", "avg", NULL};
	const char *hdr_val[] = {"use", "values"};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static rnd_box_t prvbb = {0, 0, RND_MM_TO_COORD(10), RND_MM_TO_COORD(10)};
	int n;
	rnd_hid_attr_val_t hv;

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_HPANE(ctx->dlg);

			/* left: property tree and filter */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(ctx->dlg, 5, 1, hdr);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					ctx->wtree = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, prop_select_node_cb);
					RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_STRING(ctx->dlg);
						RND_DAD_HELP(ctx->dlg, "Filter text:\nlist properties with\nmatching name only");
						RND_DAD_CHANGE_CB(ctx->dlg, prop_filter_cb);
						ctx->wfilter = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "add");
						RND_DAD_CHANGE_CB(ctx->dlg, prop_add_cb);
						RND_DAD_HELP(ctx->dlg, "Create a new attribute\n(in the a/ subtree)");
					RND_DAD_BUTTON(ctx->dlg, "del");
						RND_DAD_CHANGE_CB(ctx->dlg, prop_del_cb);
						RND_DAD_HELP(ctx->dlg, "Remove the selected attribute\n(from the a/ subtree)");
					RND_DAD_BUTTON(ctx->dlg, "rfr");
						RND_DAD_CHANGE_CB(ctx->dlg, prop_refresh_cb);
						RND_DAD_HELP(ctx->dlg, "Refresh: rebuild the tree\nupdating all values from the board");
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);

			/* right: preview and per type edit */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					RND_DAD_PREVIEW(ctx->dlg, prop_prv_expose_cb, prop_prv_mouse_cb, NULL, &prvbb, 100, 100, ctx);
				RND_DAD_END(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "<scope>");
					ctx->wscope = RND_DAD_CURRENT(ctx->dlg);
					RND_DAD_HELP(ctx->dlg, "Scope: list of objects affected");
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					RND_DAD_TREE(ctx->dlg, 2, 0, hdr_val);
						ctx->wvals = RND_DAD_CURRENT(ctx->dlg);
						RND_DAD_CHANGE_CB(ctx->dlg, prop_preset_cb);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BEGIN_VBOX(ctx->dlg);
					build_propval(ctx);
				RND_DAD_END(ctx->dlg);
				RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	RND_DAD_NEW("propedit", ctx->dlg, "Property editor", ctx, rnd_false, propdlgclose_cb);

	prop_refresh(ctx);
	gdl_append(&propdlgs, ctx, link);

	/* default all abs */
	hv.lng = 1;
	for(n = 0; n < PCB_PROPT_max; n++)
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wabs[n], &hv);
}

extern int prop_scope_add(pcb_propedit_t *pe, const char *cmd, int quiet);

const char pcb_acts_propedit[] = "propedit(object[:id]|layer[:id]|layergrp[:id]|pcb|subc|selection|selected)\n";
const char pcb_acth_propedit[] = "";
fgw_error_t pcb_act_propedit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	propdlg_t *ctx = calloc(sizeof(propdlg_t), 1);
	int a, r;

	pcb_props_init(&ctx->pe, PCB);

	if (argc > 1) {
		for(a = 1; a < argc; a++) {
			const char *cmd;
			RND_ACT_CONVARG(a, FGW_STR, propedit, cmd = argv[a].val.str);
			r = prop_scope_add(&ctx->pe, cmd, 0);
			if (r != 0)
				return r;
		}
	}
	else
		ctx->pe.selection = 1;

	pcb_dlg_propdlg(ctx);
	return 0;
}

static void propdlg_unit_change(rnd_conf_native_t *cfg, int arr_idx)
{
	propdlg_t *ctx;
	gdl_iterator_t it;

	gdl_foreach(&propdlgs, &it, ctx) {
		prop_pcb2dlg(ctx);
	}
}

static rnd_conf_hid_id_t propdlg_conf_id;
static const char *propdlg_cookie = "propdlg";
void pcb_propdlg_init(void)
{
	static rnd_conf_hid_callbacks_t cbs;
	rnd_conf_native_t *n = rnd_conf_get_field("editor/grid_unit");
	propdlg_conf_id = rnd_conf_hid_reg(propdlg_cookie, NULL);

	if (n != NULL) {
		cbs.val_change_post = propdlg_unit_change;
		rnd_conf_hid_set_cb(n, propdlg_conf_id, &cbs);
	}
}

void pcb_propdlg_uninit(void)
{
	rnd_conf_hid_unreg(propdlg_cookie);
}
