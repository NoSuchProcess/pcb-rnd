/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include "event.h"
#include "netlist.h"
#include <genvector/vtp0.h>
#include <librnd/core/hidlib_conf.h>

const char *dlg_netlist_cookie = "netlist dialog";

/* timeout for handling two clicks as a double click */
#define DBL_CLICK_TIME 0.7

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	rnd_rnd_box_t bb_prv;
	int wnetlist, wprev, wtermlist;
	int wsel, wunsel, wfind, wunfind, wrats, wnorats, wallrats, wnoallrats, wripup, waddrats, wrename, wmerge, wattr, wnlcalc;
	void *last_selected_row;
	double last_selected_time;
	int active; /* already open - allow only one instance */
} netlist_ctx_t;

netlist_ctx_t netlist_ctx;

static void netlist_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	netlist_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(netlist_ctx_t));
	rnd_event(&PCB->hidlib, RND_EVENT_GUI_LEAD_USER, "cci", 0, 0, 0);
}

/* returns allocated net name for the currently selected net */
static char *netlist_data2dlg_netlist(netlist_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[6], *cursor_path = NULL;
	pcb_net_t **n, **nets;

	attr = &ctx->dlg[ctx->wnetlist];
	tree = attr->wdata;

	/* remember cursor */
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = rnd_strdup(r->cell[0]);

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	nets = pcb_netlist_sort(&ctx->pcb->netlist[1]);
	if (nets != NULL) {
		cell[4] = NULL;
		for(n = nets; *n != NULL; n++) {
			cell[0] = rnd_strdup((*n)->name);
			cell[1] = rnd_strdup((*n)->inhibit_rats ? "*" : "");
			cell[2] = rnd_strdup((*n)->auto_len ? "*" : "");
			cell[3] = rnd_strdup("");
			rnd_dad_tree_append(attr, NULL, cell);
		}
		free(nets);

		/* restore cursor */
		if (cursor_path != NULL) {
			rnd_hid_attr_val_t hv;
			hv.str = cursor_path;
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnetlist, &hv);
		}
	}
	return cursor_path;
}

static void netlist_data2dlg_connlist(netlist_ctx_t *ctx, pcb_net_t *net)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	rnd_hid_row_t *r;
	char *cell[2], *cursor_path = NULL;
	pcb_net_term_t *t;

	attr = &ctx->dlg[ctx->wtermlist];
	tree = attr->wdata;

	/* remember cursor */
	if (net != NULL) {
		r = rnd_dad_tree_get_selected(attr);
		if (r != NULL)
			cursor_path = rnd_strdup(r->cell[0]);
	}

	/* remove existing items */
	rnd_dad_tree_clear(tree);

	if (net == NULL)
		return;

	cell[1] = NULL;
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
		cell[0] = rnd_concat(t->refdes, "-", t->term, NULL);
		rnd_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = cursor_path;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtermlist, &hv);
		free(cursor_path);
	}
}


static void netlist_data2dlg(netlist_ctx_t *ctx)
{
	pcb_net_t *curnet = NULL;
	char *curnetname = netlist_data2dlg_netlist(ctx);

	if (curnetname != NULL)
		curnet = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_EDITED], curnetname, 0);
	free(curnetname);
	netlist_data2dlg_connlist(ctx, curnet);
}

static void netlist_force_redraw(netlist_ctx_t *ctx)
{
	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wprev], &ctx->bb_prv);
}


static void netlist_row_selected(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	netlist_ctx_t *ctx= tree->user_ctx;
	const char *netname = NULL;
	pcb_net_t *net = NULL;

	if (row != NULL) {
		netname = row->cell[0];
		if ((row == ctx->last_selected_row) && ((rnd_dtime() - ctx->last_selected_time) < DBL_CLICK_TIME)) {
			int disabled = (row->cell[1][0] == '*');
			if (netname != NULL)
				rnd_actionva(&ctx->pcb->hidlib, "netlist", disabled ? "rats" : "norats", netname, NULL);
			ctx->last_selected_row = NULL;
			return;
		}
	}
	if (netname != NULL)
		net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_EDITED], netname, 0);
	netlist_data2dlg_connlist(ctx, net);
	rnd_event(&PCB->hidlib, RND_EVENT_GUI_LEAD_USER, "cci", 0, 0, 0);
	netlist_force_redraw(ctx);

	ctx->last_selected_row = row;
	ctx->last_selected_time = rnd_dtime();
}

