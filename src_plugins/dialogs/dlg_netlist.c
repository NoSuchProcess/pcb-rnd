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

#include "event.h"
#include "netlist2.h"

const char *dlg_netlist_cookie = "netlist dialog";

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wnetlist, wprev, wtermlist;
	int wsel, wunsel, wfind, wunfind, wrats, wnorats, wripup, waddrats;
	int active; /* already open - allow only one instance */
} netlist_ctx_t;

netlist_ctx_t netlist_ctx;

static void netlist_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	netlist_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(netlist_ctx_t));
	pcb_event(PCB_EVENT_GUI_LEAD_USER, "cci", 0, 0, 0);
}

/* returns allocated net name for the currently selected net */
static char *netlist_data2dlg_netlist(netlist_ctx_t *ctx)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[4], *cursor_path = NULL;
	pcb_net_t **n, **nets;

	attr = &ctx->dlg[ctx->wnetlist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);


	nets = pcb_netlist_sort(&PCB->netlist[1]);
	cell[2] = NULL;
	for(n = nets; *n != NULL; n++) {
		cell[0] = pcb_strdup((*n)->name);
		cell[1] = pcb_strdup((*n)->inhibit_rats ? "*" : "");
		pcb_dad_tree_append(attr, NULL, cell);
	}
	free(nets);

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wnetlist, &hv);
	}
	return cursor_path;
}

static void netlist_data2dlg_connlist(netlist_ctx_t *ctx, pcb_net_t *net)
{
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	pcb_hid_row_t *r;
	char *cell[2], *cursor_path = NULL;
	pcb_net_term_t *t;

	attr = &ctx->dlg[ctx->wtermlist];
	tree = (pcb_hid_tree_t *)attr->enumerations;

	/* remember cursor */
	if (net != NULL) {
		r = pcb_dad_tree_get_selected(attr);
		if (r != NULL)
			cursor_path = pcb_strdup(r->cell[0]);
	}

	/* remove existing items */
	for(r = gdl_first(&tree->rows); r != NULL; r = gdl_first(&tree->rows))
		pcb_dad_tree_remove(attr, r);

	if (net == NULL)
		return;

	cell[1] = NULL;
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
		cell[0] = pcb_concat(t->refdes, "-", t->term, NULL);
		pcb_dad_tree_append(attr, NULL, cell);
	}

	/* restore cursor */
	if (cursor_path != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = cursor_path;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->wtermlist, &hv);
		free(cursor_path);
	}
}


static void netlist_data2dlg(netlist_ctx_t *ctx)
{
	pcb_net_t *curnet = NULL;
	char *curnetname = netlist_data2dlg_netlist(ctx);

	if (curnetname != NULL)
		curnet = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], curnetname, 0);
	free(curnetname);
	netlist_data2dlg_connlist(ctx, curnet);
}

static void netlist_row_selected(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	netlist_ctx_t *ctx= tree->user_ctx;
	const char *netname = NULL;

	if (row != NULL)
		netname = row->cell[0];
	netlist_data2dlg_connlist(ctx, pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], netname, 0));
	pcb_event(PCB_EVENT_GUI_LEAD_USER, "cci", 0, 0, 0);
}

static void termlist_row_selected(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	netlist_ctx_t *ctx= tree->user_ctx;
	char *refdes, *term;
	pcb_any_obj_t *obj;

	pcb_event(PCB_EVENT_GUI_LEAD_USER, "cci", 0, 0, 0);
	if (row == NULL)
		return;
	refdes = pcb_strdup(row->cell[0]);
	term = strchr(refdes, '-');
	if (term != NULL) {
		*term = '\0';
		term++;
		obj = pcb_term_find_name(ctx->pcb, ctx->pcb->Data, PCB_LYT_COPPER, refdes, term, 0, NULL, NULL);
		if (obj != NULL) {
			pcb_coord_t x, y;
			pcb_obj_center(obj, &x, &y);
			pcb_event(PCB_EVENT_GUI_LEAD_USER, "cci", x, y, 1);
		}
	}
	free(refdes);
}

static void netlist_button_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	netlist_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *atree;
	int w = attr - ctx->dlg;
	pcb_hid_row_t *r;
	const char *name;

	atree = &ctx->dlg[ctx->wnetlist];
	r = pcb_dad_tree_get_selected(atree);
	if (r == NULL)
		return;
	name = pcb_strdup(r->cell[0]);

	if (w == ctx->wsel)
		pcb_actionl("netlist", "select", name, NULL);
	else if (w == ctx->wunsel)
		pcb_actionl("netlist", "unselect", name, NULL);
	else if (w == ctx->wfind) {
		pcb_actionl("connection", "reset", NULL);
		pcb_actionl("netlist", "find", name, NULL);
	}
	else if (w == ctx->wunfind)
		pcb_actionl("connection", "reset", NULL);
	if (w == ctx->wrats)
		pcb_actionl("netlist", "rats", name, NULL);
	else if (w == ctx->wnorats)
		pcb_actionl("netlist", "norats", name, NULL);
	else if (w == ctx->wripup)
		pcb_actionl("netlist", "ripup", name, NULL);
	else if (w == ctx->waddrats)
		pcb_actionl("netlist", "AddRats", name, NULL);
	else {
		pcb_message(PCB_MSG_ERROR, "Internal error: netlist_button_cb() called from an invalid widget\n");
		return;
	}
	pcb_gui->invalidate_all();
}

