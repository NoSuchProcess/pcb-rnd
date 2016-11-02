/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* This used to be file.c; some of the code landed in the io_pcb plugin,
   the format-independent parts ended up here. */

/* file save, load, merge ... routines */

#warning TODO: do not hardwire this, make a function to decide
#define DEFAULT_FMT "pcb"

/* for popen() */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "config.h"
#include "conf_core.h"

#include <locale.h>
#include "global.h"

#include <dirent.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "change.h"
#include "create.h"
#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "misc.h"
#include "remove.h"
#include "set.h"
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

/* for opendir */
#include "compat_inc.h"

plug_io_t *plug_io_chain = NULL;
int pcb_io_err_inhibit = 0;

static void plug_io_err(int res, const char *what, const char *filename)
{
	if (pcb_io_err_inhibit)
		return;
	if (res != 0) {
		const char *reason = "", *comment = "";
		if (plug_io_chain != NULL) {
			if (filename == NULL) {
				reason = "none of io plugins could successfully write the file";
				filename = "";
			}
			else {
				FILE *f;
				reason = "none of io plugins could successfully read file";
				f = fopen(filename, "r");
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
		Message(PCB_MSG_DEFAULT, "IO error during %s: %s %s %s\n", what, reason, filename, comment);
	}
}

int ParsePCB(PCBTypePtr Ptr, const char *Filename, const char *fmt, int load_settings)
{
	int res = -1;

	if (load_settings)
		event(EVENT_LOAD_PRE, "s", Filename);

	Ptr->Data->loader = NULL;

	if (fmt != NULL) {
		pcb_find_io_t available[PCB_IO_MAX_FORMATS];
		int len, n;
		len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), PCB_IOT_PCB, 0, fmt);
		if (len <= 0) {
			Message(PCB_MSG_DEFAULT, "Error: can't find a IO_ plugin to load a PCB using format %s\n", fmt);
			return -1;
		}
		for(n = 0; n < len; n++) {
			if (available[0].plug->parse_pcb == NULL)
				continue;
			res = available[0].plug->parse_pcb(available[0].plug, Ptr, Filename, load_settings);
			if (res == 0) {
				Ptr->Data->loader = available[0].plug;
				break;
			}
		}
	}
	else /* try all parsers until we find one that works */
		HOOK_CALL_DO(plug_io_t, plug_io_chain, parse_pcb, res, == 0, (self, Ptr, Filename, load_settings), if (Ptr->Data->loader == NULL) Ptr->Data->loader = self);

	if ((res == 0) && (load_settings))
		conf_load_project(NULL, Filename);

	if (load_settings)
		event(EVENT_LOAD_POST, "si", Filename, res);

	plug_io_err(res, "load pcb", Filename);
	return res;
}

int ParseElement(DataTypePtr Ptr, const char *name)
{
	int res = -1;

	Ptr->loader = NULL;
	HOOK_CALL_DO(plug_io_t, plug_io_chain, parse_element, res, == 0, (self, Ptr, name), Ptr->loader = self);

	plug_io_err(res, "load element", name);
	return res;
}

int ParseFont(FontTypePtr Ptr, char *Filename)
{
	int res = -1;
	HOOK_CALL(plug_io_t, plug_io_chain, parse_font, res, == 0, (self, Ptr, Filename));

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

int pcb_find_io(pcb_find_io_t *available, int avail_len, plug_iot_t typ, int is_wr, const char *fmt)
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

	HOOK_CALL_ALL(plug_io_t, plug_io_chain, fmt_support_prio, cb_append, (self, typ, is_wr, (fmt == NULL ? self->default_fmt : fmt)));

	if (len > 0)
		qsort(available, len, sizeof(available[0]), find_prio_cmp);
#undef cb_append

	return len;
}

/* Find the plugin that offers the highest write prio for the format */
static plug_io_t *find_writer(plug_iot_t typ, const char *fmt)
{
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int len;

	if (fmt == NULL) {
#warning TODO: make this configurable, default to lihata
		fmt = "pcb";
	}

	len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), typ, 1, fmt);

	if (len == 0)
		return NULL;

	return available[0].plug;
}


int WriteBuffer(FILE *f, BufferType *buff, const char *fmt)
{
	int res, newfmt = 0;
	plug_io_t *p = find_writer(PCB_IOT_BUFFER, fmt);

	if (p != NULL) {
		res = p->write_buffer(p, f, buff);
		newfmt = 1;
	}

/*	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;*/

	plug_io_err(res, "write buffer", NULL);
	return res;
}