static void termlist_row_selected(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	netlist_ctx_t *ctx= tree->user_ctx;
	char *refdes, *term;
	pcb_any_obj_t *obj;

	rnd_event(&PCB->hidlib, RND_EVENT_GUI_LEAD_USER, "cci", 0, 0, 0);
	if (row == NULL)
		return;
	refdes = rnd_strdup(row->cell[0]);
	term = strchr(refdes, '-');
	if (term != NULL) {
		*term = '\0';
		term++;
		obj = pcb_term_find_name(ctx->pcb, ctx->pcb->Data, PCB_LYT_COPPER, refdes, term, NULL, NULL);
		if (obj != NULL) {
			rnd_coord_t x, y;
			pcb_obj_center(obj, &x, &y);
			rnd_event(&PCB->hidlib, RND_EVENT_GUI_LEAD_USER, "cci", x, y, 1);
		}
	}
	free(refdes);
}

static void netlist_button_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	netlist_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *atree;
	int w = attr - ctx->dlg;
	rnd_hid_row_t *r;
	const char *name;

	atree = &ctx->dlg[ctx->wnetlist];

	/* no selection required */
	if (w == ctx->wallrats) {
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "allrats", NULL);
		return;
	}
	else if (w == ctx->wnoallrats) {
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "noallrats", NULL);
		return;
	}

	r = rnd_dad_tree_get_selected(atree);
	if (r == NULL)
		return;
	name = rnd_strdup(r->cell[0]);

	if (w == ctx->wsel)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "select", name, NULL);
	else if (w == ctx->wunsel)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "unselect", name, NULL);
	else if (w == ctx->wfind) {
		rnd_actionva(&ctx->pcb->hidlib, "connection", "reset", NULL);
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "find", name, NULL);
	}
	else if (w == ctx->wunfind)
		rnd_actionva(&ctx->pcb->hidlib, "connection", "reset", NULL);
	else if (w == ctx->wrats)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "rats", name, NULL);
	else if (w == ctx->wnorats)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "norats", name, NULL);
	else if (w == ctx->wripup)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "ripup", name, NULL);
	else if (w == ctx->waddrats)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "AddRats", name, NULL);
	else if (w == ctx->wrename)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "rename", name, NULL);
	else if (w == ctx->wmerge)
		rnd_actionva(&ctx->pcb->hidlib, "netlist", "merge", name, NULL);
	else if (w == ctx->wattr) {
		char *tmp = rnd_concat("net:", name, NULL);
		rnd_actionva(&ctx->pcb->hidlib, "propedit", tmp, NULL);
		free(tmp);
	}
	else if (w == ctx->wnlcalc) {
		rnd_hid_row_t *row;
		rnd_hid_attribute_t *atree = &ctx->dlg[ctx->wnetlist];
		rnd_hid_tree_t *tree = atree->wdata;
		fgw_arg_t nlres;
		fgw_arg_t nlargv[2];
		nlargv[1].type = FGW_STR; nlargv[1].val.cstr = name;
		fgw_error_t err = rnd_actionv_bin(&ctx->pcb->hidlib, "QueryCalcNetLen", &nlres, 2, nlargv);
		if (err != 0) {
			rnd_message(RND_MSG_ERROR, "Internal error: failed to execute QueryCalcNetLen() - is the query plugin enabled?\n");
			return;
		}

		row = htsp_get(&tree->paths, name);

		if (nlres.type == FGW_COORD) {
			char tmp[128];
			rnd_snprintf(tmp, sizeof(tmp), "%$m*", rnd_conf.editor.grid_unit->suffix, fgw_coord(&nlres));
			rnd_dad_tree_modify_cell(atree, row, 3, tmp);
		}
		else if ((nlres.type & FGW_STR) == FGW_STR)
			rnd_dad_tree_modify_cell(atree, row, 3, nlres.val.str);
		else
			rnd_dad_tree_modify_cell(atree, row, 3, "invalid return");
	}
	else {
		rnd_message(RND_MSG_ERROR, "Internal error: netlist_button_cb() called from an invalid widget\n");
		return;
	}
	rnd_gui->invalidate_all(rnd_gui);
}

