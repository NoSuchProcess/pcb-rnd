/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2021 Tibor 'Igor2' Palinkas
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

#include <librnd/hid/hid_dad_tree.h>
#include "route_style.h"
#include "data.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	int curr;
	int wname, wlineth, wclr, wtxtscale, wtxtth, wfont, wproto, wprotoname, wattr;

	rnd_hidval_t name_timer;

	char name[PCB_RST_NAME_LEN];
	unsigned name_pending:1;
} rstdlg_ctx_t;

rstdlg_ctx_t rstdlg_ctx;


int rstdlg_lookup_route_style_pen_bestfit(pcb_board_t *pcb)
{
	if (rstdlg_ctx.active)
		return rstdlg_ctx.curr;
	return pcb_lookup_route_style_pen_bestfit(PCB);
}


static void rstdlg_pcb2dlg(int rst_idx)
{
	int n;
	rnd_hid_attr_val_t hv;
	pcb_route_style_t *rst;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	pcb_attribute_t *a;
	char tmp[128];

	if (!rstdlg_ctx.active)
		return;

	attr = &rstdlg_ctx.dlg[rstdlg_ctx.wattr];
	tree = attr->wdata;

	if (rst_idx < 0)
		rst_idx = rstdlg_ctx.curr;

	if ((rst_idx < 0) || (rst_idx >= vtroutestyle_len(&PCB->RouteStyle))) {
		hv.str = "<invalid>";
		rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wname, &hv);
		return;
	}


	rst = vtroutestyle_get(&PCB->RouteStyle, rst_idx, 0);

	hv.str = rst->name;
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wname, &hv);

	hv.crd = rst->Thick;
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wlineth, &hv);

	hv.crd = rst->textt;
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wtxtth, &hv);

	if (rst->fid != -1) {
		sprintf(tmp, "#%ld", (long int)rst->fid);
		hv.str = tmp;
	}
	else
		hv.str = "<unset>";
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wfont, &hv);

	hv.lng = rst->texts;
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wtxtscale, &hv);

	hv.crd = rst->Clearance;
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wclr, &hv);

	if (rst->via_proto_set) {
		sprintf(tmp, "#%ld", (long int)rst->via_proto);
		hv.str = tmp;
	}
	else
		hv.str = "<unset>";
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wproto, &hv);

	if (rst->via_proto_set) {
		hv.str = "<invalid>";
		if ((rst->via_proto >= 0) && (rst->via_proto < PCB->Data->ps_protos.used)) {
			pcb_pstk_proto_t *proto = &PCB->Data->ps_protos.array[rst->via_proto];
			if (proto->in_use) {
				if ((proto->name != NULL) && (*proto->name != '\0'))
					hv.str = proto->name;
				else
					hv.str = "<unnamed>";
			}
			else
				hv.str = "<deleted>";
		}
	}
	else
		hv.str = "";
	rnd_gui->attr_dlg_set_value(rstdlg_ctx.dlg_hid_ctx, rstdlg_ctx.wprotoname, &hv);


	rnd_dad_tree_clear(tree);

	for(a = rst->attr.List, n = 0; n < rst->attr.Number; a++,n++) {
		char *cell[3]= {NULL};
		cell[0] = rnd_strdup(a->name);
		cell[1] = rnd_strdup(a->value);
		rnd_dad_tree_append(attr, NULL, cell);
	}

	rstdlg_ctx.curr = rst_idx;
}

