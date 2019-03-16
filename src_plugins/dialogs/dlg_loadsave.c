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
#include "board.h"
#include "data.h"
#include "event.h"
#include "hid_dad.h"
#include "compat_fs.h"
#include "conf_core.h"
#include "plug_io.h"

#include "dlg_loadsave.h"

extern fgw_error_t pcb_act_LoadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv);
extern fgw_error_t pcb_act_SaveTo(fgw_arg_t *res, int argc, fgw_arg_t *argv);

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
		name = pcb_gui->fileselect("Load footprint to buffer", "Import footprint from file", last_footprint, NULL, NULL, "footprint", PCB_HID_FSD_READ, NULL);
	else if (pcb_strcasecmp(function, "LayoutToBuffer") == 0)
		name = pcb_gui->fileselect("Load layout to buffer", "load layout (board) to buffer", last_layout, NULL, NULL, "board", PCB_HID_FSD_READ, NULL);
	else if (pcb_strcasecmp(function, "Layout") == 0)
		name = pcb_gui->fileselect("Load layout file", "load layout (board) as board to edit", last_layout, NULL, NULL, "board", PCB_HID_FSD_READ, NULL);
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

/*** Save ***/

typedef struct {
	pcb_hid_dad_subdialog_t *fmtsub;
	pcb_io_formats_t *avail;
	int pick;
} save_t;

static void fmt_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_subdialog_t *fmtsub = caller_data;
	save_t *save = fmtsub->sub_ctx;
	char *bn, *fn, *s;
	const char *ext;
	pcb_event_arg_t res, argv[4];
	int selection = attr->default_val.int_value;;

	if ((save->avail == NULL) || (save->avail->extension == NULL))
		return;

	if (fmtsub->parent_poke(fmtsub, "get_path", &res, 0, NULL) != 0)
		return;
	fn = (char *)res.d.s;

	/* find and truncate extension */
	for (s = fn + strlen(fn) - 1; *s != '.'; s--) {
		if ((s <= fn) || (*s == PCB_DIR_SEPARATOR_C)) {
			free(fn);
			return;
		}
	}
	*s = '\0';

	/* calculate basename in bn */
	bn = strrchr(fn, PCB_DIR_SEPARATOR_C);
	if (bn == NULL)
		bn = fn;
	else
		bn++;

	/* fetch the desired extension */
	ext = save->avail->extension[selection];
	if (ext == NULL)
		ext = ".";

	/* build a new file name with the right extension */

	argv[0].type = PCB_EVARG_STR;
	argv[0].d.s = pcb_concat(bn, ext, NULL);;
	fmtsub->parent_poke(fmtsub, "set_file_name", &res, 1, argv);
	free(fn);

	/* remember the selection for the save action */
	save->pick = selection;
}


static void setup_fmt_sub(save_t *save)
{
	PCB_DAD_BEGIN_VBOX(save->fmtsub->dlg);
		PCB_DAD_BEGIN_HBOX(save->fmtsub->dlg);
			PCB_DAD_LABEL(save->fmtsub->dlg, "File format:");
			PCB_DAD_ENUM(save->fmtsub->dlg, (const char **)save->avail->digest);
				PCB_DAD_DEFAULT_NUM(save->fmtsub->dlg, save->pick);
				PCB_DAD_CHANGE_CB(save->fmtsub->dlg, fmt_chg);
		PCB_DAD_END(save->fmtsub->dlg);
	PCB_DAD_END(save->fmtsub->dlg);
}

