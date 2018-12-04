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

/* Common dialogs: simple, modal dialogs for all parts of the code, not just
   the GUI HIDs. Even the core will run some of these, through a dispatcher. */

static const char nope[] = "Do not use.";

static fgw_error_t pcb_act_gui_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *label, *default_str = "", *title = "pcb-rnd user input";
	const char *pcb_acts_gui_PromptFor =  nope, *pcb_acth_gui_PromptFor = nope;
	int ws;
	PCB_DAD_DECL(dlg);

	PCB_ACT_CONVARG(1, FGW_STR, gui_PromptFor, label = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, gui_PromptFor, default_str = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, gui_PromptFor, title = argv[3].val.str);

	PCB_DAD_BEGIN_VBOX(dlg);
		PCB_DAD_LABEL(dlg, label);
		PCB_DAD_STRING(dlg);
			ws = PCB_DAD_CURRENT(dlg);
			dlg[ws].default_val.str_value = pcb_strdup(default_str);
	PCB_DAD_END(dlg);

	PCB_DAD_NEW(dlg, title, NULL, NULL, pcb_true, NULL);
	PCB_DAD_RUN(dlg);

	res.type = FGW_STR | FGW_DYN;
	res.val.str = pcb_strdup(dlg[ws].default_val.str_value);
	PCB_DAD_FREE(dlg);

	return 0;
}

