#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gpmi.h>
#include "src/global_typedefs.h"
#include "src/hid.h"
#include "src/error.h"
#include "actions.h"
#include "src/actions.h"
#include "src/compat_misc.h"
#include "src/crosshair.h"
#include "../../gpmi_plugin.h"

typedef struct acontext_s  acontext_t;

struct acontext_s {
	pcb_action_t action;
	char *name;
	gpmi_module *module;
	acontext_t *next;
};

static int action_argc = 0;
static const char **action_argv = NULL;

const char *action_arg(int argn)
{
	if ((argn < 0) || (argn >= action_argc))
		return NULL;
	return action_argv[argn];
}


static int action_cb(int argc, const char **argv)
{
	acontext_t *ctx = (acontext_t *)pcb_current_action;
	int action_argc_old;
	const char **action_argv_old;

	/* save argc/argv for action_arg() */
	action_argc_old = action_argc;
	action_argv_old = action_argv;
	action_argc = argc;
	action_argv = argv;

	/* call event */
	gpmi_event(ctx->module, ACTE_action, ctx->name, argc, pcb_crosshair.ptr_x, pcb_crosshair.ptr_y);

	/* restore argc/argv of action_arg() */
	action_argc = action_argc_old;
	action_argv = action_argv_old;

	return 0;
}

static void cleanup_action(gpmi_module *mod, gpmi_cleanup *cl)
{
	acontext_t *ctx = cl->argv[0].p;
	pcb_remove_actions(&ctx->action, 1);
	free((char *)ctx->action.name);
	free((char *)ctx->action.description);
	free((char *)ctx->action.syntax);
	free(ctx);
}

int action_register(const char *name, const char *need_xy, const char *description, const char *syntax)
{
	acontext_t *ctx;

	ctx = malloc(sizeof(acontext_t));
	ctx->action.name           = pcb_strdup(name);
	ctx->action.description    = pcb_strdup(description);
	ctx->action.syntax         = pcb_strdup(syntax);
	ctx->action.trigger_cb     = action_cb;
	ctx->name                  = pcb_strdup(name);
	ctx->module                = gpmi_get_current_module();
	ctx->next                  = NULL;

	pcb_register_action(&ctx->action, gpmi_cookie);

	gpmi_mod_cleanup_insert(ctx->module, cleanup_action, "p", ctx);

	pcb_trace("action %d registered.\n", name);
	return 0;
}

int action(const char *cmdline)
{
	return pcb_parse_command(cmdline);
}

void create_menu(const char *path, const char *action, const char *mnemonic, const char *hotkey, const char *tooltip)
{
	pcb_menu_prop_t mp;

	memset(&mp, 0, sizeof(mp));
	mp.action = action;
	mp.mnemonic = mnemonic;
	mp.accel = hotkey;
	mp.tip = tooltip;
	mp.cookie = "TODO#2";

	pcb_gui->create_menu(path, &mp);
}
