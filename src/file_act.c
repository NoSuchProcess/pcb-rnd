/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/hid/hid_export.h>
#include "layer_vis.h"
#include <librnd/core/safe_fs.h>
#include <librnd/hid/tool.h>
#include "netlist.h"
#include "plug_io.h"
#include "obj_pstk_inlines.h"

#define PCB (do_not_use_PCB)

static const char pcb_acts_LoadFrom[] = "LoadFrom(Layout|LayoutToBuffer|SubcToBuffer|Netlist|Revert,filename[,format])";
static const char pcb_acth_LoadFrom[] = "Load layout data from a file.";
/* DOC: loadfrom.html */
fgw_error_t pcb_act_LoadFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *name, *format = NULL;
	int op;

	RND_ACT_CONVARG(1, FGW_KEYWORD, LoadFrom, op = fgw_keyword(&argv[1]));
	RND_ACT_CONVARG(2, FGW_STR, LoadFrom, name = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, LoadFrom, format = argv[3].val.str);

	switch(op) {
		case F_ElementToBuffer:
		case F_FootprintToBuffer:
		case F_Element:
		case F_SubcToBuffer:
		case F_Subcircuit:
		case F_Footprint:
			rnd_hid_notify_crosshair_change(RND_ACT_DESIGN, rnd_false);
			if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, name, format))
				rnd_tool_select_by_name(RND_ACT_DESIGN, "buffer");
			rnd_hid_notify_crosshair_change(RND_ACT_DESIGN, rnd_true);
			break;

		case F_LayoutToBuffer:
			rnd_hid_notify_crosshair_change(RND_ACT_DESIGN, rnd_false);
			if (pcb_buffer_load_layout(pcb, PCB_PASTEBUFFER, name, format))
				rnd_tool_select_by_name(RND_ACT_DESIGN, "buffer");
			rnd_hid_notify_crosshair_change(RND_ACT_DESIGN, rnd_true);
			break;

		case F_Layout:
			if (!pcb->Changed ||  rnd_hid_message_box(RND_ACT_DESIGN, "warning", "File overwrite", "OK to override layout data?", "cancel", 0, "ok", 1, NULL))
				pcb_load_pcb(name, format, rnd_true, 0);
			break;

		case F_Netlist:
			if (pcb->Netlistname)
				free(pcb->Netlistname);
			pcb->Netlistname = rnd_strdup_strip_wspace(name);
			{
				int i;
				for (i = 0; i < PCB_NUM_NETLISTS; i++) {
					pcb_netlist_uninit(&(pcb->netlist[i]));
					pcb_netlist_init(&(pcb->netlist[i]));
				}
			}
			if (!pcb_import_netlist(RND_ACT_DESIGN, pcb->Netlistname))
				pcb_netlist_changed(1);
			else
				rnd_message(RND_MSG_ERROR, "None of the netlist import plugins could handle that file:\nunknown/broken file format or partial load due to errors in the file\n");
			break;

		case F_Revert:
			if (RND_ACT_DESIGN->loadname && (!pcb->Changed || (rnd_hid_message_box(RND_ACT_DESIGN, "warning", "Revert: lose data", "Really revert all modifications?", "no", 0, "yes", 1, NULL) == 1)))
				pcb_revert_pcb();
			break;

		default:
			rnd_message(RND_MSG_ERROR, "LoadFrom(): invalid command (first arg)\n");
			RND_ACT_IRES(1);
			return 0;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_New[] = "New([name])";
