#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gpmi.h>
#include "src/global.h"
#include "src/hid.h"
#include "src/error.h"
#include "actions.h"
#include "src/hid_actions.h"
#include "src/compat_misc.h"
#include "../../gpmi_plugin.h"

typedef struct acontext_s  acontext_t;

struct acontext_s {
	HID_Action action;
	char *name;
	gpmi_module *module;
	acontext_t *next;
};

static int action_argc = 0;
static char **action_argv = NULL;

const char *action_arg(int argn)
{
	if ((argn < 0) || (argn >= action_argc))
		return NULL;
	return action_argv[argn];
}


static int action_cb(int argc, char **argv, Coord x, Coord y)
{
	acontext_t *ctx = (acontext_t *)current_action;
	int action_argc_old;
	char **action_argv_old;

	/* save argc/argv for action_arg() */
	action_argc_old = action_argc;
	action_argv_old = action_argv;
	action_argc = argc;
	action_argv = argv;

	/* call event */
	gpmi_event(ctx->module, ACTE_action, ctx->name, argc, x, y);

	/* restore argc/argv of action_arg() */
	action_argc = action_argc_old;
	action_argv = action_argv_old;

	return 0;
}

static void cleanup_action(gpmi_module *mod, gpmi_cleanup *cl)
{
	acontext_t *ctx = cl->argv[0].p;
	hid_remove_action(&ctx->action);
	free(ctx->action.name);
	if (ctx->action.need_coord_msg != NULL)
		free((char *)ctx->action.need_coord_msg);
	free((char *)ctx->action.description);
	free((char *)ctx->action.syntax);
	free(ctx);
}

int action_register(const char *name, const char *need_xy, const char *description, const char *syntax)
{
	acontext_t *ctx;
	

	if ((need_xy != NULL) && (*need_xy == '\0'))
		need_xy = NULL;


	ctx = malloc(sizeof(acontext_t));
	ctx->action.name           = pcb_strdup(name);
	ctx->action.need_coord_msg = pcb_strdup_null(need_xy);
	ctx->action.description    = pcb_strdup(description);
	ctx->action.syntax         = pcb_strdup(syntax);
	ctx->action.trigger_cb     = action_cb;
	ctx->name                  = pcb_strdup(name);
	ctx->module                = gpmi_get_current_module();
	ctx->next                  = NULL;

	hid_register_action(&ctx->action, gpmi_cookie, 0);

	gpmi_mod_cleanup_insert(ctx->module, cleanup_action, "p", ctx);

	printf("registered.\n");
	return 0;
}

int action(const char *cmdline)
{
	return hid_parse_command(cmdline);
}

void create_menu(const char *path, const char *action, const char *mnemonic, const char *hotkey, const char *tooltip)
{
	gui->create_menu(path, action, mnemonic, hotkey, tooltip, "TODO#2");
}