const char pcb_acts_Save[] = "Save()\n" "Save(Layout|LayoutAs)\n" "Save(AllConnections|AllUnusedPins|ElementConnections)\n" "Save(PasteBuffer)";
const char pcb_acth_Save[] = "Save layout data to a user-selected file.";
/* DOC: save.html */
fgw_error_t pcb_act_Save(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function = "Layout";
	static char *cwd = NULL;
	pcb_hid_dad_subdialog_t *fmtsub = NULL, fmtsub_local;
	char *final_name, *name_in = NULL;
	const char *prompt;
	pcb_io_formats_t avail;
	const char **extensions_param = NULL;
	int fmt, *fmt_param = NULL;
	save_t save;

	if (cwd == NULL) cwd = dup_cwd();

	if (argc > 2)
		return PCB_ACT_CALL_C(pcb_act_SaveTo, res, argc, argv);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, Save, function = argv[1].val.str);

	if (pcb_strcasecmp(function, "Layout") == 0)
		if (PCB->Filename != NULL)
			return pcb_actionl("SaveTo", "Layout", NULL);

	if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
		int num_fmts, n;
		prompt = "Save element as";
		num_fmts = pcb_io_list(&avail, PCB_IOT_BUFFER, 1, 1, PCB_IOL_EXT_FP);
		if (num_fmts > 0) {
			const char *default_pattern = conf_core.rc.save_fp_fmt;
			extensions_param = (const char **)avail.extension;
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
			PCB_ACT_IRES(-1);
			return 0;
		}
	}
	else {
		int num_fmts, n;
		prompt = "Save layout as";
		num_fmts = pcb_io_list(&avail, PCB_IOT_PCB, 1, 1, PCB_IOL_EXT_BOARD);
		if (num_fmts > 0) {
			extensions_param = (const char **)avail.extension;
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
			fmtsub = &fmtsub_local;
			memset(&fmtsub_local, 0, sizeof(fmtsub_local));
			save.avail = &avail;
			save.fmtsub = fmtsub;
			save.pick = fmt;
			fmtsub->sub_ctx = &save;
			setup_fmt_sub(&save);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Error: no IO plugin avaialble for saving a buffer.");
			PCB_ACT_IRES(-1);
			return 0;
		}
	}

	/* construct the input file name and run a file selection dialog to get the final file name */
	if (name_in == NULL) {
		if (PCB->Filename == NULL)
			name_in = pcb_concat("unnamed", extensions_param[fmt], NULL);
		else
			name_in = pcb_strdup(PCB->Filename);
	}
	final_name = pcb_gui->fileselect(prompt, NULL, name_in, NULL, NULL, "board", PCB_HID_FSD_MAY_NOT_EXIST, fmtsub);
	free(name_in);

	if (final_name == NULL) { /* cancel */
		pcb_io_list_free(&avail);
		PCB_ACT_IRES(1);
		return 0;
	}

	if (conf_core.rc.verbose)
		fprintf(stderr, "Save:  Calling SaveTo(%s, %s)\n", function, final_name);

	if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
		pcb_actionl("PasteBuffer", "Save", final_name, avail.plug[fmt]->description, "1", NULL);
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
			sfmt = avail.plug[save.pick]->description;
		if (pcb_strcasecmp(function, "Layout") == 0)
			pcb_actionl("SaveTo", "LayoutAs", final_name, sfmt, NULL);
		else
			pcb_actionl("SaveTo", function, final_name, sfmt, NULL);
	}

	free(final_name);
	pcb_io_list_free(&avail);
	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_ImportGUI[] = "ImportGUI()";
const char pcb_acth_ImportGUI[] = "Asks user which schematics to import into PCB.\n";
/* DOC: importgui.html */
fgw_error_t pcb_act_ImportGUI(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *name;
	static char *cwd = NULL;
	static int lock = 0;
	int rv = 0;

	if (cwd == NULL)
		cwd = dup_cwd();

	if (lock)
		return 1;

	name = pcb_gui->fileselect("Load schematics", "Import netlist and footprints from schematics", cwd, NULL, NULL, "schematics", PCB_HID_FSD_MAY_NOT_EXIST, NULL);
	if (name != NULL) {
		pcb_attrib_put(PCB, "import::src0", name);
		free(name);

		lock = 1;
		rv = pcb_action("Import");
		lock = 0;
	}

	PCB_ACT_IRES(rv);
	return 0;
}