static void netlist_claim_obj_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	netlist_ctx_t *ctx = caller_data;
	rnd_actionva(&ctx->pcb->hidlib, "ClaimNet", "object", NULL);
}

static void netlist_claim_sel_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	netlist_ctx_t *ctx = caller_data;
	rnd_actionva(&ctx->pcb->hidlib, "ClaimNet", "selected", NULL);
}

static void netlist_claim_fnd_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	netlist_ctx_t *ctx = caller_data;
	rnd_actionva(&ctx->pcb->hidlib, "ClaimNet", "found", NULL);
}

static vtp0_t netlist_color_save;

static void netlist_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	netlist_ctx_t *ctx = prv->user_ctx;
	rnd_xform_t xform;
	size_t n;
	void **p;
	rnd_hid_attribute_t *attr;
	rnd_hid_row_t *r;
	pcb_net_t *net = NULL;

	attr = &ctx->dlg[ctx->wnetlist];
	r = rnd_dad_tree_get_selected(attr);
	if (r != NULL)
		net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_EDITED], r->cell[0], 0);

	if (net != NULL) { /* save term object colors */
		pcb_net_term_t *t;
		vtp0_truncate(&netlist_color_save, 0);
		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
			pcb_any_obj_t *obj = pcb_term_find_name(ctx->pcb, ctx->pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
			if (obj == NULL)
				continue;

			vtp0_append(&netlist_color_save, obj);
			if (obj->override_color != NULL)
				vtp0_append(&netlist_color_save, (char *)obj->override_color);
			else
				vtp0_append(&netlist_color_save, NULL);
			obj->override_color = rnd_color_magenta;
		}
	}

	/* draw the board */
	memset(&xform, 0, sizeof(xform));
	xform.layer_faded = 1;
	rnd_expose_main(rnd_gui, e, &xform);

	if (net != NULL) {/* restore object color */
		for(n = 0, p = netlist_color_save.array; n < netlist_color_save.used; n+=2,p+=2) {
			pcb_any_obj_t *obj = p[0];
			rnd_color_t *s = p[1];
			obj->override_color = s;
		}
		vtp0_truncate(&netlist_color_save, 0);
	}
}

static rnd_bool netlist_mouse(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	return rnd_false;
}

