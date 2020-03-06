/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <stdlib.h>
#include <librnd/core/plugins.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/event.h>

#include <librnd/core/actions.h>
#include "grid_menu.h"
#include "cli_history.h"
#include "lead_user.h"
#include "place.h"

#include "lib_hid_common.h"
#include "dialogs_conf.h"
#include "dlg_comm_m.h"
#include "dlg_log.h"
#include "act_dad.h"
#include "../src_plugins/lib_hid_common/zoompan.h"
#include "../src_plugins/lib_hid_common/conf_internal.c"

const conf_dialogs_t dialogs_conf;
#define DIALOGS_CONF_FN "dialogs.conf"


static const char *grid_cookie = "lib_hid_common/grid";
static const char *lead_cookie = "lib_hid_common/user_lead";
static const char *wplc_cookie = "lib_hid_common/window_placement";

extern void pcb_dad_spin_update_global_coords(void);
static void grid_unit_chg_ev(conf_native_t *cfg, int arr_idx)
{
	pcb_dad_spin_update_global_coords();
}

const char pcb_acts_Command[] = "Command()";
const char pcb_acth_Command[] = "Displays the command line input in the status area.";
/* DOC: command */
fgw_error_t pcb_act_Command(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_GUI_NOGUI();
	pcb_gui->open_command(pcb_gui);
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acth_gui[] = "Intenal: GUI frontend action. Do not use directly.";

pcb_action_t hid_common_action_list[] = {
	{"dad", pcb_act_dad, pcb_acth_dad, pcb_acts_dad},
	{"Pan", pcb_act_Pan, pcb_acth_Pan, pcb_acts_Pan},
	{"Center", pcb_act_Center, pcb_acth_Center, pcb_acts_Center},
	{"Scroll", pcb_act_Scroll, pcb_acth_Scroll, pcb_acts_Scroll},
	{"LogDialog", pcb_act_LogDialog, pcb_acth_LogDialog, pcb_acts_LogDialog},
	{"Command", pcb_act_Command, pcb_acth_Command, pcb_acts_Command},
	{"gui_PromptFor", pcb_act_gui_PromptFor, pcb_acth_gui, NULL},
	{"gui_MessageBox", pcb_act_gui_MessageBox, pcb_acth_gui, NULL},
	{"gui_FallbackColorPick", pcb_act_gui_FallbackColorPick, pcb_acth_gui, NULL},
	{"gui_MayOverwriteFile", pcb_act_gui_MayOverwriteFile, pcb_acth_gui, NULL}
};

static const char *hid_common_cookie = "lib_hid_common plugin";

int pplg_check_ver_lib_hid_common(int ver_needed) { return 0; }

static conf_hid_id_t conf_id;

void pplg_uninit_lib_hid_common(void)
{
	pcb_conf_unreg_file(DIALOGS_CONF_FN, dialogs_conf_internal);
	pcb_clihist_save();
	pcb_clihist_uninit();
	pcb_event_unbind_allcookie(grid_cookie);
	pcb_event_unbind_allcookie(lead_cookie);
	pcb_event_unbind_allcookie(wplc_cookie);
	pcb_conf_hid_unreg(grid_cookie);
	pcb_dialog_place_uninit();
	pcb_remove_actions_by_cookie(hid_common_cookie);
	pcb_act_dad_uninit();
	pcb_conf_unreg_fields("plugins/lib_hid_common/");
	pcb_dlg_log_uninit();
}

int pplg_init_lib_hid_common(void)
{
	static conf_hid_callbacks_t ccb, ccbu;
	conf_native_t *nat;

	PCB_API_CHK_VER;
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(dialogs_conf, field,isarray,type_name,cpath,cname,desc,flags);
/*#include "lib_hid_common_conf_fields.h"*/
#include "dialogs_conf_fields.h"

	pcb_dlg_log_init();
	PCB_REGISTER_ACTIONS(hid_common_action_list, hid_common_cookie)
	pcb_act_dad_init();

	pcb_conf_reg_file(DIALOGS_CONF_FN, dialogs_conf_internal);

	pcb_dialog_place_init();

	pcb_event_bind(PCB_EVENT_GUI_INIT, pcb_grid_update_ev, NULL, grid_cookie);
	pcb_event_bind(PCB_EVENT_GUI_LEAD_USER, pcb_lead_user_ev, NULL, lead_cookie);
	pcb_event_bind(PCB_EVENT_GUI_DRAW_OVERLAY_XOR, pcb_lead_user_draw_ev, NULL, lead_cookie);
	pcb_event_bind(PCB_EVENT_DAD_NEW_DIALOG, pcb_dialog_place, NULL, wplc_cookie);
	pcb_event_bind(PCB_EVENT_DAD_NEW_GEO, pcb_dialog_resize, NULL, wplc_cookie);

	conf_id = pcb_conf_hid_reg(grid_cookie, NULL);

	memset(&ccb, 0, sizeof(ccb));
	ccb.val_change_post = pcb_grid_update_conf;
	nat = pcb_conf_get_field("editor/grids");
	if (nat != NULL)
		pcb_conf_hid_set_cb(nat, conf_id, &ccb);

	memset(&ccbu, 0, sizeof(ccbu));
	ccbu.val_change_post = grid_unit_chg_ev;
	nat = pcb_conf_get_field("editor/grid_unit");
	if (nat != NULL)
		pcb_conf_hid_set_cb(nat, conf_id, &ccbu);

	return 0;
}
