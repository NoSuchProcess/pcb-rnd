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
#include "hidlib_conf.h"
#include "plug_io.h"
#include "../src_plugins/lib_hid_common/dialogs_conf.h"

#include "dlg_loadsave.h"

extern const conf_dialogs_t dialogs_conf;

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
		name = pcb_gui->fileselect(pcb_gui, "Load netlist file", "Import netlist from file", last_netlist, ".net", NULL, "netlist", PCB_HID_FSD_READ, NULL);
	else if ((pcb_strcasecmp(function, "FootprintToBuffer") == 0) || (pcb_strcasecmp(function, "ElementToBuffer") == 0))
		name = pcb_gui->fileselect(pcb_gui, "Load footprint to buffer", "Import footprint from file", last_footprint, NULL, NULL, "footprint", PCB_HID_FSD_READ, NULL);
	else if (pcb_strcasecmp(function, "LayoutToBuffer") == 0)
		name = pcb_gui->fileselect(pcb_gui, "Load layout to buffer", "load layout (board) to buffer", last_layout, NULL, NULL, "board", PCB_HID_FSD_READ, NULL);
	else if (pcb_strcasecmp(function, "Layout") == 0)
		name = pcb_gui->fileselect(pcb_gui, "Load layout file", "load layout (board) as board to edit", last_layout, NULL, NULL, "board", PCB_HID_FSD_READ, NULL);
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid subcommand for Load(): '%s'\n", function);
		PCB_ACT_IRES(1);
		return 0;
	}

	if (name != NULL) {
		if (pcbhl_conf.rc.verbose)
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
	int *opt_tab; /* plugion options tab index for each avail[]; 0 means "no options" (the first tab) */
	const char **fmt_tab_names;
	void **fmt_plug_data;
	int tabs; /* number of option tabs, including the dummy 0th tab */
	int wfmt, wguess, wguess_err, wopts;
	int pick, num_fmts;
	pcb_hidval_t timer;
	char last_ext[32];
	unsigned fmt_chg_lock:1;
	unsigned timer_active:1;
	unsigned inited:1;
} save_t;

static void update_opts(save_t *save)
{
	pcb_hid_attr_val_t hv;
	int selection = save->fmtsub->dlg[save->wfmt].val.lng;

	hv.lng = save->opt_tab[selection];
	pcb_gui->attr_dlg_set_value(save->fmtsub->dlg_hid_ctx, save->wopts, &hv);
}

static void fmt_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_subdialog_t *fmtsub = caller_data;
	save_t *save = fmtsub->sub_ctx;
	char *bn, *fn, *s;
	const char *ext;
	pcb_event_arg_t res, argv[4];
	int selection = attr->val.lng;
	pcb_hid_attr_val_t hv;

	if ((save->avail == NULL) || save->fmt_chg_lock)
		return;

	if (fmtsub->parent_poke(fmtsub, "get_path", &res, 0, NULL) != 0)
		return;

	/* turn off guessing becuase the user explicitly selected a format */
	hv.lng = 0;
	pcb_gui->attr_dlg_set_value(save->fmtsub->dlg_hid_ctx, save->wguess, &hv);

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

	/* set the tab for format specific settings */
	update_opts(save);
}

static void guess_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hid_dad_subdialog_t *fmtsub = caller_data;
	save_t *save = fmtsub->sub_ctx;

	if (save->fmtsub->dlg[save->wguess].val.lng) {
		/* when guess is activated, make sure to recalculate the guess whatever
		   the format or the last file name was - this closes out all corner cases
		   with initial format mismatching the file name and multiple formats for
		   an ext */
		save->last_ext[0] = '\0';
	}
}


static void save_guess_format(save_t *save, const char *ext)
{
	int n;

	if (strcmp(ext, save->last_ext) == 0)
		return;

	strncpy(save->last_ext, ext, sizeof(save->last_ext));

	for(n = 0; n < save->num_fmts; n++) {
		if (strcmp(save->avail->extension[n], ext) == 0) {
			pcb_hid_attr_val_t hv;
			save->fmt_chg_lock = 1;
			hv.lng = n;
			pcb_gui->attr_dlg_set_value(save->fmtsub->dlg_hid_ctx, save->wfmt, &hv);
			save->fmt_chg_lock = 0;
			update_opts(save);
			pcb_gui->attr_dlg_widget_hide(save->fmtsub->dlg_hid_ctx, save->wguess_err, 1);
			return;
		}
	}
	pcb_gui->attr_dlg_widget_hide(save->fmtsub->dlg_hid_ctx, save->wguess_err, 0);
}

