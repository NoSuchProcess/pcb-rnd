/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015,2016 Tibor 'Igor2' Palinkas
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
 *
 */


/* This used to be file.c; some of the code landed in the io_pcb plugin,
   the format-independent parts ended up here. */

/* file save, load, merge ... routines */

#warning TODO: do not hardwire this, make a function to decide
#define DEFAULT_FMT "pcb"

#include "config.h"
#include "conf_core.h"

#include <locale.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "change.h"
#include "conf.h"
#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "remove.h"
#include "paths.h"
#include "rats_patch.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "event.h"
#include "compat_misc.h"
#include "route_style.h"
#include "compat_fs.h"
#include "compat_nls.h"
#include "compat_lrealpath.h"
#include "layer_vis.h"
#include "safe_fs.h"
#include "plug_footprint.h"

/* for opendir */
#include "compat_inc.h"

pcb_plug_io_t *pcb_plug_io_chain = NULL;
int pcb_io_err_inhibit = 0;

static void plug_io_err(int res, const char *what, const char *filename)
{
	if (pcb_io_err_inhibit)
		return;
	if (res != 0) {
		const char *reason = "", *comment = "";
		if (pcb_plug_io_chain != NULL) {
			if (filename == NULL) {
				reason = "none of io plugins could successfully write the file";
				filename = "";
			}
			else {
				FILE *f;
				reason = "none of io plugins could successfully read file";
				f = pcb_fopen(filename, "r");
				if (f != NULL) {
					fclose(f);
					comment = "(unknown/invalid file format?)";
				}
				else
					comment = "(can not open the file for reading)";
			}
		}
		else {
			reason = "no io plugin loaded, I don't know any file format";
			if (filename == NULL)
				filename = "";
		}
		pcb_message(PCB_MSG_ERROR, "IO error during %s: %s %s %s\n", what, reason, filename, comment);
	}
}

static char *last_design_dir = NULL;
void pcb_set_design_dir(const char *fn)
{
	char *end;

	free(last_design_dir);
	last_design_dir = NULL;

	if (fn != NULL)
		last_design_dir = pcb_lrealpath(fn);

	if (last_design_dir == NULL) {
		last_design_dir = pcb_strdup("<invalid>");
		conf_force_set_str(conf_core.rc.path.design, last_design_dir);
		pcb_conf_ro("rc/path/design");
		return;
	}

	end = strrchr(last_design_dir, PCB_DIR_SEPARATOR_C);
	if (end != NULL)
		*end = '\0';

	conf_force_set_str(conf_core.rc.path.design, last_design_dir);
	pcb_conf_ro("rc/path/design");
}

static int pcb_test_parse_all(FILE *ft, const char *Filename, const char *fmt, pcb_plug_iot_t type, pcb_find_io_t *available, int *accepts, int *accept_total, int maxav, int ignore_missing, int gen_event)
{
	int len, n;

	if (ft == NULL) {
		if (!ignore_missing)
			pcb_message(PCB_MSG_ERROR, "Error: can't open %s for reading (format is %s)\n", Filename, fmt);
		return -1;
	}

	if (gen_event)
		pcb_event(PCB_EVENT_LOAD_PRE, "s", Filename);

	len = pcb_find_io(available, maxav, type, 0, fmt);
	if (fmt != NULL) {
		/* explicit format */
		for(n = 0; n < len; n++) {
			void (*f)();
			switch(type) {
				case PCB_IOT_PCB: f = (void (*)())available[n].plug->parse_pcb; break;
				default: assert(!"internal error: pcb_test_parse_all: wrong type"); f = NULL;
			}
			if (f != NULL) {
				accepts[n] = 1; /* force-accept - if it can handle the format, and the user explicitly wanted this format, let's try it */
				(*accept_total)++;
			}
		}

		if (*accept_total <= 0) {
			pcb_message(PCB_MSG_ERROR, "can't find a IO_ plugin to load a PCB using format %s\n", fmt);
			return -1;
		}

		if (*accept_total > 1) {
			pcb_message(PCB_MSG_INFO, "multiple IO_ plugins can handle format %s - I'm going to try them all, but you may want to be more specific next time; formats found:\n", fmt);
			for(n = 0; n < len; n++)
				pcb_message(PCB_MSG_INFO, "    %s\n", available[n].plug->description);
		}
	}
	else {
		/* test-parse with all plugins to see who can handle the syntax */
		if((fgetc(ft) != EOF)) {
			rewind(ft);
			for(n = 0; n < len; n++) {
				if ((available[n].plug->test_parse == NULL) || (available[n].plug->test_parse(available[n].plug, type, Filename, ft))) {
					accepts[n] = 1;
					(*accept_total)++;
				}
				else
					accepts[n] = 0;
				rewind(ft);
			}
		}
	}

	if (*accept_total == 0) {
		pcb_message(PCB_MSG_ERROR, "none of the IO_ plugin recognized the file format of %s - it's either not a valid board file or does not match the format specified\n", Filename);
		return -1;
	}
	return len;
}

