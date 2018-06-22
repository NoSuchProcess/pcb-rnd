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
#include "config.h"
#include "actions.h"
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "error.h"
#include "find.h"
#include "compat_nls.h"

/* -------------------------------------------------------------------------- */

static const char drc_syntax[] = "DRC()";

static const char drc_help[] = "Invoke the DRC check.";

/* %start-doc actions DRC

Note that the design rule check uses the current board rule settings,
not the current style settings.

%end-doc */

static fgw_error_t pcb_act_DRCheck(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	int count;

	if (pcb_gui->drc_gui == NULL || pcb_gui->drc_gui->log_drc_overview) {
		pcb_message(PCB_MSG_INFO, _("%m+Rules are minspace %$mS, minoverlap %$mS "
							"minwidth %$mS, minsilk %$mS\n"
							"min drill %$mS, min annular ring %$mS\n"),
						conf_core.editor.grid_unit->allow, conf_core.design.bloat, conf_core.design.shrink, conf_core.design.min_wid, conf_core.design.min_slk, conf_core.design.min_drill, conf_core.design.min_ring);
	}
	count = pcb_drc_all();
	if (pcb_gui->drc_gui == NULL || pcb_gui->drc_gui->log_drc_overview) {
		if (count == 0)
			pcb_message(PCB_MSG_INFO, _("No DRC problems found.\n"));
		else if (count > 0)
			pcb_message(PCB_MSG_INFO, _("Found %d design rule errors.\n"), count);
		else
			pcb_message(PCB_MSG_INFO, _("Aborted DRC after %d design rule errors.\n"), -count);
	}
	return 0;
	PCB_OLD_ACT_END;
}

pcb_action_t find_action_list[] = {
	{"DRC", pcb_act_DRCheck, drc_help, drc_syntax}
};

PCB_REGISTER_ACTIONS(find_action_list, NULL)
