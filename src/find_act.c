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

#include "find.h"
#include "pcb-printf.h"
/* -------------------------------------------------------------------------- */

static const char drc_syntax[] = "DRC()";

static const char drc_help[] = "Invoke the DRC check.";

/* %start-doc actions DRC

Note that the design rule check uses the current board rule settings,
not the current style settings.

%end-doc */

static int ActionDRCheck(int argc, char **argv, Coord x, Coord y)
{
	int count;

	if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview) {
		Message(_("%m+Rules are minspace %$mS, minoverlap %$mS "
							"minwidth %$mS, minsilk %$mS\n"
							"min drill %$mS, min annular ring %$mS\n"),
						Settings.grid_unit->allow, PCB->Bloat, PCB->Shrink, PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
	}
	count = DRCAll();
	if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview) {
		if (count == 0)
			Message(_("No DRC problems found.\n"));
		else if (count > 0)
			Message(_("Found %d design rule errors.\n"), count);
		else
			Message(_("Aborted DRC after %d design rule errors.\n"), -count);
	}
	return 0;
}

HID_Action find_action_list[] = {
	{"DRC", 0, ActionDRCheck,
	 drc_help, drc_syntax}
};

REGISTER_ACTIONS(find_action_list)
