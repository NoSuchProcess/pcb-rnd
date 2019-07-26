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

/* Common dialogs: simple, modal dialogs and info bars.
   Even the core will run some of these, through a dispatcher (e.g.
   action). */

#include "config.h"
#include "actions.h"
#include "board.h"
#include "hid_dad.h"
#include "../src_plugins/lib_hid_common/xpm.h"
#include "../src_plugins/lib_hid_common/dlg_comm_m.h"
#include "plug_io.h"

static void ifb_file_chg_reload_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_revert_pcb();
	pcb_actionl("InfoBarFileChanged", "close", NULL);
}

static void ifb_file_chg_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_actionl("InfoBarFileChanged", "close", NULL);
}

const char pcb_acts_InfoBarFileChanged[] = "InfoBarFileChanged(open|close)\n";
const char pcb_acth_InfoBarFileChanged[] = "Present the \"file changed\" warning info bar with buttons to reload or cancel";
fgw_error_t pcb_act_InfoBarFileChanged(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static pcb_hid_dad_subdialog_t sub;
	static int active = 0, wlab[2];
	pcb_hid_attr_val_t hv;
	const char *cmd;

	if (!PCB_HAVE_GUI_ATTR_DLG) {
		PCB_ACT_IRES(0);
		return 0;
	}

	PCB_ACT_CONVARG(1, FGW_STR, InfoBarFileChanged, cmd = argv[1].val.str);

	if (strcmp(cmd, "open") == 0) {
		if (!active) {
			PCB_DAD_BEGIN_HBOX(sub.dlg);
				PCB_DAD_COMPFLAG(sub.dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
				PCB_DAD_BEGIN_VBOX(sub.dlg);
					PCB_DAD_PICTURE(sub.dlg, pcp_dlg_xpm_by_name("warning"));
				PCB_DAD_END(sub.dlg);
				PCB_DAD_BEGIN_VBOX(sub.dlg);
					PCB_DAD_COMPFLAG(sub.dlg, PCB_HATF_EXPFILL);
					PCB_DAD_BEGIN_HBOX(sub.dlg); PCB_DAD_COMPFLAG(sub.dlg, PCB_HATF_EXPFILL); PCB_DAD_END(sub.dlg);
					PCB_DAD_LABEL(sub.dlg, "line0");
						wlab[0] = PCB_DAD_CURRENT(sub.dlg);
					PCB_DAD_LABEL(sub.dlg, "line1");
						wlab[1] = PCB_DAD_CURRENT(sub.dlg);
					PCB_DAD_BEGIN_HBOX(sub.dlg); PCB_DAD_COMPFLAG(sub.dlg, PCB_HATF_EXPFILL); PCB_DAD_END(sub.dlg);
				PCB_DAD_END(sub.dlg);
				PCB_DAD_BEGIN_VBOX(sub.dlg);
					PCB_DAD_BUTTON(sub.dlg, "Reload");
						PCB_DAD_HELP(sub.dlg, "Load the new verison of the file from disk,\ndiscarding any in-memory change on the board");
						PCB_DAD_CHANGE_CB(sub.dlg, ifb_file_chg_reload_cb);
					PCB_DAD_BEGIN_HBOX(sub.dlg); PCB_DAD_COMPFLAG(sub.dlg, PCB_HATF_EXPFILL); PCB_DAD_END(sub.dlg);
					PCB_DAD_BUTTON(sub.dlg, "Cancel");
						PCB_DAD_HELP(sub.dlg, "Hide this info bar until the file changes again on disk");
						PCB_DAD_CHANGE_CB(sub.dlg, ifb_file_chg_close_cb);
				PCB_DAD_END(sub.dlg);
			PCB_DAD_END(sub.dlg);
			if (pcb_hid_dock_enter(&sub, PCB_HID_DOCK_TOP_INFOBAR, "file_changed") != 0) {
				PCB_ACT_IRES(1);
				return 0;
			}
			active = 1;
		}

		/* update labels */
		hv.str = pcb_strdup_printf("The file %s has changed on disk", PCB->hidlib.filename);
		pcb_gui->attr_dlg_set_value(sub.dlg_hid_ctx, wlab[0], &hv);
		free((char *)hv.str);

		hv.str = (PCB->Changed ? "Do you want to drop your changes and reload the file?" : "Do you want to reload the file?");
		pcb_gui->attr_dlg_set_value(sub.dlg_hid_ctx, wlab[1], &hv);
	}
	else if (strcmp(cmd, "close") == 0) {
		if (active) {
			pcb_hid_dock_leave(&sub);
			active = 0;
		}
	}
	else
		PCB_ACT_FAIL(InfoBarFileChanged);

	PCB_ACT_IRES(0);
	return 0;
}
