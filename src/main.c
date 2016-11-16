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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>								/* Seed for srand() */
#include <locale.h>

#include "board.h"
#include "error.h"
#include "plug_io.h"
#include "set.h"
#include "buffer.h"
#include "crosshair.h"
#include "compat_lrealpath.h"
#include "free_atexit.h"
#include "polygon.h"
#include "buildin.h"
#include "build_run.h"
#include "flag_str.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "event.h"
#include "funchash.h"
#include "conf.h"
#include "conf_core.h"

#include "hid_actions.h"
#include "hid_init.h"
#include "compat_misc.h"
#include "compat_nls.h"

/* ----------------------------------------------------------------------
 * initialize signal and error handlers
 */
static void InitHandler(void)
{
#ifdef PCB_HAVE_SIGHUP
	signal(SIGHUP, pcb_catch_signal);
#endif
#ifdef PCB_HAVE_SIGQUIT
	signal(SIGQUIT, pcb_catch_signal);
#endif
#ifdef PCB_HAVE_SIGTERM
	signal(SIGTERM, pcb_catch_signal);
#endif
#ifdef PCB_HAVE_SIGINT
	signal(SIGINT, pcb_catch_signal);
#endif

#ifdef NDEBUG
/* so that we get a core dump on segfault in debug mode */
#	ifdef PCB_HAVE_SIGABRT
		signal(SIGABRT, pcb_catch_signal);
#	endif
#	ifdef PCB_HAVE_SIGSEGV
		signal(SIGSEGV, pcb_catch_signal);
#	endif
#endif

	/* calling external program by popen() may cause a PIPE signal,
	 * so we ignore it
	 */
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
}

/* ----------------------------------------------------------------------
 * Figure out the canonical name of the executed program
 * and fix up the defaults for various paths
 */

static void InitPaths(char *argv0)
{
	size_t l;
	int haspath;
	char *t1, *t2;
	int found_bindir = 0;
	char *exec_prefix = NULL;
	char *bindir = NULL;


	/* see if argv0 has enough of a path to let lrealpath give the
	 * real path.  This should be the case if you invoke pcb with
	 * something like /usr/local/bin/pcb or ./pcb or ./foo/pcb
	 * but if you just use pcb and it exists in your path, you'll
	 * just get back pcb again.
	 */

#ifndef NO_BINDIR_HEURISTICS
	{
		int i;
		haspath = 0;
		for (i = 0; i < strlen(argv0); i++) {
			if (argv0[i] == PCB_DIR_SEPARATOR_C)
				haspath = 1;
		}
	}
#endif

#ifdef DEBUG
	printf("InitPaths (%s): haspath = %d\n", argv0, haspath);
#endif

	if (haspath) {
#ifndef NO_BINDIR_HEURISTICS
		bindir = pcb_strdup(pcb_lrealpath(argv0));
#endif
		found_bindir = 1;
	}
	else {
		char *path, *p, *tmps;
		struct stat sb;
		int r;

		tmps = getenv("PATH");

		if (tmps != NULL) {
			path = pcb_strdup(tmps);

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
					bindir = pcb_lrealpath(tmps);
					found_bindir = 1;
					free(tmps);
					break;
				}
				free(tmps);
			}
			free(path);
		}
	}

	if (found_bindir) {
		/* strip off the executable name leaving only the path */
		t2 = NULL;
		t1 = strchr(bindir, PCB_DIR_SEPARATOR_C);
		while (t1 != NULL && *t1 != '\0') {
			t2 = t1;
			t1 = strchr(t2 + 1, PCB_DIR_SEPARATOR_C);
		}
		if (t2 != NULL)
			*t2 = '\0';
	}
	else {
		/* we have failed to find out anything from argv[0] so fall back to the original
		 * install prefix
		 */
		bindir = pcb_strdup(BINDIR);
	}

	/* now find the path to exec_prefix */
	l = strlen(bindir) + 1 + strlen(BINDIR_TO_EXECPREFIX) + 1;
	if ((exec_prefix = (char *) malloc(l * sizeof(char))) == NULL) {
		fprintf(stderr, "InitPaths():  malloc failed\n");
		exit(1);
	}
	sprintf(exec_prefix, "%s%s%s", bindir, PCB_DIR_SEPARATOR_S, BINDIR_TO_EXECPREFIX);
	conf_set(CFR_INTERNAL, "rc/path/exec_prefix", -1, exec_prefix, POL_OVERWRITE);
	free(exec_prefix);
	free(bindir);
}

