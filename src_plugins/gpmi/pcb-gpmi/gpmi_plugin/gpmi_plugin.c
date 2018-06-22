#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gpmi.h>
#include "src/global_typedefs.h"
#include "src/misc_util.h"
#include "src/compat_misc.h"
#include "src/conf_core.h"
#include "src/event.h"
#include "src/paths.h"
#include "src/error.h"
#include "src/plugins.h"
#include "src/actions.h"
#include "scripts.h"
#include "manage_scripts.h"

const char *gpmi_cookie = "GPMI plugin cookie";

/* This function is used to print a detailed GPMI error message */
void gpmi_hid_print_error(gpmi_err_stack_t *entry, char *string)
{
	pcb_message(PCB_MSG_ERROR, "[GPMI] %s\n", string);
}

static void start_timer(void);
static void timer_cb(pcb_hidval_t hv)
{
	static int mono = 0;
	hid_gpmi_script_info_t *i;
	for(i = hid_gpmi_script_info; i != NULL; i = i->next)
		gpmi_call_RunTick(i->module, mono++);
	start_timer();
}

static void start_timer(void)
{
	pcb_hidval_t hv;
	pcb_gui->add_timer(timer_cb, 100, hv);
}

int gpmi_hid_gui_inited = 0;
static void ev_gui_init(void *user_data, int argc, pcb_event_arg_t argv[])
{
	int ev;
	char *ev_args;
	hid_gpmi_script_info_t *i;
	const char *menu = "/main_menu/Plugins/GPMI scripting/Scripts";
	pcb_menu_prop_t mp;

	memset(&mp, 0, sizeof(mp));
	mp.action = "gpmi_scripts()";
	mp.mnemonic = "S";
	mp.accel = "<Key>p;<key>g;<key>s";
	mp.tip = "Manage GPMI scripts";
	mp.cookie = gpmi_cookie;

	pcb_gui->create_menu(menu, &mp);

	ev = gpmi_event_find("ACTE_gui_init", &ev_args);
	if (ev >= 0) {
		for(i = hid_gpmi_script_info; i != NULL; i = i->next)
			if (i->module != NULL)
				gpmi_event(i->module, ev, argc, argv);
	}
	gpmi_hid_gui_inited = 1;

	start_timer();
}

static void cmd_reload(const char *name)
{
	hid_gpmi_script_info_t *i;
	if (name != NULL) {
		i = hid_gpmi_lookup(name);
		if (i != NULL)
			hid_gpmi_reload_module(i);
		else
			pcb_message(PCB_MSG_ERROR, "Script %s not found\n", name);
	}
	else {
		for(i = hid_gpmi_script_info; i != NULL; i = i->next)
			hid_gpmi_reload_module(i);
	}
}

static fgw_error_t pcb_act_gpmi_scripts(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	if (argc == 0) {
		gpmi_hid_manage_scripts();
		return 0;
	}
	if (pcb_strcasecmp(argv[0], "reload") == 0) {
		if (argc > 1)
			cmd_reload(argv[1]);
		else
			cmd_reload(NULL);
	}
	else if (pcb_strcasecmp(argv[0], "load") == 0) {
		if (argc == 3) {
			if (hid_gpmi_load_module(NULL, argv[1], argv[2], NULL) == NULL)
				pcb_message(PCB_MSG_ERROR, "Failed to load %s %s\n", argv[1], argv[2]);
		}
		else
			pcb_message(PCB_MSG_ERROR, "Invalid number of arguments for load\n");
	}
	else if (pcb_strcasecmp(argv[0], "unload") == 0) {
		if (argc == 2) {
			hid_gpmi_script_info_t *i = hid_gpmi_lookup(argv[1]);
			if (i != NULL) {
				if (gpmi_hid_script_unload(i) != 0) {
					pcb_message(PCB_MSG_ERROR, "Failed to unload %s\n", argv[1]);
					return 1;
				}
			}
			else {
				pcb_message(PCB_MSG_ERROR, "Failed to unload %s: not loaded\n", argv[1]);
				return 1;
			}
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Invalid number of arguments for unload\n");
			return 1;
		}
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid arguments in gpmi_scripts()\n");
		return 1;
	}
	return 0;
	PCB_OLD_ACT_END;
}

static fgw_error_t pcb_act_gpmi_rehash(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	cmd_reload(NULL);
	return 0;
	PCB_OLD_ACT_END;
}

