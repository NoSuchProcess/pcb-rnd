/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
#include "conf_core.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "board.h"
#include "build_run.h"
#include "buffer.h"
#include "command.h"
#include "data.h"
#include "error.h"
#include "event.h"
#include "plug_io.h"
#include "plugins.h"
#include "actions.h"
#include "compat_misc.h"
#include "misc_util.h"
#include "tool.h"

static const char pcb_acts_Help[] = "h";
static const char pcb_acth_Help[] = "Print a help message for commands.";
static fgw_error_t pcb_act_Help(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_message(PCB_MSG_INFO,
		"following shorthand commands are supported:\n"
		"  h           display this help message\n"
		"  l  [file]   load layout\n"
		"  le [file]   load element to buffer\n"
		"  m  [file]   load layout to buffer (merge)\n"
		"  q           quits the application\n"
		"  q!          quits without save warning\n"
		"  rn [file]   read in a net-list file\n"
		"  s  [file]   save layout\n"
		"  w  [file]   save layout\n"
		"  wq [file]   save layout and quit\n");
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_LoadLayout[] = "l [name] [format]";
static const char pcb_acth_LoadLayout[] = "Loads layout data.";
/* DOC: l.html */
static fgw_error_t pcb_act_LoadLayout(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *filename, *format = NULL;

	PCB_ACT_CONVARG(1, FGW_STR, LoadLayout, filename = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, LoadLayout, format = argv[2].val.str);

	if (!PCB->Changed || (pcb_hid_message_box("warning", "Load data lose", "OK to override layout data?", "cancel", 0, "ok", 1, NULL) == 1))
		pcb_load_pcb(filename, format, pcb_true, 0);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_LoadElementToBuffer[] = "le [name]";
static const char pcb_acth_LoadElementToBuffer[] = "Loads an element (subcircuit, footprint) into the current buffer.";
/* DOC: le.html */
static fgw_error_t pcb_act_LoadElementToBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *filename;

	PCB_ACT_CONVARG(1, FGW_STR, LoadElementToBuffer, filename = argv[1].val.str);

	if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, filename, NULL))
		pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_LoadLayoutToBuffer[] = "m [name]";
static const char pcb_acth_LoadLayoutToBuffer[] = "Loads a layout into the current buffer.";
/* DOC: m.html */
static fgw_error_t pcb_act_LoadLayoutToBuffer(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *filename, *format = NULL;

	PCB_ACT_CONVARG(1, FGW_STR, LoadLayoutToBuffer, filename = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, LoadLayoutToBuffer, format = argv[2].val.str);

	if (pcb_buffer_load_layout(PCB, PCB_PASTEBUFFER, filename, format))
		pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Quit[] = "q";
static const char pcb_acth_Quit[] = "Quits the application after confirming.";
/* DOC: q.html */
static fgw_error_t pcb_act_Quit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (!PCB->Changed || (pcb_hid_message_box("warning", "Close: lose data", "OK to lose data?", "cancel", 0, "ok", 1, NULL) == 1))
		pcb_quit_app();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_ReallyQuit[] = "q!";
static const char pcb_acth_ReallyQuit[] = "Quits the application without confirming.";
/* DOC: q.html */
static fgw_error_t pcb_act_ReallyQuit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_quit_app();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_LoadNetlist[] = "rn [name]";
static const char pcb_acth_LoadNetlist[] = "Reads netlist.";
/* DOC: rn.html */
static fgw_error_t pcb_act_LoadNetlist(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *filename;

	PCB_ACT_CONVARG(1, FGW_STR, LoadNetlist, filename = argv[1].val.str);

	if (PCB->Netlistname)
		free(PCB->Netlistname);
	PCB->Netlistname = pcb_strdup_strip_wspace(filename);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_SaveLayout[] = "s [name]\nw [name]";
static const char pcb_acth_SaveLayout[] = "Saves layout data.";
/* DOC: s.html w.html */
static fgw_error_t pcb_act_SaveLayout(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *filename = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, SaveLayout, filename = argv[1].val.str);

	if (filename == NULL) {
		if (PCB->hidlib.filename) {
			if (pcb_save_pcb(PCB->hidlib.filename, NULL) == 0)
				pcb_board_set_changed_flag(pcb_false);
		}
		else
			pcb_message(PCB_MSG_ERROR, "No filename to save to yet\n");
	}
	else {
		if (pcb_save_pcb(filename, NULL) == 0) {
			pcb_board_set_changed_flag(pcb_false);
			free(PCB->hidlib.filename);
			PCB->hidlib.filename = pcb_strdup(filename);
			pcb_event(&PCB->hidlib, PCB_EVENT_BOARD_FN_CHANGED, NULL);
		}
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_SaveLayoutAndQuit[] = "wq";
static const char pcb_acth_SaveLayoutAndQuit[] = "Saves the layout data and quits.";
/* DOC: wq.html */
static fgw_error_t pcb_act_SaveLayoutAndQuit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (PCB_ACT_CALL_C(pcb_act_SaveLayout, res, argc, argv) == 0)
		return pcb_act_Quit(res, argc, argv);

	PCB_ACT_IRES(1);
	return 0;
}

pcb_action_t shand_cmd_action_list[] = {
	{"h", pcb_act_Help, pcb_acth_Help, pcb_acts_Help},
	{"l", pcb_act_LoadLayout, pcb_acth_LoadLayout, pcb_acts_LoadLayout},
	{"le", pcb_act_LoadElementToBuffer, pcb_acth_LoadElementToBuffer, pcb_acts_LoadElementToBuffer},
	{"m", pcb_act_LoadLayoutToBuffer, pcb_acth_LoadLayoutToBuffer, pcb_acts_LoadLayoutToBuffer},
	{"q", pcb_act_Quit, pcb_acth_Quit, pcb_acts_Quit},
	{"q!", pcb_act_ReallyQuit, pcb_acth_ReallyQuit, pcb_acts_ReallyQuit},
	{"rn", pcb_act_LoadNetlist, pcb_acth_LoadNetlist, pcb_acts_LoadNetlist},
	{"s", pcb_act_SaveLayout, pcb_acth_SaveLayout, pcb_acts_SaveLayout},
	{"w", pcb_act_SaveLayout, pcb_acth_SaveLayout, pcb_acts_SaveLayout},
	{"wq", pcb_act_SaveLayoutAndQuit, pcb_acth_SaveLayoutAndQuit, pcb_acts_SaveLayoutAndQuit}
};

static const char *shand_cmd_cookie = "shand_cmd plugin";

PCB_REGISTER_ACTIONS(shand_cmd_action_list, shand_cmd_cookie)

int pplg_check_ver_shand_cmd(int ver_needed) { return 0; }

void pplg_uninit_shand_cmd(void)
{
	pcb_remove_actions_by_cookie(shand_cmd_cookie);
}

#include "dolists.h"
int pplg_init_shand_cmd(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(shand_cmd_action_list, shand_cmd_cookie)
	return 0;
}
