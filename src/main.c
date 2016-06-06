/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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


/* main program, initializes some stuff and handles user input
 */
#include "config.h"
#include "conf_core.h"

#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>								/* Seed for srand() */
#include <locale.h>

#include "data.h"
#include "buffer.h"
#include "create.h"
#include "crosshair.h"
#include "error.h"
#include "plug_io.h"
#include "set.h"
#include "misc.h"
#include "compat_lrealpath.h"
#include "free_atexit.h"
#include "polygon.h"
#include "buildin.h"
#include "paths.h"
#include "strflags.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "event.h"
#include "funchash.h"
#include "conf.h"
#include "conf_core.h"

#include "hid_actions.h"
#include "hid_attrib.h"
#include "hid_init.h"

#if ENABLE_NLS
#include <libintl.h>
#endif

RCSID("$Id$");

#warning TODO: to conf
static const char *fontfile_paths_in[] = { "./default_font", PCBSHAREDIR "/default_font", NULL };
char **fontfile_paths = NULL;

/* ----------------------------------------------------------------------
 * initialize signal and error handlers
 */
static void InitHandler(void)
{
/*
	signal(SIGHUP, CatchSignal);
	signal(SIGQUIT, CatchSignal);
	signal(SIGABRT, CatchSignal);
	signal(SIGSEGV, CatchSignal);
	signal(SIGTERM, CatchSignal);
	signal(SIGINT, CatchSignal);
*/

	/* calling external program by popen() may cause a PIPE signal,
	 * so we ignore it
	 */
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
}

static char *command_line_pcb;

/* ----------------------------------------------------------------------
 * Figure out the canonical name of the executed program
 * and fix up the defaults for various paths
 */
char *bindir = NULL;
char *exec_prefix = NULL;
char *pcblibdir = NULL;

