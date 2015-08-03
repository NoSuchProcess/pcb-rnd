#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gpmi.h>
#include "src/global.h"
#include "src/hid.h"
#include "actions.h"

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

int action_register(const char *name, const char *need_xy, const char *description, const char *syntax, const char *context)
{
	acontext_t *ctx;


	if ((need_xy != NULL) && (*need_xy == '\0'))
		need_xy = NULL;


	ctx = malloc(sizeof(acontext_t));
	ctx->action.name           = strdup(name);
	ctx->action.need_coord_msg = strdup_null(need_xy);
	ctx->action.description    = strdup(description);
	ctx->action.syntax         = strdup(syntax);
	ctx->action.trigger_cb     = action_cb;
	ctx->name   = strdup(name);
	ctx->module = gpmi_get_current_module();
	ctx->next   = NULL;

	hid_register_action(&ctx->action);
	printf("registered.\n");
	return 0;
}

int action(const char *cmdline)
{
	return hid_parse_command(cmdline);
}