int pcb_parse_pcb(pcb_board_t *Ptr, const char *Filename, const char *fmt, int load_settings, int ignore_missing)
{
	int res = -1, len, n;
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int accepts[PCB_IO_MAX_FORMATS]; /* test-parse output */
	int accept_total = 0;
	FILE *ft;

	ft = pcb_fopen(Filename, "r");
	len = pcb_test_parse_all(ft, Filename, fmt, PCB_IOT_PCB, available, accepts, &accept_total, sizeof(available)/sizeof(available[0]), ignore_missing, load_settings);
	if (ft != NULL)
		fclose(ft);
	if (len < 0)
		return -1;

	Ptr->Data->loader = NULL;

	/* try all plugins that said it could handle the file */
	for(n = 0; n < len; n++) {
		if ((available[n].plug->parse_pcb == NULL) || (!accepts[n])) /* can't parse or doesn't want to parse this file */
			continue;
		res = available[n].plug->parse_pcb(available[n].plug, Ptr, Filename, load_settings);
		if (res == 0) {
			if (Ptr->Data->loader == NULL) /* if the loader didn't set this (to some more fine grained, e.g. depending on file format version) */
				Ptr->Data->loader = available[n].plug;
			break;
		}
	}

	if ((res == 0) && (load_settings))
		conf_load_project(NULL, Filename);

	if (res == 0)
		pcb_set_design_dir(Filename);

	if (load_settings)
		pcb_event(PCB_EVENT_LOAD_POST, "si", Filename, res);

	plug_io_err(res, "load pcb", Filename);
	return res;
}

int pcb_parse_element(pcb_data_t *Ptr, const char *Filename, const char *fmt)
{
	int res = -1, len, n;
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int accepts[PCB_IO_MAX_FORMATS]; /* test-parse output */
	int accept_total = 0;
	FILE *f;
	pcb_fp_fopen_ctx_t fctx;

	f = pcb_fp_fopen(pcb_fp_default_search_path(), Filename, &fctx);
	len = pcb_test_parse_all(f, Filename, fmt, PCB_IOT_FOOTPRINT, available, accepts, &accept_total, sizeof(available)/sizeof(available[0]), 0, 0);
	if (f != NULL)
		pcb_fp_fclose(f, &fctx);
	if (len < 0)
		return -1;

	Ptr->loader = NULL;

	/* try all plugins that said it could handle the file */
	for(n = 0; n < len; n++) {
		if ((available[n].plug->parse_element == NULL) || (!accepts[n])) /* can't parse or doesn't want to parse this file */
			continue;
		res = available[n].plug->parse_element(available[n].plug, Ptr, Filename);
		if (res == 0) {
			if (Ptr->loader == NULL) /* if the loader didn't set this (to some more fine grained, e.g. depending on file format version) */
				Ptr->loader = available[n].plug;
			break;
		}
	}

	/* remove selected/found flag when loading reusable footprints */
	if (res == 0)
		pcb_data_flag_change(Ptr, PCB_OBJ_CLASS_REAL, PCB_CHGFLG_CLEAR, PCB_FLAG_FOUND | PCB_FLAG_SELECTED);

	plug_io_err(res, "load footprint", Filename);
	return res;
}

int pcb_parse_font(pcb_font_t *Ptr, const char *Filename)
{
	int res = -1;
	PCB_HOOK_CALL(pcb_plug_io_t, pcb_plug_io_chain, parse_font, res, == 0, (self, Ptr, Filename));

	plug_io_err(res, "load font", Filename);
	return res;
}