static void save_timer(pcb_hidval_t user_data)
{
	save_t *save = user_data.ptr;

	if ((save->fmtsub == NULL) || (save->avail == NULL)) {
		save->timer_active = 0;
		return; /* do not even restart the timer */
	}

	if (!save->inited) {
		update_opts(save);
		save->inited = 1;
	}

	save->timer = pcb_gui->add_timer(pcb_gui, save_timer, 300, user_data);

	if ((save->fmtsub->parent_poke != NULL) && (save->fmtsub->dlg_hid_ctx != NULL) && (save->fmtsub->dlg[save->wguess].val.lng)) {
		pcb_event_arg_t res;
		char *end;

		save->fmtsub->parent_poke(save->fmtsub, "get_path", &res, 0, NULL);
		end = strrchr(res.d.s, '.');
		if (end != NULL)
			save_guess_format(save, end);

		free((char *)res.d.s);
	}
}

static void save_on_close(pcb_hid_dad_subdialog_t *sub, pcb_bool ok)
{
	save_t *save = sub->sub_ctx;
	int n, i;
	char *seen;
	int tab_selection = save->opt_tab[save->fmtsub->dlg[save->wfmt].val.lng];

	seen = calloc(1, save->tabs);
	for(n = 1; n < save->tabs; n++) {
		for(i = 0; i < save->num_fmts; i++) {
			if ((seen[n] == 0) && (save->opt_tab[i] == n)) {
				const pcb_plug_io_t *plug = save->avail->plug[i];
				seen[n] = 1;
				if (plug->save_as_subd_uninit != NULL)
					plug->save_as_subd_uninit(plug, save->fmt_plug_data[n], sub, (ok && (n == tab_selection)));
			}
		}
	}
	free(seen);
}


static void setup_fmt_tabs(save_t *save, pcb_plug_iot_t save_type)
{
	int n, i, tabs;

	save->opt_tab = calloc(sizeof(int), save->num_fmts);
	for(n = 0, tabs = 0; n < save->num_fmts; n++) {
		const pcb_plug_io_t *plug = save->avail->plug[n];
		if (plug->save_as_subd_init != NULL) {
			int link = -1;
			
			/* check if init() matches an already existing tab's and link them if so */
			for(i = 0; i < n; i++) {
				const pcb_plug_io_t *old_plug = save->avail->plug[i];
				if (plug->save_as_subd_init == old_plug->save_as_subd_init) {
					link = save->opt_tab[i];
					break;
				}
			}

			if (link < 0) {
				save->opt_tab[n] = tabs+1;
				tabs++;
			}
			else
				save->opt_tab[n] = link;
		}
	}

	/* tab 0 is for the no-option tab (most plugs will use that) */
	save->fmt_tab_names = calloc(tabs+2, sizeof(char *));
	save->fmt_plug_data = calloc(tabs+2, sizeof(void *));
	save->fmt_tab_names[0] = "no-opt";

	/* fill in rest of the tabs - not visible, only for debugging */
	for(n = 0; n < save->num_fmts; n++) {
		const pcb_plug_io_t *plug = save->avail->plug[n];
		if ((plug->save_as_subd_init != NULL) && (save->opt_tab[n] >= 0))
			save->fmt_tab_names[save->opt_tab[n]] = plug->description;
	}
	save->fmt_tab_names[tabs+1] = NULL;

	PCB_DAD_BEGIN_TABBED(save->fmtsub->dlg, save->fmt_tab_names);
		PCB_DAD_COMPFLAG(save->fmtsub->dlg, PCB_HATF_HIDE_TABLAB);
		save->wopts = PCB_DAD_CURRENT(save->fmtsub->dlg);
/*	pre-creation tab switch not yet supported:	PCB_DAD_DEFAULT_NUM(save->fmtsub->dlg, save->opt_tab[0]);*/

		/* the no-options tab */
		PCB_DAD_LABEL(save->fmtsub->dlg, "(no format options)");
			PCB_DAD_HELP(save->fmtsub->dlg, "Some formats offer format-specific options\nwhich are normally displayed here.\nThe currently selected format does\nnot offer any options.");

		/* all other tabs, filled in by the plug code */
		for(n = 1; n < tabs+1; n++) {
			for(i = 0; i < save->num_fmts; i++) {
				if (save->opt_tab[i] == n) {
					const pcb_plug_io_t *plug = save->avail->plug[i];
					PCB_DAD_BEGIN_VBOX(save->fmtsub->dlg);
						save->fmt_plug_data[n] = plug->save_as_subd_init(plug, save->fmtsub, save_type);
					PCB_DAD_END(save->fmtsub->dlg);
					break;
				}
			}
		}
		save->tabs = tabs+1;
	PCB_DAD_END(save->fmtsub->dlg);
}

