/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */


/* main program, initializes some stuff and handles user input
 */
#include "config.h"
#include <stdlib.h>

static const char *EXPERIMENTAL = NULL;

#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>								/* Seed for srand() */
#include <libminuid/libminuid.h>

#include "board.h"
#include "brave.h"
#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "buffer.h"
#include "crosshair.h"
#include "compat_lrealpath.h"
#include "polygon.h"
#include "buildin.h"
#include "build_run.h"
#include "file_loaded.h"
#include "flag.h"
#include "flag_str.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "plug_import.h"
#include "event.h"
#include "funchash.h"
#include "conf.h"
#include "conf_core.h"
#include "hidlib_conf.h"
#include <genvector/vtp0.h>
#include "layer_vis.h"
#include "layer_ui.h"
#include "obj_text.h"
#include "pcb_minuid.h"
#include "tool.h"
#include "color.h"
#include "netlist.h"
#include "extobj.h"

#include "actions.h"
#include "actions_pcb.h"
#include "hid_init.h"
#include "compat_misc.h"

const char *pcbhl_menu_file_paths[] = { "./", "~/.pcb-rnd/", PCBSHAREDIR "/", NULL };
const char *pcbhl_menu_name_fmt = "pcb-menu-%s.lht";

#define CONF_USER_DIR "~/" DOT_PCB_RND
const char *pcbhl_conf_userdir_path = CONF_USER_DIR;
const char *pcphl_conf_user_path = CONF_USER_DIR "/pcb-conf.lht";
const char *pcbhl_conf_sysdir_path = PCBSHAREDIR;
const char *pcbhl_conf_sys_path = PCBSHAREDIR "/pcb-conf.lht";
const char *pcbhl_app_package = PCB_PACKAGE;
const char *pcbhl_app_version = PCB_VERSION;
const char *pcbhl_app_url = "http://repo.hu/projects/pcb-rnd";


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
	   so we ignore it */
#ifdef PCB_HAVE_SIGPIPE
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
	int found_bindir = 0, se = 0;
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
		bindir = pcb_lrealpath(argv0);
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
	pcb_conf_set(CFR_INTERNAL, "rc/path/exec_prefix", -1, exec_prefix, POL_OVERWRITE);

	/* export the most important paths and data for child processes (e.g. parametric footprints) */
	se |= pcb_setenv("PCB_RND_VERSION",     PCB_VERSION,           1);
	se |= pcb_setenv("PCB_RND_REVISION",    PCB_REVISION,          1);
	se |= pcb_setenv("PCB_RND_PCBLIB",      PCBSHAREDIR "/pcblib", 1);
	se |= pcb_setenv("PCB_RND_SHARE",       PCBSHAREDIR,           1);
	se |= pcb_setenv("PCB_RND_LIB",         PCBLIBDIR,             1);
	se |= pcb_setenv("PCB_RND_EXEC_PREFIX", exec_prefix,           1);

	if (se != 0)
		fprintf(stderr, "WARNING: setenv() failed - external commands such as parametric footprints may not have a proper environment\n");

	free(exec_prefix);
	free(bindir);
}

/* ----------------------------------------------------------------------
 * main program
 */

#include "dolists.h"

static void gui_support_plugins(int load)
{
	static int loaded = 0;
	static pup_plugin_t *puphand;

	if (load && !loaded) {
		static const char *plugin_name = "dialogs";
		int state = 0;
		loaded = 1;
		pcb_message(PCB_MSG_DEBUG, "Loading GUI support plugin: '%s'\n", plugin_name);
		puphand = pup_load(&pcb_pup, (const char **)pcb_pup_paths, plugin_name, 0, &state);
		if (puphand == NULL)
			pcb_message(PCB_MSG_ERROR, "Error: failed to load GUI support plugin '%s'\n-> expect missing widgets and dialog boxes\n", plugin_name);
	}
	if (!load && loaded && (puphand != NULL)) {
		pup_unload(&pcb_pup, puphand, NULL);
		loaded = 0;
		puphand = NULL;
	}
}

extern void pcb_poly_uninit(void);