/* ----------------------------------------------------------------------
 * main program
 */

#include "dolists.h"

static char **hid_argv_orig;

void pcb_main_uninit(void)
{
	if (conf_isdirty(CFR_USER))
		conf_save_file(NULL, NULL, CFR_USER, NULL);

	pcb_uninit_buffers();

	/* Free up memory allocated to the PCB. Why bother when we're about to exit ?
	 * Because it removes some false positives from heap bug detectors such as
	 * lib dmalloc.
	 */
	pcb_board_free(PCB);
	free(PCB);
	PCB = NULL;

	pcb_plugins_uninit();
	pcb_hid_uninit();
	pcb_events_uninit();

	pcb_strflg_uninit_buf();
	pcb_strflg_uninit_layerlist();

	pcb_fp_uninit();
	pcb_fp_host_uninit();
	pcb_funchash_uninit();
	free(hid_argv_orig);
	conf_uninit();
}

static int arg_match(const char *in, const char *shrt, const char *lng)
{
	return ((shrt != NULL) && (strcmp(in, shrt) == 0)) || ((lng != NULL) && (strcmp(in, lng) == 0));
}

const char *pcb_action_args[] = {
/*short, -long, action, help */
	NULL, "-show-actions", "PrintActions()",     "Print all available actions (human readable) and exit",
	NULL, "-dump-actions", "DumpActions()",      "Print all available actions (script readable) and exit",
	NULL, "-show-paths",   "PrintPaths()",       "Print all configured paths and exit",
	NULL, "-dump-config",  "dumpconf(native,1)", "Print the config tree and exit",
	"V",  "-version",      "PrintVersion()",     "Print version info and exit",
	NULL, "-copyright",    "PrintCopyright()",   "Print copyright and exit",
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
	int n, hid_argc = 0;
	char *cmd, *arg, *hid_name = NULL, **hid_argv;
	const char **cs;
	const char *main_action = NULL;
	char *command_line_pcb = NULL;

	hid_argv_orig = hid_argv = calloc(sizeof(char *), argc);
	/* init application:
	 * - make program name available for error handlers
	 * - initialize infrastructure (e.g. the conf system)
	 * - evaluate options
	 * - create an empty PCB with default symbols
	 * - register 'call on exit()' function
	 */

	/* Minimal conf setup before we do anything else */
	conf_init();
	conf_core_init();
	conf_core_postproc(); /* to get all the paths initialized */

	/* process arguments */
	for(n = 1; n < argc; n++) {
		cmd = argv[n];
		arg = argv[n+1];
		if (*cmd == '-') {
			cmd++;
			if ((strcmp(cmd, "?") == 0) || (strcmp(cmd, "h") == 0) || (strcmp(cmd, "-help") == 0)) {
				if (arg != NULL) {
					/* memory leak, but who cares for --help? */
					main_action = pcb_strdup_printf("PrintUsage(%s)", arg);
					n++;
				}
				else
					main_action = "PrintUsage()";
				goto next_arg;
			}
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

			for(cs = pcb_action_args; cs[2] != NULL; cs += 4) {
				if (arg_match(cmd, cs[0], cs[1])) {
					if (main_action == NULL)
						main_action = cs[2];
					else
						fprintf(stderr, "Warning: can't execute multiple command line actions, ignoring %s\n", argv[n]);
					goto next_arg;
				}
			}
			if (arg_match(cmd, "c", "-conf")) {
				const char *why;
				n++; /* eat up arg */
				if (conf_set_from_cli(NULL, arg, NULL, &why) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, why);
					exit(1);
				}
				goto next_arg;
			}
		}

		/* didn't handle argument, save it for the HID */
		hid_argv[hid_argc++] = argv[n];

		next_arg:;
	}
	conf_load_all(NULL, NULL);

	setbuf(stdout, 0);
	InitPaths(argv[0]);

	pcb_fp_init();