static void setup_fmt_sub(save_t *save, pcb_plug_iot_t save_type)
{
	const char *guess_help =
		"allow guessing format from the file name:\n"
		"when enabled, the format is automatically determined\n"
		"from the file name after an edit to the file name\n"
		"(guessing will NOT change the initial format selection\n"
		"when the dialog box is fresh open)";

	PCB_DAD_BEGIN_VBOX(save->fmtsub->dlg);
		PCB_DAD_BEGIN_HBOX(save->fmtsub->dlg);
			PCB_DAD_LABEL(save->fmtsub->dlg, "File format:");
			PCB_DAD_ENUM(save->fmtsub->dlg, (const char **)save->avail->digest);
				save->wfmt = PCB_DAD_CURRENT(save->fmtsub->dlg);
				PCB_DAD_DEFAULT_NUM(save->fmtsub->dlg, save->pick);
				PCB_DAD_CHANGE_CB(save->fmtsub->dlg, fmt_chg);
		PCB_DAD_END(save->fmtsub->dlg);
		PCB_DAD_BEGIN_HBOX(save->fmtsub->dlg);
			PCB_DAD_LABEL(save->fmtsub->dlg, "Guess format:");
				PCB_DAD_HELP(save->fmtsub->dlg, guess_help);
			PCB_DAD_BOOL(save->fmtsub->dlg, "");
				save->wguess = PCB_DAD_CURRENT(save->fmtsub->dlg);
				PCB_DAD_CHANGE_CB(save->fmtsub->dlg, guess_chg);
				PCB_DAD_DEFAULT_NUM(save->fmtsub->dlg, !!dialogs_conf.plugins.dialogs.file_select_dialog.save_as_format_guess);
				PCB_DAD_HELP(save->fmtsub->dlg, guess_help);
			PCB_DAD_LABEL(save->fmtsub->dlg, "(guess failed)");
				PCB_DAD_COMPFLAG(save->fmtsub->dlg, PCB_HATF_HIDE);
				PCB_DAD_HELP(save->fmtsub->dlg, "This file name is not naturally connected to\nany file format; file format\nis left on what was last selected/recognized");
				save->wguess_err = PCB_DAD_CURRENT(save->fmtsub->dlg);
		PCB_DAD_END(save->fmtsub->dlg);


		setup_fmt_tabs(save, save_type);
	PCB_DAD_END(save->fmtsub->dlg);
}

