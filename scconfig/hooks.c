#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "libs.h"
#include "db.h"
#include "tmpasm.h"
#include "tmpasm_scconfig.h"
#include "util/arg_auto_set.h"

#define version "1.0.1"

/* Runs when a custom command line argument is found
 returns true if no furhter argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	static const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
		{"disable-gtk",       "libs/gui/gtk2",           arg_lib_nodes},
		{"disable-lesstif",   "libs/gui/lesstif2",       arg_lib_nodes},
		{"disable-xrender",   "libs/gui/xrender",        arg_lib_nodes},
		{"disable-xinerama",  "libs/gui/xinerama",       arg_lib_nodes},
		{"disable-gd",        "libs/gui/gd",             arg_lib_nodes},
		{"disable-gd-gif",    "libs/gui/gd/gdImageGif",  arg_lib_nodes},
		{"disable-gd-png",    "libs/gui/gd/gdImagePng",  arg_lib_nodes},
		{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg", arg_lib_nodes},

		{NULL, NULL, NULL}
	};

	if (strcmp(key, "prefix") == 0) {
		report(0, "Setting prefix to '%s'\n", value);
		put("/local/prefix", strclone(value));
		return 1;
	}

	return arg_auto_set(key, value, disable_libs);
}


/* Runs before anything else */
int hook_preinit()
{
	return 0;
}

/* Runs after initialization */
int hook_postinit()
{
	db_mkdir("/local");
	put("/local/prefix", "/usr/local");
	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	return 0;
}

/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	return 0;
}

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	require("fstools/mkdir", 0, 1);
	require("libs/gui/gtk2/presents", 0, 0);
	require("libs/gui/lesstif2/presents", 0, 0);

	if (istrue(get("libs/gui/lesstif2/presents"))) {
		require("libs/gui/xinerama/presents", 0, 0);
		require("libs/gui/xrender/presents", 0, 0);
	}
	else {
		report("Since there's no lesstif2, disabling xinerama and xrender...\n");	
		hook_custom_arg("disable-xinerama", NULL);
		hook_custom_arg("disable-xrender", NULL);
	}

	/* for the exporters */
	require("libs/gui/gd/presents", 0, 0);

	if (!istrue(get("libs/gui/gd/presents"))) {
		report("Since there's no libgd, disabling raster output formats...\n");
		hook_custom_arg("disable-gd-gif", NULL);
		hook_custom_arg("disable-gd-png", NULL);
		hook_custom_arg("disable-gd-jpg", NULL);
	}


	/* for core, the toporouter and gsch2pcb: */
	require("libs/sul/glib", 0, 1);

	/* generic utils for Makefiles */
	require("fstools/rm",  0, 1);
	require("fstools/ar",  0, 1);
	require("fstools/cp",  0, 1);
	require("fstools/ln",  0, 1);
	require("fstools/mkdir",  0, 1);
	require("sys/ext_exe", 0, 1);

	/* options for config.h */
	require("sys/path_sep", 0, 1);
	require("cc/alloca/presents", 0, 0);
	require("libs/env/putenv/presents", 0, 0);
	require("libs/env/setenv/presents", 0, 0);
	require("libs/snprintf", 0, 0);
	require("libs/vsnprintf", 0, 0);
	require("libs/fs/getcwd", 0, 0);
	require("libs/math/expf", 0, 0);
	require("libs/math/logf", 0, 0);
	require("libs/script/m4/bin/*", 0, 0);
	require("libs/gui/gd/gdImagePng/presents", 0, 0);
	require("libs/gui/gd/gdImageGif/presents", 0, 0);
	require("libs/gui/gd/gdImageJpeg/presents", 0, 0);
	require("libs/fs/stat/macros/*", 0, 0);

	return 0;
}

#ifdef GENCALL
/* If enabled, generator implements ###call *### and generator_callback is
   the callback function that will be executed upon ###call### */
void generator_callback(char *cmd, char *args)
{
	printf("* generator_callback: '%s' '%s'\n", cmd, args);
}
#endif

/* Runs after detection hooks, should generate the output (Makefiles, etc.) */
int hook_generate()
{
	char *rev = "non-svn", *tmp;

	tmp = svn_info(0, "../src", "Revision:");
	if (tmp != NULL) {
		rev = str_concat("", "svn r", tmp, NULL);
		free(tmp);
	}
	put("/local/revision", rev);
	put("/local/version",  version);

	printf("Generating Makefile.conf (%d)\n", tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating gts/Makefile (%d)\n", tmpasm("../gts", "Makefile.in", "Makefile"));
	printf("Generating pcb/Makefile (%d)\n", tmpasm("../src", "Makefile.in", "Makefile"));

	/* Has to be after pcb/Makefile so that all the modules are loaded. */
	printf("Generating pcb/hidlist  (%d)\n", tmpasm("../src/hid/common", "hidlist.h.in", "hidlist.h"));

	printf("Generating util/Makefile (%d)\n", tmpasm("../util", "Makefile.in", "Makefile"));

	printf("Generating config.auto.h (%d)\n", tmpasm("..", "config.auto.h.in", "config.auto.h"));

	if (!exists("../config.manual.h")) {
		printf("Generating config.manual.h (%d)\n", tmpasm("..", "config.manual.h.in", "config.manual.h"));
		printf(" * NOTE: you may want to edit config.manual.h (user preferences) *\n");
	}

	return 0;
}

/* Runs before everything is uninitialized */
void hook_preuninit()
{
}

/* Runs at the very end, when everything is already uninitialized */
void hook_postuninit()
{
}