static void register_actions()
{
	static pcb_action_t act1, act2;

	act1.name           = "gpmi_scripts";
	act1.description    = "Manage gpmi scripts";
	act1.syntax         = "TODO";
	act1.trigger_cb     = pcb_act_gpmi_scripts;
	pcb_register_action(&act1, gpmi_cookie);

	act2.name           = "rehash";
	act2.description    = "Reload all gpmi scripts";
	act2.syntax         = "TODO";
	act2.trigger_cb     = pcb_act_gpmi_rehash;
	pcb_register_action(&act2, gpmi_cookie);
}

static gpmi_package *pkg_scripts = NULL;

static void load_base_and_cfg(void)
{
	char *dir, *libdirg, *libdirh, *wdir, *wdirh, *hdirh;
	const char *home;
	void **gpmi_asm_scriptname;

	libdirg = pcb_path_resolve_inplace(pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S "plugins", NULL), 0);
	libdirh = pcb_path_resolve_inplace(pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S "plugins" PCB_DIR_SEPARATOR_S, HOST, NULL), 0);
	wdirh = pcb_path_resolve_inplace(pcb_concat("plugins" PCB_DIR_SEPARATOR_S, HOST, NULL), 0);
	wdir = pcb_concat("plugins", NULL);

	home = getenv ("PCB_RND_GPMI_HOME");
	if (home == NULL)
		home = conf_core.rc.path.home;

	hdirh = pcb_path_resolve_inplace(pcb_concat(home, PCB_DIR_SEPARATOR_S ".pcb" PCB_DIR_SEPARATOR_S "plugins" PCB_DIR_SEPARATOR_S, HOST, NULL), 0);

	pcb_message(PCB_MSG_DEBUG, "gpmi dirs: lg=%s lh=%s wh=%s w=%s hh=%s\n", libdirg, libdirh, wdirh, wdir, hdirh);

	/* first add package search path to all host-specific plugin dirs
	   This is needed because a script installed in ~/.pcb/plugins/ *.conf
	   (added automatically from, the gui)
	   could depend on a package being anywhere else
	*/
	gpmi_path_insert(GPMI_PATH_PACKAGES, libdirh);
	gpmi_path_insert(GPMI_PATH_PACKAGES, libdirg);
	gpmi_path_insert(GPMI_PATH_PACKAGES, wdirh);
	gpmi_path_insert(GPMI_PATH_PACKAGES, hdirh);

	/* the final fallback - append this as loading anything from here is arch-unsafe */
	gpmi_path_append(GPMI_PATH_PACKAGES, wdir);


	gpmi_err_stack_enable();
	if (gpmi_pkg_load("gpmi_scripts", 0, NULL, NULL, &pkg_scripts))
	{
		gpmi_err_stack_process_str(gpmi_hid_print_error);
		abort();
	}
	gpmi_err_stack_destroy(NULL);


	gpmi_asm_scriptname = gpmi_pkg_resolve(pkg_scripts, "gpmi_scripts_asm_scriptname");
	assert(gpmi_asm_scriptname != NULL);
	*gpmi_asm_scriptname = gpmi_hid_asm_scriptname;

	register_actions();
	pcb_event_bind(PCB_EVENT_GUI_INIT, ev_gui_init, NULL, gpmi_cookie);

	hid_gpmi_load_dir(libdirh, 0);
	hid_gpmi_load_dir(libdirg, 0);

	dir = pcb_concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S "plugins", NULL);
	hid_gpmi_load_dir(dir, 1);
	free(dir);

	if (home != NULL) {
		hid_gpmi_load_dir (hdirh, 0);

		dir = pcb_path_resolve_inplace(pcb_concat(home, PCB_DIR_SEPARATOR_S ".pcb" PCB_DIR_SEPARATOR_S "plugins", NULL), 0);
		hid_gpmi_load_dir(dir, 1);
		free(dir);
	}

	hid_gpmi_load_dir(wdirh, 0);
	hid_gpmi_load_dir(wdir, 0);

	free(wdir);
	free(wdirh);
	free(libdirg);
	free(libdirh);
	free(hdirh);
}

int pplg_check_ver_gpmi(int ver_needed)
{
	return 0;
}

void pplg_uninit_gpmi(void)
{
	pcb_event_unbind_allcookie(gpmi_cookie);
	pcb_remove_actions_by_cookie(gpmi_cookie);
	hid_gpmi_script_info_uninit();
	gpmi_pkg_unload(pkg_scripts);
	gpmi_uninit();
}

int pplg_init_gpmi(void)
{
	gpmi_init();
	load_base_and_cfg();
	return 0;
}

/* Workaround: can't call it gpmi.so so basename is gpmi_plugin thus init name must be that too for the loader */
int pplg_init_gpmi_plugin(void)
{
	PCB_API_CHK_VER;
	return pplg_init_gpmi();
}