static void netlist_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{

}

static pcb_bool netlist_mouse(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_false;
}

static void pcb_dlg_netlist(pcb_board_t *pcb)
{
	static const char *hdr[] = {"network", "FR", NULL};
	static const char *hdr2[] = {"terminals", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pcb_box_t bb_prv;
	int wvpan;

	if (netlist_ctx.active)
		return; /* do not open another */

	bb_prv.X1 = 0;
	bb_prv.Y1 = 0;
	bb_prv.X2 = PCB->MaxWidth;
	bb_prv.Y2 = PCB->MaxHeight;
	netlist_ctx.pcb = pcb;

	PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* layout */
		PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);

		PCB_DAD_BEGIN_HPANE(netlist_ctx.dlg);
			PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);

			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* left */
				PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_TREE(netlist_ctx.dlg, 2, 0, hdr);
					PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
					netlist_ctx.wnetlist = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_TREE_SET_CB(netlist_ctx.dlg, selected_cb, netlist_row_selected);
					PCB_DAD_TREE_SET_CB(netlist_ctx.dlg, ctx, &netlist_ctx);
			PCB_DAD_END(netlist_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right */
				PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_BEGIN_VPANE(netlist_ctx.dlg);
					PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
					wvpan = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right-top */
						PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
						PCB_DAD_PREVIEW(netlist_ctx.dlg, netlist_expose, netlist_mouse, NULL, &bb_prv, 100, 100, &netlist_ctx);
							PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
							netlist_ctx.wprev = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_END(netlist_ctx.dlg);
					PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* right-bottom */
						PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
						PCB_DAD_TREE(netlist_ctx.dlg, 1, 0, hdr2);
							PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
							netlist_ctx.wtermlist = PCB_DAD_CURRENT(netlist_ctx.dlg);
							PCB_DAD_TREE_SET_CB(netlist_ctx.dlg, selected_cb, termlist_row_selected);
							PCB_DAD_TREE_SET_CB(netlist_ctx.dlg, ctx, &netlist_ctx);
					PCB_DAD_END(netlist_ctx.dlg);
				PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_END(netlist_ctx.dlg);
		PCB_DAD_END(netlist_ctx.dlg);

		PCB_DAD_BEGIN_HBOX(netlist_ctx.dlg); /* bottom button row */
			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "select");
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wsel = PCB_DAD_CURRENT(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "unsel.");
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wunsel = PCB_DAD_CURRENT(netlist_ctx.dlg);
			PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "find ");
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wfind = PCB_DAD_CURRENT(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "clear");
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wunfind = PCB_DAD_CURRENT(netlist_ctx.dlg);
			PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "rat disable");
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
					netlist_ctx.wnorats = PCB_DAD_CURRENT(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "rat enable");
					netlist_ctx.wrats = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
			PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "add rats");
					netlist_ctx.waddrats = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
				PCB_DAD_BUTTON(netlist_ctx.dlg, "rip up  ");
					netlist_ctx.wripup = PCB_DAD_CURRENT(netlist_ctx.dlg);
					PCB_DAD_CHANGE_CB(netlist_ctx.dlg, netlist_button_cb);
			PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(netlist_ctx.dlg); /* fill between buttons and close */
				PCB_DAD_COMPFLAG(netlist_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(netlist_ctx.dlg);
			PCB_DAD_BUTTON_CLOSES(netlist_ctx.dlg, clbtn);
		PCB_DAD_END(netlist_ctx.dlg);
	PCB_DAD_END(netlist_ctx.dlg);

	/* set up the context */
	netlist_ctx.active = 1;

	PCB_DAD_DEFSIZE(netlist_ctx.dlg, 300, 350);
	PCB_DAD_NEW("netlist", netlist_ctx.dlg, "pcb-rnd netlist", &netlist_ctx, pcb_false, netlist_close_cb);

	{
		pcb_hid_attr_val_t hv;
		hv.real_value = 0.33;
		pcb_gui->attr_dlg_set_value(netlist_ctx.dlg_hid_ctx, wvpan, &hv);
	}

	netlist_data2dlg(&netlist_ctx);
}

static const char pcb_acts_NetlistDialog[] = "NetlistDialog()\n";
static const char pcb_acth_NetlistDialog[] = "Open the netlist dialog.";
static fgw_error_t pcb_act_NetlistDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_netlist(PCB);
	PCB_ACT_IRES(0);
	return 0;
}

/* update the dialog after a netlist change */
static void pcb_dlg_netlist_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	netlist_ctx_t *ctx = user_data;
	if (!ctx->active)
		return;
	netlist_data2dlg(ctx);
}
static void pcb_dlg_netlist_init(void)
{
	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, pcb_dlg_netlist_ev, &netlist_ctx, dlg_netlist_cookie);
}

static void pcb_dlg_netlist_uninit(void)
{
	pcb_event_unbind_allcookie(dlg_netlist_cookie);
}
