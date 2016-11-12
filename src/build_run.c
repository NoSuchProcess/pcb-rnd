/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include <genvector/gds_char.h>
#include "conf_core.h"
#include "board.h"
#include "build_run.h"
#include "hid_init.h"
#include "plug_io.h"
#include "compat_misc.h"

/* ---------------------------------------------------------------------------
 * quits application
 */
extern void pcb_main_uninit(void);
void QuitApplication(void)
{
	/*
	 * save data if necessary.  It not needed, then don't trigger EmergencySave
	 * via our atexit() registering of EmergencySave().  We presumably wanted to
	 * exit here and thus it is not an emergency.
	 */
	if (PCB->Changed && conf_core.editor.save_in_tmp)
		EmergencySave();
	else
		DisableEmergencySave();

	if (gui->do_exit == NULL) {
		pcb_main_uninit();
		exit(0);
	}
	else
		gui->do_exit(gui);
}

/* ---------------------------------------------------------------------------
 * Returns a string that has a bunch of information about the program.
 * Can be used for things like "about" dialog boxes.
 */

char *GetInfoString(void)
{
	pcb_hid_t **hids;
	int i;
	static gds_t info;
	static int first_time = 1;

#define TAB "    "

	if (first_time) {
		first_time = 0;
		gds_init(&info);
		gds_append_str(&info, "This is PCB-rnd " VERSION " (" REVISION ")" "\n an interactive\n");
		gds_append_str(&info, "printed circuit board editor\n");
		gds_append_str(&info, "PCB-rnd forked from gEDA/PCB.");
		gds_append_str(&info, "\n\n" "PCB is by harry eaton and others\n\n");
		gds_append_str(&info, "\nPCB-rnd adds a collection of\n");
		gds_append_str(&info, "useful-looking random patches.\n");
		gds_append_str(&info, "\n");
		gds_append_str(&info, "Copyright (C) Thomas Nau 1994, 1995, 1996, 1997\n");
		gds_append_str(&info, "Copyright (C) harry eaton 1998-2007\n");
		gds_append_str(&info, "Copyright (C) C. Scott Ananian 2001\n");
		gds_append_str(&info, "Copyright (C) DJ Delorie 2003, 2004, 2005, 2006, 2007, 2008\n");
		gds_append_str(&info, "Copyright (C) Dan McMahill 2003, 2004, 2005, 2006, 2007, 2008\n\n");
		gds_append_str(&info, "Copyright (C) Tibor Palinkas 2013-2016 (pcb-rnd patches)\n\n");
		gds_append_str(&info, "It is licensed under the terms of the GNU\n");
		gds_append_str(&info, "General Public License version 2\n");
		gds_append_str(&info, "See the LICENSE file for more information\n\n");
		gds_append_str(&info, "For more information see:\n\n");
		gds_append_str(&info, "PCB-rnd homepage: http://repo.hu/projects/pcb-rnd\n");
		gds_append_str(&info, "PCB homepage: http://pcb.geda-project.org\n");
		gds_append_str(&info, "gEDA homepage: http://www.geda-project.org\n");
		gds_append_str(&info, "gEDA Wiki: http://wiki.geda-project.org\n\n");

		gds_append_str(&info, "----- Compile Time Options -----\n");
		hids = hid_enumerate();
		gds_append_str(&info, "GUI:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->gui) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}

		gds_append_str(&info, "Exporters:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->exporter) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}

		gds_append_str(&info, "Printers:\n");
		for (i = 0; hids[i]; i++) {
			if (hids[i]->printer) {
				gds_append_str(&info, TAB);
				gds_append_str(&info, hids[i]->name);
				gds_append_str(&info, " : ");
				gds_append_str(&info, hids[i]->description);
				gds_append_str(&info, "\n");
			}
		}
	}
#undef TAB
	return info.array;
}

const char *pcb_author(void)
{
	if (conf_core.design.fab_author && conf_core.design.fab_author[0])
		return conf_core.design.fab_author;
	else
		return get_user_name();
}

