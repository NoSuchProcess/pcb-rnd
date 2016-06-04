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

#include "global.h"
#include "data.h"
#include "buffer.h"
#include "create.h"
#include "crosshair.h"
#include "draw.h"
#include "error.h"
#include "plug_io.h"
#include "set.h"
#include "action_helper.h"
#include "misc.h"
#include "compat_lrealpath.h"
#include "free_atexit.h"
#include "polygon.h"
#include "pcb-printf.h"
#include "buildin.h"
#include "paths.h"
#include "strflags.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "event.h"
#include "funchash.h"
#include "conf.h"

#include "hid_actions.h"
#include "hid_attrib.h"
#include "hid_init.h"

#ifdef HAVE_DBUS
#include "dbus.h"
#endif

#if ENABLE_NLS
#include <libintl.h>
#endif


RCSID("$Id$");


#define PCBLIBPATH ".:" PCBSHAREDIR

#ifdef HAVE_LIBSTROKE
extern void stroke_init(void);
#endif

#warning TODO: to conf
static const char *fontfile_paths_in[] = { "./default_font", PCBSHAREDIR "/default_font", NULL };
char **fontfile_paths = NULL;


/* Try gui libs in this order when not explicitly specified by the user
   if there are multiple GUIs available this sets the order of preference */
#warning TODO: to conf
static const char *try_gui_hids[] = { "gtk", "lesstif", NULL };

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


	/* ----------------------------------------------------------------------
	   |  command line and rc file processing.
	 */
static char *command_line_pcb;

void copyright(void)
{
	printf("\n"
				 "                COPYRIGHT for %s version %s\n\n"
				 "    PCB, interactive printed circuit board design\n"
				 "    Copyright (C) 1994,1995,1996,1997 Thomas Nau\n"
				 "    Copyright (C) 1998, 1999, 2000 Harry Eaton\n\n"
				 "    This program is free software; you can redistribute it and/or modify\n"
				 "    it under the terms of the GNU General Public License as published by\n"
				 "    the Free Software Foundation; either version 2 of the License, or\n"
				 "    (at your option) any later version.\n\n"
				 "    This program is distributed in the hope that it will be useful,\n"
				 "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
				 "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
				 "    GNU General Public License for more details.\n\n"
				 "    You should have received a copy of the GNU General Public License\n"
				 "    along with this program; if not, write to the Free Software\n"
				 "    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n", Progname, VERSION);
	exit(0);
}

static inline void u(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

typedef struct UsageNotes {
	struct UsageNotes *next;
	HID_Attribute *seen;
} UsageNotes;

static UsageNotes *usage_notes = NULL;

static void usage(void)
{
	HID **hl = hid_enumerate();
	HID_AttrNode *ha;
	UsageNotes *note;
	int i;
	int n_printer = 0, n_gui = 0, n_exporter = 0;

	for (i = 0; hl[i]; i++) {
		if (hl[i]->gui)
			n_gui++;
		if (hl[i]->printer)
			n_printer++;
		if (hl[i]->exporter)
			n_exporter++;
	}

	u("PCB Printed Circuit Board editing program, http://pcb.gpleda.org");
	u("%s [-h|-V|--copyright]\t\t\tHelp, version, copyright", Progname);
	u("%s [--gui GUI] [gui options] <pcb file>\t\tto edit", Progname);
	u("Available GUI hid%s:", n_gui == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->gui)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);
	u("%s -p [printing options] <pcb file>\tto print", Progname);
	u("Available printing hid%s:", n_printer == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->printer)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);
	u("%s -x hid [export options] <pcb file>\tto export", Progname);
	u("Available export hid%s:", n_exporter == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->exporter)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);

#warning TODO: do this on a per plugin basis
#if 0
	for (i = 0; hl[i]; i++)
		if (hl[i]->gui)
			usage_hid(hl[i]);
	for (i = 0; hl[i]; i++)
		if (hl[i]->printer)
			usage_hid(hl[i]);
	for (i = 0; hl[i]; i++)
		if (hl[i]->exporter)
			usage_hid(hl[i]);
#endif

	u("\nCommon options:");
#warning TODO: do this from conf_
#if 0
	for (ha = hid_attr_nodes; ha; ha = ha->next) {
		for (note = usage_notes; note && note->seen != ha->attributes; note = note->next);
		if (note)
			continue;
		for (i = 0; i < ha->n; i++) {
			usage_attr(ha->attributes + i);
		}
	}
#endif

	exit(1);
}

/* ---------------------------------------------------------------------- 
 * Print help or version messages.
 */

static void print_version()
{
	printf("PCB version %s\n", VERSION);
	exit(0);
}

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

#ifdef DEBUG
	printf("bindir      = %s\n", bindir);
	printf("pcblibdir   = %s\n", pcblibdir);
