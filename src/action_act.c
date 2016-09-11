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

#include "error.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "undo.h"

/* actions about actions
 */
/* --------------------------------------------------------------------------- */

static const char executefile_syntax[] = "ExecuteFile(filename)";

static const char executefile_help[] = "Run actions from the given file.";

/* %start-doc actions ExecuteFile

Lines starting with @code{#} are ignored.

%end-doc */

int ActionExecuteFile(int argc, const char **argv, Coord x, Coord y)
{
	FILE *fp;
	const char *fname;
	char line[256];
	int n = 0;
	char *sp;

	if (argc != 1)
		AFAIL(executefile);

	fname = argv[0];

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, _("Could not open actions file \"%s\".\n"), fname);
		return 1;
	}

	defer_updates = 1;
	defer_needs_update = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		n++;
		sp = line;

		/* eat the trailing newline */
		while (*sp && *sp != '\r' && *sp != '\n')
			sp++;
		*sp = '\0';

		/* eat leading spaces and tabs */
		sp = line;
		while (*sp && (*sp == ' ' || *sp == '\t'))
			sp++;

		/*
		 * if we have anything left and its not a comment line
		 * then execute it
		 */

		if (*sp && *sp != '#') {
			/*Message ("%s : line %-3d : \"%s\"\n", fname, n, sp); */
			hid_parse_actions(sp);
		}
	}

	defer_updates = 0;
	if (defer_needs_update) {
		IncrementUndoSerialNumber();
		gui->invalidate_all();
	}
	fclose(fp);
	return 0;
}

/* --------------------------------------------------------------------------- */

HID_Action action_action_list[] = {
	{"ExecuteFile", 0, ActionExecuteFile,
	 executefile_help, executefile_syntax}
};

REGISTER_ACTIONS(action_action_list, NULL)
