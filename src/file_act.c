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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "board.h"
#include "build_run.h"
#include "conf_core.h"
#include "funchash_core.h"
#include "data.h"
#include "buffer.h"

#include "plug_io.h"
#include "plug_import.h"
#include "remove.h"
#include "draw.h"
#include "event.h"
#include "find.h"
#include "search.h"
#include "actions.h"
#include "compat_misc.h"
#include "hid_init.h"
#include "layer_vis.h"
#include "safe_fs.h"
#include "tool.h"
#include "netlist2.h"


static const char pcb_acts_LoadFrom[] = "LoadFrom(Layout|LayoutToBuffer|SubcToBuffer|Netlist|Revert,filename[,format])";
static const char pcb_acth_LoadFrom[] = "Load layout data from a file.";
/* DOC: loadfrom.html */
fgw_error_t pcb_act_LoadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *name, *format = NULL;
	int op;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, LoadFrom, op = fgw_keyword(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_STR, LoadFrom, name = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LoadFrom, format = argv[3].val.str);

	switch(op) {
		case F_ElementToBuffer:
		case F_FootprintToBuffer:
		case F_Element:
		case F_SubcToBuffer:
		case F_Subcircuit:
		case F_Footprint:
			pcb_notify_crosshair_change(pcb_false);
			if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, name, format))
				pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_LayoutToBuffer:
			pcb_notify_crosshair_change(pcb_false);
			if (pcb_buffer_load_layout(PCB, PCB_PASTEBUFFER, name, format))
				pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_PASTE_BUFFER);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_Layout:
			if (!PCB->Changed ||  pcb_hid_message_box("warning", "File overwrite", "OK to override layout data?", "cancel", 0, "ok", 1, NULL))
				pcb_load_pcb(name, format, pcb_true, 0);
			break;

		case F_Netlist:
			if (PCB->Netlistname)
				free(PCB->Netlistname);
			PCB->Netlistname = pcb_strdup_strip_wspace(name);
			{
				int i;
				for (i = 0; i < PCB_NUM_NETLISTS; i++) {
					pcb_netlist_uninit(&(PCB->netlist[i]));
					pcb_netlist_init(&(PCB->netlist[i]));
				}
			}
			if (!pcb_import_netlist(&PCB->hidlib, PCB->Netlistname))
				pcb_netlist_changed(1);
			break;

		case F_Revert:
			if (PCB->hidlib.filename && (!PCB->Changed || (pcb_hid_message_box("warning", "Revert: lose data", "Really revert all modifications?", "no", 0, "yes", 1, NULL) == 1)))
				pcb_revert_pcb();
			break;

		default:
			pcb_message(PCB_MSG_ERROR, "LoadFrom(): invalid command (first arg)\n");
			PCB_ACT_IRES(1);
			return 0;
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_New[] = "New([name])";
static const char pcb_acth_New[] = "Starts a new layout.";
/* DOC: new.html */
static fgw_error_t pcb_act_New(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *argument_name = NULL;
	char *name = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, New, argument_name = argv[1].val.str);

	if (!PCB->Changed || (pcb_hid_message_box("warning", "New pcb", "OK to clear layout data?", "cancel", 0, "yes", 1, NULL) == 1)) {
		if (argument_name)
			name = pcb_strdup(argument_name);
		else
			name = pcb_hid_prompt_for("Enter the layout name:", "", "Layout name");

		if (!name)
			return 1;

		pcb_notify_crosshair_change(pcb_false);
		/* do emergency saving
		 * clear the old struct and allocate memory for the new one
		 */
		if (PCB->Changed && conf_core.editor.save_in_tmp)
			pcb_save_in_tmp();
		if (pcb_gui->set_hidlib != NULL)
			pcb_gui->set_hidlib(pcb_gui, NULL);
		pcb_board_remove(PCB);
		PCB = pcb_board_new(1);
		pcb_board_new_postproc(PCB, 1);
		pcb_set_design_dir(NULL);
		conf_set(CFR_DESIGN, "design/text_font_id", 0, "0", POL_OVERWRITE); /* we have only one font now, make sure it is selected */

		/* setup the new name and reset some values to default */
		free(PCB->hidlib.name);
		PCB->hidlib.name = name;

		pcb_layervis_reset_stack();
		pcb_crosshair_set_range(0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y);
		pcb_center_display(PCB->hidlib.size_x / 2, PCB->hidlib.size_y / 2);
		pcb_hid_redraw(PCB);
		pcb_board_changed(0);
		pcb_notify_crosshair_change(pcb_true);
		PCB_ACT_IRES(0);
		return 0;
	}
	PCB_ACT_IRES(-1);
	return 0;
}

/* --------------------------------------------------------------------------- */
static const char pcb_acts_normalize[] = "Normalize()";
static const char pcb_acth_normalize[] = "Move all objects within the drawing area, align the drawing to 0;0";
static fgw_error_t pcb_act_normalize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(pcb_board_normalize(PCB));
	return 0;
}


static const char pcb_acts_SaveTo[] =
	"SaveTo(Layout|LayoutAs,filename,[fmt])\n"
	"SaveTo(PasteBuffer,filename,[fmt])";
