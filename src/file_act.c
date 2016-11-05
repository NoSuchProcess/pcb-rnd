/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "board.h"
#include "build_run.h"
#include "conf_core.h"
#include "data.h"
#include "action_helper.h"

#include "set.h"
#include "plug_io.h"
#include "plug_import.h"
#include "misc.h"
#include "remove.h"
#include "draw.h"
#include "find.h"
#include "search.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "compat_nls.h"

/* ---------------------------------------------------------------- */
static const char execcommand_syntax[] = "ExecCommand(command)";

static const char execcommand_help[] = "Runs a command.";

/* %start-doc actions execcommand

Runs the given command, which is a system executable.

%end-doc */

static int ActionExecCommand(int argc, const char **argv, Coord x, Coord y)
{
	const char *command;

	if (argc < 1) {
		AFAIL(execcommand);
	}

	command = ACTION_ARG(0);

	if (system(command))
		return 1;
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char loadfrom_syntax[] = "LoadFrom(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert,filename[,format])";

static const char loadfrom_help[] = "Load layout data from a file.";

/* %start-doc actions LoadFrom

This action assumes you know what the filename is.  The various GUIs
should have a similar @code{Load} action where the filename is
optional, and will provide their own file selection mechanism to let
you choose the file name.

@table @code

@item Layout
Loads an entire PCB layout, replacing the current one.

@item LayoutToBuffer
Loads an entire PCB layout to the paste buffer.

@item ElementToBuffer
Loads the given element file into the paste buffer.  Element files
contain only a single @code{Element} definition, such as the
``newlib'' library uses.

@item Netlist
Loads a new netlist, replacing any current netlist.

@item Revert
Re-loads the current layout from its disk file, reverting any changes
you may have made.

@end table

%end-doc */

static int ActionLoadFrom(int argc, const char **argv, Coord x, Coord y)
{
	const char *function, *name, *format = NULL;

	if (argc < 2)
		AFAIL(loadfrom);

	function = argv[0];
	name = argv[1];
	if (argc > 2)
		format = argv[2];

	if (strcasecmp(function, "ElementToBuffer") == 0) {
		notify_crosshair_change(pcb_false);
		if (LoadElementToBuffer(PASTEBUFFER, name))
			SetMode(PCB_MODE_PASTE_BUFFER);
		notify_crosshair_change(pcb_true);
	}

	else if (strcasecmp(function, "LayoutToBuffer") == 0) {
		notify_crosshair_change(pcb_false);
		if (LoadLayoutToBuffer(PASTEBUFFER, name, format))
			SetMode(PCB_MODE_PASTE_BUFFER);
		notify_crosshair_change(pcb_true);
	}

	else if (strcasecmp(function, "Layout") == 0) {
		if (!PCB->Changed || gui->confirm_dialog(_("OK to override layout data?"), 0))
			LoadPCB(name, format, pcb_true, 0);
	}

	else if (strcasecmp(function, "Netlist") == 0) {
		if (PCB->Netlistname)
			free(PCB->Netlistname);
		PCB->Netlistname = StripWhiteSpaceAndDup(name);
		{
			int i;
			for (i = 0; i < NUM_NETLISTS; i++)
				FreeLibraryMemory(&(PCB->NetlistLib[i]));
		}
		if (!ImportNetlist(PCB->Netlistname))
			pcb_netlist_changed(1);
	}
	else if (strcasecmp(function, "Revert") == 0 && PCB->Filename
					 && (!PCB->Changed || gui->confirm_dialog(_("OK to override changes?"), 0))) {
		RevertPCB();
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char new_syntax[] = "New([name])";

static const char new_help[] = "Starts a new layout.";

/* %start-doc actions New

If a name is not given, one is prompted for.

%end-doc */

static int ActionNew(int argc, const char **argv, Coord x, Coord y)
{
	const char *argument_name = ACTION_ARG(0);
	char *name = NULL;

	if (!PCB->Changed || gui->confirm_dialog(_("OK to clear layout data?"), 0)) {
		if (argument_name)
			name = pcb_strdup(argument_name);
		else
			name = gui->prompt_for(_("Enter the layout name:"), "");

		if (!name)
			return 1;

		notify_crosshair_change(pcb_false);
		/* do emergency saving
		 * clear the old struct and allocate memory for the new one
		 */
		if (PCB->Changed && conf_core.editor.save_in_tmp)
			SaveInTMP();
		RemovePCB(PCB);
		PCB = CreateNewPCB();
		CreateNewPCBPost(PCB, 1);

		/* setup the new name and reset some values to default */
		free(PCB->Name);
		PCB->Name = name;

		ResetStackAndVisibility();
		SetCrosshairRange(0, 0, PCB->MaxWidth, PCB->MaxHeight);
		CenterDisplay(PCB->MaxWidth / 2, PCB->MaxHeight / 2);
		Redraw();

		if (gui != NULL)
			hid_action("PCBChanged");
		notify_crosshair_change(pcb_true);
		return 0;
	}
	return 1;
}

/* --------------------------------------------------------------------------- */

static const char saveto_syntax[] =
	"SaveTo(Layout|LayoutAs,filename)\n"
	"SaveTo(AllConnections|AllUnusedPins|ElementConnections,filename)\n" "SaveTo(PasteBuffer,filename)";

static const char saveto_help[] = "Saves data to a file.";

/* %start-doc actions SaveTo

@table @code

@item Layout
Saves the current layout.

@item LayoutAs
Saves the current layout, and remembers the filename used.

@item AllConnections
Save all connections to a file.

@item AllUnusedPins
List all unused pins to a file.

@item ElementConnections
Save connections to the element at the cursor to a file.

@item PasteBuffer
Save the content of the active Buffer to a file. This is the graphical way to create a footprint.

@end table

%end-doc */

static int ActionSaveTo(int argc, const char **argv, Coord x, Coord y)
{
	const char *function;
	const char *name;
	const char *fmt = NULL;

	function = argv[0];
	name = argv[1];

	if (strcasecmp(function, "Layout") == 0) {
		if (SavePCB(PCB->Filename, NULL) == 0)
			SetChangedFlag(pcb_false);
		if (gui->notify_filename_changed != NULL)
			gui->notify_filename_changed();
		return 0;
	}

	if ((argc != 2) && (argc != 3))
		AFAIL(saveto);

	if (argc >= 3)
		fmt = argv[2];

	if (strcasecmp(function, "LayoutAs") == 0) {
		if (SavePCB(name, fmt) == 0) {
			SetChangedFlag(pcb_false);
			free(PCB->Filename);
			PCB->Filename = pcb_strdup(name);
			if (gui->notify_filename_changed != NULL)
				gui->notify_filename_changed();
		}
		return 0;
	}

	if (strcasecmp(function, "AllConnections") == 0) {
		FILE *fp;
		pcb_bool result;
		if ((fp = CheckAndOpenFile(name, pcb_true, pcb_false, &result, NULL)) != NULL) {
			LookupConnectionsToAllElements(fp);
			fclose(fp);
			SetChangedFlag(pcb_true);
		}
		return 0;
	}

	if (strcasecmp(function, "AllUnusedPins") == 0) {
		FILE *fp;
		pcb_bool result;
		if ((fp = CheckAndOpenFile(name, pcb_true, pcb_false, &result, NULL)) != NULL) {
			LookupUnusedPins(fp);
			fclose(fp);
			SetChangedFlag(pcb_true);
		}
		return 0;
	}

	if (strcasecmp(function, "ElementConnections") == 0) {
		ElementTypePtr element;
		void *ptrtmp;
		FILE *fp;
		pcb_bool result;

		if ((SearchScreen(Crosshair.X, Crosshair.Y, PCB_TYPE_ELEMENT, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_TYPE_NONE) {
			element = (ElementTypePtr) ptrtmp;
			if ((fp = CheckAndOpenFile(name, pcb_true, pcb_false, &result, NULL)) != NULL) {
				LookupElementConnections(element, fp);
				fclose(fp);
				SetChangedFlag(pcb_true);
			}
		}
		return 0;
	}

	if (strcasecmp(function, "PasteBuffer") == 0) {
		return SaveBufferElements(name, fmt);
	}

	AFAIL(saveto);
}

/* --------------------------------------------------------------------------- */

static const char quit_syntax[] = "Quit()";

static const char quit_help[] = "Quits the application after confirming.";

/* %start-doc actions Quit

If you have unsaved changes, you will be prompted to confirm (or
save) before quitting.

%end-doc */

static int ActionQuit(int argc, const char **argv, Coord x, Coord y)
{
	const char *force = ACTION_ARG(0);
	if (force && strcasecmp(force, "force") == 0) {
		PCB->Changed = 0;
		exit(0);
	}
	if (!PCB->Changed || gui->close_confirm_dialog() == HID_CLOSE_CONFIRM_OK)
		QuitApplication();
	return 1;
}


HID_Action file_action_list[] = {
	{"ExecCommand", 0, ActionExecCommand,
	 execcommand_help, execcommand_syntax}
	,
	{"LoadFrom", 0, ActionLoadFrom,
	 loadfrom_help, loadfrom_syntax}
	,
	{"New", 0, ActionNew,
	 new_help, new_syntax}
	,
	{"SaveTo", 0, ActionSaveTo,
	 saveto_help, saveto_syntax}
	,
	{"Quit", 0, ActionQuit,
	 quit_help, quit_syntax}
};

REGISTER_ACTIONS(file_action_list, NULL)