static int find_prio_cmp(const void *p1, const void *p2)
{
	const pcb_find_io_t *f1 = p1, *f2 = p2;
	if (f1->prio < f2->prio)
		return 1;
	return -1;
}

int pcb_find_io(pcb_find_io_t *available, int avail_len, pcb_plug_iot_t typ, int is_wr, const char *fmt)
{
	int len = 0;

#define cb_append(pl, pr) \
	do { \
		if (pr > 0) { \
			assert(len < avail_len ); \
			available[len].plug = pl; \
			if (fmt == NULL) \
				available[len].prio = pl->save_preference_prio; \
			else \
				available[len].prio = pr; \
			len++; \
		} \
	} while(0)

	PCB_HOOK_CALL_ALL(pcb_plug_io_t, pcb_plug_io_chain, fmt_support_prio, cb_append, (self, typ, is_wr, (fmt == NULL ? self->default_fmt : fmt)));

	if (len > 0)
		qsort(available, len, sizeof(available[0]), find_prio_cmp);
#undef cb_append

	return len;
}

/* Find the plugin that offers the highest write prio for the format */
static pcb_plug_io_t *find_writer(pcb_plug_iot_t typ, const char *fmt)
{
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int len;

	if (fmt == NULL) {
		if (PCB->Filename != NULL) { /* have a file name, guess from extension */
			int fn_len = strlen(PCB->Filename);
			const char *end = PCB->Filename + fn_len;
			pcb_plug_io_t *n, *best = NULL;
			int best_score = 0;

			/* the the plugin that has a matching extension and has the highest save_preference_prio value */
			for(n = pcb_plug_io_chain; n != NULL; n = n->next) {
				if (n->default_extension != NULL) {
					int elen = strlen(n->default_extension);
					if ((elen < fn_len) && (strcmp(end-elen, n->default_extension) == 0)) {
						if (n->save_preference_prio > best_score) {
							best_score = n->save_preference_prio;
							best = n;
						}
					}
				}
			}
			if (best != NULL)
				return best;
		}

		/* no file name or format hint, or file name not recognized: choose the ultimate default */
		fmt = conf_core.rc.save_final_fallback_fmt;
		if (fmt == NULL) {
			pcb_message(PCB_MSG_WARNING, "Saving a file with unknown format: failed to guess format from file name, no value configured in rc/save_final_fallback_fmt - CAN NOT SAVE FILE, try save as.\n");
			return NULL;
		}
		else {
			if (PCB->Filename != NULL)
				pcb_message(PCB_MSG_WARNING, "Saving a file with unknown format: failed to guess format from file name, falling back to %s as configured in rc/save_final_fallback_fmt\n", fmt);
		}
	}

	len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), typ, 1, fmt);

	if (len == 0)
		return NULL;

	return available[0].plug;
}


int pcb_write_buffer(FILE *f, pcb_buffer_t *buff, const char *fmt, pcb_bool elem_only)
{
	int res/*, newfmt = 0*/;
	pcb_plug_io_t *p = find_writer(PCB_IOT_BUFFER, fmt);

	if (p != NULL) {
		res = p->write_buffer(p, f, buff, elem_only);
		/*newfmt = 1;*/
	}

/*	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;*/

	plug_io_err(res, "write buffer", NULL);
	return res;
}

int pcb_write_element_data(FILE *f, pcb_data_t *e, const char *fmt)
{
	int res, newfmt = 0;
	pcb_plug_io_t *p = e->loader;

	if ((p == NULL) || ((fmt != NULL) && (*fmt != '\0'))) {
		p = find_writer(PCB_IOT_FOOTPRINT, fmt);
		newfmt = 1;
	}

	if ((p != NULL) && (p->write_element != NULL))
		res = p->write_element(p, f, e);

	if ((res == 0) && (newfmt))
		e->loader = p;

	plug_io_err(res, "write element", NULL);
	return res;
}

int pcb_write_font(pcb_font_t *Ptr, const char *Filename, const char *fmt)
{
	int res/*, newfmt = 0*/;
	pcb_plug_io_t *p = find_writer(PCB_IOT_FONT, fmt);

	if (p != NULL) {
		res = p->write_font(p, Ptr, Filename);
		/*newfmt = 1;*/
	}
	else
		res = -1;

/*	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;*/

	plug_io_err(res, "write font", NULL);
	return res;
}


