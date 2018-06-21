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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <glib.h>
#include <unistd.h>

#include "act_fileio.h"

#include "actions.h"
#include "conf_core.h"
#include "compat_nls.h"
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

int pcb_gtk_act_load(GtkWidget *top_window, int argc, const char **argv)
{
	const char *function;
	char *name = NULL;

	static gchar *current_element_dir = NULL;
	static gchar *current_layout_dir = NULL;
	static gchar *current_netlist_dir = NULL;

	if (!current_element_dir)
		current_element_dir = dup_cwd();
	if (!current_layout_dir)
		current_layout_dir = dup_cwd();
	if (!current_netlist_dir)
		current_netlist_dir = dup_cwd();

	/* we've been given the file name */
	if (argc > 1)
		return pcb_actionv("LoadFrom", argc, argv);

	function = argc ? argv[0] : "Layout";

	if (pcb_strcasecmp(function, "Netlist") == 0) {
		name = ghid_dialog_file_select_open(top_window, _("Load netlist file"), &current_netlist_dir, conf_core.rc.file_path);
	}
	else if (pcb_strcasecmp(function, "ElementToBuffer") == 0) {
		gchar *path = (gchar *) pcb_fp_default_search_path();
		name = ghid_dialog_file_select_open(top_window, _("Load element to buffer"), &current_element_dir, path);
	}
	else if (pcb_strcasecmp(function, "LayoutToBuffer") == 0) {
		name = ghid_dialog_file_select_open(top_window, _("Load layout file to buffer"), &current_layout_dir, conf_core.rc.file_path);
	}
	else if (pcb_strcasecmp(function, "Layout") == 0) {
		name = ghid_dialog_file_select_open(top_window, _("Load layout file"), &current_layout_dir, conf_core.rc.file_path);
	}

	if (name) {
		if (conf_core.rc.verbose)
			fprintf(stderr, "Load:  Calling LoadFrom(%s, %s)\n", function, name);
		pcb_actionl("LoadFrom", function, name, NULL);
		g_free(name);
	}

	return 0;
}

#warning TODO: this should be more or less common with lesstif
const char pcb_gtk_acts_save[] = "Save()\n" "Save(Layout|LayoutAs)\n" "Save(AllConnections|AllUnusedPins|ElementConnections)\n" "Save(PasteBuffer)";
const char pcb_gtk_acth_save[] = N_("Save layout and/or element data to a user-selected file.");

/* %start-doc actions Save

This action is a GUI front-end to the core's @code{SaveTo} action
(@pxref{SaveTo Action}).  If you happen to pass a filename, like
@code{SaveTo}, then @code{SaveTo} is called directly.  Else, the
user is prompted for a filename to save, and then @code{SaveTo} is
called with that filename.

%end-doc */