const char pcb_acts_Save[] = 
	"Save()\n"
	"Save(Layout|LayoutAs)\n"
	"Save(AllConnections|AllUnusedPins|ElementConnections)\n"
	"Save(PasteBuffer)\n"
	"Save(DialogByPattern, pcb|footprint|font|buffer, none|board|fp, prompt, [default_pattern])";
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
	int num_fmts, fmt, *fmt_param = NULL;
	save_t save;
	pcb_hidval_t timer_ctx;
	const char *default_pattern = NULL;
	pcb_plug_iot_t list_iot;
	pcb_io_list_ext_t list_ext;
	int dialog_by_pattern = 0, is_dialog;

	if (cwd == NULL) cwd = dup_cwd();


	PCB_ACT_MAY_CONVARG(1, FGW_STR, Save, function = argv[1].val.str);

	is_dialog = (function != NULL) && (pcb_strncasecmp(function, "Dialog", 6) == 0);

	if ((!is_dialog) && (argc > 2))
		return PCB_ACT_CALL_C(pcb_act_SaveTo, res, argc, argv);

	memset(&save, 0, sizeof(save));

	if (pcb_strcasecmp(function, "Layout") == 0)
		if (PCB->hidlib.filename != NULL)
			return pcb_actionl("SaveTo", "Layout", NULL);

	if (is_dialog) {
		const char *siot, *sext;

		PCB_ACT_CONVARG(2, FGW_STR, Save, siot = argv[2].val.str);
		PCB_ACT_CONVARG(3, FGW_STR, Save, sext = argv[3].val.str);
		PCB_ACT_CONVARG(4, FGW_STR, Save, prompt = argv[4].val.str);
		PCB_ACT_MAY_CONVARG(5, FGW_STR, Save, default_pattern = argv[5].val.str);

		if (pcb_strcasecmp(siot, "pcb") == 0) list_iot = PCB_IOT_PCB;
		else if (pcb_strcasecmp(siot, "footprint") == 0) list_iot = PCB_IOT_FOOTPRINT;
		else if (pcb_strcasecmp(siot, "font") == 0) list_iot = PCB_IOT_FONT;
		else if (pcb_strcasecmp(siot, "buffer") == 0) list_iot = PCB_IOT_BUFFER;
		else PCB_ACT_FAIL(Save);

		if (pcb_strcasecmp(sext, "none") == 0) list_ext = PCB_IOL_EXT_NONE;
		else if (pcb_strcasecmp(sext, "board") == 0) list_ext = PCB_IOL_EXT_BOARD;
		else if (pcb_strcasecmp(sext, "fp") == 0) list_ext = PCB_IOL_EXT_FP;
		else PCB_ACT_FAIL(Save);
	}

	if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
		default_pattern = conf_core.rc.save_fp_fmt;
		num_fmts = pcb_io_list(&avail, PCB_IOT_FOOTPRINT, 1, 1, PCB_IOL_EXT_FP);
		prompt = "Save subcircuit as";
		list_iot = PCB_IOT_FOOTPRINT;
		list_ext = PCB_IOL_EXT_FP;
		goto list_by_pattern;
	}
	else if (pcb_strcasecmp(function, "DialogByPattern") == 0) {
		int n;

		dialog_by_pattern = 1;

		list_by_pattern:;
		num_fmts = pcb_io_list(&avail, list_iot, 1, 1, list_ext);
		if (num_fmts > 0) {
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

			fmtsub = &fmtsub_local;

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
		int n;
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
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Error: no IO plugin avaialble for saving a buffer.");
			PCB_ACT_IRES(-1);
			return 0;
		}
	}

	if (fmtsub != NULL) {
		memset(&fmtsub_local, 0, sizeof(fmtsub_local));
		save.avail = &avail;
		save.num_fmts = num_fmts;
		save.fmtsub = fmtsub;
		save.pick = fmt;
		fmtsub->on_close = save_on_close;
		fmtsub->sub_ctx = &save;
		setup_fmt_sub(&save, PCB_IOT_PCB);
	}

	/* construct the input file name and run a file selection dialog to get the final file name */
	if (name_in == NULL) {
		if (PCB->hidlib.filename == NULL)
			name_in = pcb_concat("unnamed", extensions_param[fmt], NULL);
		else
			name_in = pcb_strdup(PCB->hidlib.filename);
	}


	{
		/* save initial extension so the timer doesn't immedaitely overwrite the
		   original format with a guess - important when the format is known
		   but doesn't match the name and guessing is enabled */
		const char *end;
		end = strrchr(name_in, '.');
		if (end != NULL)
			strncpy(save.last_ext, end, sizeof(save.last_ext));
	}
	
	timer_ctx.ptr = &save;
	save.timer_active = 1;
	save.timer = pcb_gui->add_timer(pcb_gui, save_timer, 300, timer_ctx); /* the timer needs to run at least once, to get some initialization done that can be done only after fmtsub got created */
	final_name = pcb_gui->fileselect(pcb_gui, prompt, NULL, name_in, NULL, NULL, "board", PCB_HID_FSD_MAY_NOT_EXIST, fmtsub);
	if (save.timer_active)
		pcb_gui->stop_timer(pcb_gui, save.timer);
	free(name_in);
	free(save.fmt_tab_names);
	free(save.fmt_plug_data);

	/* early return for dialog-by-pattern: don't do anything real, just build a string and return */
	if (dialog_by_pattern) {
		const char *sfmt = NULL;
		if (fmt_param != NULL)
			sfmt = avail.plug[save.pick]->default_fmt;
		res->type = FGW_STR | FGW_DYN;
		if (final_name != NULL)
			res->val.str = pcb_concat(final_name, "*", sfmt, NULL);
		else
			res->val.str = NULL;
		free(final_name);
		pcb_io_list_free(&avail);
		return 0;
	}

	if (final_name == NULL) { /* cancel */
		pcb_io_list_free(&avail);
		PCB_ACT_IRES(1);
		return 0;
	}


	if (pcbhl_conf.rc.verbose)
		fprintf(stderr, "Save:  Calling SaveTo(%s, %s)\n", function, final_name);

	if (pcb_strcasecmp(function, "PasteBuffer") == 0) {
		const char *sfmt = avail.plug[fmt]->description;
		if (fmt_param != NULL)
			sfmt = avail.plug[save.pick]->description;
		pcb_actionl("PasteBuffer", "Save", final_name, sfmt, NULL);
	}
	else {
		const char *sfmt = NULL;
		if (fmt_param != NULL)
			sfmt = avail.plug[save.pick]->description;
		/*
		 * if we got this far and the function is Layout, then
		 * we really needed it to be a LayoutAs.  Otherwise
		 * ActionSaveTo() will ignore the new file name we
		 * just obtained.
		 */
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

	name = pcb_gui->fileselect(pcb_gui, "Load schematics", "Import netlist and footprints from schematics", cwd, NULL, NULL, "schematics", PCB_HID_FSD_MAY_NOT_EXIST, NULL);
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