static const char pcb_acth_New[] = "Starts a new layout.";
/* DOC: new.html */
static fgw_error_t pcb_act_New(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *argument_name = NULL;
	char *name = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, New, argument_name = argv[1].val.str);

	if (!pcb->Changed || (rnd_hid_message_box(RND_ACT_DESIGN, "warning", "New pcb", "OK to clear layout data?", "cancel", 0, "yes", 1, NULL) == 1)) {
		if (argument_name)
			name = rnd_strdup(argument_name);
		else
			name = rnd_hid_prompt_for(RND_ACT_DESIGN, "Enter the layout name:", "", "Layout name");

		if (!name)
			return 1;

/* pcb usage: at the moment, while having only one global pcb, this function
   legitimately uses that */

		rnd_hid_notify_crosshair_change(RND_ACT_DESIGN, rnd_false);
		/* do emergency saving
		 * clear the old struct and allocate memory for the new one
		 */
		if (pcb->Changed && conf_core.editor.save_in_tmp)
			pcb_save_in_tmp();

		pcb_crosshair_move_absolute(pcb, RND_COORD_MAX, RND_COORD_MAX); /* make sure the crosshair is not above any object so ch* plugins release their highlights */
		pcb_draw_inhibit_inc();
		rnd_hid_menu_merge_inhibit_inc();
		pcb_board_remove(pcb);
#undef PCB
		PCB = pcb = pcb_board_new(1);
#define PCB do_not_use
		pcb_board_new_postproc(pcb, 1);

		pcb_set_design_dir(NULL);
		rnd_conf_set(RND_CFR_DESIGN, "design/text_font_id", 0, "0", RND_POL_OVERWRITE); /* we have only one font now, make sure it is selected */

		/* setup the new name and reset some values to default */
		free(pcb->hidlib.name);
		pcb->hidlib.name = name;

		rnd_single_switch_to(&pcb->hidlib);
		pcb_draw_inhibit_dec();
		rnd_hid_menu_merge_inhibit_dec();

		pcb_layervis_reset_stack(&pcb->hidlib);
		pcb_center_display(pcb, (pcb->hidlib.dwg.X1+pcb->hidlib.dwg.X2) / 2, (pcb->hidlib.dwg.Y1+pcb->hidlib.dwg.Y2) / 2);
		pcb_board_replaced();
		rnd_hid_redraw(&PCB->hidlib);
		rnd_hid_notify_crosshair_change(RND_ACT_DESIGN, rnd_true);
		RND_ACT_IRES(0);
		return 0;
	}
	RND_ACT_IRES(-1);
	return 0;
}

/* --------------------------------------------------------------------------- */
static const char pcb_acts_normalize[] = "Normalize([board|buffer[n]])";
static const char pcb_acth_normalize[] = "Move all objects within the drawing area (or buffer 0;0), align the drawing to 0;0 (or set buffer grab point to 0;0)";
static fgw_error_t pcb_act_normalize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *target = "board";

	RND_ACT_MAY_CONVARG(1, FGW_STR, normalize, target = argv[1].val.str);
	
	if (strcmp(target, "board") == 0)
		RND_ACT_IRES(pcb_board_normalize(pcb));
	else if (strncmp(target, "buffer", 6) == 0) {
		int bn;
		char *end;

		target += 6;
		if (*target != '\0') {
			bn = strtol(target, &end, 10);
			if (*end != '\0') {
				rnd_message(RND_MSG_ERROR, "Expected buffer number, got '%s'\n", target);
				RND_ACT_IRES(-1);
			}
			bn--;
			if ((bn < 0) || (bn >= PCB_MAX_BUFFER)) {
				rnd_message(RND_MSG_ERROR, "Buffer number out of range\n");
				RND_ACT_IRES(-1);
			}
		}
		else
			bn = conf_core.editor.buffer_number;

		if ((pcb_buffers[bn].bbox_naked.X1 != 0) || (pcb_buffers[bn].bbox_naked.Y1 != 0)) {
			pcb_data_move(pcb_buffers[bn].Data, -pcb_buffers[bn].bbox_naked.X1, -pcb_buffers[bn].bbox_naked.Y1, 0);
			pcb_set_buffer_bbox(&pcb_buffers[bn]);
		}

		pcb_buffers[bn].X = pcb_buffers[bn].Y = 0;
		RND_ACT_IRES(0);
	}
	else
		RND_ACT_FAIL(normalize);
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

	RND_ACT_CONVARG(1, FGW_KEYWORD, SaveTo, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, SaveTo, name = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, SaveTo, fmt = argv[3].val.str);
	RND_ACT_IRES(0);

	if ((op != F_Layout) && (name == NULL))
		RND_ACT_FAIL(SaveTo);

	switch(op) {
		case F_Layout:
			if (argc != 2) {
				rnd_message(RND_MSG_ERROR, "SaveTo(Layout) doesn't take file name or format - did you mean SaveTo(LayoutAs)?\n");
				return FGW_ERR_ARGC;
			}
			if (pcb_save_pcb(RND_ACT_DESIGN->loadname, NULL) == 0)
				pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_false);
			rnd_event(RND_ACT_DESIGN, RND_EVENT_DESIGN_FN_CHANGED, NULL);
			return 0;

		case F_LayoutAs:
			if (pcb_save_pcb(name, fmt) == 0) {
				pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_false);
				free(RND_ACT_DESIGN->loadname);
				RND_ACT_DESIGN->loadname = rnd_strdup(name);
				rnd_event(RND_ACT_DESIGN, RND_EVENT_DESIGN_FN_CHANGED, NULL);
			}
			return 0;


		case F_PasteBuffer:
			if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) == 0) {
				rnd_message(RND_MSG_ERROR, "Can not save subcircuit: there is no subcircuit in the paste buffer.\n");
				RND_ACT_IRES(-1);
			}
			else if (pcb_subclist_length(&PCB_PASTEBUFFER->Data->subc) > 1) {
				rnd_message(RND_MSG_ERROR, "Can not save subcircuit: there are more than one subcircuits in the paste buffer.\nDid you mean saving a library instead?\n");
				RND_ACT_IRES(-1);
			}
			else
				RND_ACT_IRES(pcb_save_buffer_subcs(name, fmt, 0));
			return 0;

		/* shorthand kept only for compatibility reasons - do not use */
		case F_AllConnections:
			rnd_message(RND_MSG_WARNING, "Please use action ExportOldConn() instead of SaveTo() for connections.\n");
			return rnd_actionva(RND_ACT_DESIGN, "ExportOldConn", "AllConnections", name, NULL);
		case F_AllUnusedPins:
			rnd_message(RND_MSG_WARNING, "Please use action ExportOldConn() instead of SaveTo() for connections.\n");
			return rnd_actionva(RND_ACT_DESIGN, "ExportOldConn", "AllUnusedPins", name, NULL);
		case F_ElementConnections:
		case F_SubcConnections:
			rnd_message(RND_MSG_WARNING, "Please use action ExportOldConn() instead of SaveTo() for connections.\n");
			return rnd_actionva(RND_ACT_DESIGN, "ExportOldConn", "SubcConnections", name, NULL);
	}

	RND_ACT_FAIL(SaveTo);
}