void pcb_main_uninit(void)
{
	if (pcb_log_last != NULL)
		pcb_log_last->seen = 1; /* ignore anything unseen before the uninit */


	pcb_brave_uninit();
	pcb_polygon_uninit();

	pcb_text_uninit();
	pcb_layer_vis_uninit();

	pcb_strflg_uninit_buf();
	pcb_strflg_uninit_layerlist();

	gui_support_plugins(0);
	pcb_render = pcb_gui = NULL;
	pcb_hidlib_uninit(); /* plugin unload */

	if (PCB != NULL) {
		pcb_uninit_buffers(PCB);

		/* Free up memory allocated to the PCB. Why bother when we're about to exit ?
		 * Because it removes some false positives from heap bug detectors such as
		 * valgrind. */
		pcb_board_free(PCB);
		free(PCB);
	}
	PCB = NULL;

	pcb_funchash_uninit();
	pcb_file_loaded_uninit();
	pcb_uilayer_uninit();
	pcb_cli_uninit();
	pcb_dynflag_uninit();

	pcb_extobj_uninit();
	pcb_import_uninit();
	pcb_io_uninit();
	pcb_fp_uninit();
	pcb_fp_host_uninit();
	pcb_tool_uninit();
	pcb_poly_uninit();

	pcbhl_log_print_uninit_errs("Log produced during uninitialization");
	pcb_log_uninit();
}

/* action table number of columns for a single action */
const char *pcb_action_args[] = {
/*short, -long, action, help, hint-on-error */
	NULL, "-show-actions",    "PrintActions()",     "Print all available actions (human readable) and exit",   NULL,
	NULL, "-dump-actions",    "DumpActions()",      "Print all available actions (script readable) and exit",  NULL,
	NULL, "-dump-plugins",    "DumpPlugins()",      "Print all available plugins (script readable) and exit",  NULL,
	NULL, "-dump-plugindirs", "DumpPluginDirs()",   "Print directories plugins might be loaded from and exit", NULL,
	NULL, "-dump-oflags",     "DumpObjFlags()",     "Print object flags and exit",                             NULL,
	NULL, "-show-paths",      "PrintPaths()",       "Print all configured paths and exit",                     NULL,
	NULL, "-dump-config",     "dumpconf(native,1)", "Print the config tree and exit",                          "Config dump not available - make sure you have configured pcb-rnd with --buildin-diag",
	"V",  "-version",         "PrintVersion()",     "Print version info and exit",                             NULL,
	"V",  "-dump-version",    "DumpVersion()",      "Print version info in script readable format and exit",   NULL,
	NULL, "-copyright",       "PrintCopyright()",   "Print copyright and exit",                                NULL,
	NULL, NULL, NULL, NULL, NULL /* terminator */
};

void print_pup_err(pup_err_stack_t *entry, char *string)
{
	pcb_message(PCB_MSG_ERROR, "puplug: %s\n", string);
}

