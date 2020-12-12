/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017..2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/plugins.h>
#include "funchash_core.h"
#include <librnd/plugins/lib_hid_common/dialogs_conf.h>

/* from lib_hid_common */
extern conf_dialogs_t dialogs_conf;

/* include them all for static inlines */
#include "dlg_test.c"
#include "dlg_about.h"
#include "dlg_export.h"
#include "dlg_flag_edit.h"
#include "dlg_fontsel.h"
#include "dlg_infobar.h"
#include "dlg_layer_binding.h"
#include "dlg_layer_flags.h"
#include "dlg_lib_pstk.h"
#include "dlg_library.h"
#include "dlg_loadsave.h"
#include "dlg_padstack.h"
#include "dlg_pinout.c"
#include "dlg_undo.c"
#include "dlg_fpmap.c"
#include "dlg_obj_list.c"
#include "dlg_netlist.c"
#include "dlg_plugins.c"
#include "dlg_printcalib.c"

#include "dlg_view.h"
#include "dlg_pref.h"

#include "adialogs_conf.h"
#include "conf_internal.c"

conf_adialogs_t adialogs_conf;
#define ADIALOGS_CONF_FN "adialogs.conf"

extern const char pcb_acts_dlg_confval_edit[];
extern const char pcb_acth_dlg_confval_edit[];
extern fgw_error_t pcb_act_dlg_confval_edit(fgw_arg_t *res, int argc, fgw_arg_t *argv);


rnd_action_t dialogs_action_list[] = {
	{"dlg_test", pcb_act_dlg_test, dlg_test_help, dlg_test_syntax},
	{"dlg_confval_edit", pcb_act_dlg_confval_edit, pcb_acth_dlg_confval_edit, pcb_acts_dlg_confval_edit},
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
	{"NetlistDialog", pcb_act_NetlistDialog, pcb_acth_NetlistDialog, pcb_acts_NetlistDialog},
	{"ManagePlugins", pcb_act_ManagePlugins, pcb_acth_ManagePlugins, pcb_acts_ManagePlugins},
	{"DrcDialog", pcb_act_DrcDialog, pcb_acth_DrcDialog, pcb_acts_DrcDialog},
	{"IOIncompatListDialog", pcb_act_IOIncompatListDialog, pcb_acth_IOIncompatListDialog, pcb_acts_IOIncompatListDialog},
	{"ViewList", pcb_act_ViewList, pcb_acth_ViewList, pcb_acts_ViewList},
	{"Fontsel", pcb_act_Fontsel, pcb_acth_Fontsel, pcb_acts_Fontsel},
	{"PrintCalibrate", pcb_act_PrintCalibrate, pcb_acth_PrintCalibrate, pcb_acts_PrintCalibrate},
	{"Load", pcb_act_Load, pcb_acth_Load, pcb_acts_Load},
	{"Save", pcb_act_Save, pcb_acth_Save, pcb_acts_Save},
	{"LibraryDialog", pcb_act_LibraryDialog, pcb_acth_LibraryDialog, pcb_acts_LibraryDialog},
	{"InfoBarFileChanged", pcb_act_InfoBarFileChanged, pcb_acth_InfoBarFileChanged, pcb_acts_InfoBarFileChanged},
	{"dlg_obj_list", pcb_act_dlg_obj_list, pcb_acth_dlg_obj_list, pcb_acts_dlg_obj_list},
	{"gui_fpmap_choose", pcb_act_gui_fpmap_choose, pcb_acth_gui_fpmap_choose, pcb_acts_gui_fpmap_choose}
};

static const char *dialogs_cookie = "dialogs plugin";

int pplg_check_ver_dialogs(int ver_needed) { return 0; }

void pplg_uninit_dialogs(void)
{
	pcb_dlg_library_uninit();
	pcb_dlg_netlist_uninit();
	pcb_dlg_undo_uninit();
	pcb_dlg_pstklib_uninit();
	pcb_dlg_pref_uninit();
	rnd_remove_actions_by_cookie(dialogs_cookie);
	pcb_view_dlg_uninit();
	pcb_dlg_fontsel_uninit();

	rnd_conf_unreg_file(ADIALOGS_CONF_FN, adialogs_conf_internal);
	rnd_conf_unreg_fields("plugins/dialogs/");
}

int pplg_init_dialogs(void)
{
	RND_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(adialogs_conf, field,isarray,type_name,cpath,cname,desc,flags);
#include "adialogs_conf_fields.h"

	rnd_conf_reg_file(ADIALOGS_CONF_FN, adialogs_conf_internal);

	RND_REGISTER_ACTIONS(dialogs_action_list, dialogs_cookie)
	pcb_dlg_pref_init();
	pcb_dlg_pstklib_init();
	pcb_dlg_undo_init();
	pcb_dlg_netlist_init();
	pcb_view_dlg_init();
	pcb_dlg_fontsel_init();
	pcb_dlg_library_init();

	return 0;
}
