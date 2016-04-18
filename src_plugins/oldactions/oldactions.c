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
#include "global.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
/* -------------------------------------------------------------------------- */

static const char dumplibrary_syntax[] = "DumpLibrary()";

static const char dumplibrary_help[] = "Display the entire contents of the libraries.";

/* %start-doc actions DumpLibrary


%end-doc */

static int ActionDumpLibrary(int argc, char **argv, Coord x, Coord y)
{
	int i, j;

	printf("**** Do not count on this format.  It will change ****\n\n");
	printf("MenuN   = %d\n", Library.MenuN);
	printf("MenuMax = %d\n", Library.MenuMax);
	for (i = 0; i < Library.MenuN; i++) {
		printf("Library #%d:\n", i);
		printf("    EntryN    = %d\n", Library.Menu[i].EntryN);
		printf("    EntryMax  = %d\n", Library.Menu[i].EntryMax);
		printf("    Name      = \"%s\"\n", UNKNOWN(Library.Menu[i].Name));
		printf("    directory = \"%s\"\n", UNKNOWN(Library.Menu[i].directory));
		printf("    Style     = \"%s\"\n", UNKNOWN(Library.Menu[i].Style));
		printf("    flag      = %d\n", Library.Menu[i].flag);

		for (j = 0; j < Library.Menu[i].EntryN; j++) {
			printf("    #%4d: ", j);
			printf("newlib: \"%s\"\n", UNKNOWN(Library.Menu[i].Entry[j].ListEntry));
		}
	}

	return 0;
}

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
static const char bell_syntax[] = "Bell()";

static const char bell_help[] = "Attempt to produce audible notification (e.g. beep the speaker).";

static int ActionBell(int argc, char **argv, Coord x, Coord y)
{
	gui->beep();
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char debug_syntax[] = "Debug(...)";

static const char debug_help[] = "Debug action.";

/* %start-doc actions Debug

This action exists to help debug scripts; it simply prints all its
arguments to stdout.

%end-doc */

static const char debugxy_syntax[] = "DebugXY(...)";

static const char debugxy_help[] = "Debug action, with coordinates";

/* %start-doc actions DebugXY

Like @code{Debug}, but requires a coordinate.  If the user hasn't yet
indicated a location on the board, the user will be prompted to click
on one.

%end-doc */

static int Debug(int argc, char **argv, Coord x, Coord y)
{
	int i;
	printf("Debug:");
	for (i = 0; i < argc; i++)
		printf(" [%d] `%s'", i, argv[i]);
	pcb_printf(" x,y %$mD\n", x, y);
	return 0;
}

static const char return_syntax[] = "Return(0|1)";

static const char return_help[] = "Simulate a passing or failing action.";

/* %start-doc actions Return

This is for testing.  If passed a 0, does nothing and succeeds.  If
passed a 1, does nothing but pretends to fail.

%end-doc */

static int Return(int argc, char **argv, Coord x, Coord y)
{
	return atoi(argv[0]);
}



HID_Action oldactions_action_list[] = {
	{"DumpLibrary", 0, ActionDumpLibrary,
	 dumplibrary_help, dumplibrary_syntax},
	{"Bell", 0, ActionBell,
	 bell_help, bell_syntax},
	{"Debug", 0, Debug,
	 debug_help, debug_syntax},
	{"DebugXY", "Click X,Y for Debug", Debug,
	 debugxy_help, debugxy_syntax},
	{"Return", 0, Return,
	 return_help, return_syntax}
};

static const char *oldactions_cookie = "oldactions plugin";

REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)

static void hid_oldactions_uninit(void)
{
	hid_remove_actions_by_cookie(oldactions_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_oldactions_init(void)
{
	REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)
	return hid_oldactions_uninit;
}