static void InitPaths(char *argv0)
{
	size_t l;
	int i;
	int haspath;
	char *t1, *t2;
	int found_bindir = 0;

	/* see if argv0 has enough of a path to let lrealpath give the
	 * real path.  This should be the case if you invoke pcb with
	 * something like /usr/local/bin/pcb or ./pcb or ./foo/pcb
	 * but if you just use pcb and it exists in your path, you'll
	 * just get back pcb again.
	 */

#ifdef FAKE_BINDIR
	haspath = 1;
#else
	haspath = 0;
	for (i = 0; i < strlen(argv0); i++) {
		if (argv0[i] == PCB_DIR_SEPARATOR_C)
			haspath = 1;
	}
#endif

#ifdef DEBUG
	printf("InitPaths (%s): haspath = %d\n", argv0, haspath);
#endif

	if (haspath) {
#ifdef FAKE_BINDIR
		bindir = strdup(FAKE_BINDIR "/");
#else
		bindir = strdup(lrealpath(argv0));
#endif
		found_bindir = 1;
	}
	else {
		char *path, *p, *tmps;
		struct stat sb;
		int r;

		tmps = getenv("PATH");

		if (tmps != NULL) {
			path = strdup(tmps);

			/* search through the font path for a font file */
			for (p = strtok(path, PCB_PATH_DELIMETER); p && *p; p = strtok(NULL, PCB_PATH_DELIMETER)) {
#ifdef DEBUG
				printf("Looking for %s in %s\n", argv0, p);
#endif
				if ((tmps = (char *) malloc((strlen(argv0) + strlen(p) + 2) * sizeof(char))) == NULL) {
					fprintf(stderr, "InitPaths():  malloc failed\n");
					exit(1);
				}
				sprintf(tmps, "%s%s%s", p, PCB_DIR_SEPARATOR_S, argv0);
				r = stat(tmps, &sb);
				if (r == 0) {
#ifdef DEBUG
					printf("Found it:  \"%s\"\n", tmps);
#endif
					bindir = lrealpath(tmps);
					found_bindir = 1;
					free(tmps);
					break;
				}
				free(tmps);
			}
			free(path);
		}
	}

#ifdef DEBUG
	printf("InitPaths():  bindir = \"%s\"\n", bindir);
#endif

	if (found_bindir) {
		/* strip off the executible name leaving only the path */
		t2 = NULL;
		t1 = strchr(bindir, PCB_DIR_SEPARATOR_C);
		while (t1 != NULL && *t1 != '\0') {
			t2 = t1;
			t1 = strchr(t2 + 1, PCB_DIR_SEPARATOR_C);
		}
		if (t2 != NULL)
			*t2 = '\0';

#ifdef DEBUG
		printf("After stripping off the executible name, we found\n");
		printf("bindir = \"%s\"\n", bindir);
#endif
	}
	else {
		/* we have failed to find out anything from argv[0] so fall back to the original
		 * install prefix
		 */
		bindir = strdup(BINDIR);
	}

	/* now find the path to exec_prefix */
	l = strlen(bindir) + 1 + strlen(BINDIR_TO_EXECPREFIX) + 1;
	if ((exec_prefix = (char *) malloc(l * sizeof(char))) == NULL) {
		fprintf(stderr, "InitPaths():  malloc failed\n");
		exit(1);
	}
	sprintf(exec_prefix, "%s%s%s", bindir, PCB_DIR_SEPARATOR_S, BINDIR_TO_EXECPREFIX);

	/* now find the path to PCBSHAREDIR */
	l = strlen(bindir) + 1 + strlen(BINDIR_TO_PCBSHAREDIR) + 1;
	if ((pcblibdir = (char *) malloc(l * sizeof(char))) == NULL) {
		fprintf(stderr, "InitPaths():  malloc failed\n");
		exit(1);
	}
	sprintf(pcblibdir, "%s%s%s", bindir, PCB_DIR_SEPARATOR_S, BINDIR_TO_PCBSHAREDIR);

#warning TODO do we still need this or can we get it from the embedded lihata?
#if 0
	l = sizeof(main_attribute_list) / sizeof(main_attribute_list[0]);
	for (i = 0; i < l; i++) {
		if (NSTRCMP(main_attribute_list[i].name, "lib-command-dir") == 0) {
			main_attribute_list[i].default_val.str_value = pcblibdir;
		}

		if ((NSTRCMP(main_attribute_list[i].name, "font-path") == 0)
				|| (NSTRCMP(main_attribute_list[i].name, "element-path") == 0)
				|| (NSTRCMP(main_attribute_list[i].name, "lib-path") == 0)) {
			main_attribute_list[i].default_val.str_value = pcblibdir;
		}
	}
#endif

	paths_init_homedir();

	resolve_all_paths(fontfile_paths_in, fontfile_paths, 0);
}

/* ---------------------------------------------------------------------- 
 * main program
 */

char *program_name = 0;
char *program_basename = 0;
char *program_directory = 0;

#include "dolists.h"

static char **hid_argv_orig;

void pcb_main_uninit(void)
{
	int i;
	char **s;

	UninitBuffers();

	/* Free up memory allocated to the PCB. Why bother when we're about to exit ?
	 * Because it removes some false positives from heap bug detectors such as
	 * lib dmalloc.
	 */
	FreePCBMemory(PCB);
	free(PCB);
	PCB = NULL;

	plugins_uninit();
	hid_uninit();
	events_uninit();

	for (i = 0; i < MAX_LAYER; i++)
		free(conf_core.design.default_layer_name[i]);

	if (fontfile_paths != NULL) {
		for (s = fontfile_paths; *s != NULL; s++)
			free(*s);
		free(fontfile_paths);
		fontfile_paths = NULL;
	}

	uninit_strflags_buf();
	uninit_strflags_layerlist();

	fp_uninit();
	fp_host_uninit();
	funchash_uninit();

#define free0(ptr) \
	do {  \
	 if (ptr != NULL) { \
			free(ptr); \
			ptr = 0; \
		} \
	} while(0)

	free0(pcblibdir);
	free0(homedir);
	free0(bindir);
	free0(exec_prefix);
	free0(program_directory);
#undef free0
	free(hid_argv_orig);
}

