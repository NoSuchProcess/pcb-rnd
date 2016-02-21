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
#include "action.h"
#include "change.h"
#include "error.h"
#include "undo.h"
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

HID_Action oldactions_action_list[] = {
	{"DumpLibrary", 0, ActionDumpLibrary,
	 dumplibrary_help, dumplibrary_syntax}
};

REGISTER_ACTIONS(oldactions_action_list)

#include "dolists.h"
void hid_oldactions_init(void)
{
	REGISTER_ACTIONS(oldactions_action_list)
}

