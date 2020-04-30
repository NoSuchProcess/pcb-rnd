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

#include "config.h"
#include <stdlib.h>

static const char *EXPERIMENTAL = NULL;

#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h> /* Seed for srand() */
#include <libminuid/libminuid.h>

#include "board.h"
#include "brave.h"
#include "data.h"
#include <librnd/core/error.h>
#include "plug_io.h"
#include "buffer.h"
#include "crosshair.h"
#include <librnd/core/compat_lrealpath.h>
#include "polygon.h"
#include "buildin.h"
#include "build_run.h"
#include <librnd/core/file_loaded.h>
#include "flag.h"
#include "flag_str.h"
#include <librnd/core/plugins.h>
#include "plug_footprint.h"
#include "plug_import.h"
#include "event.h"
#include <librnd/core/funchash.h>
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <genvector/vtp0.h>
#include "layer_vis.h"
#include "layer_ui.h"
#include "obj_text.h"
#include "pcb_minuid.h"
#include <librnd/core/tool.h>
#include <librnd/core/color.h>
#include "netlist.h"
#include "extobj.h"
#include <librnd/core/pixmap.h>

#include <librnd/core/actions.h>
#include "actions_pcb.h"
#include <librnd/core/hid_init.h>
#include <librnd/core/compat_misc.h>
#include "tool_logic.h"
#include "pixmap_pcb.h"

const char *pcbhl_menu_file_paths[4];
const char *pcbhl_menu_name_fmt = "pcb-menu-%s.lht";

#define CONF_USER_DIR "~/" DOT_PCB_RND
const char *pcbhl_conf_userdir_path, *pcphl_conf_user_path;
const char *pcbhl_conf_sysdir_path, *pcbhl_conf_sys_path;
const char *pcbhl_app_package = PCB_PACKAGE;
const char *pcbhl_app_version = PCB_VERSION;
const char *pcbhl_app_url = "http://repo.hu/projects/pcb-rnd";

/* Figure out the canonical name of the executed program
   and fix up the defaults for various paths; returns exec prefix that
   should be saved in the config */
