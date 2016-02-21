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

#define version "1.0.7"

const char *gui_list[] = {
	"GTK",     "libs/gui/gtk2/presents",
	"lesstif", "libs/gui/lesstif2/presents",
	NULL, NULL
};

const char *exporter_list[] = {
	"png",   "libs/gui/gd/gdImagePng/presents",
	"gif",   "libs/gui/gd/gdImageGif/presents",
	"jpg",   "libs/gui/gd/gdImageJpeg/presents",
	"gcode", "libs/gui/gd/presents",
	"nelma", "libs/gui/gd/presents",
	NULL, NULL
};

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-gtk",       "libs/gui/gtk2",                arg_lib_nodes, "$do not compile the gtk HID"},
	{"disable-lesstif",   "libs/gui/lesstif2",            arg_lib_nodes, "$do not compile the lesstif HID"},
	{"disable-xrender",   "libs/gui/xrender",             arg_lib_nodes, "$do not use xrender for lesstif"},
	{"disable-xinerama",  "libs/gui/xinerama",            arg_lib_nodes, "$do not use xinerama for lesstif"},
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"disable-gd-gif",    "libs/gui/gd/gdImageGif",       arg_lib_nodes, "$no gif support in the png exporter"},
	{"disable-gd-png",    "libs/gui/gd/gdImagePng",       arg_lib_nodes, "$no png support in the png exporter"},
	{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg",      arg_lib_nodes, "$no jpeg support in the png exporter"},
	{"disable-gpmi",      "libs/script/gpmi",             arg_lib_nodes, "$do not compile the gpmi (scripting) plugin"},

	{"buildin-gpmi",      "/local/pcb/gpmi/buildin",      arg_true,      "$static link the gpmi plugin into the executable"},
	{"plugin-gpmi",       "/local/pcb/gpmi/buildin",      arg_false,     "$the gpmi plugin is dynamic loadable"},

	{"disable-autoroute", "/local/pcb/autoroute/enable",   arg_false,     "$do not compile the autorouter"},
	{"buildin-autoroute", "/local/pcb/autoroute/buildin",  arg_true,      "$static link the autorouter plugin into the executable"},
	{"plugin-autoroute",  "/local/pcb/autoroute/buildin",  arg_false,     "$the autorouter plugin is dynamic loadable"},

	{"disable-autoplace",  "/local/pcb/autoplace/enable",   arg_false,     "$do not compile the autoplace"},
	{"buildin-autoplace",  "/local/pcb/autoplace/buildin",  arg_true,      "$static link the autoplace plugin into the executable"},
	{"plugin-autoplace",   "/local/pcb/autoplace/buildin",  arg_false,     "$the autoplace plugin is dynamic loadable"},

	{"disable-vendordrill",  "/local/pcb/vendordrill/enable",   arg_false,     "$do not compile the vendor drill mapping"},
	{"buildin-vendordrill",  "/local/pcb/vendordrill/buildin",  arg_true,      "$static link the vendor drill mapping plugin into the executable"},
	{"plugin-vendordrill",   "/local/pcb/vendordrill/buildin",  arg_false,     "$the vendor drill mapping plugin is dynamic loadable"},

	{"disable-puller",  "/local/pcb/puller/enable",   arg_false,     "$do not compile the puller"},
	{"buildin-puller",  "/local/pcb/puller/buildin",  arg_true,      "$static link the puller plugin into the executable"},
	{"plugin-puller",   "/local/pcb/puller/buildin",  arg_false,     "$the puller plugin is dynamic loadable"},

	{"disable-edif",  "/local/pcb/edif/enable",   arg_false,     "$do not compile the edif"},
	{"buildin-edif",  "/local/pcb/edif/buildin",  arg_true,      "$static link the edif plugin into the executable"},
	{"plugin-edif",   "/local/pcb/edif/buildin",  arg_false,     "$the edif plugin is dynamic loadable"},

	{"disable-djopt",  "/local/pcb/djopt/enable",   arg_false,     "$do not compile the djopt"},
	{"buildin-djopt",  "/local/pcb/djopt/buildin",  arg_true,      "$static link the djopt plugin into the executable"},
	{"plugin-djopt",   "/local/pcb/djopt/buildin",  arg_false,     "$the djopt plugin is dynamic loadable"},

	{"disable-mincut",  "/local/pcb/mincut/enable",   arg_false,     "$do not compile the mincut"},
	{"buildin-mincut",  "/local/pcb/mincut/buildin",  arg_true,      "$static link the mincut plugin into the executable"},
	{"plugin-mincut",   "/local/pcb/mincut/buildin",  arg_false,     "$the mincut plugin is dynamic loadable"},

	{"disable-toporouter",  "/local/pcb/toporouter/enable",   arg_false,     "$do not compile the toporouter"},
	{"buildin-toporouter",  "/local/pcb/toporouter/buildin",  arg_true,      "$static link the toporouter plugin into the executable"},
	{"plugin-toporouter",   "/local/pcb/toporouter/buildin",  arg_false,     "$the toporouter plugin is dynamic loadable"},

	{"disable-oldactions",  "/local/pcb/oldactions/enable",   arg_false,     "$do not compile the oldactions"},
	{"buildin-oldactions",  "/local/pcb/oldactions/buildin",  arg_true,      "$static link the oldactions plugin into the executable"},
	{"plugin-oldactions",   "/local/pcb/oldactions/buildin",  arg_false,     "$the oldactions plugin is dynamic loadable"},

	{"disable-renumber",  "/local/pcb/renumber/enable",   arg_false,     "$do not compile the renumber action"},
	{"buildin-renumber",  "/local/pcb/renumber/buildin",  arg_true,      "$static link the renumber action into the executable"},
	{"plugin-renumber",   "/local/pcb/renumber/buildin",  arg_false,     "$the renumber action is dynamic loadable plugin"},

	{"disable-stroke",  "/local/pcb/stroke/enable",   arg_false,     "$do not compile libstroke gestures"},
	{"buildin-stroke",  "/local/pcb/stroke/buildin",  arg_true,      "$static link libstroke gestures into the executable"},
	{"plugin-stroke",   "/local/pcb/stroke/buildin",  arg_false,     "$libstroke gestures is dynamic loadable plugin"},

	{NULL, NULL, NULL, NULL}
};


