/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2019 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <stdlib.h>
#include <locale.h>

#ifdef __WIN32__
#include <wchar.h>
#endif

#include <genvector/vts0.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/event.h>

/* for dlopen() and friends; will also solve all system-dependent includes
   and provides a dl-compat layer on windows. Also solves the opendir related
   includes. */
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_dad_unit.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/compat_inc.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/file_loaded.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/conf.h>
#include <librnd/core/funchash.h>

char *rnd_conf_dot_dir = ".librnd";
char *rnd_conf_lib_dir = "/usr/lib/librnd";

static const char *flt_any[] = {"*", "*.*", NULL};

const rnd_hid_fsd_filter_t rnd_hid_fsd_filter_any[] = {
	{ "all", NULL, flt_any },
	{ NULL, NULL, NULL }
};

rnd_hid_t **rnd_hid_list = 0;
int rnd_hid_num_hids = 0;

rnd_hid_t *rnd_gui = NULL;
rnd_hid_t *rnd_render = NULL;
rnd_hid_t *rnd_exporter = NULL;

int rnd_pixel_slop = 1;

rnd_plugin_dir_t *rnd_plugin_dir_first = NULL, *rnd_plugin_dir_last = NULL;

void rnd_hid_init()
{
	char *tmp;

	/* Setup a "nogui" default HID */
	rnd_render = rnd_gui = rnd_hid_nogui_get_hid();

TODO("make this configurable - add to conf_board_ignores avoid plugin injection")
	tmp = rnd_concat(rnd_conf.rc.path.exec_prefix, RND_DIR_SEPARATOR_S, "lib", RND_DIR_SEPARATOR_S, "pcb-rnd", RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	tmp = rnd_concat(rnd_conf.rc.path.exec_prefix, RND_DIR_SEPARATOR_S, "lib", RND_DIR_SEPARATOR_S, "pcb-rnd", RND_DIR_SEPARATOR_S, "plugins", NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	/* hardwired libdir, just in case exec-prefix goes wrong (e.g. linstall) */
	tmp = rnd_concat(rnd_conf_lib_dir, RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);
	tmp = rnd_concat(rnd_conf_lib_dir, RND_DIR_SEPARATOR_S, "plugins", NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	/* rnd_conf.rc.path.home is set by the conf_core immediately on startup */
	if (rnd_conf.rc.path.home != NULL) {
		tmp = rnd_concat(rnd_conf.rc.path.home, RND_DIR_SEPARATOR_S, rnd_conf_dot_dir, RND_DIR_SEPARATOR_S, "plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
		rnd_plugin_add_dir(tmp);
		free(tmp);

		tmp = rnd_concat(rnd_conf.rc.path.home, RND_DIR_SEPARATOR_S, rnd_conf_dot_dir, RND_DIR_SEPARATOR_S, "plugins", NULL);
		rnd_plugin_add_dir(tmp);
		free(tmp);
	}

	tmp = rnd_concat("plugins", RND_DIR_SEPARATOR_S, HOST, NULL);
	rnd_plugin_add_dir(tmp);
	free(tmp);

	rnd_plugin_add_dir("plugins");
}

void rnd_hid_uninit(void)
{
	rnd_plugin_dir_t *pd, *next;

	if (rnd_hid_num_hids > 0) {
		int i;
		for (i = rnd_hid_num_hids-1; i >= 0; i--) {
			if (rnd_hid_list[i]->uninit != NULL)
				rnd_hid_list[i]->uninit(rnd_hid_list[i]);
		}
	}
	free(rnd_hid_list);

	pup_uninit(&rnd_pup);

	rnd_export_uninit();

	for(pd = rnd_plugin_dir_first; pd != NULL; pd = next) {
		next = pd->next;
		free(pd->path);
		free(pd);
	}
	rnd_plugin_dir_first = rnd_plugin_dir_last = NULL;
}

void rnd_hid_register_hid(rnd_hid_t * hid)
{
	int i;
	int sz = (rnd_hid_num_hids + 2) * sizeof(rnd_hid_t *);

	if (hid->struct_size != sizeof(rnd_hid_t)) {
		fprintf(stderr, "Warning: hid \"%s\" has an incompatible ABI.\n", hid->name);
		return;
	}

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (hid == rnd_hid_list[i])
			return;

	rnd_hid_num_hids++;
	if (rnd_hid_list)
		rnd_hid_list = (rnd_hid_t **) realloc(rnd_hid_list, sz);
	else
		rnd_hid_list = (rnd_hid_t **) malloc(sz);

	rnd_hid_list[rnd_hid_num_hids - 1] = hid;
	rnd_hid_list[rnd_hid_num_hids] = 0;
}

void rnd_hid_remove_hid(rnd_hid_t * hid)
{
	int i;

	for (i = 0; i < rnd_hid_num_hids; i++) {
		if (hid == rnd_hid_list[i]) {
			rnd_hid_list[i] = rnd_hid_list[rnd_hid_num_hids - 1];
			rnd_hid_list[rnd_hid_num_hids - 1] = 0;
			rnd_hid_num_hids--;
			return;
		}
	}
}


rnd_hid_t *rnd_hid_find_gui(const char *preference)
{
	int i;

	/* ugly hack for historical reasons: some old configs and veteran users are used to the --gui gtk option */
	if ((preference != NULL) && (strcmp(preference, "gtk") == 0)) {
		rnd_hid_t *g;

		g = rnd_hid_find_gui("gtk2_gl");
		if (g != NULL)
			return g;

		g = rnd_hid_find_gui("gtk2_gdk");
		if (g != NULL)
			return g;

		return NULL;
	}

	/* normal search */
	if (preference != NULL) {
		for (i = 0; i < rnd_hid_num_hids; i++)
			if (!rnd_hid_list[i]->printer && !rnd_hid_list[i]->exporter && !strcmp(rnd_hid_list[i]->name, preference))
				return rnd_hid_list[i];
		return NULL;
	}

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (!rnd_hid_list[i]->printer && !rnd_hid_list[i]->exporter)
			return rnd_hid_list[i];

	fprintf(stderr, "Error: No GUI available.\n");
	exit(1);
}

rnd_hid_t *rnd_hid_find_printer()
{
	int i;

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (rnd_hid_list[i]->printer)
			return rnd_hid_list[i];

	return 0;
}

void pcb_hid_print_exporter_list(FILE *f, const char *prefix, const char *suffix)
{
	int i;
	for (i = 0; i < rnd_hid_num_hids; i++)
		if (rnd_hid_list[i]->exporter)
			fprintf(f, "%s%s%s", prefix, rnd_hid_list[i]->name, suffix);
}

rnd_hid_t *rnd_hid_find_exporter(const char *which)
{
	int i;

	if (which == NULL) {
		fprintf(stderr, "Invalid exporter: need an exporter name, one of:");
		goto list;
	}

	if (strcmp(which, "-list-") == 0) {
		pcb_hid_print_exporter_list(stdout, "", "\n");
		return 0;
	}

	for (i = 0; i < rnd_hid_num_hids; i++)
		if (rnd_hid_list[i]->exporter && strcmp(which, rnd_hid_list[i]->name) == 0)
			return rnd_hid_list[i];

	fprintf(stderr, "Invalid exporter %s, available ones:", which);

	list:;
	pcb_hid_print_exporter_list(stderr, " ", "");
	fprintf(stderr, "\n");

	return 0;
}

rnd_hid_t **rnd_hid_enumerate()
{
	return rnd_hid_list;
}

const char *rnd_hid_export_fn(const char *filename)
{
	if (rnd_conf.rc.export_basename) {
		const char *outfn = strrchr(filename, RND_DIR_SEPARATOR_C);
		if (outfn == NULL)
			return filename;
		return outfn + 1;
	}
	else
		return filename;
}

extern void pcb_hid_dlg_uninit(void);
extern void pcb_hid_dlg_init(void);

static char *get_homedir(void)
{
	char *homedir = getenv("HOME");
	if (homedir == NULL)
		homedir = getenv("USERPROFILE");
	return homedir;
}

void pcbhl_conf_postproc(void)
{
	rnd_conf_force_set_str(rnd_conf.rc.path.home, get_homedir()); rnd_conf_ro("rc/path/home");
}

void rnd_hidlib_init1(void (*conf_core_init)(void))
{
	pcb_events_init();
	rnd_file_loaded_init();
	rnd_conf_init();
	conf_core_init();
	pcbhl_conf_postproc();
	rnd_hidlib_conf_init();
	rnd_hidlib_event_init();
	pcb_hid_dlg_init();
	rnd_hid_init();
	rnd_color_init();
}

static vts0_t hidlib_conffile;

extern void pcb_hidlib_error_init2(void);
extern void pcb_hid_dlg_init2(void);
extern void pcb_hid_nogui_init2(void);
extern void rnd_conf_act_init2(void);
extern void rnd_tool_act_init2(void);
extern void rnd_gui_act_init2(void);
extern void rnd_main_act_init2(void);

void rnd_hidlib_init2(const pup_buildin_t *buildins, const pup_buildin_t *local_buildins)
{
	rnd_actions_init();

	/* load custom config files in the order they were specified */
	if (hidlib_conffile.used > 0) {
		int n;
		for(n = 0; n < hidlib_conffile.used; n++) {
			rnd_conf_role_t role = RND_CFR_CLI;
			char *srole, *sep, *fn = hidlib_conffile.array[n];
			sep = strchr(fn, ';');
			if (sep != NULL) {
				srole = fn;
				*sep = '\0';
				fn = sep+1;
				role = rnd_conf_role_parse(srole);
				if (role == RND_CFR_invalid) {
					fprintf(stderr, "Can't load -C config file '%s': invalid role '%s'\n", fn, srole);
					free(hidlib_conffile.array[n]);
					continue;
				}
			}
			rnd_conf_load_as(role, fn, 0);
			free(hidlib_conffile.array[n]);
		}
		vts0_uninit(&hidlib_conffile);
	}

	rnd_conf_load_all(NULL, NULL);

	pup_init(&rnd_pup);
	rnd_pup.error_stack_enable = 1;
	pup_buildin_load(&rnd_pup, buildins);
	if (local_buildins != NULL)
		pup_buildin_load(&rnd_pup, local_buildins);
	pup_autoload_dir(&rnd_pup, NULL, NULL);

	rnd_conf_load_extra(NULL, NULL);
	rnd_units_init();
	pcb_funchash_init();

	/* actions */
	pcb_hidlib_error_init2();
	pcb_hid_dlg_init2();
	pcb_hid_nogui_init2();
	rnd_conf_act_init2();
	rnd_tool_act_init2();
	rnd_gui_act_init2();
	rnd_main_act_init2();
}


void rnd_hidlib_uninit(void)
{
	rnd_hidlib_event_uninit();
	pcb_hid_dlg_uninit();

	if (rnd_conf_isdirty(RND_CFR_USER))
		rnd_conf_save_file(NULL, NULL, NULL, RND_CFR_USER, NULL);

	rnd_hid_uninit();
	pcb_events_uninit();
	rnd_conf_uninit();
	rnd_plugin_uninit();
	rnd_actions_uninit();
	rnd_dad_unit_uninit();
}

/* parse arguments using the gui; if fails and fallback is enabled, try the next gui */
int rnd_gui_parse_arguments(int autopick_gui, int *hid_argc, char **hid_argv[])
{
	rnd_conf_listitem_t *apg = NULL;

	if ((autopick_gui >= 0) && (rnd_conf.rc.hid_fallback)) { /* start from the GUI we are initializing first */
		int n;
		const char *g;

		rnd_conf_loop_list_str(&rnd_conf.rc.preferred_gui, apg, g, n) {
			if (n == autopick_gui)
				break;
		}
	}

	for(;;) {
		int res;
		if (rnd_gui->get_export_options != NULL)
			rnd_gui->get_export_options(rnd_gui, NULL);
		res = rnd_gui->parse_arguments(rnd_gui, hid_argc, hid_argv);
		if (res == 0)
			break; /* HID accepted, don't try anything else */
		if (res < 0) {
			rnd_message(RND_MSG_ERROR, "Failed to initialize HID %s (unrecoverable, have to give up)\n", rnd_gui->name);
			return -1;
		}
		fprintf(stderr, "Failed to initialize HID %s (recoverable)\n", rnd_gui->name);
		if (apg == NULL) {
			if (rnd_conf.rc.hid_fallback) {
				ran_out_of_hids:;
				rnd_message(RND_MSG_ERROR, "Tried all available HIDs, all failed, giving up.\n");
			}
			else
				rnd_message(RND_MSG_ERROR, "Not trying any other hid as fallback because rc/hid_fallback is disabled.\n");
			return -1;
		}

		/* falling back to the next HID */
		do {
			int n;
			const char *g;

			apg = rnd_conf_list_next_str(apg, &g, &n);
			if (apg == NULL)
				goto ran_out_of_hids;
			rnd_render = rnd_gui = rnd_hid_find_gui(g);
		} while(rnd_gui == NULL);
	}
	return 0;
}

#ifdef __WIN32__
/* truncate the last dir segment; returns remaining length or 0 on failure */
static int truncdir(char *dir)
{
	char *s;

	for(s = dir + strlen(dir) - 1; s >= dir; s--) {
		if ((*s == '/') || (*s == '\\')) {
			*s = '\0';
			return s - dir;
		}
	}
	*dir = '\0';
	return 0;
}
extern int pcb_mkdir_(const char *path, int mode);
char *rnd_w32_root;
char *rnd_w32_libdir, *rnd_w32_bindir, *rnd_w32_sharedir, *rnd_w32_cachedir;
#endif

void rnd_fix_locale_and_env()
{
	static const char *lcs[] = { "LANG", "LC_NUMERIC", "LC_ALL", NULL };
	const char **lc;

	/* some Xlib calls tend ot hardwire setenv() to "" or NULL so a simple
	   setlocale() won't do the trick on GUI. Also set all related env var
	   to "C" so a setlocale(LC_ALL, "") will also do the right thing. */
	for(lc = lcs; *lc != NULL; lc++)
		rnd_setenv(*lc, "C", 1);

	setlocale(LC_ALL, "C");


#ifdef __WIN32__
	{
		char *s, exedir[MAX_PATH];
		wchar_t *w, wexedir[MAX_PATH];

		if (!GetModuleFileNameW(NULL, wexedir, MAX_PATH)) {
			fprintf(stderr, "pcb-rnd: GetModuleFileNameW(): failed to determine executable full path\n");
			exit(1);
		}

		for(w = wexedir, s = exedir; *w != 0; w++)
			s += wctomb(s, *w);
		*s = '\0';

		truncdir(exedir);

		for(s = exedir; *s != '\0'; s++)
			if (*s == '\\')
				*s = '/';

		rnd_w32_bindir = rnd_strdup(exedir);
		truncdir(exedir);
		rnd_w32_root = rnd_strdup(exedir);
		rnd_w32_libdir = rnd_concat(exedir, "/lib/pcb-rnd", NULL);
		rnd_w32_sharedir = rnd_concat(exedir, "/share/pcb-rnd", NULL);

		rnd_w32_cachedir = rnd_concat(rnd_w32_root, "/cache", NULL);
		pcb_mkdir_(rnd_w32_cachedir, 0755);

/*		printf("WIN32 root='%s' libdir='%s' sharedir='%s'\n", rnd_w32_root, rnd_w32_libdir, rnd_w32_sharedir);*/
	}
#endif
}

static int pcbhl_main_arg_match(const char *in, const char *shrt, const char *lng)
{
	return ((shrt != NULL) && (strcmp(in, shrt) == 0)) || ((lng != NULL) && (strcmp(in, lng) == 0));
}

void rnd_main_args_init(rnd_main_args_t *ga, int argc, const char **action_args)
{
	memset(ga, 0, sizeof(rnd_main_args_t));
	ga->hid_argv_orig = ga->hid_argv = calloc(sizeof(char *), argc);
	vtp0_init(&ga->plugin_cli_conf);
	ga->action_args = action_args;
	ga->autopick_gui = -1;
}

void rnd_main_args_uninit(rnd_main_args_t *ga)
{
	vtp0_uninit(&ga->plugin_cli_conf);
	free(ga->hid_argv_orig);
}

int rnd_main_args_add(rnd_main_args_t *ga, char *cmd, char *arg)
{
	const char **cs;
	char *orig_cmd = cmd;

	if (*cmd == '-') {
		cmd++;
		if ((strcmp(cmd, "?") == 0) || (strcmp(cmd, "h") == 0) || (strcmp(cmd, "-help") == 0)) {
			if (arg != NULL) {
			/* memory leak, but who cares for --help? */
				ga->main_action = rnd_strdup_printf("PrintUsage(%s)", arg);
				return 1;
			}
			ga->main_action = "PrintUsage()";
			return 0;
		}

		if ((strcmp(cmd, "g") == 0) || (strcmp(cmd, "-gui") == 0) || (strcmp(cmd, "-hid") == 0)) {
			ga->do_what = RND_DO_GUI;
			ga->hid_name = arg;
			return 1;
		}
		if ((strcmp(cmd, "x") == 0) || (strcmp(cmd, "-export") == 0)) {
			ga->do_what = RND_DO_EXPORT;
			ga->hid_name = arg;
			return 1;
		}
		if ((strcmp(cmd, "p") == 0) || (strcmp(cmd, "-print") == 0)) {
			ga->do_what = RND_DO_PRINT;
			return 0;
		}

		for(cs = ga->action_args; cs[2] != NULL; cs += RND_ACTION_ARGS_WIDTH) {
			if (pcbhl_main_arg_match(cmd, cs[0], cs[1])) {
				if (ga->main_action == NULL) {
					ga->main_action = cs[2];
					ga->main_action_hint = cs[4];
				}
				else
					fprintf(stderr, "Warning: can't execute multiple command line actions, ignoring %s\n", orig_cmd);
				return 0;
			}
		}
		if (pcbhl_main_arg_match(cmd, "c", "-conf")) {
			const char *why;
			if (strncmp(arg, "plugins/", 8)  == 0) {
				/* plugins are not yet loaded or initialized so their configs are
				   unavailable. Store these settings until plugins are up. This
				   should not happen to non-plugin config items as those might
				   affect how plugins are searched/loaded. */
				const void **a = (const void **)vtp0_alloc_append(&ga->plugin_cli_conf, 1);
				*a = arg;
			}
			else {
				if (rnd_conf_set_from_cli(NULL, arg, NULL, &why) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, why);
					exit(1);
				}
			}
			return 1;
		}
		if (pcbhl_main_arg_match(cmd, "C", "-conffile")) {
			vts0_append(&hidlib_conffile, rnd_strdup(arg));
			return 1;
		}
	}
	/* didn't handle argument, save it for the HID */
	ga->hid_argv[ga->hid_argc++] = orig_cmd;
	return 0;
}

int rnd_main_args_setup1(rnd_main_args_t *ga)
{
	int n;
	/* Now that plugins are already initialized, apply plugin config items */
	for(n = 0; n < vtp0_len(&ga->plugin_cli_conf); n++) {
		const char *why, *arg = ga->plugin_cli_conf.array[n];
		if (rnd_conf_set_from_cli(NULL, arg, NULL, &why) != 0) {
			fprintf(stderr, "Error: failed to set config %s: %s\n", arg, why);
			return 1;
		}
	}
	vtp0_uninit(&ga->plugin_cli_conf);

	/* Export pcb from command line if requested.  */
	switch(ga->do_what) {
		case RND_DO_PRINT:   rnd_render = rnd_exporter = rnd_gui = rnd_hid_find_printer(); break;
		case RND_DO_EXPORT:  rnd_render = rnd_exporter = rnd_gui = rnd_hid_find_exporter(ga->hid_name); break;
		case RND_DO_GUI:
			rnd_render = rnd_gui = rnd_hid_find_gui(ga->hid_name);
			if (rnd_gui == NULL) {
				rnd_message(RND_MSG_ERROR, "Can't find the gui (%s) requested.\n", ga->hid_name);
				return 1;
			}
			break;
		default: {
			const char *g;
			rnd_conf_listitem_t *i;

			rnd_render = rnd_gui = NULL;
			rnd_conf_loop_list_str(&rnd_conf.rc.preferred_gui, i, g, ga->autopick_gui) {
				rnd_render = rnd_gui = rnd_hid_find_gui(g);
				if (rnd_gui != NULL)
					break;
			}

			/* try anything */
			if (rnd_gui == NULL) {
				rnd_message(RND_MSG_WARNING, "Warning: can't find any of the preferred GUIs, falling back to anything available...\nYou may want to check if the plugin is loaded, try --dump-plugins and --dump-plugindirs\n");
				rnd_render = rnd_gui = rnd_hid_find_gui(NULL);
			}
		}
	}

	/* Exit with error if GUI failed to start. */
	if (!rnd_gui)
		return 1;

	return 0;
}

int rnd_main_args_setup2(rnd_main_args_t *ga, int *exitval)
{
	*exitval = 0;

	/* plugins may have installed their new fields, reinterpret the config
	   (memory lht -> memory bin) to get the new fields */
	rnd_conf_update(NULL, -1);

	if (ga->main_action != NULL) {
		int res = rnd_parse_command(NULL, ga->main_action, rnd_true); /* hidlib is NULL because there is no context yet */
		if ((res != 0) && (ga->main_action_hint != NULL))
			rnd_message(RND_MSG_ERROR, "\nHint: %s\n", ga->main_action_hint);
		rnd_log_print_uninit_errs("main_action parse error");
		*exitval = res;
		return 1;
	}


	if (rnd_gui_parse_arguments(ga->autopick_gui, &ga->hid_argc, &ga->hid_argv) != 0) {
		rnd_log_print_uninit_errs("Export plugin argument parse error");
		return 1;
	}

	return 0;
}

int rnd_main_exported(rnd_main_args_t *ga, rnd_hidlib_t *hidlib, rnd_bool is_empty)
{
	if (!rnd_main_exporting)
		return 0;

	if (is_empty)
		rnd_message(RND_MSG_WARNING, "Exporting empty board (nothing loaded or drawn).\n");
	if (rnd_gui->set_hidlib != NULL)
		rnd_gui->set_hidlib(rnd_gui, hidlib);
	rnd_event(hidlib, RND_EVENT_EXPORT_SESSION_BEGIN, NULL);
	rnd_gui->do_export(rnd_gui, 0);
	rnd_event(hidlib, RND_EVENT_EXPORT_SESSION_END, NULL);
	rnd_log_print_uninit_errs("Exporting");
	return 1;
}

void rnd_mainloop_interactive(rnd_main_args_t *ga, rnd_hidlib_t *hidlib)
{
	if (rnd_gui->set_hidlib != NULL)
		rnd_gui->set_hidlib(rnd_gui, hidlib);
	rnd_gui->do_export(rnd_gui, 0);
}