#endif

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
}

static int arg_match(const char *in, const char *shrt, const char *lng)
{
	return (strcmp(in, shrt) == 0) || (strcmp(in, lng) == 0);
}

int main(int argc, char *argv[])
{
	int i, n;
	char *cmd, *arg, *stmp;

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
			if (arg_match(cmd, "c", "-conf")) {
				if (conf_set_from_cli(arg, &stmp) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, stmp);
					exit(1);
				}
			}
		}
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


#warning TODO: move this in the cli arg proc loop

	/* Print usage or version if requested.  Then exit.  */
	if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "--help") == 0))
		usage();
	if (argc > 1 && strcmp(argv[1], "-V") == 0)
		print_version();
	/* Export pcb from command line if requested.  */
	if (argc > 1 && strcmp(argv[1], "-p") == 0) {
		exporter = gui = hid_find_printer();
		argc--;
		argv++;
	}
	else if (argc > 2 && strcmp(argv[1], "-x") == 0) {
		exporter = gui = hid_find_exporter(argv[2]);
		argc -= 2;
		argv += 2;
	}
	/* Otherwise start GUI. */
	else if (argc > 2 && strcmp(argv[1], "--gui") == 0) {
		gui = hid_find_gui(argv[2]);
		if (gui == NULL) {
			Message("Can't find the gui requested.\n");
			exit(1);
		}
		argc -= 2;
		argv += 2;
	}
	else {
		const char **g;

		gui = NULL;
		for (g = try_gui_hids; (*g != NULL) && (gui == NULL); g++) {
			gui = hid_find_gui(*g);
		}

		/* try anything */
		if (gui == NULL) {
			Message("Warning: can't find any of the preferred GUIs, falling back to anything available...\n");
			gui = hid_find_gui(NULL);
		}
	}

	/* Exit with error if GUI failed to start. */
	if (!gui)
		exit(1);

/* Initialize actions only when the gui is already known so only the right
   one is registered (there can be only one GUI). */
#include "generated_lists.h"

#warning TODO: this should be coming from the hardwired lht config file
	/* Set up layers. */
	for (i = 0; i < MAX_LAYER; i++) {
		char buf[20];
		sprintf(buf, "signal%d", i + 1);
		if (conf_set(CFR_DESIGN, "design/default_layer_name", i, buf, POL_OVERWRITE) != 0)
			printf("Can't set layer name\n");
	}

	gui->parse_arguments(&argc, &argv);

#warning make these actions
#if 0
	if (show_help || (argc > 1 && argv[1][0] == '-'))
		usage();
	if (show_version)
		print_version();
	if (show_defaults)
		print_defaults();
	if (show_copyright)
		copyright();
#endif

#warning TODO: move this to actions
#if 0
	if (show_actions) {
		print_actions();
		exit(0);
	}

	if (do_dump_actions) {
		extern void dump_actions(void);
		dump_actions();
		exit(0);
	}
#endif

	/* plugins may have installed their new fields, reinterpret
	   (memory lht -> memory bin) to get the new fields */
	conf_update();

	/* Create a new PCB object in memory */
	PCB = CreateNewPCB();

	if (PCB == NULL) {
		Message("Can't load the default pcb for creating an empty layout\n");
		exit(1);
	}

	/* Add silk layers to newly created PCB */
	CreateNewPCBPost(PCB, 1);
	if (argc > 1)
		command_line_pcb = argv[1];

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

#if HAVE_DBUS
	pcb_dbus_setup();
#endif

	EnableAutosave();

#warning TODO: update for new settings; also convert into a debug action
#ifdef DEBUG
	printf("Settings.FontPath            = \"%s\"\n", Settings.FontPath);
	printf("conf_core.appearance.color.elementPath         = \"%s\"\n", conf_core.appearance.color.elementPath);
	printf("conf_core.rc.library_search_paths  = \"%s\"\n", conf_core.rc.library_search_paths);
	printf("conf_core.rc.library_shell        = \"%s\"\n", conf_core.rc.library_shell);
	printf("Settings.MakeProgram = \"%s\"\n", UNKNOWN(Settings.MakeProgram));
	printf("Settings.GnetlistProgram = \"%s\"\n", UNKNOWN(Settings.GnetlistProgram));
#endif

	do {
		gui->do_export(0);
		gui = next_gui;
		next_gui = NULL;
		if (gui != NULL) {
			/* init the next GUI */
			gui->parse_arguments(&argc, &argv);
			if (gui->gui)
				InitCrosshair();
			SetMode(ARROW_MODE);
				hid_action("LibraryChanged");
		}
	} while(gui != NULL);

#if HAVE_DBUS
	pcb_dbus_finish();
#endif

	pcb_main_uninit();

	return (0);
}