static char *main_path_init(char *argv0)
{
	size_t l;
	int haspath;
	char *t1, *t2;
	int found_bindir = 0, se = 0;
	char *exec_prefix = NULL;
	char *bindir = NULL, *tmp;


	/* see if argv0 has enough of a path to let lrealpath give the
	   real path.  This should be the case if you invoke pcb with
	   something like /usr/local/bin/pcb or ./pcb or ./foo/pcb
	   but if you just use pcb and it exists in your path, you'll
	   just get back pcb again. */
	haspath = (strchr(argv0, RND_DIR_SEPARATOR_C) != NULL);

#ifdef DEBUG
	printf("main_path_init (%s): haspath = %d\n", argv0, haspath);
#endif

	if (haspath) {
		bindir = rnd_lrealpath(argv0);
		found_bindir = 1;
	}
	else {
		char *path, *p, *tmps;
		struct stat sb;
		int r;

		tmps = getenv("PATH");

		if (tmps != NULL) {
			path = rnd_strdup(tmps);

			/* search through the font path for a font file */
			for (p = strtok(path, RND_PATH_DELIMETER); p && *p; p = strtok(NULL, RND_PATH_DELIMETER)) {
#ifdef DEBUG
				printf("Looking for %s in %s\n", argv0, p);
#endif
				if ((tmps = (char *) malloc((strlen(argv0) + strlen(p) + 2) * sizeof(char))) == NULL) {
					fprintf(stderr, "main_path_init():  malloc failed\n");
					exit(1);
				}
				sprintf(tmps, "%s%s%s", p, RND_DIR_SEPARATOR_S, argv0);
				r = stat(tmps, &sb);
				if (r == 0) {
#ifdef DEBUG
					printf("Found it:  \"%s\"\n", tmps);
#endif
					bindir = rnd_lrealpath(tmps);
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
		t1 = strchr(bindir, RND_DIR_SEPARATOR_C);
		while (t1 != NULL && *t1 != '\0') {
			t2 = t1;
			t1 = strchr(t2 + 1, RND_DIR_SEPARATOR_C);
		}
		if (t2 != NULL)
			*t2 = '\0';
	}
	else {
		/* we have failed to find out anything from argv[0] so fall back to the original install prefix */
		bindir = rnd_strdup(BINDIR);
	}

	/* now find the path to exec_prefix */
	l = strlen(bindir) + 1 + strlen(BINDIR_TO_EXECPREFIX) + 1;
	if ((exec_prefix = (char *) malloc(l * sizeof(char))) == NULL) {
		fprintf(stderr, "main_path_init():  malloc failed\n");
		exit(1);
	}
	sprintf(exec_prefix, "%s%s%s", bindir, RND_DIR_SEPARATOR_S, BINDIR_TO_EXECPREFIX);

	/* export the most important paths and data for child processes (e.g. parametric footprints) */
	tmp = pcb_concat(PCBSHAREDIR, "/pcblib", NULL);
	se |= rnd_setenv("PCB_RND_VERSION",     PCB_VERSION,           1);
	se |= rnd_setenv("PCB_RND_REVISION",    PCB_REVISION,          1);
	se |= rnd_setenv("PCB_RND_PCBLIB",      tmp,                   1);
	se |= rnd_setenv("PCB_RND_SHARE",       PCBSHAREDIR,           1);
	se |= rnd_setenv("PCB_RND_LIB",         PCBLIBDIR,             1);
	se |= rnd_setenv("PCB_RND_EXEC_PREFIX", exec_prefix,           1);
	free(tmp);

	if (se != 0)
		fprintf(stderr, "WARNING: setenv() failed - external commands such as parametric footprints may not have a proper environment\n");

	pcbhl_menu_file_paths[0] = "./";
	pcbhl_menu_file_paths[1] = "~/.pcb-rnd/";
	pcbhl_menu_file_paths[2] = pcb_concat(PCBCONFDIR, "/", NULL);
	pcbhl_menu_file_paths[3] = NULL;

	pcbhl_conf_userdir_path = CONF_USER_DIR;
	pcphl_conf_user_path = pcb_concat(CONF_USER_DIR, "/pcb-conf.lht", NULL);
	pcbhl_conf_sysdir_path = PCBCONFDIR;
	pcbhl_conf_sys_path = pcb_concat(PCBCONFDIR, "/pcb-conf.lht", NULL);

	free(bindir);
	return exec_prefix;
}

static void main_path_uninit(void)
{
	/* const for all other parts of the code, but we had to concat (alloc) it above */
	free((char *)pcbhl_menu_file_paths[2]);
	free((char *)pcphl_conf_user_path);
	free((char *)pcbhl_conf_sys_path);
}


/* initialize signal and error handlers */
static void main_sighand_init(void)
{
#ifdef RND_HAVE_SIGHUP
	signal(SIGHUP, pcb_catch_signal);
#endif
#ifdef RND_HAVE_SIGQUIT
	signal(SIGQUIT, pcb_catch_signal);
#endif
#ifdef RND_HAVE_SIGTERM
	signal(SIGTERM, pcb_catch_signal);
#endif
#ifdef RND_HAVE_SIGINT
	signal(SIGINT, pcb_catch_signal);
#endif

#ifdef NDEBUG
/* so that we get a core dump on segfault in debug mode */
#	ifdef RND_HAVE_SIGABRT
		signal(SIGABRT, pcb_catch_signal);
#	endif
#	ifdef RND_HAVE_SIGSEGV
		signal(SIGSEGV, pcb_catch_signal);
#	endif
#endif

/* calling external program by popen() may cause a PIPE signal, so we ignore it */
#ifdef RND_HAVE_SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
}

/* ----------------------------------------------------------------------
 * main program
 */
static void gui_support_plugins(int load)
{
	static int loaded = 0;
	static pup_plugin_t *puphand;

	if (load && !loaded) {
		static const char *plugin_name = "dialogs";
		int state = 0;
		loaded = 1;
		rnd_message(RND_MSG_DEBUG, "Loading GUI support plugin: '%s'\n", plugin_name);
		puphand = pup_load(&pcb_pup, (const char **)pcb_pup_paths, plugin_name, 0, &state);
		if (puphand == NULL)
			rnd_message(RND_MSG_ERROR, "Error: failed to load GUI support plugin '%s'\n-> expect missing widgets and dialog boxes\n", plugin_name);
	}
	if (!load && loaded && (puphand != NULL)) {
		pup_unload(&pcb_pup, puphand, NULL);
		loaded = 0;
		puphand = NULL;
	}
}

extern void pcb_buffer_init2(void);
extern void pcb_brave_init2(void);
extern void pcb_change_act_init2(void);
extern void pcb_conf_act_init2(void);
extern void pcb_drc_act_init2(void);
extern void pcb_extobj_act_init2(void);
extern void pcb_file_act_init2(void);
extern void pcb_font_act_init2(void);
extern void pcb_gui_act_init2(void);
extern void pcb_main_act_init2(void);
extern void pcb_netlist_act_init2(void);
extern void pcb_object_act_init2(void);
extern void pcb_poly_act_init2(void);
extern void pcb_plug_footprint_act_init2(void);
extern void pcb_pstk_act_init2(void);
extern void pcb_rats_act_init2(void);
extern void pcb_rats_patch_init2(void);
extern void pcb_remove_act_init2(void);
extern void pcb_select_act_init2(void);
extern void pcb_undo_act_init2(void);

void pcb_main_init_actions(void)
{
	pcb_buffer_init2();
	pcb_brave_init2();
	pcb_change_act_init2();
	pcb_conf_act_init2();
	pcb_drc_act_init2();
	pcb_extobj_act_init2();
	pcb_file_act_init2();
	pcb_font_act_init2();
	pcb_gui_act_init2();
	pcb_main_act_init2();
	pcb_netlist_act_init2();
	pcb_object_act_init2();
	pcb_poly_act_init2();
	pcb_plug_footprint_act_init2();
	pcb_pstk_act_init2();
	pcb_rats_act_init2();
	pcb_rats_patch_init2();
	pcb_remove_act_init2();
	pcb_select_act_init2();
	pcb_undo_act_init2();
}

extern void pcb_poly_uninit(void);

void pcb_main_uninit(void)
{
	if (rnd_log_last != NULL)
		rnd_log_last->seen = 1; /* ignore anything unseen before the uninit */

	conf_core_uninit_pre();
	pcb_brave_uninit();
	pcb_polygon_uninit();

	pcb_text_uninit();
	pcb_layer_vis_uninit();

	pcb_strflg_uninit_buf();
	pcb_strflg_uninit_layerlist();

	gui_support_plugins(0);
	rnd_render = rnd_gui = NULL;
	pcb_crosshair_uninit();
	pcb_tool_logic_uninit();

	if (PCB != NULL) {
		pcb_uninit_buffers(PCB);

		/* Free up memory allocated to the PCB. Why bother when we're about to exit ?
		   Because it removes some false positives from heap bug detectors such as
		   valgrind. */
		pcb_board_free(PCB);
		free(PCB);
	}
	PCB = NULL;

	rnd_hidlib_uninit(); /* plugin unload */

	pcb_funchash_uninit();
	rnd_file_loaded_uninit();
	pcb_uilayer_uninit();
	rnd_cli_uninit();
	pcb_dynflag_uninit();

	pcb_extobj_uninit();
	pcb_import_uninit();
	pcb_pixmap_uninit();
	pcb_io_uninit();
	pcb_fp_uninit();
	pcb_fp_host_uninit();
	pcb_tool_uninit();
	pcb_poly_uninit();

	pcbhl_log_print_uninit_errs("Log produced during uninitialization");
	rnd_log_uninit();
	main_path_uninit();
	conf_core_uninit();
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
	rnd_message(RND_MSG_ERROR, "puplug: %s\n", string);
}

#include "funchash_core.h"
#define action_entry(x) { #x, F_ ## x},
static pcb_funchash_table_t Functions[] = {
#include "funchash_core_list.h"
	{"F_END", F_END}
};

int main(int argc, char *argv[])
{
	int n;
	char **sp, *exec_prefix, *command_line_pcb = NULL;

	rnd_main_args_t ga;

	rnd_conf_dot_dir = DOT_PCB_RND;
	rnd_conf_lib_dir = PCBLIBDIR;

	rnd_fix_locale_and_env();
	exec_prefix = main_path_init(argv[0]);

	pcb_crosshair_pre_init();

	rnd_main_args_init(&ga, argc, pcb_action_args);

	/* init application:
	   - make program name available for error handlers
	   - initialize infrastructure (e.g. the conf system)
	   - evaluate options
	   - create an empty PCB with default symbols
	   - register 'call on exit()' function */

	/* Minimal conf setup before we do anything else */
	pcb_netlist_geo_init();
	pcb_minuid_init();
	pcb_extobj_init();

	rnd_hidlib_init1(conf_core_init);
	pcb_event_init_app();
	rnd_conf_set(RND_CFR_INTERNAL, "rc/path/exec_prefix", -1, exec_prefix, RND_POL_OVERWRITE);
	free(exec_prefix);

	pcb_layer_vis_init();
	pcb_brave_init();
	pcb_pixmap_init();

	/* process arguments */
	for(n = 1; n < argc; n++) {
		/* optionally: handle extra arguments, not processed by the hidlib, here */
		n += rnd_main_args_add(&ga, argv[n], argv[n+1]);
	}
	pcb_tool_init();
	pcb_tool_logic_init();

	rnd_hidlib_init2(pup_buildins, NULL);
	pcb_actions_init_pcb_only();

	setbuf(stdout, 0);

	pcb_fp_init();

	srand(time(NULL));  /* Set seed for rand() */

	pcb_funchash_set_table(Functions, PCB_ENTRIES(Functions), NULL);
	pcb_polygon_init();

	/* Register a function to be called when the program terminates. This makes
	   sure that data is saved even if LEX/YACC routines abort the program.
	   If the OS doesn't have at least one of them, the critical sections will
	   be handled by parse_l.l */
	atexit(pcb_emergency_save);

	pcb_text_init();

	if (pcb_pup.err_stack != NULL) {
		rnd_message(RND_MSG_ERROR, "Some of the static linked buildins could not be loaded:\n");
		pup_err_stack_process_str(&pcb_pup, print_pup_err);
	}

	for(sp = pcb_pup_paths; *sp != NULL; sp++) {
		rnd_message(RND_MSG_DEBUG, "Loading plugins from '%s'\n", *sp);
		pup_autoload_dir(&pcb_pup, *sp, (const char **)pcb_pup_paths);
	}
	if (pcb_pup.err_stack != NULL) {
		rnd_message(RND_MSG_ERROR, "Some of the dynamic linked plugins could not be loaded:\n");
		pup_err_stack_process_str(&pcb_pup, print_pup_err);
	}

	if (rnd_main_args_setup1(&ga) != 0) {
		pcb_main_uninit();
		rnd_main_args_uninit(&ga);
		exit(1);
	}

/* Initialize actions only when the gui is already known so only the right
   one is registered (there can be only one GUI). */
	pcb_main_init_actions();

	if (rnd_main_args_setup2(&ga, &n) != 0) {
		pcb_main_uninit();
		rnd_main_args_uninit(&ga);
		exit(n);
	}

	if (RND_HAVE_GUI_ATTR_DLG)
		gui_support_plugins(1);

	/* Create a new PCB object in memory */
	PCB = pcb_board_new(0);

	if (PCB == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't create an empty layout, exiting\n");
		pcbhl_log_print_uninit_errs("Initialization");
		exit(1);
	}

	/* Update board layer colors and buffer bindings */
	pcb_board_new_postproc(PCB, 1);

	pcb_layervis_reset_stack(&PCB->hidlib);

	if (rnd_gui->gui)
		pcb_crosshair_init();
	main_sighand_init();
	pcb_init_buffers(PCB);

	if (ga.hid_argc > 0)
		command_line_pcb = ga.hid_argv[0];
	if (command_line_pcb) {
		int how = conf_core.rc.silently_create_on_load ? 0x10 : 0;
		if (pcb_load_pcb(command_line_pcb, NULL, pcb_true, how) != 0) {
			if (rnd_main_exporting) {
				rnd_message(RND_MSG_ERROR, "Can not load file '%s' (specified on command line) for exporting or printing\n", command_line_pcb);
				pcbhl_log_print_uninit_errs("Export load error");
				exit(1);
			}
			/* keep filename if load failed: file might not exist, save it by that name */
			PCB->hidlib.filename = rnd_strdup(command_line_pcb);
		}
	}

	if (conf_core.design.initial_layer_stack && conf_core.design.initial_layer_stack[0])
		rnd_message(RND_MSG_ERROR, "Config setting desgin/initial_layer_stack is set but is deprecated and ignored. Please edit your config files to remove it.\n");

	/* read the library file and display it if it's not empty */
	if (!pcb_fp_read_lib_all() && pcb_library.data.dir.children.used)
		rnd_event(&PCB->hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);

	if (conf_core.rc.script_filename) {
		rnd_message(RND_MSG_INFO, "Executing startup script file %s\n", conf_core.rc.script_filename);
		rnd_actionva(&PCB->hidlib, "ExecuteFile", conf_core.rc.script_filename, NULL);
	}
	if (conf_core.rc.action_string) {
		rnd_message(RND_MSG_INFO, "Executing startup action %s\n", conf_core.rc.action_string);
		rnd_parse_actions(&PCB->hidlib, conf_core.rc.action_string);
	}

	if (rnd_main_exported(&ga, &PCB->hidlib, pcb_data_is_empty(PCB->Data))) {
		pcb_main_uninit();
		rnd_main_args_uninit(&ga);
		exit(0);
	}

	pcb_enable_autosave();

	/* main loop */
	if (RND_HAVE_GUI_ATTR_DLG)
		gui_support_plugins(1);

	if (EXPERIMENTAL != NULL) {
		rnd_message(RND_MSG_ERROR, "******************************** IMPORTANT ********************************\n");
		rnd_message(RND_MSG_ERROR, "This revision of pcb-rnd is experimental, unstable, do NOT attempt to use\n");
		rnd_message(RND_MSG_ERROR, "it for production. The reason for this state is:\n");
		rnd_message(RND_MSG_ERROR, "%s\n", EXPERIMENTAL);
		rnd_message(RND_MSG_ERROR, "******************************** IMPORTANT ********************************\n");
	}
	pcb_tool_select_by_name(&PCB->hidlib, "arrow");
	rnd_event(&PCB->hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
	rnd_mainloop_interactive(&ga, &PCB->hidlib);

	pcb_main_uninit();
	rnd_main_args_uninit(&ga);
	return 0;
}
