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
#include <librnd/core/actions.h>
#include "board.h"
#include <librnd/core/hid_dad.h>
#include <librnd/plugins/lib_hid_common/xpm.h>
#include <librnd/plugins/lib_hid_common/dlg_comm_m.h>
#include "plug_io.h"

static void ifb_file_chg_reload_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	pcb_revert_pcb();
	rnd_actionva(&PCB->hidlib, "InfoBarFileChanged", "close", NULL);
}

static void ifb_file_chg_close_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(&PCB->hidlib, "InfoBarFileChanged", "close", NULL);
}

const char pcb_acts_InfoBarFileChanged[] = "InfoBarFileChanged(open|close)\n";
const char pcb_acth_InfoBarFileChanged[] = "Present the \"file changed\" warning info bar with buttons to reload or cancel";
fgw_error_t pcb_act_InfoBarFileChanged(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static rnd_hid_dad_subdialog_t sub;
	static int active = 0, wlab[2];
	rnd_hid_attr_val_t hv;
	const char *cmd;

	if (!RND_HAVE_GUI_ATTR_DLG) {
		RND_ACT_IRES(0);
		return 0;
	}

	RND_ACT_CONVARG(1, FGW_STR, InfoBarFileChanged, cmd = argv[1].val.str);

	if (strcmp(cmd, "open") == 0) {
		if (!active) {
			RND_DAD_BEGIN_HBOX(sub.dlg);
				RND_DAD_COMPFLAG(sub.dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
				RND_DAD_BEGIN_VBOX(sub.dlg);
					RND_DAD_PICTURE(sub.dlg, pcp_dlg_xpm_by_name("warning"));
				RND_DAD_END(sub.dlg);
				RND_DAD_BEGIN_VBOX(sub.dlg);
					RND_DAD_COMPFLAG(sub.dlg, RND_HATF_EXPFILL);
					RND_DAD_BEGIN_HBOX(sub.dlg); RND_DAD_COMPFLAG(sub.dlg, RND_HATF_EXPFILL); RND_DAD_END(sub.dlg);
					RND_DAD_LABEL(sub.dlg, "line0");
						wlab[0] = RND_DAD_CURRENT(sub.dlg);
					RND_DAD_LABEL(sub.dlg, "line1");
						wlab[1] = RND_DAD_CURRENT(sub.dlg);
					RND_DAD_BEGIN_HBOX(sub.dlg); RND_DAD_COMPFLAG(sub.dlg, RND_HATF_EXPFILL); RND_DAD_END(sub.dlg);
				RND_DAD_END(sub.dlg);
				RND_DAD_BEGIN_VBOX(sub.dlg);
					RND_DAD_BUTTON(sub.dlg, "Reload");
						RND_DAD_HELP(sub.dlg, "Load the new verison of the file from disk,\ndiscarding any in-memory change on the board");
						RND_DAD_CHANGE_CB(sub.dlg, ifb_file_chg_reload_cb);
					RND_DAD_BEGIN_HBOX(sub.dlg); RND_DAD_COMPFLAG(sub.dlg, RND_HATF_EXPFILL); RND_DAD_END(sub.dlg);
					RND_DAD_BUTTON(sub.dlg, "Cancel");
						RND_DAD_HELP(sub.dlg, "Hide this info bar until the file changes again on disk");
						RND_DAD_CHANGE_CB(sub.dlg, ifb_file_chg_close_cb);
				RND_DAD_END(sub.dlg);
			RND_DAD_END(sub.dlg);
			if (rnd_hid_dock_enter(&sub, RND_HID_DOCK_TOP_INFOBAR, "file_changed") != 0) {
				RND_ACT_IRES(1);
				return 0;
			}
			active = 1;
		}

		/* update labels */
		hv.str = rnd_strdup_printf("The file %s has changed on disk", PCB->hidlib.filename);
		rnd_gui->attr_dlg_set_value(sub.dlg_hid_ctx, wlab[0], &hv);
		free((char *)hv.str);

		hv.str = (PCB->Changed ? "Do you want to drop your changes and reload the file?" : "Do you want to reload the file?");
		rnd_gui->attr_dlg_set_value(sub.dlg_hid_ctx, wlab[1], &hv);
	}
	else if (strcmp(cmd, "close") == 0) {
		if (active) {
			rnd_hid_dock_leave(&sub);
			active = 0;
		}
	}
	else
		RND_ACT_FAIL(InfoBarFileChanged);

	RND_ACT_IRES(0);
	return 0;
}