int WriteElementData(FILE *f, DataTypePtr e, const char *fmt)
{
	int res, newfmt = 0;
	plug_io_t *p = e->loader;

	if ((p == NULL) || ((fmt != NULL) && (*fmt != '\0'))) {
		p = find_writer(PCB_IOT_FOOTPRINT, fmt);
		newfmt = 1;
	}

	if (p != NULL)
		res = p->write_element(p, f, e);

	if ((res == 0) && (newfmt))
		e->loader = p;

	plug_io_err(res, "write element", NULL);
	return res;
}

static int pcb_write_pcb(FILE *f, const char *old_filename, const char *new_filename, const char *fmt, pcb_bool emergency)
{
	int res, newfmt = 0;
	plug_io_t *p = PCB->Data->loader;

	if ((p == NULL) || ((fmt != NULL) && (*fmt != '\0'))) {
		p = find_writer(PCB_IOT_PCB, fmt);
		newfmt = 1;
	}

	if (p != NULL) {
		event(EVENT_SAVE_PRE, "s", fmt);
		res = p->write_pcb(p, f, old_filename, new_filename, emergency);
		event(EVENT_SAVE_POST, "si", fmt, res);
	}

	if ((res == 0) && (newfmt))
		PCB->Data->loader = p;

	plug_io_err(res, "write pcb", NULL);
	return res;
}

/* ---------------------------------------------------------------------------
 * load PCB
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 *
 * If revert is pcb_true, we pass "revert" as a parameter
 * to the HID's PCBChanged action.
 */
static int real_load_pcb(const char *Filename, const char *fmt, pcb_bool revert, pcb_bool require_font, int how)
{
	const char *unit_suffix;
	char *new_filename;
	PCBTypePtr newPCB = CreateNewPCB_(pcb_false);
	PCBTypePtr oldPCB;
	conf_role_t settings_dest;
#ifdef DEBUG
	double elapsed;
	clock_t start, end;

	start = clock();
#endif

	resolve_path(Filename, &new_filename, 0);

	oldPCB = PCB;
	PCB = newPCB;

	/* mark the default font invalid to know if the file has one */
	newPCB->Font.Valid = pcb_false;

	switch(how) {
		case 0: settings_dest = CFR_DESIGN; break;
		case 1: settings_dest = CFR_DEFAULTPCB; break;
		case 2: settings_dest = CFR_invalid; break;
		default: abort();
	}

	/* new data isn't added to the undo list */
	if (!ParsePCB(PCB, new_filename, fmt, settings_dest)) {
		RemovePCB(oldPCB);

		CreateNewPCBPost(PCB, 0);
		ResetStackAndVisibility();

		if (how == 0) {
			/* update cursor location */
			Crosshair.X = PCB_CLAMP(PCB->CursorX, 0, PCB->MaxWidth);
			Crosshair.Y = PCB_CLAMP(PCB->CursorY, 0, PCB->MaxHeight);

			/* update cursor confinement and output area (scrollbars) */
			ChangePCBSize(PCB->MaxWidth, PCB->MaxHeight);
		}

		/* enable default font if necessary */
		if (!PCB->Font.Valid) {
			if (require_font)
				Message(PCB_MSG_DEFAULT, _("File '%s' has no font information, using default font\n"), new_filename);
			PCB->Font.Valid = pcb_true;
		}

		/* clear 'changed flag' */
		SetChangedFlag(pcb_false);
		PCB->Filename = new_filename;
		/* just in case a bad file saved file is loaded */

		/* Use attribute PCB::grid::unit as unit, if we can */
		unit_suffix = AttributeGet(PCB, "PCB::grid::unit");
		if (unit_suffix && *unit_suffix) {
			const Unit *new_unit = get_unit_struct(unit_suffix);
			if (new_unit)
				conf_set(settings_dest, "editor/grid_unit", -1, unit_suffix, POL_OVERWRITE);
		}
		AttributePut(PCB, "PCB::grid::unit", conf_core.editor.grid_unit->suffix);

		pcb_sort_netlist();
		rats_patch_make_edited(PCB);

/* set route style to the first one, if the current one doesn't
   happen to match any.  This way, "revert" won't change the route style. */
		if (hid_get_flag("GetStyle()") < 0)
			pcb_use_route_style_idx(&PCB->RouteStyle, 0);

		if (((how == 0) || (revert)) && (gui != NULL)) {
			if (revert)
				hid_actionl("PCBChanged", "revert", NULL);
			else
				hid_action("PCBChanged");
		}

#ifdef DEBUG
		end = clock();
		elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
		gui->log("Loading file %s took %f seconds of CPU time\n", new_filename, elapsed);
#endif

		return (0);
	}

	PCB = oldPCB;
	if (PCB == NULL) {
		/* bozo: we are trying to revert back to a non-existing pcb... create one to avoid a segfault */
		PCB = CreateNewPCB_(pcb_false);
		if (PCB == NULL) {
			Message(PCB_MSG_DEFAULT, "FATAL: can't create a new empty pcb!");
			exit(1);
		}
	}

	if (gui != NULL)
		hid_action("PCBChanged");

	/* release unused memory */
	RemovePCB(newPCB);

	return (1);
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

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

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
	const LibraryMenuType *am = (const LibraryMenuType *) va;
	const LibraryMenuType *bm = (const LibraryMenuType *) vb;
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
	const LibraryEntryType *am = (const LibraryEntryType *) va;
	const LibraryEntryType *bm = (const LibraryEntryType *) vb;
	const char *a = am->ListEntry;
	const char *b = bm->ListEntry;
	return string_cmp(a, b);
}