/* Run the save dialog, either the full version from the dialogs plugin
(if available) or a simplified version. Return 0 on success and fill in
name_out and fmt_out. */
static int save_fmt_dialog(const char *title, const char *descr, char **default_file, const char *history_tag, rnd_hid_fsd_flags_t flags, char **name_out, const char **fmt_out)
{
	const fgw_func_t *f = rnd_act_lookup("save");

	*name_out = NULL;
	*fmt_out = NULL;

	if (f != NULL) { /* has dialogs plugin */
		fgw_error_t err;
		fgw_arg_t res, argv[6];
		char *sep;

		argv[1].type = FGW_STR; argv[1].val.str = "DialogByPattern";
		argv[2].type = FGW_STR; argv[2].val.str = "footprint";
		argv[3].type = FGW_STR; argv[3].val.str = "rf";
		argv[4].type = FGW_STR; argv[4].val.cstr = descr;
		argv[5].type = FGW_STR; argv[5].val.cstr = conf_core.rc.save_fp_fmt;
		err = rnd_actionv_(f, &res, 5, argv);
		if ((err != 0) || (res.val.str == NULL)) /* cancel */
			return -1;
		if ((res.type & (FGW_STR | FGW_DYN)) != (FGW_STR | FGW_DYN)) {
			rnd_message(RND_MSG_ERROR, "Internal error: Save(DialogByPattern) did not return a dynamic string\n");
			return -1;
		}
		*name_out = res.val.str; /* will be free'd by the caller */
		sep = strchr(res.val.str, '*');
		if (sep != NULL) {
			*sep = '\0';
			*fmt_out = sep+1;
		}
		printf("RES2: '%s' '%s'\n", *name_out, *fmt_out);
	}
	else { /* fallback to simpler fileselect */
		char *name = rnd_hid_fileselect(rnd_gui, title, descr, *default_file, "", NULL, history_tag, flags, NULL);

		*name_out = name;
		if (name == NULL)
			return -1;

		if (*default_file != NULL)
			free(*default_file);
		*default_file = rnd_strdup(name);
	}
	return 0;
}