static int pcb_write_pcb(FILE *f, const char *old_filename, const char *new_filename, const char *fmt, pcb_bool emergency)
{
	int res, newfmt = 0;
	pcb_plug_io_t *p = PCB->Data->loader;

	if ((p == NULL) || ((fmt != NULL) && (*fmt != '\0'))) {
		p = find_writer(PCB_IOT_PCB, fmt);
		newfmt = 1;
	}

	if (p != NULL) {
		if (p->write_pcb != NULL) {
			pcb_event(PCB_EVENT_SAVE_PRE, "s", fmt);
			res = p->write_pcb(p, f, old_filename, new_filename, emergency);
			pcb_event(PCB_EVENT_SAVE_POST, "si", fmt, res);
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Can't write PCB: internal error: io plugin %s doesn't implement write_pcb()\n", p->description);
			res = -1;
		}
	}

	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;

	if (res == 0)
		pcb_set_design_dir(new_filename);

	plug_io_err(res, "write pcb", NULL);
	return res;
}

/* ---------------------------------------------------------------------------
 * load PCB
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 *
 * If revert is pcb_true, we pass "revert" as a parameter
 * to the pcb changed event.
 */
static int real_load_pcb(const char *Filename, const char *fmt, pcb_bool revert, pcb_bool require_font, int how)
{
	const char *unit_suffix;
	char *new_filename;
	pcb_board_t *newPCB = pcb_board_new_(pcb_false);
	pcb_board_t *oldPCB;
	conf_role_t settings_dest;
#ifdef DEBUG
	double elapsed;
	clock_t start, end;

	start = clock();
#endif

	pcb_path_resolve(Filename, &new_filename, 0);

	oldPCB = PCB;
	PCB = newPCB;

	/* mark the default font invalid to know if the file has one */
	newPCB->fontkit.valid = pcb_false;

	switch(how & 0x0F) {
		case 0: settings_dest = CFR_DESIGN; break;
		case 1: settings_dest = CFR_DEFAULTPCB; break;
		case 2: settings_dest = CFR_invalid; break;
		default: abort();
	}

	/* new data isn't added to the undo list */
	if (!pcb_parse_pcb(PCB, new_filename, fmt, settings_dest, how & 0x10)) {
		pcb_board_remove(oldPCB);

		pcb_board_new_postproc(PCB, 0);
		if (how == 0) {
			/* update cursor location */
			pcb_crosshair.X = PCB_CLAMP(PCB->CursorX, 0, PCB->MaxWidth);
			pcb_crosshair.Y = PCB_CLAMP(PCB->CursorY, 0, PCB->MaxHeight);

			/* update cursor confinement and output area (scrollbars) */
			pcb_board_resize(PCB->MaxWidth, PCB->MaxHeight);
		}

		/* have to be called after pcb_board_resize() so vis update is after a board changed update */
		pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
		pcb_layervis_reset_stack();

		/* enable default font if necessary */
		if (!PCB->fontkit.valid) {
			if ((require_font) && (!PCB->is_footprint))
				pcb_message(PCB_MSG_WARNING, _("File '%s' has no font information, using default font\n"), new_filename);
			PCB->fontkit.valid = pcb_true;
		}

		/* footprint edition: let the user directly manipulate subc parts */
		PCB->loose_subc = PCB->is_footprint;

		/* clear 'changed flag' */
		pcb_board_set_changed_flag(pcb_false);
		PCB->Filename = new_filename;
		/* just in case a bad file saved file is loaded */

		/* Use attribute PCB::grid::unit as unit, if we can */
		unit_suffix = pcb_attrib_get(PCB, "PCB::grid::unit");
		if (unit_suffix && *unit_suffix) {
			const pcb_unit_t *new_unit = get_unit_struct(unit_suffix);
			if (new_unit)
				conf_set(settings_dest, "editor/grid_unit", -1, unit_suffix, POL_OVERWRITE);
		}
		pcb_attrib_put(PCB, "PCB::grid::unit", conf_core.editor.grid_unit->suffix);

		pcb_sort_netlist();
		pcb_ratspatch_make_edited(PCB);

/* set route style to the first one, if the current one doesn't
   happen to match any.  This way, "revert" won't change the route style. */
		if (pcb_hid_get_flag("GetStyle()") < 0)
			pcb_use_route_style_idx(&PCB->RouteStyle, 0);

		if ((how == 0) || (revert))
			pcb_board_changed(revert);

#ifdef DEBUG
		end = clock();
		elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
		pcb_gui->log("Loading file %s took %f seconds of CPU time\n", new_filename, elapsed);
#endif

		return 0;
	}

	free(new_filename);
	PCB = oldPCB;
	if (PCB == NULL) {
		/* bozo: we are trying to revert back to a non-existing pcb... create one to avoid a segfault */
		PCB = pcb_board_new_(pcb_false);
		if (PCB == NULL) {
			pcb_message(PCB_MSG_ERROR, "FATAL: can't create a new empty pcb!");
			exit(1);
		}
	}

	if (!(how & PCB_INHIBIT_BOARD_CHANGED))
		pcb_board_changed(0);

	/* release unused memory */
	pcb_board_remove(newPCB);

	return 1;
}