int pcb_gtk_act_save(GtkWidget *top_window, int argc, const char **argv)
{
	const char *function;
	char *name, *name_in = NULL;
	const char *prompt;
	pcb_io_formats_t avail;
	const char **formats_param = NULL, **extensions_param = NULL;
	int fmt, *fmt_param = NULL;

	static gchar *current_dir = NULL;

	if (!current_dir)
		current_dir = dup_cwd();

	if (argc > 1)
		return pcb_actionv("SaveTo", argc, argv);

	function = argc ? argv[0] : "Layout";

	if (pcb_strcasecmp(function, "Layout") == 0)
		if (PCB->Filename)
			return pcb_actionl("SaveTo", "Layout", NULL);

	if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
		int num_fmts, n;
		prompt = _("Save element as");
		num_fmts = pcb_io_list(&avail, PCB_IOT_BUFFER, 1, 1, PCB_IOL_EXT_FP);
		if (num_fmts > 0) {
			const char *default_pattern = conf_core.rc.save_fp_fmt;
			formats_param = (const char **) avail.digest;
			extensions_param = (const char **) avail.extension;
			fmt_param = &fmt;
			fmt = -1;

			if (default_pattern != NULL) {
				/* look for exact match, case sensitive */
				for (n = 0; n < num_fmts; n++)
					if (strcmp(avail.plug[n]->description, default_pattern) == 0)
						fmt = n;

				/* look for exact match, case insensitive */
				if (fmt < 0)
					for (n = 0; n < num_fmts; n++)
						if (pcb_strcasecmp(avail.plug[n]->description, default_pattern) == 0)
							fmt = n;

				/* look for partial match */
				if (fmt < 0) {
					for (n = 0; n < num_fmts; n++) {
						if (strstr(avail.plug[n]->description, default_pattern) != NULL) {
							fmt = n;
							break; /* pick the first one, that has the highest prio */
						}
					}
				}

				if (fmt < 0) {
					static int warned = 0;
					if (!warned)
						pcb_message(PCB_MSG_WARNING, "Could not find an io_ plugin for the preferred footprint save format (configured in rc/save_fp_fmt): '%s'\n", default_pattern);
					warned = 1;
				}
			}

			if (fmt < 0) /* fallback: choose the frist format */
				fmt = 0;

			name_in = pcb_concat("unnamed", avail.plug[fmt]->fp_extension, NULL);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Error: no IO plugin avaialble for saving a buffer.");
			return -1;
		}
	}
	else {
		int num_fmts, n;
		prompt = _("Save layout as");
		num_fmts = pcb_io_list(&avail, PCB_IOT_PCB, 1, 1, PCB_IOL_EXT_BOARD);
		if (num_fmts > 0) {
			formats_param = (const char **) avail.digest;
			extensions_param = (const char **) avail.extension;
			fmt_param = &fmt;
			fmt = 0;
			if (PCB->Data->loader != NULL) {
				for (n = 0; n < num_fmts; n++) {
					if (avail.plug[n] == PCB->Data->loader) {
						fmt = n;
						break;
					}
				}
			}
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Error: no IO plugin avaialble for saving a buffer.");
			return -1;
		}
	}

	{															/* invent an input file name if needed and run the save dialog to get an output file name */
		if (name_in == NULL) {
			if (PCB->Filename == NULL)
				name_in = pcb_concat("unnamed", extensions_param[fmt], NULL);
			else
				name_in = pcb_strdup(PCB->Filename);
		}
		name = ghid_dialog_file_select_save(top_window, prompt, &current_dir, name_in, conf_core.rc.file_path, formats_param, extensions_param, fmt_param);
		free(name_in);
	}

	if (name) {
		if (conf_core.rc.verbose)
			fprintf(stderr, "Save:  Calling SaveTo(%s, %s)\n", function, name);

		if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
			pcb_actionl("PasteBuffer", "Save", name, avail.plug[fmt]->description, "1", NULL);

		}
		else {
			const char *sfmt = NULL;
			/*
			 * if we got this far and the function is Layout, then
			 * we really needed it to be a LayoutAs.  Otherwise
			 * ActionSaveTo() will ignore the new file name we
			 * just obtained.
			 */
			if (fmt_param != NULL)
				sfmt = avail.plug[fmt]->description;
			if (pcb_strcasecmp(function, "Layout") == 0)
				pcb_actionl("SaveTo", "LayoutAs", name, sfmt, NULL);
			else
				pcb_actionl("SaveTo", function, name, sfmt, NULL);
		}
		g_free(name);
		pcb_io_list_free(&avail);
	}
	else {
		pcb_io_list_free(&avail);
		return 1;
	}

	return 0;
}

const char pcb_gtk_acts_importgui[] = "ImportGUI()";
const char pcb_gtk_acth_importgui[] = N_("Asks user which schematics to import into PCB.\n");
int pcb_gtk_act_importgui(GtkWidget *top_window, int argc, const char **argv)
{
	char *name = NULL;
	static gchar *current_layout_dir = NULL;
	static int I_am_recursing = 0;
	int rv;

	if (!current_layout_dir)
		current_layout_dir = dup_cwd();

	if (I_am_recursing)
		return 1;


	name = ghid_dialog_file_select_open(top_window, _("Load schematics"), &current_layout_dir, conf_core.rc.file_path);

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