static const char pcb_acts_SaveLib[] =
	"SaveLib(file|dir, board|buffer, [filename], [fmt])\n";
static const char pcb_acth_SaveLib[] = "Saves all subcircuits to a library file or directory from a board or buffer.";
/* DOC: savelib.html */
fgw_error_t pcb_act_SaveLib(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *method, *source, *fn = NULL, *fmt = NULL;
	pcb_data_t *src;

	RND_ACT_CONVARG(1, FGW_STR, SaveLib, method = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, SaveLib, source = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, SaveLib, fn = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, SaveLib, fmt = argv[4].val.str);

	if (rnd_strcasecmp(source, "board") == 0) src = pcb->Data;
	else if (rnd_strcasecmp(source, "buffer") == 0) src = PCB_PASTEBUFFER->Data;
	else
		RND_ACT_FAIL(SaveLib);

	if (rnd_strcasecmp(method, "file") == 0) {
		char *name;
		FILE *f;
		static char *default_file;

		if (fn == NULL) {
			int sr = save_fmt_dialog("Save footprint lib to file ...", "Choose a file to save all subcircuits to.\n",
				&default_file, "save_lib_file", 0, &name, &fmt);
			if (sr != 0) {
				RND_ACT_IRES(-1);
				return 0;
			}
		}
		else
			name = rnd_strdup(fn);

		f = rnd_fopen(RND_ACT_DESIGN, name, "w");
		if (f == NULL) {
			rnd_message(RND_MSG_ERROR, "Failed to open %s for write\n", name);
			free(name);
			RND_ACT_IRES(-1);
			return 0;
		}
		free(name);
		RND_ACT_IRES(pcb_write_footprint_data(f, src, fmt, -1));
		fclose(f);
	}
	else if (rnd_strcasecmp(method, "dir") == 0) {
		unsigned int ares = 0;
		void *udata;
		gdl_iterator_t sit;
		char *name, *sep;
		const char *ending;
		static char *default_file;
		pcb_subc_t *subc;
		pcb_plug_io_t *p;
		

		if (fn == NULL) {
			int sr = save_fmt_dialog("Save footprint lib to directory ...", "Choose a file name pattern to save all subcircuits to.\n",
				&default_file, "save_lib_dir", 0, &name, &fmt);
			if (sr != 0) {
				RND_ACT_IRES(-1);
				return 0;
			}
		}
		else
			name = rnd_strdup(fn);

		p = pcb_io_find_writer(PCB_IOT_FOOTPRINT, fmt);
		if (p == NULL) {
			if (fmt == NULL)
				rnd_message(RND_MSG_ERROR, "Failed to find a plugin that can write subcircuits", fmt);
			else
				rnd_message(RND_MSG_ERROR, "Failed to find a plugin for format %s", fmt);
			RND_ACT_IRES(-1);
			return 0;
		}

		sep = strrchr(name, '.');
		if ((sep != NULL) && (strchr(sep, RND_DIR_SEPARATOR_C) == NULL)) {
			*sep = '\0';
			ending = sep+1;
			sep = ".";
		}
		else {
			ending = p->fp_extension;
			sep = "";
		}

		subclist_foreach(&src->subc, &sit, subc) {
			FILE *f;
			char *fullname = rnd_strdup_printf("%s.%ld%s%s", name, (long)sit.count, sep, ending);

			f = rnd_fopen(RND_ACT_DESIGN, fullname, "w");
			free(fullname);
			if (f != NULL) {
				if (p->write_subcs_head(p, &udata, f, 0, 1) == 0) {
					ares |= p->write_subcs_subc(p, &udata, f, subc);
					ares |= p->write_subcs_tail(p, &udata, f);
				}
				else ares |= 1;
			}
			else ares |= 1;
			fclose(f);
		}

		if (ares != 0)
			rnd_message(RND_MSG_ERROR, "Some of the subcircuits failed to export\n");
		RND_ACT_IRES(ares);
		free(name);
	}
	else
		RND_ACT_FAIL(SaveLib);

	return 0;
}