static void pcb_dlg_netlist(pcb_board_t *pcb)
{
	static const char *hdr[] = {"network", "no-rat", "auto-len", "network length", NULL};
	static const char *hdr2[] = {"terminals", NULL};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	int wvpan;

	if (netlist_ctx.active)
		return; /* do not open another */

	netlist_ctx.bb_prv.X1 = 0;
	netlist_ctx.bb_prv.Y1 = 0;
	netlist_ctx.bb_prv.X2 = pcb->hidlib.size_x;
	netlist_ctx.bb_prv.Y2 = pcb->hidlib.size_y;
	netlist_ctx.pcb = pcb;

	RND_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* layout */
		RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);

		RND_DAD_BEGIN_HPANE(netlist_ctx.dlg);
			RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);

			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* left */
				RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_TREE(netlist_ctx.dlg, 4, 0, hdr);
					RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
					netlist_ctx.wnetlist = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_TREE_SET_CB(netlist_ctx.dlg, selected_cb, netlist_row_selected);
					RND_DAD_TREE_SET_CB(netlist_ctx.dlg, ctx, &netlist_ctx);
			RND_DAD_END(netlist_ctx.dlg);

			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right */
				RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);
				RND_DAD_BEGIN_VPANE(netlist_ctx.dlg);
					RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);
					wvpan = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right-top */
						RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);
						RND_DAD_PREVIEW(netlist_ctx.dlg, netlist_expose, netlist_mouse, NULL, &netlist_ctx.bb_prv, 100, 100, &netlist_ctx);
							RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_PRV_BOARD);
							netlist_ctx.wprev = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_END(netlist_ctx.dlg);
					RND_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right-bottom */
						RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);
						RND_DAD_TREE(netlist_ctx.dlg, 1, 0, hdr2);
							RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
							netlist_ctx.wtermlist = RND_DAD_CURRENT(netlist_ctx.dlg);
							RND_DAD_TREE_SET_CB(netlist_ctx.dlg, selected_cb, termlist_row_selected);
							RND_DAD_TREE_SET_CB(netlist_ctx.dlg, ctx, &netlist_ctx);
					RND_DAD_END(netlist_ctx.dlg);
				RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_END(netlist_ctx.dlg);
		RND_DAD_END(netlist_ctx.dlg);

		RND_DAD_BEGIN_HBOX(netlist_ctx.dlg); /* bottom button row */
			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "select");
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "select all objects on the selected network");
					netlist_ctx.wsel = RND_DAD_CURRENT(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "unsel.");
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wunsel = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_HELP(netlist_ctx.dlg, "unselect all objects on the selected network");
			RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "find ");
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wfind = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_HELP(netlist_ctx.dlg, "find-mark (green) all object on the selected network");
				RND_DAD_BUTTON(netlist_ctx.dlg, "clear");
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wunfind = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_HELP(netlist_ctx.dlg, "remove find-mark (green) from all object on the selected network");
			RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "rat disable");
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wnorats = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_HELP(netlist_ctx.dlg, "disable adding rats for missing connection for the selected network");
				RND_DAD_BUTTON(netlist_ctx.dlg, "rat enable");
					netlist_ctx.wrats = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "re-enable adding rats for missing connection for the selected network");
			RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "all disable");
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wnoallrats = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_HELP(netlist_ctx.dlg, "disable adding rats for missing connection for all networks");
				RND_DAD_BUTTON(netlist_ctx.dlg, "all enable");
					netlist_ctx.wallrats = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "re-enable adding rats for missing connection for all networks");
			RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "add rats");
					netlist_ctx.waddrats = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "add rats for missing connections within the selected network");
				RND_DAD_BUTTON(netlist_ctx.dlg, "rip up  ");
					netlist_ctx.wripup = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "remove all objects (except for terminals) on the selected network");
			RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				RND_DAD_BUTTON(netlist_ctx.dlg, "rename");
					netlist_ctx.wrename = RND_DAD_CURRENT(netlist_ctx.dlg);
					RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "change the name of the selected network");
				RND_DAD_BEGIN_HBOX(netlist_ctx.dlg);
					RND_DAD_BUTTON(netlist_ctx.dlg, "merge");
						netlist_ctx.wmerge = RND_DAD_CURRENT(netlist_ctx.dlg);
						RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					RND_DAD_HELP(netlist_ctx.dlg, "merge the selected network into another network");
					RND_DAD_BUTTON(netlist_ctx.dlg, "attr");
						netlist_ctx.wattr = RND_DAD_CURRENT(netlist_ctx.dlg);
						RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
						RND_DAD_HELP(netlist_ctx.dlg, "change attributes of the selected network");
				RND_DAD_END(netlist_ctx.dlg);
			RND_DAD_END(netlist_ctx.dlg);
		RND_DAD_END(netlist_ctx.dlg);

		RND_DAD_BEGIN_HBOX(netlist_ctx.dlg); /* bottom button row */
			RND_DAD_LABEL(netlist_ctx.dlg, "Claim:");
				RND_DAD_HELP(netlist_ctx.dlg, "Map galvanic connection of existing objects and convert them into a network on the netlist");
			RND_DAD_BUTTON(netlist_ctx.dlg, "click");
				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_claim_obj_cb);
				RND_DAD_HELP(netlist_ctx.dlg, "Click on an object to create a new network of all connected objects from there");
			RND_DAD_BUTTON(netlist_ctx.dlg, "select");
				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_claim_sel_cb);
				RND_DAD_HELP(netlist_ctx.dlg, "create a new network of all selected objects");
			RND_DAD_BUTTON(netlist_ctx.dlg, "found");
				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_claim_fnd_cb);
				RND_DAD_HELP(netlist_ctx.dlg, "create a new network of all found (green) objects");

			RND_DAD_LABEL(netlist_ctx.dlg, "Len:");
			RND_DAD_BUTTON(netlist_ctx.dlg, "calc");
				netlist_ctx.wnlcalc = RND_DAD_CURRENT(netlist_ctx.dlg);
				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
				RND_DAD_HELP(netlist_ctx.dlg, "Calculate the length of the selected network immediately");
			RND_DAD_BUTTON(netlist_ctx.dlg, "on");