static void help1(void)
{
	printf("./configure: configure pcn-rnd.\n");
	printf("\n");
	printf("Usage: ./configure [options]\n");
	printf("\n");
	printf("options are:\n");
	printf(" --prefix=path         change installation prefix from /usr to path\n");
}

static void help2(void)
{
	printf("\n");
	printf("The --disable options will make ./configure to skip detection of the ");
	printf("given feature and mark them \"not found\".");
	printf("\n");
}

/* Runs when a custom command line argument is found
 returns true if no furhter argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	if (strcmp(key, "prefix") == 0) {
		report("Setting prefix to '%s'\n", value);
		put("/local/prefix", strclone(value));
		return 1;
	}
	else if (strcmp(key, "help") == 0) {
		help1();
		arg_auto_print_options(stdout, " ", "                    ", disable_libs);
		help2();
		exit(0);
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
	db_mkdir("/local/pcb");

	/* DEFAULTS */
	db_mkdir("/local/pcb/gpmi");
	put("/local/pcb/gpmi/buildin", strue);
	put("/local/prefix", "/usr/local");


	db_mkdir("/local/pcb/autoroute");
	put("/local/pcb/autoroute/enable", strue);
	put("/local/pcb/autoroute/buildin", strue);

	db_mkdir("/local/pcb/autoplace");
	put("/local/pcb/autoplace/enable", strue);
	put("/local/pcb/autoplace/buildin", strue);

	db_mkdir("/local/pcb/vendordrill");
	put("/local/pcb/vendordrill/enable", strue);
	put("/local/pcb/vendordrill/buildin", strue);

	db_mkdir("/local/pcb/puller");
	put("/local/pcb/puller/enable", strue);
	put("/local/pcb/puller/buildin", strue);

	db_mkdir("/local/pcb/edif");
	put("/local/pcb/edif/enable", strue);
	put("/local/pcb/edif/buildin", strue);

	db_mkdir("/local/pcb/djopt");
	put("/local/pcb/djopt/enable", strue);
	put("/local/pcb/djopt/buildin", strue);

	db_mkdir("/local/pcb/mincut");
	put("/local/pcb/mincut/enable", strue);
	put("/local/pcb/mincut/buildin", strue);

	db_mkdir("/local/pcb/toporouter");
	put("/local/pcb/toporouter/enable", strue);
	put("/local/pcb/toporouter/buildin", strue);

	db_mkdir("/local/pcb/oldactions");
	put("/local/pcb/oldactions/enable", strue);
	put("/local/pcb/oldactions/buildin", strue);

	db_mkdir("/local/pcb/renumber");
	put("/local/pcb/renumber/enable", strue);
	put("/local/pcb/renumber/buildin", strue);

	db_mkdir("/local/pcb/stroke");
	put("/local/pcb/stroke/enable", sfalse);
	put("/local/pcb/stroke/buildin", strue);

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

	require("cc/fpic",  0, 1);
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
	require("sys/sysid", 0, 1);

	/* options for config.h */
	require("sys/path_sep", 0, 1);
	require("cc/alloca/presents", 0, 0);
	require("cc/rdynamic", 0, 0);
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

	require("libs/script/gpmi/presents", 0, 0);


	if (get("cc/rdynamic") == NULL)
		put("cc/rdynamic", "");


	{
		char *tmp, *fpic, *debug;
		fpic = get("/target/cc/fpic");
		if (fpic == NULL) fpic = "";
		debug = get("/arg/debug");
		if (debug == NULL) debug = "";
		tmp = str_concat(" ", fpic, debug, NULL);
		put("/local/global_cflags", tmp);
	}

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