static void rst_updated(pcb_route_style_t *rst)
{
	if (rst != NULL)
		pcb_use_route_style_(rst);
	rnd_event(&PCB->hidlib, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	pcb_board_set_changed_flag(PCB, 1);
}

/* if the change is silent, do not use the new route style */
static void name_chg_cb_silent(int silent)
{
	if (rstdlg_ctx.name_pending) {
		pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
		pcb_route_style_change_name(PCB, rstdlg_ctx.curr, rstdlg_ctx.name, 1);
		rstdlg_ctx.name_pending = 0;
		rst_updated(silent ? NULL : rst);
	}
}

/* delayed name change */
static void name_chg_cb(rnd_hidval_t data)
{
	name_chg_cb_silent(0);
}

/* call this if idx changes */
static void idx_changed(void)
{
	if (rstdlg_ctx.name_pending) { /* make sure pending name update is executed on the old style */
		if (rnd_gui->stop_timer != NULL)
			rnd_gui->stop_timer(rnd_gui, rstdlg_ctx.name_timer);
		name_chg_cb_silent(1);
	}
}

static void rst_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int idx = attr - rstdlg_ctx.dlg;
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	rnd_hid_attr_val_t hv;

	if (rst == NULL) {
		rnd_message(RND_MSG_ERROR, "route style does not exist");
		return;
	}

	if (idx == rstdlg_ctx.wname) {
		const char *s = attr->val.str;
		while(isspace(*s)) s++;
		strncpy(rstdlg_ctx.name, s, sizeof(rst->name));

		/* If we already have a timer going, then cancel it out */
		if (rstdlg_ctx.name_pending && (rnd_gui->stop_timer != NULL))
			rnd_gui->stop_timer(rnd_gui, rstdlg_ctx.name_timer);

		/* Start up a new timer */
		rstdlg_ctx.name_pending = 1;
		if (rnd_gui->add_timer != NULL)
			rstdlg_ctx.name_timer = rnd_gui->add_timer(rnd_gui, name_chg_cb, 1000 * 2, rstdlg_ctx.name_timer);
		else
			name_chg_cb(rstdlg_ctx.name_timer);
	}
	else if (idx == rstdlg_ctx.wlineth)
		pcb_route_style_change(PCB, rstdlg_ctx.curr, &attr->val.crd, NULL, NULL, NULL, NULL, NULL, 1);
	else if (idx == rstdlg_ctx.wtxtth)
		pcb_route_style_change(PCB, rstdlg_ctx.curr, NULL, &attr->val.crd, NULL, NULL, NULL, NULL, 1);
	else if (idx == rstdlg_ctx.wtxtscale) {
		int tmp = attr->val.lng;
		pcb_route_style_change(PCB, rstdlg_ctx.curr, NULL, NULL, &tmp, NULL, NULL, NULL, 1);
	}
	else if (idx == rstdlg_ctx.wfont) {

		fgw_error_t err;
		fgw_arg_t res, args[2];
		
		args[1].type = FGW_STR;
		args[1].val.cstr = "fontid";
		err = rnd_actionv_bin(&PCB->hidlib, "fontsel", &res, 2, args);
		if ((err == 0) && (res.type == FGW_LONG)) {
			rnd_font_id_t tmp = res.val.nat_long;
			pcb_route_style_change(PCB, rstdlg_ctx.curr, NULL, NULL, NULL, &tmp, NULL, NULL, 1);
		}
	}
	else if (idx == rstdlg_ctx.wclr)
		pcb_route_style_change(PCB, rstdlg_ctx.curr, NULL, NULL, NULL, NULL, &attr->val.crd, NULL, 1);
	else if (idx == rstdlg_ctx.wproto) {
		fgw_error_t err;
		fgw_arg_t res, args[4];
		
		args[1].type = FGW_STR;
		args[1].val.cstr = "board";
		args[2].type = FGW_STR;
		args[2].val.cstr = "retpid";
		args[3].type = FGW_LONG;
		args[3].val.nat_long = rst->via_proto;
		err = rnd_actionv_bin(&PCB->hidlib, "pstklib", &res, 4, args);
		if ((err == 0) && (res.type == FGW_LONG)) {
			rnd_cardinal_t tmp = res.val.nat_long;
			pcb_route_style_change(PCB, rstdlg_ctx.curr, NULL, NULL, NULL, NULL, NULL, &tmp, 1);
		}
	}
	else {
		rnd_message(RND_MSG_ERROR, "Internal error: route style field does not exist");
		return;
	}

	/* something got modified - re-select the current route style so "pen" is updated */
	pcb_use_route_style_idx(PCB, rstdlg_ctx.curr);
}

