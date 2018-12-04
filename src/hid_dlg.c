/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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
 *
 */

#include "config.h"

#include "actions.h"
#include "hid.h"


/* Action and wrapper implementation for dialogs. If GUI is available, the
   gui_ prefixed action is executed, else the cli_ prefixed one is used. If
   nothing is available, the effect is equivalent to cancel. */


/* Call the gui_ or the cli_ action; act_name must be all lowercase! */
static fgw_error_t call_dialog(const char *act_name, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char tmp[128];

	strcpy(tmp, "gui_");
	strncpy(tmp+4, act_name, sizeof(tmp)-5);
	if (fgw_func_lookup(&pcb_fgw, tmp) != NULL)
		return pcb_actionv_bin(tmp, res, argc, argv);

	tmp[0] = 'c'; tmp[1] = 'l';
	if (fgw_func_lookup(&pcb_fgw, tmp) != NULL)
		return pcb_actionv_bin(tmp, res, argc, argv);

	return FGW_ERR_NOT_FOUND;
}

static const char pcb_acts_PromptFor[] = "PromptFor([message[,default]])";
static const char pcb_acth_PromptFor[] = "Prompt for a response.";
/* DOC: promptfor.html */
static fgw_error_t pcb_act_PromptFor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return call_dialog("promptfor", res, argc, argv);
}

char *pcb_hid_prompt_for(const char *msg, const char *default_string, const char *title)
{
	fgw_arg_t res, argv[4];

	argv[1].type = FGW_STR; argv[1].val.cstr = msg;
	argv[2].type = FGW_STR; argv[2].val.cstr = default_string;
	argv[3].type = FGW_STR; argv[3].val.cstr = title;

	if (pcb_actionv_bin("PromptFor", &res, 4, argv) != 0)
		return NULL;

	if (res.type == (FGW_STR | FGW_DYN))
		return res.val.str;

	fgw_arg_free(&pcb_fgw, &res);
	return NULL;
}

static pcb_action_t hid_dlg_action_list[] = {
	{"PromptFor", pcb_act_PromptFor, pcb_acth_PromptFor, pcb_acts_PromptFor}
};

PCB_REGISTER_ACTIONS(hid_dlg_action_list, NULL)
