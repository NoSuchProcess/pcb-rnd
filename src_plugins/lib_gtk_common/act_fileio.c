/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <glib.h>
#include <unistd.h>

#include "act_fileio.h"

#include "actions.h"
#include "conf_core.h"
#include "plug_footprint.h"
#include "compat_misc.h"
#include "conf_core.h"
#include "board.h"
#include "data.h"

#include "dlg_file_chooser.h"




static char *dup_cwd()
{
#if defined (PATH_MAX)
	char tmp[PATH_MAX + 1];
#else
	char tmp[8193];
#endif
	return pcb_strdup(getcwd(tmp, sizeof(tmp)));
}

const char pcb_gtk_acts_importgui[] = "ImportGUI()";
const char pcb_gtk_acth_importgui[] = "Asks user which schematics to import into PCB.\n";
/* DOC: importgui.html */
fgw_error_t pcb_gtk_act_importgui(GtkWidget *top_window, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *name = NULL;
	static gchar *current_layout_dir = NULL;
	static int I_am_recursing = 0;
	int rv;

	if (!current_layout_dir)
		current_layout_dir = dup_cwd();

	if (I_am_recursing)
		return 1;


	name = ghid_dialog_file_select_open(top_window, "Load schematics", &current_layout_dir, conf_core.rc.file_path);

#ifdef DEBUG
	printf("File selected = %s\n", name);
#endif

	pcb_attrib_put(PCB, "import::src0", name);
	free(name);

	I_am_recursing = 1;
	rv = pcb_action("Import");
	I_am_recursing = 0;

	return rv;
}