#ifdef LOCALEDIR
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	setlocale(LC_ALL, "");
#endif

	srand(time(NULL));						/* Set seed for rand() */

	pcb_funchash_init();
	initialize_units();
	polygon_init();

	/* This must be called before any other atexit functions
	 * are registered, as it configures an atexit function to
	 * clean up and free various items of allocated memory,
	 * and must be the last last atexit function to run.
	 */
	pcb_leaky_init();

	/* Register a function to be called when the program terminates.
	 * This makes sure that data is saved even if LEX/YACC routines
	 * abort the program.
	 * If the OS doesn't have at least one of them,
	 * the critical sections will be handled by parse_l.l
	 */
	atexit(pcb_emergency_save);

	pcb_events_init();

	buildin_init();
	pcb_hid_init();
	pcb_plugins_init();


	/* Export pcb from command line if requested.  */
	switch(do_what) {
		case DO_PRINT:   exporter = gui = pcb_hid_find_printer(); break;
		case DO_EXPORT:  exporter = gui = pcb_hid_find_exporter(hid_name); break;
		case DO_GUI:
			gui = pcb_hid_find_gui(argv[2]);
			if (gui == NULL) {
				pcb_message(PCB_MSG_DEFAULT, "Can't find the gui requested.\n");
				exit(1);
			}
			break;
		default: {
			conf_listitem_t *i;
			int n;
			const char *g;

			gui = NULL;
			conf_loop_list_str(&conf_core.rc.preferred_gui, i, g, n) {
				gui = pcb_hid_find_gui(g);
				if (gui != NULL)
					break;
			}

			/* try anything */
			if (gui == NULL) {
				pcb_message(PCB_MSG_DEFAULT, "Warning: can't find any of the preferred GUIs, falling back to anything available...\n");
				gui = pcb_hid_find_gui(NULL);
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
	conf_update(NULL);

	if (main_action != NULL) {
		pcb_hid_parse_command(main_action);
		exit(0);
	}

	gui->parse_arguments(&hid_argc, &hid_argv);

	/* Create a new PCB object in memory */
	PCB = pcb_board_new();

	if (PCB == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't load the default pcb for creating an empty layout\n");
		exit(1);
	}

	/* Add silk layers to newly created PCB */
	pcb_board_new_postproc(PCB, 1);
	if (hid_argc > 0)
		command_line_pcb = hid_argv[0];

	ResetStackAndVisibility();

	if (gui->gui)
		pcb_crosshair_init();
	InitHandler();
	pcb_init_buffers();

	SetMode(PCB_MODE_ARROW);

	if (command_line_pcb) {
		/* keep filename even if initial load command failed;
		 * file might not exist
		 */
		if (pcb_load_pcb(command_line_pcb, NULL, pcb_true, 0))
			PCB->Filename = pcb_strdup(command_line_pcb);
	}

	if (conf_core.design.initial_layer_stack && conf_core.design.initial_layer_stack[0]) {
		LayerStringToLayerStack(conf_core.design.initial_layer_stack);
	}

	/* read the library file and display it if it's not empty
	 */
	if (!pcb_fp_read_lib_all() && library.data.dir.children.used)
		pcb_hid_action("LibraryChanged");

	if (conf_core.rc.script_filename) {
		pcb_message(PCB_MSG_DEFAULT, _("Executing startup script file %s\n"), conf_core.rc.script_filename);
		pcb_hid_actionl("ExecuteFile", conf_core.rc.script_filename, NULL);
	}
	if (conf_core.rc.action_string) {
		pcb_message(PCB_MSG_DEFAULT, _("Executing startup action %s\n"), conf_core.rc.action_string);
		pcb_hid_parse_actions(conf_core.rc.action_string);
	}

	if (gui->printer || gui->exporter) {
		/* Workaround to fix batch output for non-C locales */
		setlocale(LC_NUMERIC, "C");
		gui->do_export(0);
		exit(0);
	}

	pcb_enable_autosave();

	/* main loop */
	do {
		gui->do_export(0);
		gui = next_gui;
		next_gui = NULL;
		if (gui != NULL) {
			/* init the next GUI */
			gui->parse_arguments(&hid_argc, &hid_argv);
			if (gui->gui)
				pcb_crosshair_init();
			SetMode(PCB_MODE_ARROW);
				pcb_hid_action("LibraryChanged");
		}
	} while(gui != NULL);

	pcb_main_uninit();

	return (0);
}