static const char pcb_acts_PadstackSave[] = "PadstackSave(buffer, [filename], [fmt])\n";
static const char pcb_acth_PadstackSave[] = "Save padstack to file.";
fgw_error_t pcb_act_PadstackSave(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *source, *fn = NULL, *fmt = "lihata";
	char *name;
	FILE *f;
	static char *default_file;
	pcb_pstk_t *src;
	pcb_pstk_proto_t *proto;

	RND_ACT_CONVARG(1, FGW_STR, PadstackSave, source = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, PadstackSave, fn = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, PadstackSave, fmt = argv[3].val.str);

	if (rnd_strcasecmp(source, "buffer") == 0) {
		src = padstacklist_first(&PCB_PASTEBUFFER->Data->padstack);
		if (src == NULL) {
			rnd_message(RND_MSG_ERROR, "PadstackSave: no padstack found in the current buffer\n");
			RND_ACT_IRES(1);
			return 0;
		}
	}
	else {
			rnd_message(RND_MSG_ERROR, "PadstackSave: invalid first argument\n");
			RND_ACT_IRES(1);
			return 0;
	}

	proto = pcb_pstk_get_proto(src);
	if (proto == NULL) {
		rnd_message(RND_MSG_ERROR, "PadstackSave: padstack prototype not found\n");
		RND_ACT_IRES(1);
		return 0;
	}

	if (fn == NULL) {
		int sr = save_fmt_dialog("Save padstack to file ...", "Choose a file to save padstack to.\n",
			&default_file, "save_pstk_file", 0, &name, &fmt);
		if (sr != 0) {
			RND_ACT_IRES(-1);
			return 0;
		}
	}
	else
		name = rnd_strdup(fn);

	f = rnd_fopen(RND_ACT_DESIGN, name, "w");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open %s for write\n", name);
		free(name);
		RND_ACT_IRES(-1);
		return 0;
	}
	free(name);

	RND_ACT_IRES(pcb_write_padstack(f, proto, fmt));
	fclose(f);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Quit[] = "Quit()";
static const char pcb_acth_Quit[] = "Quits the application after confirming.";
/* DOC: quit.html */
static fgw_error_t pcb_act_Quit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *force = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, Quit, force = argv[1].val.str);

	if ((force != NULL) && (rnd_strcasecmp(force, "force") == 0))
		exit(0);
	if (!pcb->Changed || (rnd_hid_message_box(RND_ACT_DESIGN, "warning", "Close: lose data", "OK to lose data?", "cancel", 0, "ok", 1, NULL) == 1))
		pcb_quit_app();
	RND_ACT_IRES(-1);
	return 0;
}


static const char pcb_acts_Export[] = "Export(exporter, [exporter-args])";
static const char pcb_acth_Export[] = "Export the current layout, e.g. Export(png, --dpi, 600)";

static const char pcb_acts_Backup[] = "Backup()";
static const char pcb_acth_Backup[] = "Backup the current layout - save using the same method that the timed backup function uses";
static fgw_error_t pcb_act_Backup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_backup();
	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t file_action_list[] = {
	{"Backup", pcb_act_Backup, pcb_acth_Backup, pcb_acts_Backup},
	{"Export", rnd_act_Export, pcb_acth_Export, pcb_acts_Export},
	{"LoadFrom", pcb_act_LoadFrom, pcb_acth_LoadFrom, pcb_acts_LoadFrom},
	{"New", pcb_act_New, pcb_acth_New, pcb_acts_New},
	{"Normalize", pcb_act_normalize, pcb_acth_normalize, pcb_acts_normalize},
	{"SaveTo", pcb_act_SaveTo, pcb_acth_SaveTo, pcb_acts_SaveTo},
	{"SaveLib", pcb_act_SaveLib, pcb_acth_SaveLib, pcb_acts_SaveLib},
	{"PadstackSave", pcb_act_PadstackSave, pcb_acth_PadstackSave, pcb_acts_PadstackSave},
	{"Quit", pcb_act_Quit, pcb_acth_Quit, pcb_acts_Quit}
};

void pcb_file_act_init2(void)
{
	RND_REGISTER_ACTIONS(file_action_list, NULL);
}