#if !defined(HAS_ATEXIT)
/* ---------------------------------------------------------------------------
 * some local identifiers for OS without an atexit() call
 */
static char *TMPFilename = NULL;
#endif

/* ---------------------------------------------------------------------------
 * Flag helper functions
 */

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

/* --------------------------------------------------------------------------- */

static int string_cmp(const char *a, const char *b)
{
	while (*a && *b) {
		if (isdigit((int) *a) && isdigit((int) *b)) {
			int ia = atoi(a);
			int ib = atoi(b);
			if (ia != ib)
				return ia - ib;
			while (isdigit((int) *a) && *(a + 1))
				a++;
			while (isdigit((int) *b) && *(b + 1))
				b++;
		}
		else if (tolower((int) *a) != tolower((int) *b))
			return tolower((int) *a) - tolower((int) *b);
		a++;
		b++;
	}
	if (*a)
		return 1;
	if (*b)
		return -1;
	return 0;
}

static int netlist_sort_offset = 0;

static int netlist_sort(const void *va, const void *vb)
{
	const pcb_lib_menu_t *am = (const pcb_lib_menu_t *) va;
	const pcb_lib_menu_t *bm = (const pcb_lib_menu_t *) vb;
	const char *a = am->Name;
	const char *b = bm->Name;
	if (*a == '~')
		a++;
	if (*b == '~')
		b++;
	return string_cmp(a, b);
}

static int netnode_sort(const void *va, const void *vb)
{
	const pcb_lib_entry_t *am = (const pcb_lib_entry_t *) va;
	const pcb_lib_entry_t *bm = (const pcb_lib_entry_t *) vb;
	const char *a = am->ListEntry;
	const char *b = bm->ListEntry;
	return string_cmp(a, b);
}

void pcb_library_sort(pcb_lib_t *lib)
{
	int i;
	qsort(lib->Menu, lib->MenuN, sizeof(lib->Menu[0]), netlist_sort);
	for (i = 0; i < lib->MenuN; i++)
		qsort(lib->Menu[i].Entry, lib->Menu[i].EntryN, sizeof(lib->Menu[i].Entry[0]), netnode_sort);
}

void pcb_sort_netlist()
{
	netlist_sort_offset = 2;
	pcb_library_sort(&(PCB->NetlistLib[PCB_NETLIST_INPUT]));
	netlist_sort_offset = 0;
}

/* ---------------------------------------------------------------------------
 * opens a file and check if it exists
 */
FILE *pcb_check_and_open_file(const char *Filename, pcb_bool Confirm, pcb_bool AllButton, pcb_bool * WasAllButton, pcb_bool * WasCancelButton)
{
	FILE *fp = NULL;
	struct stat buffer;
	char message[PCB_PATH_MAX + 80];
	int response;

	if (Filename && *Filename) {
		if (!stat(Filename, &buffer) && Confirm) {
			sprintf(message, _("File '%s' exists, use anyway?"), Filename);
			if (WasAllButton)
				*WasAllButton = pcb_false;
			if (WasCancelButton)
				*WasCancelButton = pcb_false;
			if (AllButton)
				response = pcb_gui->confirm_dialog(message, "Cancel", "Ok", AllButton ? "Sequence OK" : 0);
			else
				response = pcb_gui->confirm_dialog(message, "Cancel", "Ok", "Sequence OK");

			switch (response) {
			case 2:
				if (WasAllButton)
					*WasAllButton = pcb_true;
				break;
			case 0:
				if (WasCancelButton)
					*WasCancelButton = pcb_true;
			}
		}
		if ((fp = pcb_fopen(Filename, "w")) == NULL)
			pcb_open_error_message(Filename);
	}
	return fp;
}

