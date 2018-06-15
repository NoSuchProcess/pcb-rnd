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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* executes commands from user
 */

#include "config.h"
#include "conf_core.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "board.h"
#include "build_run.h"
#include "action_helper.h"
#include "buffer.h"
#include "command.h"
#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "rats.h"
#include "plugins.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "misc_util.h"
#include "tool.h"

/* ---------------------------------------------------------------------- */

/*  %start-doc actions 00macros

@macro colonaction

This is one of the command box helper actions.  While it is a regular
action and can be used like any other action, its name and syntax are
optimized for use with the command box (@code{:}) and thus the syntax
is documented for that purpose.

@end macro

%end-doc */

/* ---------------------------------------------------------------------- */

static const char pcb_acts_Help[] = "h";

static const char pcb_acth_Help[] = "Print a help message for commands.";

/* %start-doc actions h

@colonaction

%end-doc */

static int pcb_act_Help(int argc, const char **argv)
{
	pcb_message(PCB_MSG_INFO, "following commands are supported:\n"
					"  pcb_act_()   execute an action command (too numerous to list)\n"
					"              see the manual for the list of action commands\n"
					"  h           display this help message\n"
					"  l  [file]   load layout\n"
					"  le [file]   load element to buffer\n"
					"  m  [file]   load layout to buffer (merge)\n"
					"  q           quits the application\n"
					"  q!          quits without save warning\n"
					"  rn [file]   read in a net-list file\n"
					"  s  [file]   save layout\n" "  w  [file]   save layout\n" "  wq [file]   save layout and quit\n");
	return 0;
}

/* ---------------------------------------------------------------------- */

static const char pcb_acts_LoadLayout[] = "l [name] [format]";

static const char pcb_acth_LoadLayout[] = "Loads layout data.";

/* %start-doc actions l

Loads a new datafile (layout) and, if confirmed, overwrites any
existing unsaved data.  The filename and the searchpath
(@emph{filePath}) are passed to the command defined by
@emph{filepcb_act_}.  If no filename is specified a file select box
will popup.

@colonaction

%end-doc */