static int arg_match(const char *in, const char *shrt, const char *lng)
{
	return ((shrt != NULL) && (strcmp(in, shrt) == 0)) || ((lng != NULL) && (strcmp(in, lng) == 0));
}

static const char *action_args[] = {
/*short, -long, action*/
	NULL, "-show-actions", "PrintActions()",
	NULL, "-show-paths",   "PrintPaths()",
	NULL, "-dump-actions", "DumpActions()",
	NULL, "-dump-config",  "dumpconf(native,1)",
	"h",  "-help",         "PrintUsage()",
	"?",  NULL,            "PrintUsage()",
	"V",  "-version",      "PrintVersion()",
	NULL, "-copyright",    "PrintCopyright()",
	NULL, NULL, NULL /* terminator */
};

int main(int argc, char *argv[])
{
	enum {
		DO_SOMETHING,
		DO_PRINT,
		DO_EXPORT,
		DO_GUI
	} do_what = DO_SOMETHING;
	int i, n, hid_argc = 0;
	char *cmd, *arg, *stmp, *hid_name = NULL, **hid_argv;
	const char **cs;
	const char *main_action = NULL;

	hid_argv_orig = hid_argv = calloc(sizeof(char *), argc);
#warning TODO: update this comment
	/* init application:
	 * - make program name available for error handlers
	 * - evaluate special options
	 * - initialize toplevel shell and resources
	 * - create an empty PCB with default symbols
	 * - initialize all other widgets
	 * - update screen and get size of drawing area
	 * - evaluate command-line arguments
	 * - register 'call on exit()' function
	 */

	conf_init();
	conf_core_init();

	/* process arguments */
	for(n = 1; n < argc; n++) {
		cmd = argv[n];
		arg = argv[n+1];
		if (*cmd == '-') {
			cmd++;
			if ((strcmp(cmd, "g") == 0) || (strcmp(cmd, "-gui") == 0)) {
				do_what = DO_GUI;
				hid_name = arg;
				n++;
				goto next_arg;
			}
			if ((strcmp(cmd, "x") == 0) || (strcmp(cmd, "-export") == 0)) {
				do_what = DO_EXPORT;
				hid_name = arg;
				n++;
				goto next_arg;
			}
			if ((strcmp(cmd, "p") == 0) || (strcmp(cmd, "-print") == 0)) {
				do_what = DO_PRINT;
				goto next_arg;
			}

			for(cs = action_args; cs[2] != NULL; cs += 3) {
				if (arg_match(cmd, cs[0], cs[1])) {
					if (main_action == NULL)
						main_action = cs[2];
					else
						fprintf(stderr, "Warning: can't execute multiple command line actions, ignoring %s\n", argv[n]);
					goto next_arg;
				}
			}
			if (arg_match(cmd, "c", "-conf")) {
				n++; /* eat up arg */
				if (conf_set_from_cli(arg, &stmp) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, stmp);
					exit(1);
				}
				goto next_arg;
			}
		}

		/* didn't handle argument, save it for the HID */
		hid_argv[hid_argc++] = argv[n];

		next_arg:;
	}
	conf_load_all();

	setbuf(stdout, 0);
	InitPaths(argv[0]);

	fp_init();