/* ---------------------------------------------------------------------------
 * opens a file for saving connection data
 */
FILE *pcb_open_connection_file(void)
{
	char *fname;
	FILE *fp;
	static char *default_file = NULL;
	pcb_bool result;									/* not used */

	/* CheckAndOpenFile deals with the case where fname already exists */
	fname = pcb_gui->fileselect(_("Save Connection Data As ..."),
													_("Choose a file to save all connection data to."), default_file, ".net", "connection_data", 0);
	if (fname == NULL)
		return NULL;

	if (default_file != NULL) {
		free(default_file);
		default_file = NULL;
	}

	if (fname && *fname)
		default_file = pcb_strdup(fname);

	fp = pcb_check_and_open_file(fname, pcb_true, pcb_false, &result, NULL);
	free(fname);

	return fp;
}

/* ---------------------------------------------------------------------------
 * save elements in the current buffer
 */
int pcb_save_buffer_elements(const char *Filename, const char *fmt)
{
	int result;

	if (PCB_SWAP_IDENT)
		pcb_buffers_flip_side(PCB);
	result = pcb_write_pipe(Filename, pcb_false, fmt, pcb_true);
	if (PCB_SWAP_IDENT)
		pcb_buffers_flip_side(PCB);
	return result;
}

/* ---------------------------------------------------------------------------
 * save PCB
 */
int pcb_save_pcb(const char *file, const char *fmt)
{
	int retcode;

	if (pcb_gui->notify_save_pcb == NULL)
		return pcb_write_pipe(file, pcb_true, fmt, pcb_false);

	pcb_gui->notify_save_pcb(file, pcb_false);
	retcode = pcb_write_pipe(file, pcb_true, fmt, pcb_false);
	pcb_gui->notify_save_pcb(file, pcb_true);

	return retcode;
}


/* ---------------------------------------------------------------------------
 * Load PCB
 */
int pcb_load_pcb(const char *file, const char *fmt, pcb_bool require_font, int how)
{
	return real_load_pcb(file, fmt, pcb_false, require_font, how);
}

/* ---------------------------------------------------------------------------
 * Revert PCB
 */
int pcb_revert_pcb(void)
{
	return real_load_pcb(PCB->Filename, NULL, pcb_true, pcb_true, 1);
}

/* ---------------------------------------------------------------------------
 * writes the quoted string created by another subroutine
 */
void pcb_print_quoted_string(FILE * FP, const char *S)
{
	const char *start;

	fputc('"', FP);
	for(start = S; *S != '\0'; S++) {
		if (*S == '"' || *S == '\\') {
			if (start != S)
				fwrite(start, S-start, 1, FP);
			fputc('\\', FP);
			fputc(*S, FP);
			start = S+1;
		}
	}

	if (start != S)
		fwrite(start, S-start, 1, FP);

	fputc('"', FP);
}

/* ---------------------------------------------------------------------------
 * saves the layout in a temporary file
 * this is used for fatal errors and does not call the program specified
 * in 'saveCommand' for safety reasons
 */
void pcb_save_in_tmp(void)
{
	char filename[256];

	/* memory might have been released before this function is called */
	if (PCB && PCB->Changed && (conf_core.rc.emergency_name != NULL) && (*conf_core.rc.emergency_name != '\0')) {
		const char *fmt = conf_core.rc.emergency_format == NULL ? DEFAULT_FMT : conf_core.rc.emergency_name;
		sprintf(filename, conf_core.rc.emergency_name, (long int)pcb_getpid());
		pcb_message(PCB_MSG_INFO, _("Trying to save your layout in '%s'\n"), filename);
		pcb_write_pcb_file(filename, pcb_true, fmt, pcb_true, pcb_false);
	}
}

/* ---------------------------------------------------------------------------
 * front-end for 'pcb_save_in_tmp()'
 * just makes sure that the routine is only called once
 */
static pcb_bool dont_save_any_more = pcb_false;
void pcb_emergency_save(void)
{

	if (!dont_save_any_more) {
		dont_save_any_more = pcb_true;
		pcb_save_in_tmp();
	}
}

void pcb_disable_emergency_save(void)
{
	dont_save_any_more = pcb_true;
}

