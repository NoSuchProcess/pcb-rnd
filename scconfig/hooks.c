#include <stdio.h>
#include <string.h>
#include "arg.h"
#include "log.h"
#include "dep.h"
#include "libs.h"
#include "db.h"
#include "tmpasm.h"
#include "tmpasm_scconfig.h"
#include "generator.h"
#include "util/arg_auto_set.h"
#include "Rev.h"

#define version "3.1.5-rc1"

#define REQ_LIBRND_MAJOR 4
#define REQ_LIBRND_MINOR 2

#define WANT_POLYBOOL_DEFAULT sfalse
#define WANT_POLYBOOL2_DEFAULT sfalse

#define TOSTR_(x) #x
#define TOSTR(x) TOSTR_(x)

#include <librnd/src_3rd/puplug/scconfig_hooks.h>
#include <librnd/src_3rd/libfungw/scconfig_hooks.h>

/* we are doing /local/pcb/ */
#define LIBRND_SCCONFIG_APP_TREE "pcb"

#include <librnd/scconfig/plugin_3state.h>
#include <librnd/scconfig/hooks_common.h>
#include <librnd/scconfig/rnd_hook_detect.h>

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"enable-bison",      "/local/pcb/want_bison",        arg_true,      "$enable generating language files using bison/flex"},
	{"disable-bison",     "/local/pcb/want_bison",        arg_false,     "$disable generating language files using bison/flex"},
	{"enable-byaccic",    "/local/pcb/want_byaccic",      arg_true,      "$enable generating language files using byaccic/ureglex"},
	{"disable-byaccic",   "/local/pcb/want_byaccic",      arg_false,     "$disable generating language files byaccic/ureglex"},
	{"enable-polybool",   "/local/pcb/want_polybool",     arg_true,      "$enable the first new polygon clipping library"},
	{"disable-polybool",  "/local/pcb/want_polybool",     arg_false,     "$disable the first new polygon clipping library"},
	{"enable-polybool2",  "/local/pcb/want_polybool2",    arg_true,      "$enable the second new polygon clipping library"},
	{"disable-polybool2", "/local/pcb/want_polybool2",    arg_false,     "$disable the second new polygon clipping library"},

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) plugin3_args(name, desc)
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"

	{NULL, NULL, NULL, NULL}
};

#define TO_STR_(payload) #payload
#define TO_STR(payload) TO_STR_(payload)

static void help1(void)
{
	rnd_help1("pcb-rnd");

	printf(" --dot_pcb_rnd=path         .pcb-rnd config path under $HOME/\n");
}

#define need_value(msg) \
	if (value == NULL) { \
		fprintf(stderr, "syntax error; %s\n", msg); \
		exit(1); \
	}

/* Runs when a custom command line argument is found
 returns true if no further argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	rnd_hook_custom_arg(key, value, disable_libs); /* call arg_auto_print_options() instead */

	if (strcmp(key, "dot_pcb_rnd") == 0) {
		need_value("use --dot_pcb_rnd=dir");
		put("/local/pcb/dot_pcb_rnd", value);
		return 1;
	}
	if (strcmp(key, "static") == 0) {
		put("/local/pcb/want_static", strue);
		return 1;
	}
	if (strcmp(key, "enable-polybool") == 0) {
		put("/local/pcb/want_polybool2", sfalse);
	}
	if (strcmp(key, "enable-polybool2") == 0) {
		put("/local/pcb/want_polybool", sfalse);
	}
	if (arg_auto_set(key, value, disable_libs) == 0) {
		fprintf(stderr, "Error: unknown argument %s\n", key);
		exit(1);
	}
	return 1; /* handled by arg_auto_set() */
}

/* Runs before anything else */
int hook_preinit()
{
	return 0;
}

void librnd_ver_req_min(int exact_major, int min_minor);

/* Runs after initialization */
int hook_postinit()
{
	db_mkdir("/local");
	db_mkdir("/local/pcb");

	rnd_hook_postinit();

	put("/local/pcb/want_bison", sfalse);
	put("/local/pcb/want_byaccic", sfalse);
	put("/local/pcb/want_static", sfalse);
	put("/local/pcb/want_polybool", WANT_POLYBOOL_DEFAULT);
	put("/local/pcb/want_polybool2", WANT_POLYBOOL2_DEFAULT);
	put("/local/pcb/dot_pcb_rnd", ".pcb-rnd");
	put("/local/pcb/librnd_prefix", TO_STR(LIBRND_PREFIX));

	librnd_ver_req_min(REQ_LIBRND_MAJOR, REQ_LIBRND_MINOR);

	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	char *tmp;
	const char *libad;

	put("/local/pcb/librnd_template", tmp = str_concat("", TO_STR(LIBRND_PREFIX), "/", get("/local/libarchdir"), "/librnd4/scconfig/template", NULL));
	free(tmp);

	/* if librnd is installed at some custom path, we'll need to have a -I on CFLAGS and -L on LDFLAGS */
	libad = get("/local/libarchdir");
	if (((strncasecmp(TO_STR(LIBRND_PREFIX), "/usr/include", 12) != 0) && (strncasecmp(TO_STR(LIBRND_PREFIX), "/usr/local/include", 18) != 0)) || (strcmp(libad, "lib") != 0)) {
		put("/local/pcb/librnd_extra_inc", "-I" TO_STR(LIBRND_PREFIX) "/include");
		put("/local/pcb/librnd_extra_ldf", tmp = str_concat("", "-L" TO_STR(LIBRND_PREFIX) "/", libad, NULL));
		free(tmp);
	}

	return rnd_hook_postarg(TO_STR(LIBRND_PREFIX), "pcb-rnd");
}