static int rst_edit_attr(char **key, char **val)
{
	int wkey, wval, res;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABLE(dlg, 2);
			RND_DAD_LABEL(dlg, "key");
			RND_DAD_STRING(dlg);
				wkey = RND_DAD_CURRENT(dlg);
				if (*key != NULL)
					RND_DAD_DEFAULT_PTR(dlg, rnd_strdup(*key));
			RND_DAD_LABEL(dlg, "value");
			RND_DAD_STRING(dlg);
				wval = RND_DAD_CURRENT(dlg);
				if (*val != NULL)
					RND_DAD_DEFAULT_PTR(dlg, rnd_strdup(*val));
		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("route_style_attr", dlg, "Edit route style attribute", NULL, rnd_true, NULL);
	res = RND_DAD_RUN(dlg);
	if ((res == 0) && (dlg[wkey].val.str != NULL) && (dlg[wval].val.str != NULL)) {
		*key = rnd_strdup(dlg[wkey].val.str);
		*val = rnd_strdup(dlg[wval].val.str);
	}
	RND_DAD_FREE(dlg);
	return res;
}

static void rst_add_attr_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	char *key = NULL, *val = NULL;

	if (rst_edit_attr(&key, &val) == 0) {
		pcb_attribute_put(&rst->attr, key, val);
		rst_updated(rst);
	}
}

static void rst_edit_attr_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	rnd_hid_attribute_t *treea = &rstdlg_ctx.dlg[rstdlg_ctx.wattr];
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(treea);
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

static void rst_del_attr_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_route_style_t *rst = vtroutestyle_get(&PCB->RouteStyle, rstdlg_ctx.curr, 0);
	rnd_hid_attribute_t *treea = &rstdlg_ctx.dlg[rstdlg_ctx.wattr];
	rnd_hid_row_t *row = rnd_dad_tree_get_selected(treea);

	if (row == NULL)
		return;

	pcb_attribute_remove(&rst->attr, row->cell[0]);
	rst_updated(rst);
}