/* ----------------------------------------------------------------------
 * Callback for the autosave
 */

static pcb_hidval_t backup_timer;

/*
 * If the backup interval is > 0 then set another timer.  Otherwise
 * we do nothing and it is up to the GUI to call pcb_enable_autosave()
 * after setting conf_core.rc.backup_interval > 0 again.
 */
static void backup_cb(pcb_hidval_t data)
{
	backup_timer.ptr = NULL;
	pcb_backup();
	if (conf_core.rc.backup_interval > 0 && pcb_gui->add_timer)
		backup_timer = pcb_gui->add_timer(backup_cb, 1000 * conf_core.rc.backup_interval, data);
}

void pcb_enable_autosave(void)
{
	pcb_hidval_t x;

	x.ptr = NULL;

	/* If we already have a timer going, then cancel it out */
	if (backup_timer.ptr != NULL && pcb_gui->stop_timer)
		pcb_gui->stop_timer(backup_timer);

	backup_timer.ptr = NULL;
	/* Start up a new timer */
	if (conf_core.rc.backup_interval > 0 && pcb_gui->add_timer)
		backup_timer = pcb_gui->add_timer(backup_cb, 1000 * conf_core.rc.backup_interval, x);
}

/* ---------------------------------------------------------------------------
 * creates backup file.  The default is to use the pcb file name with
 * a "-" appended (like "foo.pcb-") and if we don't have a pcb file name
 * then use the template in conf_core.rc.backup_name
 */
void pcb_backup(void)
{
	char *filename = NULL;
	const char *fmt = NULL;

	if (PCB && PCB->Filename) {
		filename = (char *) malloc(sizeof(char) * (strlen(PCB->Filename) + 2));
		if (filename == NULL) {
			fprintf(stderr, "pcb_backup():  malloc failed\n");
			exit(1);
		}
		sprintf(filename, "%s-", PCB->Filename);
	}
	else {
		/* conf_core.rc.backup_name has %.8i which  will be replaced by the process ID */
		filename = pcb_build_fn(conf_core.rc.backup_name);
		if (filename == NULL) {
			fprintf(stderr, "pcb_backup(): can't build file name for a backup\n");
			exit(1);
		}
	}

	if ((conf_core.rc.backup_format != NULL) && (strcmp(conf_core.rc.backup_format, "original") != 0))
		fmt = conf_core.rc.backup_format;
	pcb_write_pcb_file(filename, pcb_true, fmt, pcb_true, pcb_false);
	free(filename);
}

#if !defined(HAS_ATEXIT)
/* ---------------------------------------------------------------------------
 * makes a temporary copy of the data. This is useful for systems which
 * doesn't support calling functions on exit. We use this to save the data
 * before LEX and YACC functions are called because they are able to abort
 * the program.
 */
void pcb_tmp_data_save(void)
{
	char *fn = pcb_build_fn(conf_core.rc.emergency_name);
	pcb_write_pcb_file(fn, pcb_true, NULL, pcb_true, pcb_false);
	if (TMPFilename != NULL)
		free(TMPFilename);
	TMPFilename = fn;
}

/* ---------------------------------------------------------------------------
 * removes the temporary copy of the data file
 */
void pcb_tmp_data_remove(void)
{
	if (TMPFilename != NULL)
		unlink(TMPFilename);
}
#endif