static int pcb_act_LoadLayout(int argc, const char **argv)
{
	const char *filename, *format = NULL;

	switch (argc) {
	case 2:
		format = argv[1];
	case 1: /* filename is passed in commandline */
		filename = argv[0];
		break;

	default: /* usage */
		pcb_message(PCB_MSG_ERROR, "Usage: l [name]\n  loads layout data\n");
		return 1;
	}

	if (!PCB->Changed || pcb_gui->confirm_dialog("OK to override layout data?", 0))
		pcb_load_pcb(filename, format, pcb_true, 0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_LoadElementToBuffer[] = "le [name]";

static const char pcb_acth_LoadElementToBuffer[] = "Loads an element (subcircuit, footprint) into the current buffer.";

/* %start-doc actions le

The filename and the searchpath (@emph{elementSearchPaths}) are passed to the
element loader.  If no filename is specified a file select box will popup.

@colonaction

%end-doc */

static int pcb_act_LoadElementToBuffer(int argc, const char **argv)
{
	const char *filename;

	switch (argc) {
	case 1:											/* filename is passed in commandline */
		filename = argv[0];
		if (filename && pcb_buffer_load_footprint(PCB_PASTEBUFFER, filename, NULL))
			pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
		break;

	default:											/* usage */
		pcb_message(PCB_MSG_ERROR, pcb_false, "Usage: le [name]\n  loads element (subcircuit, footprint) data to buffer\n");
		return 1;
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_LoadLayoutToBuffer[] = "m [name]";

static const char pcb_acth_LoadLayoutToBuffer[] = "Loads a layout into the current buffer.";

/* %start-doc actions m

The filename and the searchpath (@emph{filePath}) are passed to the
command defined by @emph{filepcb_act_}.
If no filename is specified a file select box will popup.

@colonaction

%end-doc */

static int pcb_act_LoadLayoutToBuffer(int argc, const char **argv)
{
	const char *filename, *format = NULL;

	switch (argc) {
	case 2:
		format = argv[1];
	case 1:  /* filename is passed in commandline */
		filename = argv[0];
		if (filename && pcb_buffer_load_layout(PCB, PCB_PASTEBUFFER, filename, format))
			pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
		break;

	default:  /* usage */
		pcb_message(PCB_MSG_ERROR, "Usage: m [name]\n  loads layout data to buffer\n");
		return 1;
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Quit[] = "q";

static const char pcb_acth_Quit[] = "Quits the application after confirming.";

/* %start-doc actions q

If you have unsaved changes, you will be prompted to confirm (or
save) before quitting.

@colonaction

%end-doc */

static int pcb_act_Quit(int argc, const char **argv)
{
	if (!PCB->Changed || pcb_gui->close_confirm_dialog() == HID_CLOSE_CONFIRM_OK)
		pcb_quit_app();
	return 0;
}

static const char pcb_acts_ReallyQuit[] = "q!";

static const char pcb_acth_ReallyQuit[] = "Quits the application without confirming.";

/* %start-doc actions q!

Note that this command neither saves your data nor prompts for
confirmation.

@colonaction

%end-doc */

static int pcb_act_ReallyQuit(int argc, const char **argv)
{
	pcb_quit_app();
	return 0;
}

/* ---------------------------------------------------------------------- */

static const char pcb_acts_LoadNetlist[] = "rn [name]";

static const char pcb_acth_LoadNetlist[] = "Reads netlist.";

/* %start-doc actions rn

If no filename is given a file select box will pop up.  The file is
read via the command defined by the @emph{Ratpcb_act_} resource. The
command must send its output to @emph{stdout}.

Netlists are used for generating rat's nests (see @ref{Rats Nest}) and
for verifying the board layout (which is also accomplished by the
@emph{Ratsnest} command).

@colonaction

%end-doc */

static int pcb_act_LoadNetlist(int argc, const char **argv)
{
	const char *filename;

	switch (argc) {
	case 1:											/* filename is passed in commandline */
		filename = argv[0];
		break;

	default:											/* usage */
		pcb_message(PCB_MSG_ERROR, "Usage: rn [name]\n  reads in a netlist file\n");
		return 1;
	}
	if (PCB->Netlistname)
		free(PCB->Netlistname);
	PCB->Netlistname = pcb_strdup_strip_wspace(filename);

	return 0;
}

/* ---------------------------------------------------------------------- */

static const char pcb_acts_SaveLayout[] = "s [name]\nw [name]";

static const char pcb_acth_SaveLayout[] = "Saves layout data.";

/* %start-doc actions s

Data and the filename are passed to the command defined by the
resource @emph{savepcb_act_}. It must read the layout data from
@emph{stdin}.  If no filename is entered, either the last one is used
again or, if it is not available, a file select box will pop up.

@colonaction

%end-doc */

/* %start-doc actions w

This commands has been added for the convenience of @code{vi} users
and has the same functionality as @code{s}.

@colonaction

%end-doc */

static int pcb_act_SaveLayout(int argc, const char **argv)
{
	switch (argc) {
	case 0:
		if (PCB->Filename) {
			if (pcb_save_pcb(PCB->Filename, NULL) == 0)
				pcb_board_set_changed_flag(pcb_false);
		}
		else
			pcb_message(PCB_MSG_ERROR, "No filename to save to yet\n");
		break;

	case 1:
		if (pcb_save_pcb(argv[0], NULL) == 0) {
			pcb_board_set_changed_flag(pcb_false);
			free(PCB->Filename);
			PCB->Filename = pcb_strdup(argv[0]);
			if (pcb_gui->notify_filename_changed != NULL)
				pcb_gui->notify_filename_changed();
		}
		break;

	default:
		pcb_message(PCB_MSG_ERROR, "Usage: s [name] | w [name]\n  saves layout data\n");
		return 1;
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_SaveLayoutAndQuit[] = "wq";

static const char pcb_acth_SaveLayoutAndQuit[] = "Saves the layout data and quits.";

/* %start-doc actions wq

This command has been added for the convenience of @code{vi} users and
has the same functionality as @code{s} combined with @code{q}.

@colonaction

%end-doc */

static int pcb_act_SaveLayoutAndQuit(int argc, const char **argv)
{
	if (!pcb_act_SaveLayout(argc, argv))
		return pcb_act_Quit(0, 0);
	return 1;
}

/* --------------------------------------------------------------------------- */

pcb_hid_action_t shand_cmd_action_list[] = {
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
	pcb_hid_remove_actions_by_cookie(shand_cmd_cookie);
}

#include "dolists.h"
int pplg_init_shand_cmd(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(shand_cmd_action_list, shand_cmd_cookie)
	return 0;
}
