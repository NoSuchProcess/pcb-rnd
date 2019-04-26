/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

/* included from routest.c - split for clarity */

#include "hid_dad_tree.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int curr;
	int wname, wlineth, wclr, wtxtscale, wtxtth, wviahole, wviaring, wattr;
} rstdlg_ctx_t;

rstdlg_ctx_t rstdlg_ctx;

static void rstdlg_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	rstdlg_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(rstdlg_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void rstdlg_pcb2dlg(int rst_idx)
{
	int n;
	pcb_hid_attr_val_t hv;
	pcb_route_style_t *rst;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_attribute_t *a;

	if (!rstdlg_ctx.active)
		return;

	attr = &rstdlg_ctx.dlg[rstdlg_ctx.wattr];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	if (rst_idx < 0)
		rst_idx = rstdlg_ctx.curr;

	rst = vtroutestyle_get(&PCB->RouteStyle, rst_idx, 0);

	hv.str_value = rst->name;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wname, &hv);

	hv.coord_value = rst->Thick;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wlineth, &hv);

	hv.coord_value = rst->textt;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wtxtth, &hv);

	hv.coord_value = rst->texts;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wtxtscale, &hv);

	hv.coord_value = rst->Clearance;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wclr, &hv);

	hv.coord_value = rst->Hole;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wviahole, &hv);

	hv.coord_value = rst->Diameter;
	pcb_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wviaring, &hv);

	pcb_dad_tree_clear(tree);

	for(a = rst->attr.List, n = 0; n < rst->attr.Number; a++,n++) {
		char *cell[3]= {NULL};
		cell[0] = pcb_strdup(a->name);
		cell[1] = pcb_strdup(a->value);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	rstdlg_ctx.curr = rst_idx;
}

static void rst_updated(pcb_route_style_t *rst)
{
	if (rst != NULL)
		pcb_use_route_style(rst);
	pcb_event(PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	pcb_board_set_changed_flag(1);
}

static void rst_change_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int idx = attr - rstdlg_ctx.dlg;
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);

	if (rst == NULL) {
		pcb_message(PCB_MSG_ERROR, "route style does not exist");
		return;
	}

TODO("This change is not undoable");

	if (idx == rstdlg_ctx.wname) {
		const char *s = attr->default_val.str_value;
		while(isspace(*s)) s++;
		strncpy(rst->name, s, sizeof(rst->name));
	}
	else if (idx == rstdlg_ctx.wlineth)
		rst->Thick = attr->default_val.coord_value;
	else if (idx == rstdlg_ctx.wtxtth)
		rst->textt = attr->default_val.coord_value;
	else if (idx == rstdlg_ctx.wtxtscale)
		rst->texts = attr->default_val.coord_value;
	else if (idx == rstdlg_ctx.wclr)
		rst->Clearance = attr->default_val.coord_value;
	else if (idx == rstdlg_ctx.wviahole)
		rst->Hole = attr->default_val.coord_value;
	else if (idx == rstdlg_ctx.wviaring)
		rst->Diameter = attr->default_val.coord_value;
	else {
		pcb_message(PCB_MSG_ERROR, "Internal error: route style field does not exist");
		return;
	}

	rst_updated(rst);
}

static int rst_edit_attr(char **key, char **val)
{
	int wkey, wval, res;
	pcb_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	PCB_DAD_DECL(dlg);

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_COMPFLAG(dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(dlg, 2);
			PCB_DAD_LABEL(dlg, "key");
			PCB_DAD_STRING(dlg);
				wkey = PCB_DAD_CURRENT(dlg);
				if (*key != NULL)
					PCB_DAD_DEFAULT_PTR(dlg, pcb_strdup(*key));
			PCB_DAD_LABEL(dlg, "value");
			PCB_DAD_STRING(dlg);
				wval = PCB_DAD_CURRENT(dlg);
				if (*val != NULL)
					PCB_DAD_DEFAULT_PTR(dlg, pcb_strdup(*val));
		PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW("route_style_attr", dlg, "Edit route style attribute", NULL, pcb_true, NULL);
	res = PCB_DAD_RUN(dlg);
	if (res == 0) {
		*key = pcb_strdup(dlg[wkey].default_val.str_value);
		*val = pcb_strdup(dlg[wval].default_val.str_value);
	}
	PCB_DAD_FREE(dlg);
	return res;
}

static void rst_add_attr_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	char *key = NULL, *val = NULL;

	if (rst_edit_attr(&key, &val) == 0) {
		pcb_attribute_put(&rst->attr, key, val);
		rst_updated(rst);
	}
}

static void rst_edit_attr_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	pcb_hid_attribute_t *treea = &rstdlg_ctx.dlg[rstdlg_ctx.wattr];
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(treea);
	char *key, *val;

	if (row == NULL)
		return;

	key = row->cell[0];
	val = row->cell[1];

	if (rst_edit_attr(&key, &val) == 0) {
		pcb_attribute_remove(&rst->attr, row->cell[0]);
		pcb_attribute_put(&rst->attr, key, val);
		rst_updated(rst);
	}
}

