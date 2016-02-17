#include <stdlib.h>
#include <stdio.h>
#include "default/hooks.h"
#include "default/libs.h"
#include "default/main_custom_args.h"
#include "default/main_lib.h"
#include "menulib/scmenu.h"
#include "util/arg_auto_set.h"
#include "util/arg_auto_menu.h"


void re_fail(char *s, char c)
{
	fprintf(stderr, "Regex error: %s [opcode %o]\n", s, c);
	abort();
}

int cb_configure(scm_ctx_t *ctx, scm_menu_t *menu, scm_win_t *w, int selection)
{
	printf("configure!!\n");
	sleep(1);
	return -1;
}



scm_menu_entry_t me_hid_gtk[8]        = {{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}};
scm_menu_entry_t me_hid_lesstif[8]    = {{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}};
scm_menu_entry_t me_export[8]         = {{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}};
scm_menu_entry_t me_gpmi[8]           = {{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}};
scm_menu_entry_t me_autorouter[8]     = {{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}};
scm_menu_entry_t me_all_settings[32]  = {{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}};

scm_menu_entry_t me_settings[] = {
	{SCM_SUBMENU,        NULL, "HID: gtk",           "gtk settings -->",           NULL, me_hid_gtk, SCM_AUTO_RUN},
	{SCM_SUBMENU,        NULL, "HID: lesstif",       "lesstif settings -->",       NULL, me_hid_lesstif, SCM_AUTO_RUN},
	{SCM_SUBMENU,        NULL, "HID: exporters",     "export format settings -->", NULL, me_export, SCM_AUTO_RUN},
	{SCM_SUBMENU,        NULL, "scripting",          "gpmi settings -->",          NULL, me_gpmi, SCM_AUTO_RUN},
	{SCM_SUBMENU,        NULL, "auto routers",       "autorouter settings -->",    NULL, me_autorouter, SCM_AUTO_RUN},
	{SCM_EMPTY,          NULL, NULL, NULL, NULL, 0},
	{SCM_SUBMENU,        NULL, "all",                "all settings at once -->",   NULL, me_all_settings, SCM_AUTO_RUN},
	{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}
};

scm_menu_entry_t me_main[] = {
	{SCM_SUBMENU,        NULL, "Edit settings",      "Edit user configurable settings -->",       NULL, me_settings, SCM_AUTO_RUN},
	{SCM_SUBMENU,        NULL, "Manual detection",   "Run detection of individual libs -->",      NULL, NULL, SCM_AUTO_RUN},
	{SCM_SUBMENU_CB,     NULL, "Configure pcb-rnd",  "Run the configuration process -->",         NULL, cb_configure, SCM_AUTO_RUN},
	{SCM_EMPTY,          NULL, NULL, NULL, NULL, 0},
	{SCM_KEY_VALUE,      NULL, "Exit & save",        "Exit with saving the settings",             NULL, NULL, SCM_AUTO_RUN},
	{SCM_KEY_VALUE,      NULL, "Exit",               "Exit without saving the settings",          NULL, NULL, SCM_AUTO_RUN},
	{SCM_TERMINATOR,     NULL, NULL, NULL, NULL, NULL, 0}
};

extern const arg_auto_set_t disable_libs[] ;

int main()
{
	scm_ctx_t ctx;

	main_init();

	append_settings_auto_set(me_export, 32, disable_libs, "-gd", NULL, "^disable-",   SCM_SUBMENU, 0);

	scm_init(&ctx);
	scm_menu_autowin(&ctx,  "main menu", me_main);

	main_uninit();

	return 0;
}
