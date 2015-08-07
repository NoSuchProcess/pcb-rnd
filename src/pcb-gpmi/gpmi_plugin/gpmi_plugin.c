#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gpmi.h>
#include "src/misc.h"
#include "src/event.h"
#include "scripts.h"

extern char *homedir; /* detected by pcn-rnd in InitPaths() */

/* This function is used to print a detailed GPMI error message */
void gpmi_hid_print_error(gpmi_err_stack_t *entry, char *string)
{
	Message("[GPMI] %s\n", string);
}

static void load_cfg(void)
{
	char *dir, *libdirh, *wdir, *wdirh, *hdirh;

	libdirh = Concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S "plugins" PCB_DIR_SEPARATOR_S, HOST, NULL);
	wdirh = Concat ("plugins" PCB_DIR_SEPARATOR_S, HOST, NULL);
	wdir = Concat("plugins", NULL);
	hdirh = Concat(homedir, PCB_DIR_SEPARATOR_S ".pcb" PCB_DIR_SEPARATOR_S "plugins" PCB_DIR_SEPARATOR_S, HOST, NULL);

	/* first add package search path to all host-specific plugin dirs
	   This is needed because a script installed in ~/.pcb/plugins/*.conf
	   (added automatically from, the gui)
	   could depend on a package being anywhere else
	*/
	gpmi_path_insert(GPMI_PATH_PACKAGES, libdirh);
	gpmi_path_insert(GPMI_PATH_PACKAGES, wdirh);
	gpmi_path_insert(GPMI_PATH_PACKAGES, hdirh);

	/* the final fallback - append this as loading anything from here is arch-unsafe */
	gpmi_path_append(GPMI_PATH_PACKAGES, wdir);


	hid_gpmi_load_dir(libdirh, 0);
	
	dir = Concat(PCBLIBDIR, PCB_DIR_SEPARATOR_S "plugins", NULL);
	hid_gpmi_load_dir(dir, 1);
	free(dir);

	if (homedir != NULL) {
		hid_gpmi_load_dir (hdirh, 0);

		dir = Concat(homedir, PCB_DIR_SEPARATOR_S ".pcb" PCB_DIR_SEPARATOR_S "plugins", NULL);
		hid_gpmi_load_dir(dir, 1);
		free(dir);
	}

	hid_gpmi_load_dir(wdirh, 0);
	hid_gpmi_load_dir(wdir, 0);

	free(wdir);
	free(wdirh);
	free(libdirh);
	free(hdirh);
}

static void ev_gui_init(void *user_data, int argc, event_arg_t *argv[])
{
	int ev;
	char *ev_args;
	script_info_t *i;
	const char *menu[] = {"Plugins", "GPMI scripting", "Scripts", NULL};

	gui->create_menu(menu, "gpmi_scripts()", "S", "Alt<Key>g", "Manage GPMI scripts");

	ev = gpmi_event_find("ACTE_gui_init", &ev_args);
	if (ev >= 0) {
		for(i = script_info; i != NULL; i = i->next)
			if (i->module != NULL)
				gpmi_event(i->module, ev, argc, argv);
	}
}

static void cmd_reload(char *name)
{
	script_info_t *i;
	if (name != NULL) {
		i = hid_gpmi_lookup(name);
		if (i != NULL)
			hid_gpmi_reload_module(i);
		else
			Message("Script %s not found\n", name);
	}
	else {
		for(i = script_info; i != NULL; i = i->next)
			hid_gpmi_reload_module(i);
	}
}

static int action_gpmi_scripts(int argc, char **argv, Coord x, Coord y)
{
	if (argc == 0) {
		gpmi_hid_manage_scripts();
		return;
	}
	if (strcasecmp(argv[0], "reload") == 0) {
		if (argc > 1)
			cmd_reload(argv[1]);
		else
			cmd_reload(NULL);
	}
	else
		Message("Invalid arguments in gpmi_scripts()\n");
}

static int action_gpmi_rehash(int argc, char **argv, Coord x, Coord y)
{
	cmd_reload(NULL);
}


static void register_actions()
{
	HID_Action *ctx;

	ctx = malloc(sizeof(HID_Action));
	ctx->name           = strdup("gpmi_scripts");
	ctx->need_coord_msg = NULL;
	ctx->description    = strdup("Manage gpmi scripts");
	ctx->syntax         = strdup("TODO");
	ctx->trigger_cb     = action_gpmi_scripts;

	hid_register_action(ctx);

	ctx = malloc(sizeof(HID_Action));
	ctx->name           = strdup("rehash");
	ctx->need_coord_msg = NULL;
	ctx->description    = strdup("Reload all gpmi scripts");
	ctx->syntax         = strdup("TODO");
	ctx->trigger_cb     = action_gpmi_rehash;

	hid_register_action(ctx);
}


void pcb_plugin_init()
{
	void **gpmi_asm_scriptname;
	gpmi_package *scripts = NULL;

	printf("pcb-gpmi hid is loaded.\n");
	gpmi_init();

	gpmi_err_stack_enable();
	if (gpmi_pkg_load("gpmi_scripts", 0, NULL, NULL, &scripts))
	{
		gpmi_err_stack_process_str(gpmi_hid_print_error);
		abort();
	}
	gpmi_err_stack_destroy(NULL);


	gpmi_asm_scriptname = gpmi_pkg_resolve(scripts, "gpmi_scripts_asm_scriptname");
	assert(gpmi_asm_scriptname != NULL);
	*gpmi_asm_scriptname = gpmi_hid_asm_scriptname;

	register_actions();
	event_bind(EVENT_GUI_INIT, ev_gui_init, NULL, pcb_plugin_init);
	load_cfg();
}