void sort_library(LibraryTypePtr lib)
{
	int i;
	qsort(lib->Menu, lib->MenuN, sizeof(lib->Menu[0]), netlist_sort);
	for (i = 0; i < lib->MenuN; i++)
		qsort(lib->Menu[i].Entry, lib->Menu[i].EntryN, sizeof(lib->Menu[i].Entry[0]), netnode_sort);
}

void pcb_sort_netlist()
{
	netlist_sort_offset = 2;
	sort_library(&(PCB->NetlistLib[NETLIST_INPUT]));
	netlist_sort_offset = 0;
}

/* ---------------------------------------------------------------------------
 * opens a file and check if it exists
 */
FILE *CheckAndOpenFile(const char *Filename, pcb_bool Confirm, pcb_bool AllButton, pcb_bool * WasAllButton, pcb_bool * WasCancelButton)
{
	FILE *fp = NULL;
	struct stat buffer;
	char message[MAXPATHLEN + 80];
	int response;

	if (Filename && *Filename) {
		if (!stat(Filename, &buffer) && Confirm) {
			sprintf(message, _("File '%s' exists, use anyway?"), Filename);
			if (WasAllButton)
				*WasAllButton = pcb_false;
			if (WasCancelButton)
				*WasCancelButton = pcb_false;
			if (AllButton)
				response = gui->confirm_dialog(message, "Cancel", "Ok", AllButton ? "Sequence OK" : 0);
			else
				response = gui->confirm_dialog(message, "Cancel", "Ok", "Sequence OK");

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
		if ((fp = fopen(Filename, "w")) == NULL)
			OpenErrorMessage(Filename);
	}
	return (fp);
}

/* ---------------------------------------------------------------------------
 * opens a file for saving connection data
 */
FILE *OpenConnectionDataFile(void)
{
	char *fname;
	FILE *fp;
	static char *default_file = NULL;
	pcb_bool result;									/* not used */

	/* CheckAndOpenFile deals with the case where fname already exists */
	fname = gui->fileselect(_("Save Connection Data As ..."),
													_("Choose a file to save all connection data to."), default_file, ".net", "connection_data", 0);
	if (fname == NULL)
		return NULL;

	if (default_file != NULL) {
		free(default_file);
		default_file = NULL;
	}

	if (fname && *fname)
		default_file = pcb_strdup(fname);

	fp = CheckAndOpenFile(fname, pcb_true, pcb_false, &result, NULL);
	free(fname);

	return fp;
}

/* ---------------------------------------------------------------------------
 * save elements in the current buffer
 */
int SaveBufferElements(const char *Filename, const char *fmt)
{
	int result;

	if (SWAP_IDENT)
		pcb_swap_buffers();
	result = WritePipe(Filename, pcb_false, fmt);
	if (SWAP_IDENT)
		pcb_swap_buffers();
	return (result);
}

/* ---------------------------------------------------------------------------
 * save PCB
 */
int SavePCB(const char *file, const char *fmt)
{
	int retcode;

	if (gui->notify_save_pcb == NULL)
		return WritePipe(file, pcb_true, fmt);

	gui->notify_save_pcb(file, pcb_false);
	retcode = WritePipe(file, pcb_true, fmt);
	gui->notify_save_pcb(file, pcb_true);

	return retcode;
}


/* ---------------------------------------------------------------------------
 * Load PCB
 */
int LoadPCB(const char *file, const char *fmt, pcb_bool require_font, int how)
{
	return real_load_pcb(file, fmt, pcb_false, require_font, how);
}

/* ---------------------------------------------------------------------------
 * Revert PCB
 */
int RevertPCB(void)
{
	return real_load_pcb(PCB->Filename, NULL, pcb_true, pcb_true, pcb_true);
}

/* ---------------------------------------------------------------------------
 * writes the quoted string created by another subroutine
 */
void PrintQuotedString(FILE * FP, const char *S)
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
void SaveInTMP(void)
{
	char filename[256];

	/* memory might have been released before this function is called */
	if (PCB && PCB->Changed && (conf_core.rc.emergency_name != NULL) && (*conf_core.rc.emergency_name != '\0')) {
		sprintf(filename, conf_core.rc.emergency_name, (long int)pcb_getpid());
		Message(PCB_MSG_DEFAULT, _("Trying to save your layout in '%s'\n"), filename);
		WritePCBFile(filename, pcb_true, DEFAULT_FMT, pcb_true);
	}
}

/* ---------------------------------------------------------------------------
 * front-end for 'SaveInTMP()'
 * just makes sure that the routine is only called once
 */
static pcb_bool dont_save_any_more = pcb_false;
void EmergencySave(void)
{

	if (!dont_save_any_more) {
		dont_save_any_more = pcb_true;
		SaveInTMP();
	}
}

void DisableEmergencySave(void)
{
	dont_save_any_more = pcb_true;
}

/* ----------------------------------------------------------------------
 * Callback for the autosave
 */

static hidval backup_timer;

/*
 * If the backup interval is > 0 then set another timer.  Otherwise
 * we do nothing and it is up to the GUI to call EnableAutosave()
 * after setting conf_core.rc.backup_interval > 0 again.
 */
static void backup_cb(hidval data)
{
	backup_timer.ptr = NULL;
	Backup();
	if (conf_core.rc.backup_interval > 0 && gui->add_timer)
		backup_timer = gui->add_timer(backup_cb, 1000 * conf_core.rc.backup_interval, data);
}

void EnableAutosave(void)
{
	hidval x;

	x.ptr = NULL;

	/* If we already have a timer going, then cancel it out */
	if (backup_timer.ptr != NULL && gui->stop_timer)
		gui->stop_timer(backup_timer);

	backup_timer.ptr = NULL;
	/* Start up a new timer */
	if (conf_core.rc.backup_interval > 0 && gui->add_timer)
		backup_timer = gui->add_timer(backup_cb, 1000 * conf_core.rc.backup_interval, x);
}

int pcb_build_fn_cb(gds_t *s, const char **input)
{
	char buff[20];

	switch(**input) {
		case 'P':
			sprintf(buff, "%.8i", pcb_getpid());
			gds_append_str(s, buff);
			(*input)++;
			return 0;
		case 'F':
			gds_append_str(s, (PCB->Filename != NULL) ? PCB->Filename : "no_file_name");
			(*input)++;
			return 0;
		case 'B':
			if (PCB->Filename != NULL) {
				char *bn = strrchr(PCB->Filename, '/');
				if (bn != NULL)
					bn++;
				else
					bn = PCB->Filename;
				gds_append_str(s, bn);
			}
			else
				gds_append_str(s, "no_file_name");
			(*input)++;
			return 0;
		case 'D':
			if (PCB->Filename != NULL) {
				char *bn = strrchr(PCB->Filename, '/');
				if (bn != NULL)
					gds_append_len(s, PCB->Filename, bn-PCB->Filename+1);
				else
					gds_append_str(s, "./");
			}
			else
				gds_append_str(s, "./");
			(*input)++;
			return 0;
		case 'N':
			gds_append_str(s, (PCB->Name != NULL) ? PCB->Name : "no_name");
			(*input)++;
			return 0;
		case 'T':
			sprintf(buff, "%lu", (unsigned long int)time(NULL));
			gds_append_str(s, buff);
			(*input)++;
			return 0;
	}
	return -1;
}

static char *build_fn(const char *template)
{
	return pcb_strdup_subst(template, pcb_build_fn_cb);
}

/* ---------------------------------------------------------------------------
 * creates backup file.  The default is to use the pcb file name with
 * a "-" appended (like "foo.pcb-") and if we don't have a pcb file name
 * then use the template in conf_core.rc.backup_name
 */
void Backup(void)
{
	char *filename = NULL;

	if (PCB && PCB->Filename) {
		filename = (char *) malloc(sizeof(char) * (strlen(PCB->Filename) + 2));
		if (filename == NULL) {
			fprintf(stderr, "Backup():  malloc failed\n");
			exit(1);
		}
		sprintf(filename, "%s-", PCB->Filename);
	}
	else {
		/* conf_core.rc.backup_name has %.8i which  will be replaced by the process ID */
		filename = build_fn(conf_core.rc.backup_name);
		if (filename == NULL) {
			fprintf(stderr, "Backup(): can't build file name for a backup\n");
			exit(1);
		}
	}

	WritePCBFile(filename, pcb_true, DEFAULT_FMT, pcb_true);
	free(filename);
}

#if !defined(HAS_ATEXIT)
/* ---------------------------------------------------------------------------
 * makes a temporary copy of the data. This is useful for systems which
 * doesn't support calling functions on exit. We use this to save the data
 * before LEX and YACC functions are called because they are able to abort
 * the program.
 */
void SaveTMPData(void)
{
	char *fn = build_fn(conf_core.rc.emergency_name);
	WritePCBFile(fn, pcb_true, DEFAULT_FMT, pcb_true);
	if (TMPFilename != NULL)
		free(TMPFilename);
	TMPFilename = fn;
}

/* ---------------------------------------------------------------------------
 * removes the temporary copy of the data file
 */
void RemoveTMPData(void)
{
	if (TMPFilename != NULL)
		unlink(TMPFilename);
}
#endif

/* Write the pcb file, a footprint or a buffer */
static int pcb_write_file(FILE *fp, pcb_bool thePcb, const char *old_path, const char *new_path, const char *fmt, pcb_bool emergency)
{
	if (thePcb) {
		if (PCB->is_footprint)
			return WriteElementData(fp, PCB->Data, fmt);
		return pcb_write_pcb(fp, old_path, new_path, fmt, emergency);
	}
	return WriteBuffer(fp, PASTEBUFFER, fmt);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int WritePCBFile(const char *Filename, pcb_bool thePcb, const char *fmt, pcb_bool emergency)
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
		if (rename(Filename, fn_tmp) != 0) {
			if (emergency) {
				/* Try an alternative emergency file */
				strcpy(fn_tmp+len, ".emr");
				Filename = fn_tmp;
			}
			else {
				Message(PCB_MSG_ERROR, "Can't rename %s to %s before save\n", Filename, fn_tmp);
				return -1;
			}
		}
	}

	if ((fp = fopen(Filename, "w")) == NULL) {
		OpenErrorMessage(Filename);
		return (STATUS_ERROR);
	}

	result = pcb_write_file(fp, thePcb, fn_tmp, Filename, fmt, emergency);

	fclose(fp);
	if (fn_tmp != NULL) {
		if ((result == 0) && (!conf_core.rc.keep_save_backups))
			unlink(fn_tmp);
		free(fn_tmp);
	}
	return (result);
}