static void rst_del_attr_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	pcb_hid_attribute_t *treea = &rstdlg_ctx.dlg[rstdlg_ctx.wattr];
	pcb_hid_row_t *row = pcb_dad_tree_get_selected(treea);

	if (row == NULL)
		return;

	pcb_attribute_remove(&rst->attr, row->cell[0]);
	rst_updated(rst);
}


static int pcb_dlg_rstdlg(int rst_idx)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static const char *attr_hdr[] = {"attribute key", "attribute value", NULL};

	if (rstdlg_ctx.active) { /* do not open another, just refresh data to target */
		rstdlg_pcb2dlg(rst_idx);
		return 0;
	}

	PCB_DAD_BEGIN_VBOX(rstdlg_ctx.dlg);
		PCB_DAD_COMPFLAG(rstdlg_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(rstdlg_ctx.dlg, 2);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Name:");
			PCB_DAD_STRING(rstdlg_ctx.dlg);
				rstdlg_ctx.wname = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Name of the routing style");
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Line thick.:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wlineth = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Thickness of line/arc objects");
				PCB_DAD_MINMAX(rstdlg_ctx.dlg, 1, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Text scale:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wtxtscale = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Text size scale in %; 100 means normal size");
				PCB_DAD_MINMAX(rstdlg_ctx.dlg, 1, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Clearance:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wclr = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Object clearance: any object placed with this style\nwill clear this much from sorrunding clearing-enabled polygons\n(unless the object is joined to the polygon)");
				PCB_DAD_MINMAX(rstdlg_ctx.dlg, 1, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Text thick.:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wtxtth = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Text stroke thickness;\nif 0 use the default heuristics that\ncalculates it from text scale");
				PCB_DAD_MINMAX(rstdlg_ctx.dlg, 1, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "*Via hole:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wviahole = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Via hole diameter\nwarning: will be replaced with the padstack selector");
				PCB_DAD_MINMAX(rstdlg_ctx.dlg, 1, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			PCB_DAD_LABEL(rstdlg_ctx.dlg, "*Via ring:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wviaring = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Via ring diameter\nwarning: will be replaced with the padstack selector");
				PCB_DAD_MINMAX(rstdlg_ctx.dlg, 1, PCB_MAX_COORD);
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

		PCB_DAD_END(rstdlg_ctx.dlg);
		PCB_DAD_TREE(rstdlg_ctx.dlg, 2, 0, attr_hdr);
			PCB_DAD_HELP(rstdlg_ctx.dlg, "These attributes are automatically added to\nany object drawn with this routing style");
			rstdlg_ctx.wattr = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
		PCB_DAD_BEGIN_HBOX(rstdlg_ctx.dlg);
			PCB_DAD_BUTTON(rstdlg_ctx.dlg, "add");
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_add_attr_cb);
			PCB_DAD_BUTTON(rstdlg_ctx.dlg, "edit");
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_edit_attr_cb);
			PCB_DAD_BUTTON(rstdlg_ctx.dlg, "del");
				PCB_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_del_attr_cb);
		PCB_DAD_END(rstdlg_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(rstdlg_ctx.dlg, clbtn);
	PCB_DAD_END(rstdlg_ctx.dlg);

	/* set up the context */
	rstdlg_ctx.active = 1;

	PCB_DAD_NEW("route_style", rstdlg_ctx.dlg, "Edit route style", &rstdlg_ctx, pcb_false, rstdlg_close_cb);

	rstdlg_pcb2dlg(rst_idx);
	return 0;
}


const char pcb_acts_AdjustStyle[] = "AdjustStyle([routestyle_idx])\n";
const char pcb_acth_AdjustStyle[] = "Open the dialog box for editing the route styles.";
/* DOC: adjuststyle.html */
fgw_error_t pcb_act_AdjustStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long idx = -1;

	if (argc > 2)
		PCB_ACT_FAIL(AdjustStyle);

	PCB_ACT_MAY_CONVARG(1, FGW_LONG, AdjustStyle, idx = argv[1].val.nat_long);

	if (idx >= (long)vtroutestyle_len(&PCB->RouteStyle)) {
		pcb_message(PCB_MSG_ERROR, "Invalid route style %ld index; max value: %ld\n", idx, vtroutestyle_len(&PCB->RouteStyle)-1);
		PCB_ACT_IRES(-1);
		return 0;
	}

	if (idx < 0) {
		idx = pcb_route_style_lookup(&PCB->RouteStyle, conf_core.design.line_thickness, conf_core.design.via_thickness, conf_core.design.via_drilling_hole, conf_core.design.clearance, NULL);
		if (idx < 0) {
			pcb_message(PCB_MSG_ERROR, "No style selected\n");
			PCB_ACT_IRES(-1);
		}
	}

	PCB_ACT_IRES(pcb_dlg_rstdlg(idx));
	return 0;
}
