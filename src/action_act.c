/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include <stdio.h>
#include "config.h"

#include "action_helper.h"
#include "actions.h"
#include "undo.h"
#include "compat_nls.h"
#include "safe_fs.h"

/* actions about actions
 */
/* --------------------------------------------------------------------------- */

static const char pcb_acts_ExecuteFile[] = "ExecuteFile(filename)";

static const char pcb_acth_ExecuteFile[] = "Run actions from the given file.";

/* %start-doc actions ExecuteFile

Lines starting with @code{#} are ignored.

%end-doc */

fgw_error_t pcb_act_ExecuteFile(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	FILE *fp;
	const char *fname;
	char line[256];
	int n = 0;
	char *sp;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, ExecuteFile, fname = argv[1].val.str);

	if ((fp = pcb_fopen(fname, "r")) == NULL) {
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
			/*pcb_message("%s : line %-3d : \"%s\"\n", fname, n, sp); */
			pcb_parse_actions(sp);
		}
	}

	defer_updates = 0;
	if (defer_needs_update) {
		pcb_undo_inc_serial();
		pcb_gui->invalidate_all();
	}
	fclose(fp);
	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

pcb_action_t action_action_list[] = {
	{"ExecuteFile", pcb_act_ExecuteFile, pcb_acth_ExecuteFile, pcb_acts_ExecuteFile}
};

PCB_REGISTER_ACTIONS(action_action_list, NULL)