/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	return rnd_hook_detect_host();
}

	/* figure if we need the dialogs plugin */
static void calc_dialog_deps(void)
{
	const char *control;
	int buildin, plugin;

	/* TODO: get this from librnd */
	control = get("/local/module/lib_hid_common/controls");
	if (control == NULL) {
		fprintf(stderr, "librnd configuration error: can't figure how lib_hid_common is configured\n(should be coming from plugin.state as /local/module/lib_hid_common/controls)\n");
		exit(1);
	}
	buildin = strcmp(control, "buildin") == 0;
	plugin = strcmp(control, "plugin") == 0;

	if (buildin) {
		hook_custom_arg("buildin-draw_csect", NULL);
		hook_custom_arg("buildin-draw_fontsel", NULL);
		hook_custom_arg("buildin-dialogs", NULL);
		hook_custom_arg("buildin-lib_hid_pcbui", NULL);
	}
	else if (plugin) {
		if (!plug_is_buildin("draw_csect"))
			hook_custom_arg("plugin-draw_csect", NULL);
		if (!plug_is_buildin("draw_fontsel"))
			hook_custom_arg("plugin-draw_fontsel", NULL);
		if (!plug_is_buildin("dialogs"))
			hook_custom_arg("plugin-dialogs", NULL);
		if (!plug_is_buildin("lib_hid_pcbui"))
			hook_custom_arg("plugin-lib_hid_pcbui", NULL);
	}
}

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	int want_gd, want_xml2, want_freetype2, want_fuse;

	want_gd     = plug_is_enabled("export_png");
	want_xml2   = plug_is_enabled("io_eagle") || plug_is_enabled("order_pcbway") || plug_is_enabled("stl_export");
	want_freetype2 = plug_is_enabled("import_ttf");
	want_fuse = plug_is_enabled("export_vfs_fuse");

	rnd_hook_detect_cc();
	if (rnd_hook_detect_sys() != 0)
		return 1;

	if (want_fuse) {
		require("libs/sul/fuse/*", 0, 0);
		if (!istrue(get("libs/sul/fuse/presents"))) {
			report_repeat("WARNING: Since there's no fuse found, disabling export_vfs_fuse plugin...\n");
			hook_custom_arg("disable-export_vfs_fuse", NULL);
		}
	}

	if (want_freetype2) {
		require("libs/sul/freetype2/presents", 0, 0);
		if (!istrue(get("libs/sul/freetype2/presents"))) {
			report_repeat("WARNING: Since there's no libfreetype2 found, disabling the import_ttf plugin...\n");
			hook_custom_arg("disable-import_ttf", NULL);
		}
	}

	if (want_xml2) {
		require("libs/sul/libxml2/presents", 0, 0);
		if (!istrue(get("libs/sul/libxml2/presents"))) {
			report("libxml2 is not available, disabling io_eagle...\n");
			report_repeat("WARNING: Since there's no libxml2 found, disabling the Eagle IO, amf 3d models and pcbway order plugins...\n");
			hook_custom_arg("disable-io_eagle", NULL);
			hook_custom_arg("disable-order_pcbway", NULL);
		}
		put("/local/pcb/want_libxml2", strue);
	}
	else
		put("/local/pcb/want_libxml2", strue);

	calc_dialog_deps();

	if (want_gd) {
		require("libs/gui/gd/presents", 0, 0);
		if (!istrue(get("libs/gui/gd/presents"))) {
			report_repeat("WARNING: Since there's no libgd, disabling gd based exports and pixmap imports (png, gif, jpeg)...\n");
			hook_custom_arg("disable-export_png", NULL);
			want_gd = 0;
		}
	}
	else {
		put("libs/gui/gd/presents", sfalse);
	}

	/* yacc/lex - are we able to regenerate languages? */
	if (istrue(get("/local/pcb/want_bison"))) {
		require("parsgen/flex/*", 0, 0);
		require("parsgen/bison/*", 0, 0);
		if (!istrue(get("parsgen/flex/presents")) || !istrue(get("parsgen/bison/presents")))
			put("/local/pcb/want_parsgen", sfalse);
		else
			put("/local/pcb/want_parsgen", strue);
	}
	else {
		report("Bison/flex are disabled, along with parser generation.\n");
		put("/local/pcb/want_parsgen", sfalse);
	}

	/* byaccic - are we able to regenerate languages? */
	if (istrue(get("/local/pcb/want_byaccic"))) {
		require("parsgen/byaccic/*", 0, 0);
		require("parsgen/ureglex/*", 0, 0);
		if (!istrue(get("parsgen/byaccic/presents")) || !istrue(get("parsgen/ureglex/presents")))
			put("/local/pcb/want_parsgen_byaccic", sfalse);
		else
			put("/local/pcb/want_parsgen_byaccic", strue);
	}
	else {
		report("byaccic/ureglex are disabled, along with parser generation.\n");
		put("/local/pcb/want_parsgen_byaccic", sfalse);
	}

	/* plugin dependencies */
	plugin_deps(1);

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
	char *next, *rev = "non-svn", *curr, *tmp, *sep;
	int generr = 0;
	int res = 0;
	char apiver[32], version_major[32];
	long r = 0;
	int v1, v2, v3;

	tmp = svn_info(0, "../src", "Revision:");
	if (tmp != NULL) {
		r = strtol(tmp, NULL, 10);
		rev = str_concat("", "svn r", tmp, NULL);
		free(tmp);
	}
	strcpy(apiver, version);
	curr = apiver; next = strchr(curr, '.'); *next = '\n';
	v1 = atoi(curr);
	curr = next+1; next = strchr(curr, '.'); *next = '\n';
	v2 = atoi(curr);
	v3 = atoi(next+1);
	sprintf(apiver, "%01d%01d%02d%05ld", v1, v2, v3, r);

	strcpy(version_major, version);
	sep = strchr(version_major, '.');
	if (sep != NULL)
		*sep = '\0';

	if (istrue(get("/local/pcb/want_polybool")) || istrue(get("/local/pcb/want_polybool2")))
		put("/local/pcb/want_oldpoly", sfalse);
	else
		put("/local/pcb/want_oldpoly", strue);

	logprintf(0, "scconfig generate version info: version='%s' rev='%s'\n", version, rev);
	put("/local/revision", rev);
	put("/local/version",  version);
	put("/local/version_major",  version_major);
	put("/local/apiver", apiver);
	put("/local/librnd_ver_req_major", TOSTR(REQ_LIBRND_MAJOR));
	put("/local/librnd_ver_req_minor", TOSTR(REQ_LIBRND_MINOR));

	printf("\n");

	put("/local/pup/sccbox", "../../scconfig/sccbox");


	printf("Generating Makefile.conf (%d)\n", generr |= tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating src/Makefile (%d)\n", generr |= tmpasm("../src", "Makefile.in", "Makefile"));

	printf("Generating util/gsch2pcb-rnd/Makefile (%d)\n", generr |= tmpasm("../util", "gsch2pcb-rnd/Makefile.in", "gsch2pcb-rnd/Makefile"));
	printf("Generating util/bxl2txt/Makefile (%d)\n", generr |= tmpasm("../util", "bxl2txt/Makefile.in", "bxl2txt/Makefile"));

	printf("Generating pcb-rnd config.h (%d)\n", generr |= tmpasm("..", "config.h.in", "config.h"));

	if (plug_is_enabled("export_vfs_fuse"))
		printf("Generating fuse_includes.h (%d)\n", generr |= tmpasm("../src_plugins/export_vfs_fuse", "fuse_includes.h.in", "fuse_includes.h"));

	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n");

	print_sum_setting("/local/pcb/want_parsgen",   "Regenerating languages with bison & flex");
	print_sum_setting("/local/rnd/debug",          "Compilation for debugging");
	print_sum_setting_or("/local/rnd/symbols",     "Include debug symbols", istrue(get("/local/rnd/debug")));
	print_sum_cfg_val("/local/prefix",             "installation prefix (--prefix)");
	print_sum_cfg_val("/local/confdir",            "configuration directory (--confdir)");
	print_sum_cfg_val("/local/pcb/dot_pcb_rnd",    ".pcb_rnd config dir under $HOME");
	print_sum_cfg_val("/local/pcb/want_oldpoly",   "use the original/old polygon lib (poly)");
	print_sum_cfg_val("/local/pcb/want_polybool",  "use the first new polygon lib (polybool)");
	print_sum_cfg_val("/local/pcb/want_polybool2", "use the second new polygon lib (polybool2)");

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) plugin3_stat(name, desc)
#define plugin_header(sect) printf(sect);
#define plugin_dep(plg, on)
#include "plugins.h"

	if (repeat != NULL) {
		printf("\n%s\n", repeat);
	}

	if (all_plugin_check_explicit()) {
		printf("\nNevertheless the configuration is complete, if you accept these differences\nyou can go on compiling.\n\n");
		res = 1;
	}
	else
		printf("\nConfiguration complete, ready to compile.\n\n");

	{
		FILE *f;
		f = fopen("Rev.stamp", "w");
		fprintf(f, "%d", myrev);
		fclose(f);
	}

	}
	else {
		fprintf(stderr, "Error generating some of the files\n");
		res = 1;
	}

	return res;
}

/* Runs before everything is uninitialized */
void hook_preuninit()
{
}

/* Runs at the very end, when everything is already uninitialized */
void hook_postuninit()
{
}