static void list_presents(const char *prefix, const char *nodes[])
{
	char **s;
	printf("%s", prefix);
	for(s = nodes; s[0] != NULL; s+=2)
		if (node_istrue(s[1]))
			printf(" %s", s[0]);
	printf("\n");
}

static int gpmi_config(void)
{
	char *tmp;
	const char *gcfg = get("libs/script/gpmi/gpmi-config");
	int generr = 0;

	printf("Generating pcb-gpmi/Makefile.conf (%d)\n", generr |= tmpasm("../src_plugins/gpmi/pcb-gpmi", "Makefile.config.in", "Makefile.config"));


	printf("Configuring gpmi packages...\n");
	tmp = str_concat("", "cd ../src_plugins/gpmi/pcb-gpmi/gpmi_plugin/gpmi_pkg && ", gcfg, " --pkggrp && ./configure", NULL);
	generr |= system(tmp);
	free(tmp);

	printf("Configuring gpmi plugin \"app\"\n");
	tmp = str_concat("", "cd ../src_plugins/gpmi//pcb-gpmi/gpmi_plugin && ", gcfg, " --app", NULL);
	generr |= system(tmp);
	free(tmp);

	printf("Finished configuring gpmi packages\n");

	return generr;
}

static void plugin_stat(const char *header, const char *path_prefix)
{
	char path_en[256], path_bi[256];

	sprintf(path_en, "%s/enable", path_prefix);
	sprintf(path_bi, "%s/buildin", path_prefix);

	printf("%-32s", header);
	if (node_istrue(path_en)) {
		printf("yes ");
		if (node_istrue(path_bi))
			printf("(buildin)\n");
		else
			printf("(plugin)\n");
	}
	else
		printf("no\n");


}

/* Runs after detection hooks, should generate the output (Makefiles, etc.) */
int hook_generate()
{
	char *rev = "non-svn", *tmp;
	int manual_config = 0, generr = 0;

	tmp = svn_info(0, "../src", "Revision:");
	if (tmp != NULL) {
		rev = str_concat("", "svn r", tmp, NULL);
		free(tmp);
	}
	put("/local/revision", rev);
	put("/local/version",  version);

	printf("Generating Makefile.conf (%d)\n", generr |= tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating gts/Makefile (%d)\n", generr |= tmpasm("../src_3rd/gts", "Makefile.in", "Makefile"));
	printf("Generating pcb/Makefile (%d)\n", generr |= tmpasm("../src", "Makefile.in", "Makefile"));

	/* Has to be after pcb/Makefile so that all the modules are loaded. */
	printf("Generating pcb/hidlist  (%d)\n", generr |= tmpasm("../src/hid/common", "hidlist.h.in", "hidlist.h"));
	printf("Generating pcb/buildin  (%d)\n", generr |= tmpasm("../src", "buildin.c.in", "buildin.c"));

	printf("Generating util/Makefile (%d)\n", generr |= tmpasm("../util", "Makefile.in", "Makefile"));

	printf("Generating config.auto.h (%d)\n", generr |= tmpasm("..", "config.auto.h.in", "config.auto.h"));

	if (node_istrue("libs/script/gpmi/presents"))
		gpmi_config();

	if (!exists("../config.manual.h")) {
		printf("Generating config.manual.h (%d)\n", generr |= tmpasm("..", "config.manual.h.in", "config.manual.h"));
		manual_config = 1;
	}

	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n\n");
	list_presents("GUI hids: batch", gui_list);
	list_presents("Export hids: bom gerber lpr ps", exporter_list);

/* special case because the "presents" node */
	printf("%-32s", "Scripting via GPMI: ");
	if (node_istrue("libs/script/gpmi/presents")) {
		printf("yes ");
		if (node_istrue("/local/pcb/gpmi/buildin"))
			printf("(buildin)\n");
		else
			printf("(plugin)\n");
	}
	else
		printf("no\n");

	plugin_stat("Autorouter: ",            "/local/pcb/autoroute");
	plugin_stat("Toporouter: ",            "/local/pcb/toporouter");
	plugin_stat("Autoplace: ",             "/local/pcb/autoplace");
	plugin_stat("Vendor drill mapping: ",  "/local/pcb/vendordrill");
	plugin_stat("Puller: ",                "/local/pcb/puller");
	plugin_stat("Edif: ",                  "/local/pcb/edif");
	plugin_stat("djopt: ",                 "/local/pcb/djopt");
	plugin_stat("Mincut: ",                "/local/pcb/mincut");
	plugin_stat("renumber:",               "/local/pcb/renumber");
	plugin_stat("old actions:",            "/local/pcb/oldactions");
	plugin_stat("stroke:",                 "/local/pcb/stroke");


	if (manual_config)
		printf("\n\n * NOTE: you may want to edit config.manual.h (user preferences) *\n");
	}
	else
		fprintf(stderr, "Error generating some of the files\n");


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

