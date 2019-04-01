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

#include "config.h"

#include "actions.h"
#include "hid_dad.h"

#include "live_script.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	char *name;
} live_script_t;

static void lvs_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	live_script_t *lvs = caller_data;
	PCB_DAD_FREE(lvs->dlg);
	free(lvs);
}

static live_script_t *pcb_dlg_live_script(const char *name)
{
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	char *title;
	live_script_t *lvs = calloc(sizeof(live_script_t), 1);

	name = pcb_strdup(name);
	PCB_DAD_BEGIN_VBOX(lvs->dlg);
		PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_TEXT(lvs->dlg, lvs);

		PCB_DAD_BEGIN_HBOX(lvs->dlg);
			PCB_DAD_BUTTON(lvs->dlg, "re-run");
			PCB_DAD_BUTTON(lvs->dlg, "run");
			PCB_DAD_BUTTON(lvs->dlg, "undo");
			PCB_DAD_BUTTON(lvs->dlg, "save");
			PCB_DAD_BUTTON(lvs->dlg, "load");
			PCB_DAD_BEGIN_HBOX(lvs->dlg);
				PCB_DAD_COMPFLAG(lvs->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(lvs->dlg);
			PCB_DAD_BUTTON_CLOSES(lvs->dlg, clbtn);
		PCB_DAD_END(lvs->dlg);
	PCB_DAD_END(lvs->dlg);

	title = pcb_concat("Live Scripting: ", name, NULL);
	PCB_DAD_NEW("live_script", lvs->dlg, title, lvs, pcb_false, lvs_close_cb);
	free(title);
}

const char pcb_acts_LiveScript[] = "LiveScript([name], [new])\n";
const char pcb_acth_LiveScript[] = "Manage a live script";
fgw_error_t pcb_act_LiveScript(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_live_script("name");
	return 0;
}