/*				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_claim_obj_cb);*/
				RND_DAD_HELP(netlist_ctx.dlg, "Turn on auto-recaculate net length on refresh for the selected network");
			RND_DAD_BUTTON(netlist_ctx.dlg, "off");
/*				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_claim_obj_cb);*/
				RND_DAD_HELP(netlist_ctx.dlg, "Turn off auto-recaculate net length on refresh for the selected network");
			RND_DAD_BUTTON(netlist_ctx.dlg, "refresh");
/*				RND_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_claim_obj_cb);*/
				RND_DAD_HELP(netlist_ctx.dlg, "Re-calculate the length of all networks marked for auto-recalculate");

			RND_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* fill between buttons and close */
				RND_DAD_COMPFLAG(netlist_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_END(netlist_ctx.dlg);
		
			RND_DAD_BUTTON_CLOSES(netlist_ctx.dlg, clbtn);
		RND_DAD_END(netlist_ctx.dlg);
	RND_DAD_END(netlist_ctx.dlg);

	/* set up the context */
	netlist_ctx.active = 1;

	RND_DAD_DEFSIZE(netlist_ctx.dlg, 300, 350);
	RND_DAD_NEW("netlist", netlist_ctx.dlg, "pcb-rnd netlist", &netlist_ctx, rnd_false, netlist_close_cb);

	{
		rnd_hid_attr_val_t hv;
		hv.dbl = 0.33;
		rnd_gui->attr_dlg_set_value(netlist_ctx.dlg_hid_ctx, wvpan, &hv);
	}

	netlist_data2dlg(&netlist_ctx);
}

static const char pcb_acts_NetlistDialog[] = "NetlistDialog()\n";
static const char pcb_acth_NetlistDialog[] = "Open the netlist dialog.";
static fgw_error_t pcb_act_NetlistDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (strcmp(rnd_gui->name, "lesstif") == 0)
		rnd_actionva(RND_ACT_HIDLIB, "DoWindows", "netlist");
	else
		pcb_dlg_netlist(PCB);
	RND_ACT_IRES(0);
	return 0;
}

/* update the dialog after a netlist change */
static void pcb_dlg_netlist_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	netlist_ctx_t *ctx = user_data;
	if (!ctx->active)
		return;
	netlist_data2dlg(ctx);
}
static void pcb_dlg_netlist_init(void)
{
	rnd_event_bind(PCB_EVENT_NETLIST_CHANGED, pcb_dlg_netlist_ev, &netlist_ctx, dlg_netlist_cookie);
}

static void pcb_dlg_netlist_uninit(void)
{
	rnd_event_unbind_allcookie(dlg_netlist_cookie);
	vtp0_uninit(&netlist_color_save);
}
