/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <genvector/gds_char.h>
#include "actions.h"
#include "hid_dad.h"
#include "compat_fs.h"
#include "conf_core.h"

#include "dlg_loadsave.h"

extern fgw_error_t pcb_act_LoadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv);

static char *dup_cwd(void)
{
	char tmp[PCB_PATH_MAX + 1];
	return pcb_strdup(pcb_get_wd(tmp));
}

const char pcb_acts_Load[] = "Load()\n" "Load(Layout|LayoutToBuffer|ElementToBuffer|Netlist|Revert)";
const char pcb_acth_Load[] = "Load layout data from a user-selected file.";
/* DOC: load.html */
fgw_error_t pcb_act_Load(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	static char *last_footprint = NULL, *last_layout = NULL, *last_netlist = NULL;
	const char *function = "Layout";
	char *name = NULL;

	if (last_footprint == NULL)  last_footprint = dup_cwd();
	if (last_layout == NULL)     last_layout = dup_cwd();
	if (last_netlist == NULL)    last_netlist = dup_cwd();

	/* Called with both function and file name -> no gui */
	if (argc > 2)
		return PCB_ACT_CALL_C(pcb_act_LoadFrom, res, argc, argv);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Load, function = argv[1].val.str);

	if (pcb_strcasecmp(function, "Netlist") == 0)
		name = pcb_gui->fileselect("Load netlist file", "Import netlist from file", last_netlist, ".net", NULL, "netlist", PCB_HID_FSD_READ, NULL);
	else if ((pcb_strcasecmp(function, "FootprintToBuffer") == 0) || (pcb_strcasecmp(function, "ElementToBuffer") == 0))
		name = pcb_gui->fileselect("Load footprint to buffer", "Import footprint from file", last_footprint, NULL, "footprint", NULL, PCB_HID_FSD_READ, NULL);
	else if (pcb_strcasecmp(function, "LayoutToBuffer") == 0)
		name = pcb_gui->fileselect("Load layout to buffer", "load layout (board) to buffer", last_layout, NULL, "board", NULL, PCB_HID_FSD_READ, NULL);
	else if (pcb_strcasecmp(function, "Layout") == 0)
		name = pcb_gui->fileselect("Load layout file", "load layout (board) as board to edit", last_layout, NULL, "board", NULL, PCB_HID_FSD_READ, NULL);
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid subcommand for Load(): '%s'\n", function);
		PCB_ACT_IRES(1);
		return 0;
	}

	if (name != NULL) {
		if (conf_core.rc.verbose)
			fprintf(stderr, "Load:  Calling LoadFrom(%s, %s)\n", function, name);
		pcb_actionl("LoadFrom", function, name, NULL);
		free(name);
	}

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_Save[] = "Save()\n" "Save(Layout|LayoutAs)\n" "Save(AllConnections|AllUnusedPins|ElementConnections)\n" "Save(PasteBuffer)";
const char pcb_acth_Save[] = "Save layout data to a user-selected file.";
/* DOC: save.html */
fgw_error_t pcb_act_Save(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}
