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
	hid_gpmi_load_dir (Concat (PCBLIBDIR, "/plugins/", HOST, NULL));
	hid_gpmi_load_dir (Concat (PCBLIBDIR, "/plugins", NULL));
	if (homedir != NULL) {
		hid_gpmi_load_dir (Concat (homedir, "/.pcb/plugins/", HOST, NULL));
		hid_gpmi_load_dir (Concat (homedir, "/.pcb/plugins", NULL));
	}
	hid_gpmi_load_dir (Concat ("plugins/", HOST, NULL));
	hid_gpmi_load_dir (Concat ("plugins", NULL));
}

static void ev_gui_init(void *user_data, int argc, event_arg_t *argv[])
{
	const char *menu[] = {"Plugins", "GPMI scripting", "Scripts", NULL};

	gui->create_menu(menu, "gpmi_scripts()", "S", "Alt<Key>g", "Manage GPMI scripts");
}

static int action_gpmi_scripts(int argc, char **argv, Coord x, Coord y)
{
	if (argc == 0) {
		printf("Manage!\n");
		return;
	}
	Message("Invalid arguments in gpmi_scripts()\n");
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