int main(int argc, char *argv[])
{
	int n;
	char **sp, *command_line_pcb = NULL;

	pcbhl_main_args_t ga;

	pcb_fix_locale();

	pcbhl_main_args_init(&ga, argc, pcb_action_args);

	/* init application:
	 * - make program name available for error handlers
	 * - initialize infrastructure (e.g. the conf system)
	 * - evaluate options
	 * - create an empty PCB with default symbols
	 * - register 'call on exit()' function
	 */

	/* Minimal conf setup before we do anything else */
	pcb_netlist_geo_init();
	pcb_minuid_init();
	pcb_extobj_init();
	pcb_hidlib_init1(conf_core_init);
	pcb_layer_vis_init();
	pcb_brave_init();

	/* process arguments */
	for(n = 1; n < argc; n++) {
		/* optionally: handle extra arguments, not processed by the hidlib, here */
		n += pcbhl_main_args_add(&ga, argv[n], argv[n+1]);
	}
	pcb_tool_init(); /* init before the plugins so that the static tools have the lowest index */

	pcb_hidlib_init2(pup_buildins, NULL);
	pcb_actions_init_pcb_only();

	setbuf(stdout, 0);
	InitPaths(argv[0]);

	pcb_fp_init();

	srand(time(NULL));  /* Set seed for rand() */

	pcb_funchash_init();
	pcb_polygon_init();

	/* Register a function to be called when the program terminates.
	 * This makes sure that data is saved even if LEX/YACC routines
	 * abort the program.
	 * If the OS doesn't have at least one of them,
	 * the critical sections will be handled by parse_l.l */
	atexit(pcb_emergency_save);

	pcb_text_init();

	if (pcb_pup.err_stack != NULL) {
		pcb_message(PCB_MSG_ERROR, "Some of the static linked buildins could not be loaded:\n");
		pup_err_stack_process_str(&pcb_pup, print_pup_err);
	}

	for(sp = pcb_pup_paths; *sp != NULL; sp++) {
		pcb_message(PCB_MSG_DEBUG, "Loading plugins from '%s'\n", *sp);
		pup_autoload_dir(&pcb_pup, *sp, (const char **)pcb_pup_paths);
	}
	if (pcb_pup.err_stack != NULL) {
		pcb_message(PCB_MSG_ERROR, "Some of the dynamic linked plugins could not be loaded:\n");
		pup_err_stack_process_str(&pcb_pup, print_pup_err);
	}

	if (pcbhl_main_args_setup1(&ga) != 0) {
		pcb_main_uninit();
		pcbhl_main_args_uninit(&ga);
		exit(1);
	}

/* Initialize actions only when the gui is already known so only the right
   one is registered (there can be only one GUI). */
#include "generated_lists.h"

	if (pcbhl_main_args_setup2(&ga, &n) != 0) {
		pcb_main_uninit();
		pcbhl_main_args_uninit(&ga);
		exit(n);
	}

	if (PCB_HAVE_GUI_ATTR_DLG)
		gui_support_plugins(1);

	/* Create a new PCB object in memory */
	PCB = pcb_board_new(0);

	if (PCB == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't create an empty layout, exiting\n");
		pcbhl_log_print_uninit_errs("Initialization");
		exit(1);
	}

	/* Add silk layers to newly created PCB */
	pcb_board_new_postproc(PCB, 1);

	pcb_layervis_reset_stack();

	if (pcb_gui->gui)
		pcb_crosshair_init();
	InitHandler();
	pcb_init_buffers(PCB);

	pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_ARROW);

	if (ga.hid_argc > 0)
		command_line_pcb = ga.hid_argv[0];
	if (command_line_pcb) {
		if (pcb_load_pcb(command_line_pcb, NULL, pcb_true, 0) != 0) {
			if (pcbhl_main_exporting) {
				pcb_message(PCB_MSG_ERROR, "Can not load file '%s' (specified on command line) for exporting or printing\n", command_line_pcb);
				pcbhl_log_print_uninit_errs("Export load error");
				exit(1);
			}
			/* keep filename if load failed: file might not exist, save it by that name */
			PCB->hidlib.filename = pcb_strdup(command_line_pcb);
		}
	}

	if (conf_core.design.initial_layer_stack && conf_core.design.initial_layer_stack[0])
		pcb_message(PCB_MSG_ERROR, "Config setting desgin/initial_layer_stack is set but is deprecated and ignored. Please edit your config files to remove it.\n");

	/* read the library file and display it if it's not empty */
	if (!pcb_fp_read_lib_all() && pcb_library.data.dir.children.used)
		pcb_event(&PCB->hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);

	if (conf_core.rc.script_filename) {
		pcb_message(PCB_MSG_INFO, "Executing startup script file %s\n", conf_core.rc.script_filename);
		pcb_actionl("ExecuteFile", conf_core.rc.script_filename, NULL);
	}
	if (conf_core.rc.action_string) {
		pcb_message(PCB_MSG_INFO, "Executing startup action %s\n", conf_core.rc.action_string);
		pcb_parse_actions(conf_core.rc.action_string);
	}

	if (pcbhl_main_exported(&ga, &PCB->hidlib, pcb_data_is_empty(PCB->Data))) {
		pcb_main_uninit();
		pcbhl_main_args_uninit(&ga);
		exit(0);
	}

	pcb_enable_autosave();

	/* main loop */
	if (PCB_HAVE_GUI_ATTR_DLG)
		gui_support_plugins(1);
	if (EXPERIMENTAL != NULL) {
		pcb_message(PCB_MSG_ERROR, "******************************** IMPORTANT ********************************\n");
		pcb_message(PCB_MSG_ERROR, "This revision of pcb-rnd is experimental, unstable, do NOT attempt to use\n");
		pcb_message(PCB_MSG_ERROR, "it for production. The reason for this state is:\n");
		pcb_message(PCB_MSG_ERROR, "%s\n", EXPERIMENTAL);
		pcb_message(PCB_MSG_ERROR, "******************************** IMPORTANT ********************************\n");
	}
	pcb_tool_select_by_id(&PCB->hidlib, PCB_MODE_ARROW);
	pcb_event(&PCB->hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
	pcb_crosshair_init();
	pcbhl_mainloop_interactive(&ga, &PCB->hidlib);

	pcb_main_uninit();
	pcbhl_main_args_uninit(&ga);
	return 0;
}