static void rstdlg_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	rstdlg_ctx_t *ctx = caller_data;

	idx_changed(); /* flush pending timed changes */

	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(rstdlg_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static int pcb_dlg_rstdlg(int rst_idx)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static const char *attr_hdr[] = {"attribute key", "attribute value", NULL};

	if (rstdlg_ctx.active) { /* do not open another, just refresh data to target */
		idx_changed();
		rstdlg_pcb2dlg(rst_idx);
		return 0;
	}

	RND_DAD_BEGIN_VBOX(rstdlg_ctx.dlg);
		RND_DAD_COMPFLAG(rstdlg_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_BEGIN_TABLE(rstdlg_ctx.dlg, 2);

			RND_DAD_LABEL(rstdlg_ctx.dlg, "Name:");
			RND_DAD_STRING(rstdlg_ctx.dlg);
				rstdlg_ctx.wname = RND_DAD_CURRENT(rstdlg_ctx.dlg);
				RND_DAD_HELP(rstdlg_ctx.dlg, "Name of the routing style");
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			RND_DAD_LABEL(rstdlg_ctx.dlg, "Line thick.:");
			RND_DAD_COORD(rstdlg_ctx.dlg);
				rstdlg_ctx.wlineth = RND_DAD_CURRENT(rstdlg_ctx.dlg);
				RND_DAD_HELP(rstdlg_ctx.dlg, "Thickness of line/arc objects");
				RND_DAD_MINMAX(rstdlg_ctx.dlg, 1, RND_MAX_COORD);
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			RND_DAD_LABEL(rstdlg_ctx.dlg, "Text scale:");
			RND_DAD_INTEGER(rstdlg_ctx.dlg);
				rstdlg_ctx.wtxtscale = RND_DAD_CURRENT(rstdlg_ctx.dlg);
				RND_DAD_HELP(rstdlg_ctx.dlg, "Text size scale in %; 100 means normal size");
				RND_DAD_MINMAX(rstdlg_ctx.dlg, 1, 100000);
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			RND_DAD_LABEL(rstdlg_ctx.dlg, "Clearance:");
			RND_DAD_COORD(rstdlg_ctx.dlg);
				rstdlg_ctx.wclr = RND_DAD_CURRENT(rstdlg_ctx.dlg);
				RND_DAD_HELP(rstdlg_ctx.dlg, "Object clearance: any object placed with this style\nwill clear this much from sorrunding clearing-enabled polygons\n(unless the object is joined to the polygon)");
				RND_DAD_MINMAX(rstdlg_ctx.dlg, 1, RND_MAX_COORD);
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			RND_DAD_LABEL(rstdlg_ctx.dlg, "Text thick.:");
			RND_DAD_COORD(rstdlg_ctx.dlg);
				rstdlg_ctx.wtxtth = RND_DAD_CURRENT(rstdlg_ctx.dlg);
				RND_DAD_HELP(rstdlg_ctx.dlg, "Text stroke thickness;\nif 0 use the default heuristics that\ncalculates it from text scale");
				RND_DAD_MINMAX(rstdlg_ctx.dlg, 1, RND_MAX_COORD);
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);

			RND_DAD_LABEL(rstdlg_ctx.dlg, "Text font:");
			RND_DAD_BUTTON(rstdlg_ctx.dlg, "<fid>");
				rstdlg_ctx.wfont = RND_DAD_CURRENT(rstdlg_ctx.dlg);
				RND_DAD_HELP(rstdlg_ctx.dlg, "Text font ID (button invokes the font selector)");
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);


			RND_DAD_LABEL(rstdlg_ctx.dlg, "Via padstack:");
			RND_DAD_BEGIN_HBOX(rstdlg_ctx.dlg);
				RND_DAD_BUTTON(rstdlg_ctx.dlg, "<pid>");
					rstdlg_ctx.wproto = RND_DAD_CURRENT(rstdlg_ctx.dlg);
					RND_DAD_HELP(rstdlg_ctx.dlg, "Padstack prototype ID used for new vias with this style\n(button invokes the board padstack proto list dialog)");
					RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_change_cb);
				RND_DAD_LABEL(rstdlg_ctx.dlg, "<name>");
					rstdlg_ctx.wprotoname = RND_DAD_CURRENT(rstdlg_ctx.dlg);
			RND_DAD_END(rstdlg_ctx.dlg);

		RND_DAD_END(rstdlg_ctx.dlg);
		RND_DAD_TREE(rstdlg_ctx.dlg, 2, 0, attr_hdr);
			RND_DAD_HELP(rstdlg_ctx.dlg, "These attributes are automatically added to\nany object drawn with this routing style");
			rstdlg_ctx.wattr = RND_DAD_CURRENT(rstdlg_ctx.dlg);
		RND_DAD_BEGIN_HBOX(rstdlg_ctx.dlg);
			RND_DAD_BUTTON(rstdlg_ctx.dlg, "add");
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_add_attr_cb);
			RND_DAD_BUTTON(rstdlg_ctx.dlg, "edit");
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_edit_attr_cb);
			RND_DAD_BUTTON(rstdlg_ctx.dlg, "del");
				RND_DAD_CHANGE_CB(rstdlg_ctx.dlg, rst_del_attr_cb);
		RND_DAD_END(rstdlg_ctx.dlg);
		RND_DAD_BUTTON_CLOSES(rstdlg_ctx.dlg, clbtn);
	RND_DAD_END(rstdlg_ctx.dlg);

	/* set up the context */
	rstdlg_ctx.active = 1;

	RND_DAD_NEW("route_style", rstdlg_ctx.dlg, "Edit route style", &rstdlg_ctx, rnd_false, rstdlg_close_cb);

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
		RND_ACT_FAIL(AdjustStyle);

	RND_ACT_MAY_CONVARG(1, FGW_LONG, AdjustStyle, idx = argv[1].val.nat_long);

	if (idx >= (long)vtroutestyle_len(&PCB->RouteStyle)) {
		rnd_message(RND_MSG_ERROR, "Invalid route style %ld index; max value: %ld\n", idx, vtroutestyle_len(&PCB->RouteStyle)-1);
		RND_ACT_IRES(-1);
		return 0;
	}

	if (idx < 0) {
		idx = PCB_LOOKUP_ROUTE_STYLE_PEN(PCB);
		if (idx < 0) {
			rnd_message(RND_MSG_ERROR, "No style selected\n");
			RND_ACT_IRES(-1);
		}
	}

	RND_ACT_IRES(pcb_dlg_rstdlg(idx));
	return 0;
}
