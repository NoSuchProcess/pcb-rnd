/* $Id$ */

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

/* file save, load, merge ... routines
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */

#warning TODO: do not hardwire this, make a function to decide
#define DEFAULT_FMT "pcb"

#include "config.h"
#include "conf_core.h"

#include <locale.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


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
#include "stub_edif.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "plugins.h"

plug_io_t *plug_io_chain = NULL;

RCSID("$Id$");

static void plug_io_err(int res, const char *what, const char *filename)
{
	if (res != 0) {
		char *reason = "", *comment = "";
		if (plug_io_chain != NULL) {
			if (filename == NULL) {
				reason = "none of io plugins could succesfully write the file";
				filename = "";
			}
			else {
				FILE *f;
				reason = "none of io plugins could succesfully read file";
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
		Message("IO error during %s: %s %s %s\n", what, reason, filename, comment);
	}
}

int ParsePCB(PCBTypePtr Ptr, char *Filename, int load_settings)
{
	int res = -1;
	HOOK_CALL(plug_io_t, plug_io_chain, parse_pcb, res, == 0, Ptr, Filename, load_settings);

	if ((res == 0) && (load_settings))
		conf_load_project(NULL, Filename);

	plug_io_err(res, "load pcb", Filename);
	return res;
}

int ParseElement(DataTypePtr Ptr, const char *name)
{
	int res = -1;
	HOOK_CALL(plug_io_t, plug_io_chain, parse_element, res, == 0, Ptr, name);

	plug_io_err(res, "load element", name);
	return res;
}

int ParseFont(FontTypePtr Ptr, char *Filename)
{
	int res = -1;
	HOOK_CALL(plug_io_t, plug_io_chain, parse_font, res, == 0, Ptr, Filename);

	plug_io_err(res, "load font", Filename);
	return res;
}

typedef struct {
	plug_io_t *plug;
	int prio;
} find_t;

static int find_prio_cmp(const void *p1, const void *p2)
{
	const find_t *f1 = p1, *f2 = p2;
	return f1->prio > f2->prio;
}

/* Find the plugin that offers the highest write prio for the format */
static plug_io_t *find_writer(const char *fmt)
{
	find_t available[32]; /* wish we had more than 32 IO plugins... */
	int len = 0;

	if (fmt == NULL) {
#warning TODO: rather save format information on load
		fmt = "pcb";
	}

#define cb_append(pl, pr) \
	do { \
		if (pr > 0) { \
			assert(len < sizeof(available)/sizeof(available[0])); \
			available[len].plug = pl; \
			available[len++].prio = pr; \
		} \
	} while(0)

	HOOK_CALL_ALL(plug_io_t, plug_io_chain, fmt_support_prio, cb_append, 1, fmt);
	if (len == 0)
		return NULL;

	qsort(available, len, sizeof(available[0]), find_prio_cmp);
	return available[0].plug;
#undef cb_append
}

int WriteBuffer(FILE *f, BufferType *buff, const char *fmt)
{
	int res;

	plug_io_t *p = find_writer(fmt);

	if (p != NULL)
		res = p->write_buffer(p, f, buff);

	plug_io_err(res, "write buffer", NULL);
	return res;
}

int WriteElementData(FILE *f, DataTypePtr e, const char *fmt)
{
	int res;

	plug_io_t *p = find_writer(fmt);

	if (p != NULL)
		res = p->write_element(p, f, e);

	plug_io_err(res, "write element", NULL);
	return res;
}

int WritePCB(FILE *f, const char *fmt)
{
	int res;
	plug_io_t *p = find_writer(fmt);

	if (p != NULL)
		res = p->write_pcb(p, f);
	plug_io_err(res, "write pcb", NULL);
	return res;
}


/* ---------------------------------------------------------------------------
 * load PCB
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 *
 * If revert is true, we pass "revert" as a parameter
 * to the HID's PCBChanged action.
 */
static int real_load_pcb(char *Filename, bool revert, bool require_font, int how)
{
	const char *unit_suffix;
	char *new_filename;
	PCBTypePtr newPCB = CreateNewPCB_(false);
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
	newPCB->Font.Valid = false;

	switch(how) {
		case 0: settings_dest = CFR_DESIGN; break;
		case 1: settings_dest = CFR_DEFAULTPCB; break;
		case 2: settings_dest = CFR_invalid; break;
		default: abort();
	}

	/* new data isn't added to the undo list */
	if (!ParsePCB(PCB, new_filename, settings_dest)) {
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
				Message(_("File '%s' has no font information, using default font\n"), new_filename);
			PCB->Font.Valid = true;
		}

		/* clear 'changed flag' */
		SetChangedFlag(false);
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

		sort_netlist();
		rats_patch_make_edited(PCB);

		set_some_route_style();

		if (how == 0) {
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
	LibraryMenuType *am = (LibraryMenuType *) va;
	LibraryMenuType *bm = (LibraryMenuType *) vb;
	char *a = am->Name;
	char *b = bm->Name;
	if (*a == '~')
		a++;
	if (*b == '~')
		b++;
	return string_cmp(a, b);
}

static int netnode_sort(const void *va, const void *vb)
{
	LibraryEntryType *am = (LibraryEntryType *) va;
	LibraryEntryType *bm = (LibraryEntryType *) vb;
	char *a = am->ListEntry;
	char *b = bm->ListEntry;
	return string_cmp(a, b);
}

void sort_library(LibraryTypePtr lib)
{
	int i;
	qsort(lib->Menu, lib->MenuN, sizeof(lib->Menu[0]), netlist_sort);
	for (i = 0; i < lib->MenuN; i++)
		qsort(lib->Menu[i].Entry, lib->Menu[i].EntryN, sizeof(lib->Menu[i].Entry[0]), netnode_sort);
}

void sort_netlist()
{
	netlist_sort_offset = 2;
	sort_library(&(PCB->NetlistLib[NETLIST_INPUT]));
	netlist_sort_offset = 0;
}

/* ---------------------------------------------------------------------------
 * opens a file and check if it exists
 */
FILE *CheckAndOpenFile(char *Filename, bool Confirm, bool AllButton, bool * WasAllButton, bool * WasCancelButton)
{
	FILE *fp = NULL;
	struct stat buffer;
	char message[MAXPATHLEN + 80];
	int response;

	if (Filename && *Filename) {
		if (!stat(Filename, &buffer) && Confirm) {
			sprintf(message, _("File '%s' exists, use anyway?"), Filename);
			if (WasAllButton)
				*WasAllButton = false;
			if (WasCancelButton)
				*WasCancelButton = false;
			if (AllButton)
				response = gui->confirm_dialog(message, "Cancel", "Ok", AllButton ? "Sequence OK" : 0);
			else
				response = gui->confirm_dialog(message, "Cancel", "Ok", "Sequence OK");

			switch (response) {
			case 2:
				if (WasAllButton)
					*WasAllButton = true;
				break;
			case 0:
				if (WasCancelButton)
					*WasCancelButton = true;
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
	bool result;									/* not used */

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
		default_file = strdup(fname);

	fp = CheckAndOpenFile(fname, true, false, &result, NULL);
	free(fname);

	return fp;
}

/* ---------------------------------------------------------------------------
 * save elements in the current buffer
 */
int SaveBufferElements(char *Filename, const char *fmt)
{
	int result;

	if (SWAP_IDENT)
		SwapBuffers();
	result = WritePipe(Filename, false, fmt);
	if (SWAP_IDENT)
		SwapBuffers();
	return (result);
}

/* ---------------------------------------------------------------------------
 * save PCB
 */
int SavePCB(char *file, const char *fmt)
{
	int retcode;

	if (gui->notify_save_pcb == NULL)
		return WritePipe(file, true, fmt);

	gui->notify_save_pcb(file, false);
	retcode = WritePipe(file, true, fmt);
	gui->notify_save_pcb(file, true);

	return retcode;
}

/* ---------------------------------------------------------------------------
 * set the route style to the first one, if the current one doesn't
 * happen to match any.  This way, "revert" won't change the route
 * style.
 */
void set_some_route_style()
{
	if (hid_get_flag("GetStyle()"))
		return;

	if (vtroutestyle_len(&PCB->RouteStyle) > 0) {
#warning TODO: call some central setter instead so we do not need to know the field names here
		SetLineSize(PCB->RouteStyle.array[0].Thick);
		SetViaSize(PCB->RouteStyle.array[0].Diameter, true);
		SetViaDrillingHole(PCB->RouteStyle.array[0].Hole, true);
		SetClearanceWidth(PCB->RouteStyle.array[0].Clearance);
	}
}

/* ---------------------------------------------------------------------------
 * Load PCB
 */
int LoadPCB(char *file, bool require_font, int how)
{
	return real_load_pcb(file, false, require_font, how);
}

/* ---------------------------------------------------------------------------
 * Revert PCB
 */
int RevertPCB(void)
{
	return real_load_pcb(PCB->Filename, true, true, true);
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
	char filename[80];

	/* memory might have been released before this function is called */
	if (PCB && PCB->Changed) {
		sprintf(filename, conf_core.rc.emergency_name, (int) getpid());
		Message(_("Trying to save your layout in '%s'\n"), filename);
		WritePCBFile(filename, DEFAULT_FMT);
	}
}

/* ---------------------------------------------------------------------------
 * front-end for 'SaveInTMP()'
 * just makes sure that the routine is only called once
 */
static bool dont_save_any_more = false;
void EmergencySave(void)
{

	if (!dont_save_any_more) {
		SaveInTMP();
		dont_save_any_more = true;
	}
}

void DisableEmergencySave(void)
{
	dont_save_any_more = true;
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

char *build_fn(const char *template)
{
	gds_t s;
	char buff[16];
	const char *curr, *next;

	gds_init(&s);
	for(curr = template;;) {
		next = strchr(curr, '%');
		if (next == NULL) {
			gds_append_str(&s, curr);
			return s.array;
		}
		if (next > curr)
			gds_append_len(&s, curr, next-curr);
		next++;
		switch(*next) {
			case '%':
				gds_append(&s, '%');
				curr = next+1;
				break;
			case 'P':
				sprintf(buff, "%.8i", getpid());
				gds_append_str(&s, buff);
				curr = next+1;
				break;
			default:
				gds_append(&s, '%');
				curr = next;
		}
	}
	abort(); /* can't get here */
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

	WritePCBFile(filename, DEFAULT_FMT);
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
	WritePCBFile(fn, DEFAULT_FMT);
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

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int WritePCBFile(char *Filename, const char *fmt)
{
	FILE *fp;
	int result;

	if ((fp = fopen(Filename, "w")) == NULL) {
		OpenErrorMessage(Filename);
		return (STATUS_ERROR);
	}
	result = WritePCB(fp, fmt);
	fclose(fp);
	return (result);
}


/* ---------------------------------------------------------------------------
 * writes to pipe using the command defined by conf_core.rc.save_command
 * %f are replaced by the passed filename
 */
int WritePipe(char *Filename, bool thePcb, const char *fmt)
{
	FILE *fp;
	int result;
	char *p;
	static gds_t command;
	int used_popen = 0;

	if (EMPTY_STRING_P(conf_core.rc.save_command)) {
		fp = fopen(Filename, "w");
		if (fp == 0) {
			Message("Unable to write to file %s\n", Filename);
			return STATUS_ERROR;
		}
	}
	else {
		used_popen = 1;
		/* setup commandline */
		gds_truncate(&command,0);
		for (p = conf_core.rc.save_command; *p; p++) {
			/* copy character if not special or add string to command */
			if (!(*p == '%' && *(p + 1) == 'f'))
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
	}
	if (thePcb) {
		if (PCB->is_footprint) {
			WriteElementData(fp, PCB->Data, fmt);
			result = 0;
		}
		else
			result = WritePCB(fp, fmt);
	}
	else
		result = WriteBuffer(fp, PASTEBUFFER, fmt);

	if (used_popen)
		return (pclose(fp) ? STATUS_ERROR : result);
	return (fclose(fp) ? STATUS_ERROR : result);
}



#define BLANK(x) ((x) == ' ' || (x) == '\t' || (x) == '\n' \
		|| (x) == '\0')

/* ---------------------------------------------------------------------------
 * Read in a netlist and store it in the netlist menu 
 */
#warning TODO: convert this into a HOOK_* based plugin from stubs (vs. edif down there)
int ReadNetlist(char *filename)
{
	char *command = NULL;
	char inputline[MAX_NETLIST_LINE_LENGTH + 1];
	char temp[MAX_NETLIST_LINE_LENGTH + 1];
	FILE *fp;
	LibraryMenuTypePtr menu = NULL;
	LibraryEntryTypePtr entry;
	int i, j, lines, kind;
	bool continued;
	int used_popen = 0;

	if (!filename)
		return (1);									/* nothing to do */

	Message(_("Importing PCB netlist %s\n"), filename);

	if (EMPTY_STRING_P(conf_core.rc.rat_command)) {
		fp = fopen(filename, "r");
		if (!fp) {
			Message("Cannot open %s for reading", filename);
			return 1;
		}
	}
	else {
		used_popen = 1;
		command = EvaluateFilename(conf_core.rc.rat_command, conf_core.rc.rat_path, filename, NULL);

		/* open pipe to stdout of command */
		if (*command == '\0' || (fp = popen(command, "r")) == NULL) {
			PopenErrorMessage(command);
			free(command);
			return (1);
		}
		free(command);
	}
	lines = 0;
	/* kind = 0  is net name
	 * kind = 1  is route style name
	 * kind = 2  is connection
	 */
	kind = 0;
	while (fgets(inputline, MAX_NETLIST_LINE_LENGTH, fp)) {
		size_t len = strlen(inputline);
		/* check for maximum length line */
		if (len) {
			if (inputline[--len] != '\n')
				Message(_("Line length (%i) exceeded in netlist file.\n"
									"additional characters will be ignored.\n"), MAX_NETLIST_LINE_LENGTH);
			else
				inputline[len] = '\0';
		}
		continued = (inputline[len - 1] == '\\') ? true : false;
		if (continued)
			inputline[len - 1] = '\0';
		lines++;
		i = 0;
		while (inputline[i] != '\0') {
			j = 0;
			/* skip leading blanks */
			while (inputline[i] != '\0' && BLANK(inputline[i]))
				i++;
			if (kind == 0) {
				/* add two spaces for included/unincluded */
				temp[j++] = ' ';
				temp[j++] = ' ';
			}
			while (!BLANK(inputline[i]))
				temp[j++] = inputline[i++];
			temp[j] = '\0';
			while (inputline[i] != '\0' && BLANK(inputline[i]))
				i++;
			if (kind == 0) {
				menu = GetLibraryMenuMemory(&PCB->NetlistLib[NETLIST_INPUT], NULL);
				menu->Name = strdup(temp);
				menu->flag = 1;
				kind++;
			}
			else {
				if (kind == 1 && strchr(temp, '-') == NULL) {
					kind++;
					menu->Style = strdup(temp);
				}
				else {
					entry = GetLibraryEntryMemory(menu);
					entry->ListEntry = strdup(temp);
					entry->ListEntry_dontfree = 0;
				}
			}
		}
		if (!continued)
			kind = 0;
	}
	if (!lines) {
		Message(_("Empty netlist file!\n"));
		pclose(fp);
		return (1);
	}
	if (used_popen)
		pclose(fp);
	else
		fclose(fp);
	sort_netlist();
	rats_patch_make_edited(PCB);
	return (0);
}

int ImportNetlist(char *filename)
{
	FILE *fp;
	char buf[16];
	int i;
	char *p;


	if (!filename)
		return (1);									/* nothing to do */
	fp = fopen(filename, "r");
	if (!fp)
		return (1);									/* bad filename */
	i = fread(buf, 1, sizeof(buf) - 1, fp);
	fclose(fp);
	buf[i] = '\0';
	p = buf;
	while (*p) {
		*p = tolower((int) *p);
		p++;
	}
	p = strstr(buf, "edif");
	if (!p)
		return ReadNetlist(filename);
	else
		return stub_ReadEdifNetlist(filename);
}