static const char pcb_acth_SaveTo[] = "Saves data to a file.";
/* DOC: saveto.html */
fgw_error_t pcb_act_SaveTo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	const char *name = NULL;
	const char *fmt = NULL;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, SaveTo, op = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_STR, SaveTo, name = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, SaveTo, fmt = argv[3].val.str);
	PCB_ACT_IRES(0);

	if ((op != F_Layout) && (name == NULL))
		PCB_ACT_FAIL(SaveTo);

	switch(op) {
		case F_Layout:
			if (argc != 2) {
				pcb_message(PCB_MSG_ERROR, "SaveTo(Layout) doesn't take file name or format - did you mean SaveTo(LayoutAs)?\n");
				return FGW_ERR_ARGC;
			}
			if (pcb_save_pcb(PCB->hidlib.filename, NULL) == 0)
				pcb_board_set_changed_flag(pcb_false);
			pcb_event(&PCB->hidlib, PCB_EVENT_BOARD_FN_CHANGED, NULL);
			return 0;

		case F_LayoutAs:
			if (pcb_save_pcb(name, fmt) == 0) {
				pcb_board_set_changed_flag(pcb_false);
				free(PCB->hidlib.filename);
				PCB->hidlib.filename = pcb_strdup(name);
				pcb_event(&PCB->hidlib, PCB_EVENT_BOARD_FN_CHANGED, NULL);
			}
			return 0;


		case F_PasteBuffer:
			PCB_ACT_IRES(pcb_save_buffer_elements(name, fmt));
			return 0;

		/* shorthand kept only for compatibility reasons - do not use */
		case F_AllConnections:
			pcb_message(PCB_MSG_WARNING, "Please use action ExportOldConn() instead of SaveTo() for connections.\n");
			return pcb_actionl("ExportOldConn", "AllConnections", name, NULL);
		case F_AllUnusedPins:
			pcb_message(PCB_MSG_WARNING, "Please use action ExportOldConn() instead of SaveTo() for connections.\n");
			return pcb_actionl("ExportOldConn", "AllUnusedPins", name, NULL);
		case F_ElementConnections:
		case F_SubcConnections:
			pcb_message(PCB_MSG_WARNING, "Please use action ExportOldConn() instead of SaveTo() for connections.\n");
			return pcb_actionl("ExportOldConn", "SubcConnections", name, NULL);
	}

	PCB_ACT_FAIL(SaveTo);
}

static const char pcb_acts_Quit[] = "Quit()";
static const char pcb_acth_Quit[] = "Quits the application after confirming.";
/* DOC: quit.html */
static fgw_error_t pcb_act_Quit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *force = NULL;
	PCB_ACT_MAY_CONVARG(1, FGW_STR, Quit, force = argv[1].val.str);

	if ((force != NULL) && (pcb_strcasecmp(force, "force") == 0))
		exit(0);
	if (!PCB->Changed || (pcb_hid_message_box("warning", "Close: lose data", "OK to lose data?", "cancel", 0, "ok", 1, NULL) == 1))
		pcb_quit_app();
	PCB_ACT_IRES(-1);
	return 0;
}


static const char pcb_acts_Export[] = "Export(exporter, [exporter-args])";
static const char pcb_acth_Export[] = "Export the current layout, e.g. Export(png, --dpi, 600)";
static fgw_error_t pcb_act_Export(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *args[128];
	char **a;
	int n;

	if (argc < 1) {
		pcb_message(PCB_MSG_ERROR, "Export() needs at least one argument, the name of the export plugin\n");
		return 1;
	}

	if (argc > sizeof(args)/sizeof(args[0])) {
		pcb_message(PCB_MSG_ERROR, "Export(): too many arguments\n");
		return 1;
	}

	args[0] = NULL;
	for(n = 1; n < argc; n++)
		PCB_ACT_CONVARG(n, FGW_STR, Export, args[n-1] = argv[n].val.str);

	pcb_exporter = pcb_hid_find_exporter(args[0]);
	if (pcb_exporter == NULL) {
		pcb_message(PCB_MSG_ERROR, "Export plugin %s not found. Was it enabled in ./configure?\n", args[0]);
		return 1;
	}

	/* remove the name of the exporter */
	argc-=2;

	/* call the exporter */
	a = args;
	a++;
	pcb_exporter->parse_arguments(pcb_exporter, &argc, &a);
	pcb_exporter->do_export(pcb_exporter, &PCB->hidlib, NULL);

	pcb_exporter = NULL;
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Backup[] = "Backup()";
static const char pcb_acth_Backup[] = "Backup the current layout - save using the same method that the timed backup function uses";
static fgw_error_t pcb_act_Backup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_backup();
	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t file_action_list[] = {
	{"Backup", pcb_act_Backup, pcb_acth_Backup, pcb_acts_Backup},
	{"Export", pcb_act_Export, pcb_acth_Export, pcb_acts_Export},
	{"LoadFrom", pcb_act_LoadFrom, pcb_acth_LoadFrom, pcb_acts_LoadFrom},
	{"New", pcb_act_New, pcb_acth_New, pcb_acts_New},
	{"Normalize", pcb_act_normalize, pcb_acth_normalize, pcb_acts_normalize},
	{"SaveTo", pcb_act_SaveTo, pcb_acth_SaveTo, pcb_acts_SaveTo},
	{"Quit", pcb_act_Quit, pcb_acth_Quit, pcb_acts_Quit}
};

PCB_REGISTER_ACTIONS(file_action_list, NULL)
