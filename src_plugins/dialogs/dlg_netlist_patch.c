/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
#include <librnd/core/rnd_conf.h>

const char *dlg_netlist_patch_cookie = "netlist patch dialog";

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int wlist;
	pcb_board_t *pcb;
	int active; /* already open - allow only one instance */
} netlist_patch_ctx_t;

netlist_patch_ctx_t netlist_patch_ctx;

static void netlist_patch_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	netlist_patch_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(netlist_patch_ctx_t));
}

static void netlist_patch_data2dlg(netlist_patch_ctx_t *ctx)
{
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	char *cell[3];
	pcb_ratspatch_line_t *n;

	attr = &ctx->dlg[ctx->wlist];
	tree = attr->wdata;

	/* remove existing items */
	rnd_dad_tree_clear(tree);


	cell[2] = NULL;
	for(n = ctx->pcb->NetlistPatches; n != NULL; n = n->next) {
		switch(n->op) {
			case RATP_ADD_CONN:      cell[0] = rnd_strdup("add conn"); break;
			case RATP_DEL_CONN:      cell[0] = rnd_strdup("del conn"); break;
			case RATP_CHANGE_ATTRIB: cell[0] = rnd_strdup("chg attrib"); break;
			default:                 cell[0] = rnd_strdup("unknown"); break;
		}
		switch(n->op) {
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:      cell[1] = rnd_strdup_printf("%s, %s", n->arg1.net_name, n->id); break;
			case RATP_CHANGE_ATTRIB: cell[1] = rnd_strdup_printf("%s, %s, %s", n->id, n->arg1.attrib_name, n->arg2.attrib_val); break;
			default:                 cell[1] = rnd_strdup("?"); break;
		}
		rnd_dad_tree_append(attr, NULL, cell);
	}
}

static void pcb_dlg_netlist_patch(pcb_board_t *pcb)
{
	static const char *hdr[] = {"instruction", "args", NULL};
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (netlist_patch_ctx.active)
		return; /* do not open another */

	netlist_patch_ctx.pcb = pcb;

	RND_DAD_BEGIN_VBOX(netlist_patch_ctx.dlg); /* layout */
		RND_DAD_COMPFLAG(netlist_patch_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_TREE(netlist_patch_ctx.dlg, 2, 0, hdr);
				RND_DAD_COMPFLAG(netlist_patch_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				netlist_patch_ctx.wlist = RND_DAD_CURRENT(netlist_patch_ctx.dlg);
		
			RND_DAD_BUTTON_CLOSES(netlist_patch_ctx.dlg, clbtn);
		RND_DAD_END(netlist_patch_ctx.dlg);
	RND_DAD_END(netlist_patch_ctx.dlg);

	/* set up the context */
	netlist_patch_ctx.active = 1;

	RND_DAD_DEFSIZE(netlist_patch_ctx.dlg, 200, 350);
	RND_DAD_NEW("netlist_patch", netlist_patch_ctx.dlg, "pcb-rnd netlist patch", &netlist_patch_ctx, rnd_false, netlist_patch_close_cb);

	netlist_patch_data2dlg(&netlist_patch_ctx);
}

static const char pcb_acts_NetlistPatchDialog[] = "NetlistPatchDialog()\n";
static const char pcb_acth_NetlistPatchDialog[] = "Open the netlist patch dialog (back annotation).";
static fgw_error_t pcb_act_NetlistPatchDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_netlist_patch(PCB);
	RND_ACT_IRES(0);
	return 0;
}

/* update the dialog after a netlist change */
static void pcb_dlg_netlist_patch_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	netlist_patch_ctx_t *ctx = user_data;
	if (!ctx->active)
		return;
	netlist_patch_data2dlg(ctx);
}
static void pcb_dlg_netlist_patch_init(void)
{
	rnd_event_bind(PCB_EVENT_NETLIST_CHANGED, pcb_dlg_netlist_patch_ev, &netlist_patch_ctx, dlg_netlist_patch_cookie);
}

static void pcb_dlg_netlist_patch_uninit(void)
{
	rnd_event_unbind_allcookie(dlg_netlist_patch_cookie);
}