/* Write the pcb file, a footprint or a buffer */
static int pcb_write_file(FILE *fp, pcb_bool thePcb, const char *old_path, const char *new_path, const char *fmt, pcb_bool emergency, pcb_bool elem_only)
{
	if (thePcb) {
		if (PCB->is_footprint)
			return pcb_write_element_data(fp, PCB->Data, fmt);
		return pcb_write_pcb(fp, old_path, new_path, fmt, emergency);
	}
	return pcb_write_buffer(fp, PCB_PASTEBUFFER, fmt, elem_only);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int pcb_write_pcb_file(const char *Filename, pcb_bool thePcb, const char *fmt, pcb_bool emergency, pcb_bool elem_only)
{
	FILE *fp;
	int result, overwrite;
	char *fn_tmp = NULL;

	overwrite = pcb_file_readable(Filename);
	if (overwrite) {
		int len = strlen(Filename);
		fn_tmp = malloc(len+8);
		memcpy(fn_tmp, Filename, len);
		strcpy(fn_tmp+len, ".old");
		if (pcb_rename(Filename, fn_tmp) != 0) {
			if (emergency) {
				/* Try an alternative emergency file */
				strcpy(fn_tmp+len, ".emr");
				Filename = fn_tmp;
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Can't rename %s to %s before save\n", Filename, fn_tmp);
				return -1;
			}
		}
	}

	if ((fp = pcb_fopen(Filename, "w")) == NULL) {
		pcb_open_error_message(Filename);
		return (-1);
	}

	result = pcb_write_file(fp, thePcb, fn_tmp, Filename, fmt, emergency, elem_only);

	fclose(fp);
	if (fn_tmp != NULL) {
		if ((result == 0) && (!conf_core.rc.keep_save_backups))
			unlink(fn_tmp);
		free(fn_tmp);
	}
	return result;
}


/* ---------------------------------------------------------------------------
 * writes to pipe using the command defined by conf_core.rc.save_command
 * %f are replaced by the passed filename
 */
int pcb_write_pipe(const char *Filename, pcb_bool thePcb, const char *fmt, pcb_bool elem_only)
{
	FILE *fp;
	int result;
	const char *p;
	static gds_t command;
	const char *save_cmd;

	if (PCB_EMPTY_STRING_P(conf_core.rc.save_command))
		return pcb_write_pcb_file(Filename, thePcb, fmt, pcb_false, elem_only);

	if (!pcb_conf_cmd_is_safe(rc.save_command, &save_cmd, 1))
		return -1;

	/* setup commandline */
	gds_truncate(&command,0);
	for (p = save_cmd; *p; p++) {
		/* copy character if not special or add string to command */
		if (!(p[0] == '%' && p[1] == 'f'))
			gds_append(&command, *p);
		else {
			gds_append_str(&command, Filename);

			/* skip the character */
			p++;
		}
	}
	printf("write to pipe \"%s\"\n", command.array);
	if ((fp = pcb_popen(command.array, "w")) == NULL) {
		pcb_popen_error_message(command.array);
		return (-1);
	}

	result = pcb_write_file(fp, thePcb, NULL, NULL, fmt, pcb_false, elem_only);

	return (pcb_pclose(fp) ? (-1) : result);
}


int pcb_io_list(pcb_io_formats_t *out, pcb_plug_iot_t typ, int wr, int do_digest, pcb_io_list_ext_t ext)
{
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int n;

	memset(out, 0, sizeof(pcb_io_formats_t));
	out->len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), typ, 1, NULL);

	if (out->len == 0)
		return 0;

	for(n = 0; n < out->len; n++) {
		out->plug[n] = available[n].plug;
		switch(ext) {
			case PCB_IOL_EXT_NONE:  out->extension[n] = NULL; break;
			case PCB_IOL_EXT_BOARD: out->extension[n] = out->plug[n]->default_extension; break;
			case PCB_IOL_EXT_FP:    out->extension[n] = out->plug[n]->fp_extension; break;
		}
	}

	if (do_digest) {
		for(n = 0; n < out->len; n++)
			out->digest[n] = pcb_strdup_printf("%s (%s)", out->plug[n]->default_fmt, out->plug[n]->description);
		out->digest[n] = NULL;
	}

	return out->len;
}

void pcb_io_list_free(pcb_io_formats_t *out)
{
	int n;

	for(n = 0; n < out->len; n++) {
		if (out->digest[n] != NULL) {
			free(out->digest[n]);
			out->digest[n] = NULL;
		}
	}
}

pcb_cardinal_t pcb_io_incompat_save(pcb_data_t *data, pcb_any_obj_t *obj, const char *desc, const char *hint)
{
	pcb_message(PCB_MSG_ERROR, "save error: %s\n", desc);
	if (obj != NULL) {
		pcb_coord_t x = (obj->BoundingBox.X1 + obj->BoundingBox.X2)/2;
		pcb_coord_t y = (obj->BoundingBox.Y1 + obj->BoundingBox.Y2)/2;
		pcb_message(PCB_MSG_ERROR, " near %$mm %$mm\n", x, y);
	}
	if (hint != NULL)
		pcb_message(PCB_MSG_ERROR, " (%s)\n", hint);
	return 0;
}


void pcb_io_uninit(void)
{
	free(last_design_dir);
	last_design_dir = NULL;
}