#ifdef LOCALEDIR
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	setlocale(LC_ALL, "");
#endif

	srand(time(NULL));						/* Set seed for rand() */

	funchash_init();
	initialize_units();
	polygon_init();
	hid_init();

	hid_load_settings();

	program_name = argv[0];
	program_basename = strrchr(program_name, PCB_DIR_SEPARATOR_C);
	if (program_basename) {
		program_directory = strdup(program_name);
		*strrchr(program_directory, PCB_DIR_SEPARATOR_C) = 0;
		program_basename++;
	}
	else {
		program_directory = strdup(".");
		program_basename = program_name;
	}
	Progname = program_basename;

	/* This must be called before any other atexit functions
	 * are registered, as it configures an atexit function to
	 * clean up and free various items of allocated memory,
	 * and must be the last last atexit function to run.
	 */
	leaky_init();

	/* Register a function to be called when the program terminates.
	 * This makes sure that data is saved even if LEX/YACC routines
	 * abort the program.
	 * If the OS doesn't have at least one of them,
	 * the critical sections will be handled by parse_l.l
	 */
	atexit(EmergencySave);

	events_init();

	buildin_init();
	plugins_init();


	/* Export pcb from command line if requested.  */
	switch(do_what) {
		case DO_PRINT:   exporter = gui = hid_find_printer(); break;
		case DO_EXPORT:  exporter = gui = hid_find_exporter(hid_name); break;
		case DO_GUI:
			gui = hid_find_gui(argv[2]);
			if (gui == NULL) {
				Message("Can't find the gui requested.\n");
				exit(1);
			}
			break;
		default: {
			conf_listitem_t *i;
			int n;
			const char *g;

			gui = NULL;
			conf_loop_list_str(&conf_core.rc.preferred_gui, i, g, n) {
				gui = hid_find_gui(g);
				if (gui != NULL)
					break;
			}

			/* try anything */
			if (gui == NULL) {
				Message("Warning: can't find any of the preferred GUIs, falling back to anything available...\n");
				gui = hid_find_gui(NULL);
			}
		}
	}

	/* Exit with error if GUI failed to start. */
	if (!gui)
		exit(1);

/* Initialize actions only when the gui is already known so only the right
   one is registered (there can be only one GUI). */
#include "generated_lists.h"

	/* plugins may have installed their new fields, reinterpret the config
	   (memory lht -> memory bin) to get the new fields */
	conf_update();

	if (main_action != NULL) {
		hid_parse_command(main_action);
		exit(0);
	}

	gui->parse_arguments(&hid_argc, &hid_argv);

	/* Create a new PCB object in memory */
	PCB = CreateNewPCB();

	if (PCB == NULL) {
		Message("Can't load the default pcb for creating an empty layout\n");
		exit(1);
	}

	/* Add silk layers to newly created PCB */
	CreateNewPCBPost(PCB, 1);
	if (argc > 1)
		command_line_pcb = hid_argv[1];

	ResetStackAndVisibility();

	if (gui->gui)
		InitCrosshair();
	InitHandler();
	InitBuffers();

	SetMode(ARROW_MODE);

	if (command_line_pcb) {
		/* keep filename even if initial load command failed;
		 * file might not exist
		 */
		if (LoadPCB(command_line_pcb, true, false))
			PCB->Filename = strdup(command_line_pcb);
	}

	if (conf_core.design.initial_layer_stack && conf_core.design.initial_layer_stack[0]) {
		LayerStringToLayerStack(conf_core.design.initial_layer_stack);
	}

	/* read the library file and display it if it's not empty
	 */
	if (!fp_read_lib_all() && library.data.dir.children.used)
		hid_action("LibraryChanged");

	if (conf_core.rc.script_filename) {
		Message(_("Executing startup script file %s\n"), conf_core.rc.script_filename);
		hid_actionl("ExecuteFile", conf_core.rc.script_filename, NULL);
	}
	if (conf_core.rc.action_string) {
		Message(_("Executing startup action %s\n"), conf_core.rc.action_string);
		hid_parse_actions(conf_core.rc.action_string);
	}

	if (gui->printer || gui->exporter) {
		/* Workaround to fix batch output for non-C locales */
		setlocale(LC_NUMERIC, "C");
		gui->do_export(0);
		exit(0);
	}

	EnableAutosave();

	/* main loop */
	do {
		gui->do_export(0);
		gui = next_gui;
		next_gui = NULL;
		if (gui != NULL) {
			/* init the next GUI */
			gui->parse_arguments(&hid_argc, &hid_argv);
			if (gui->gui)
				InitCrosshair();
			SetMode(ARROW_MODE);
				hid_action("LibraryChanged");
		}
	} while(gui != NULL);

	pcb_main_uninit();

	return (0);
}
