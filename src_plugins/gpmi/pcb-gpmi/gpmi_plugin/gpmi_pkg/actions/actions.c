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

/*** temporary hack for smooth upgrade to fungw based actions ***/
PCB_INLINE int pcb_old_act_begin_conv(int oargc, fgw_arg_t *oargv, char **argv)
{
	int n;
	for(n = 0; n < PCB_ACTION_MAX_ARGS; n++)
		argv[n] = NULL;
	for(n = 1; n < oargc; n++) {
		if (fgw_arg_conv(&pcb_fgw, &oargv[n], FGW_STR) == 0)
			argv[n-1] = oargv[n].val.str;
		else
			argv[n-1] = "";
	}
	argv[n] = NULL;
	return oargc - 1;
}

#define PCB_OLD_ACT_BEGIN \
ores->type = FGW_INT; \
ores->val.nat_int = 0; \
{ \
	char *argv__[PCB_ACTION_MAX_ARGS]; \
	const char **argv = (const char **)argv__; \
	int argc = pcb_old_act_begin_conv(oargc, oargv, argv__)

#define PCB_OLD_ACT_END \
	(void)argc; \
	(void)argv; \
}

static fgw_error_t pcb_act_action_cb(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
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
	PCB_OLD_ACT_END;
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
	ctx->action.trigger_cb     = pcb_act_action_cb;
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
	return pcb_parse_command(cmdline, pcb_true);
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
