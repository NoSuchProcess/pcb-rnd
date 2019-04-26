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
	int wname, wlineth, wclr, wtxtscale, wtxtth, wviahole, wviaring, wattr;
} rstdlg_ctx_t;

rstdlg_ctx_t rstdlg_ctx;

static void rstdlg_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	rstdlg_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(rstdlg_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pcb_dlg_rstdlg(int rst_idx)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	static const char *attr_hdr[] = {"attribute key", "attribute value", NULL};

TODO("if already active, switch to rst_idx");
	if (rstdlg_ctx.active)
		return; /* do not open another */

	PCB_DAD_BEGIN_VBOX(rstdlg_ctx.dlg);
		PCB_DAD_COMPFLAG(rstdlg_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABLE(rstdlg_ctx.dlg, 2);
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Name:");
			PCB_DAD_STRING(rstdlg_ctx.dlg);
				rstdlg_ctx.wname = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Name of the routing style");
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Line thick.:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wlineth = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Thickness of line/arc objects");
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Text scale:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wtxtscale = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Text size scale in %; 100 means normal size");
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Clearance:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wclr = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Object clearance: any object placed with this style\nwill clear this much from sorrunding clearing-enabled polygons\n(unless the object is joined to the polygon)");
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "Text thick.:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wtxtth = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Text stroke thickness;\nif 0 use the default heuristics that\ncalculates it from text scale");
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "*Via hole:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wviahole = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Via hole diameter\nwarning: will be replaced with the padstack selector");
			PCB_DAD_LABEL(rstdlg_ctx.dlg, "*Via ring:");
			PCB_DAD_COORD(rstdlg_ctx.dlg, "");
				rstdlg_ctx.wviaring = PCB_DAD_CURRENT(rstdlg_ctx.dlg);
				PCB_DAD_HELP(rstdlg_ctx.dlg, "Via ring diameter\nwarning: will be replaced with the padstack selector");
		PCB_DAD_END(rstdlg_ctx.dlg);
		PCB_DAD_TREE(rstdlg_ctx.dlg, 2, 0, attr_hdr);
			PCB_DAD_HELP(rstdlg_ctx.dlg, "These attributes are automatically added to\nany object drawn with this routing style");
		PCB_DAD_BEGIN_HBOX(rstdlg_ctx.dlg);
			PCB_DAD_BUTTON(rstdlg_ctx.dlg, "add");
			PCB_DAD_BUTTON(rstdlg_ctx.dlg, "del");
		PCB_DAD_END(rstdlg_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(rstdlg_ctx.dlg, clbtn);
	PCB_DAD_END(rstdlg_ctx.dlg);

	/* set up the context */
	rstdlg_ctx.active = 1;

	PCB_DAD_NEW("route_style", rstdlg_ctx.dlg, "Edit route style", &rstdlg_ctx, pcb_false, rstdlg_close_cb);
}