/* ---------------------------------------------------------------------------
 * writes to pipe using the command defined by conf_core.rc.save_command
 * %f are replaced by the passed filename
 */
int WritePipe(const char *Filename, pcb_bool thePcb, const char *fmt)
{
	FILE *fp;
	int result;
	const char *p;
	static gds_t command;

	if (EMPTY_STRING_P(conf_core.rc.save_command))
		return WritePCBFile(Filename, thePcb, fmt, pcb_false);

	/* setup commandline */
	gds_truncate(&command,0);
	for (p = conf_core.rc.save_command; *p; p++) {
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
	if ((fp = popen(command.array, "w")) == NULL) {
		PopenErrorMessage(command.array);
		return (STATUS_ERROR);
	}

	result = pcb_write_file(fp, thePcb, NULL, NULL, fmt, pcb_false);

	return (pclose(fp) ? STATUS_ERROR : result);
}


int pcb_io_list(pcb_io_formats_t *out, plug_iot_t typ, int wr, int do_digest)
{
	pcb_find_io_t available[PCB_IO_MAX_FORMATS];
	int n;

	memset(out, 0, sizeof(pcb_io_formats_t));
	out->len = pcb_find_io(available, sizeof(available)/sizeof(available[0]), typ, 1, NULL);

	if (out->len == 0)
		return 0;

	for(n = 0; n < out->len; n++)
		out->plug[n] = available[n].plug;

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
