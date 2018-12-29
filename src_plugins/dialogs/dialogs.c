/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
#include "hid.h"
#include "hid_attrib.h"
#include "actions.h"
#include "hid_dad.h"
#include "plugins.h"
#include "funchash_core.h"
#include "dialogs_conf.h"

/* include them all for static inlines */
#include "dlg_test.c"
#include "dlg_layer_binding.c"
#include "dlg_layer_flags.c"
#include "dlg_flag_edit.c"
#include "dlg_padstack.c"
#include "dlg_about.c"
#include "dlg_pinout.c"
#include "dlg_export.c"
#include "dlg_lib_pstk.c"
#include "dlg_undo.c"
#include "dlg_plugins.c"
#include "dlg_fontsel.c"
#include "dlg_comm_m.c"
#include "place.c"

#include "dlg_view.h"
#include "dlg_pref.h"
#include "act_dad.h"

const conf_dialogs_t conf_dialogs;

static const char pcb_acth_gui[] = "Intenal: GUI frontend action. Do not use directly.";

pcb_action_t dialogs_action_list[] = {
	{"dlg_test", pcb_act_dlg_test, dlg_test_help, dlg_test_syntax},
	{"LayerBinding", pcb_act_LayerBinding, pcb_acth_LayerBinding, pcb_acts_LayerBinding},
	{"FlagEdit", pcb_act_FlagEdit, pcb_acth_FlagEdit, pcb_acts_FlagEdit},
	{"PadstackEdit", pcb_act_PadstackEdit, pcb_acth_PadstackEdit, pcb_acts_PadstackEdit},
	{"About", pcb_act_About, pcb_acth_About, pcb_acts_About},
	{"Pinout", pcb_act_Pinout, pcb_acth_Pinout, pcb_acts_Pinout},
	{"ExportGUI", pcb_act_ExportGUI, pcb_acth_ExportGUI, pcb_acts_ExportGUI},
	{"PrintGUI", pcb_act_PrintGUI, pcb_acth_PrintGUI, pcb_acts_PrintGUI},
	{"GroupPropGui", pcb_act_GroupPropGui, pcb_acth_GroupPropGui, pcb_acts_GroupPropGui},
	{"LayerPropGui", pcb_act_LayerPropGui, pcb_acth_LayerPropGui, pcb_acts_LayerPropGui},
	{"Preferences", pcb_act_Preferences, pcb_acth_Preferences, pcb_acts_Preferences},
	{"pstklib", pcb_act_pstklib, pcb_acth_pstklib, pcb_acts_pstklib},
	{"UndoDialog", pcb_act_UndoDialog, pcb_acth_UndoDialog, pcb_acts_UndoDialog},
	{"ManagePlugins", pcb_act_ManagePlugins, pcb_acth_ManagePlugins, pcb_acts_ManagePlugins},
	{"dad", pcb_act_dad, pcb_acth_dad, pcb_acts_dad},
	{"DrcDialog", pcb_act_DrcDialog, pcb_acth_DrcDialog, pcb_acts_DrcDialog},
	{"IOIncompatListDialog", pcb_act_IOIncompatListDialog, pcb_acth_IOIncompatListDialog, pcb_acts_IOIncompatListDialog},
	{"Fontsel", pcb_act_Fontsel, pcb_acth_Fontsel, pcb_acts_Fontsel},

	{"gui_PromptFor", pcb_act_gui_PromptFor, pcb_acth_gui, NULL},
	{"gui_MessageBox", pcb_act_gui_MessageBox, pcb_acth_gui, NULL}
};

static const char *dialogs_cookie = "dialogs plugin";

PCB_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)

int pplg_check_ver_dialogs(int ver_needed) { return 0; }

void pplg_uninit_dialogs(void)
{
	pcb_event_unbind_allcookie(dialogs_cookie);
	pcb_dlg_undo_uninit();
	dlg_pstklib_uninit();
	pcb_dlg_pref_uninit();
	pcb_act_dad_uninit();
	pcb_remove_actions_by_cookie(dialogs_cookie);
	pcb_view_dlg_uninit();
	pcb_dialog_place_uninit();
	conf_unreg_fields("plugins/dialogs/");
}

#include "dolists.h"

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_dialogs, field,isarray,type_name,cpath,cname,desc,flags);
#include "dialogs_conf_fields.h"

int pplg_init_dialogs(void)
{
	PCB_API_CHK_VER;
	pcb_dialog_place_init();
	PCB_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)
	pcb_event_bind(PCB_EVENT_DAD_NEW_DIALOG, pcb_dialog_place, NULL, dialogs_cookie);
	pcb_event_bind(PCB_EVENT_DAD_NEW_GEO, pcb_dialog_resize, NULL, dialogs_cookie);
	pcb_act_dad_init();
	pcb_dlg_pref_init();
	dlg_pstklib_init();
	pcb_dlg_undo_init();
	pcb_view_dlg_init();
	return 0;
}
