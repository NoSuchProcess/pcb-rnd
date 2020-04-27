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
#include "Rev.h"

#define version "2.2.1"

#include "../src_3rd/puplug/scconfig_hooks.h"
#include "../src_3rd/libfungw/scconfig_hooks.h"

#include "librnd/scconfig/plugin_3state.h"
#include "librnd/scconfig/hooks_common.h"
#include "librnd/scconfig/hooks_gui.c"
#include "librnd/scconfig/rnd_hook_detect.h"

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-xrender",   "libs/gui/xrender",             arg_lib_nodes, "$do not use xrender for lesstif"},
	{"disable-xinerama",  "libs/gui/xinerama",            arg_lib_nodes, "$do not use xinerama for lesstif"},
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"disable-gd-gif",    "libs/gui/gd/gdImageGif",       arg_lib_nodes, "$no gif support in the png pcb_exporter"},
	{"disable-gd-png",    "libs/gui/gd/gdImagePng",       arg_lib_nodes, "$no png support in the png pcb_exporter"},
	{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg",      arg_lib_nodes, "$no jpeg support in the png pcb_exporter"},
	{"enable-bison",      "/local/pcb/want_bison",        arg_true,      "$enable generating language files using bison/flex"},
	{"disable-bison",     "/local/pcb/want_bison",        arg_false,     "$disable generating language files using bison/flex"},
	{"enable-byaccic",    "/local/pcb/want_byaccic",      arg_true,      "$enable generating language files using byaccic/ureglex"},
	{"disable-byaccic",   "/local/pcb/want_byaccic",      arg_false,     "$disable generating language files byaccic/ureglex"},
	{"enable-dmalloc",    "/local/pcb/want_dmalloc",      arg_true,      "$compile with lib dmalloc"},
	{"disable-dmalloc",   "/local/pcb/want_dmalloc",      arg_false,     "$compile without lib dmalloc"},

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) plugin3_args(name, desc)
#define plugin_header(sect)
#define plugin_dep(plg, on, hidlib)
#include "plugins.h"

	{NULL, NULL, NULL, NULL}
};

static void help1(void)
{
	rnd_help1("pcb-rnd");

	printf(" --coord=32|64              set coordinate integer type's width in bits\n");
	printf(" --dot_pcb_rnd=path         .pcb-rnd config path under $HOME/\n");
	printf(" --workaround-gtk-ctrl      enable GTK control key query workaround\n");
}

/* Runs when a custom command line argument is found
 returns true if no further argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	rnd_hook_custom_arg(key, value);

	if (strcmp(key, "dot_pcb_rnd") == 0) {
		put("/local/pcb/dot_pcb_rnd", value);
		return 1;
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

/* Runs after initialization */
int hook_postinit()
{
	db_mkdir("/local");
	db_mkdir("/local/pcb");

	pup_hook_postinit();
	fungw_hook_postinit();

	rnd_hook_postinit();

	put("/local/pcb/want_bison", sfalse);
	put("/local/pcb/want_byaccic", sfalse);
	put("/local/pcb/dot_pcb_rnd", ".pcb-rnd");

	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	return rnd_hook_postarg();
}



/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	pup_hook_detect_host();
	fungw_hook_detect_host();

	require("fstools/ar",  0, 1);
	require("fstools/mkdir", 0, 1);
	require("fstools/rm",  0, 1);
	require("fstools/cp",  0, 1);
	require("fstools/ln",  0, 1);

/* until we rewrite the generators in C */
	require("fstools/awk",  0, 1);

	require("cc/argstd/*", 0, 0);

	require("cc/func_attr/unused/*", 0, 0);
	require("cc/inline", 0, 0);

	return 0;
}

	/* figure if we need the dialogs plugin */
static void calc_dialog_deps(void)
{
	int buildin, plugin;

	rnd_calc_dialog_deps(); /* remove after librnd separation */

	buildin = istrue("/target/librnd/dialogs/buildin");
	plugin = istrue("/target/librnd/dialogs/plugin");

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
	int need_gtklibs = 0, want_glib = 0, want_gtk, want_gtk2, want_gd, want_stroke, want_xml2, has_gtk2 = 0, want_gl, want_freetype2, want_fuse;
	int can_live_without_dynlib = 0; /* special optional exception for windows cross compilation for now */

	if (istrue(get("/target/pcb/can_live_without_dynlib")))
		can_live_without_dynlib = 1;

	want_gtk2   = plug_is_enabled("hid_gtk2_gdk") || plug_is_enabled("hid_gtk2_gl");
	want_gtk    = want_gtk2; /* plus |gtkN */
	want_gd     = plug_is_enabled("export_png") || plug_is_enabled("import_pxm_gd");
	want_stroke = plug_is_enabled("stroke");
	want_xml2   = plug_is_enabled("io_eagle") || plug_is_enabled("order_pcbway");
	want_freetype2 = plug_is_enabled("import_ttf");
	want_fuse = plug_is_enabled("export_vfs_fuse");

/****** TODO #21: core depends on this plugin (yes, this is a bug) ******/
	hook_custom_arg("buildin-lib_compat_help", NULL);

	plugin_db_hidlib();

	rnd_hook_detect_cc();

	pup_hook_detect_target();

	require("signal/names/*",  0, 0);
	require("libs/env/setenv/*",  0, 0);
	if (!istrue(get("libs/env/setenv/presents")))
		require("libs/env/putenv/*",  0, 0);
	require("libs/fs/mkdtemp/*",  0, 0);
	require("libs/fs/realpath/*",  0, 0);
	require("libs/fs/readdir/*",  0, 1);
	require("libs/io/fileno/*",  0, 1);
	require("libs/io/popen/*",  0, 1);
	require("libs/math/rint/*",  0, 0);
	require("libs/math/round/*",  0, 0);
	require("libs/userpass/getpwuid/*",  0, 0);
	require("libs/script/fungw/*",  0, 0);

	if (istrue(get("libs/script/fungw/presents"))) {
		require("libs/script/fungw/user_call_ctx/*",  0, 0);
		if (!istrue(get("libs/script/fungw/user_call_ctx/presents"))) {
			put("libs/script/fungw/presents", sfalse);
			report_repeat("\nWARNING: system installed fungw is too old, can not use it, please install a newer version (falling back to minimal fungw shipped with pcb-rnd).\n\n");
		}
	}

	if (!istrue(get("libs/script/fungw/presents")))
		fungw_hook_detect_target();

	{
		int miss_select = require("libs/socket/select/*",  0, 0);
		if (require("libs/time/usleep/*",  0, 0) && require("libs/time/Sleep/*",  0, 0) && miss_select) {
			report_repeat("\nERROR: can not find usleep() or Sleep() or select() - no idea how to sleep ms.\n\n");
			return 1;
		}
	}

	require("libs/time/gettimeofday/*",  0, 1);

	if (require("libs/ldl",  0, 0) != 0) {
		if (require("libs/LoadLibrary",  0, 0) != 0) {
			if (can_live_without_dynlib) {
				report_repeat("\nWARNING: no dynamic linking found on your system. Dynamic plugin loading will fail.\n\n");
			}
			else {
				report_repeat("\nERROR: no dynamic linking found on your system. Can not compile pcb-rnd.\n\n");
				return 1;
			}
		}
	}

	if (require("libs/proc/wait",  0, 0) != 0) {
		if (require("libs/proc/_spawnvp",  0, 0) != 0) {
			report_repeat("\nERROR: no fork or _spawnvp. Can not compile pcb-rnd.\n\n");
			return 1;
		}
	}

	if (require("libs/fs/_mkdir",  0, 0) != 0) {
		if (require("libs/fs/mkdir",  0, 0) != 0) {
			report_repeat("\nERROR: no mkdir() or _mkdir(). Can not compile pcb-rnd.\n\n");
			return 1;
		}
	}

	if (require("libs/fs/getcwd/*",  0, 0) != 0)
		if (require("libs/fs/_getcwd/*",  0, 0) != 0)
			if (require("libs/fs/getwd/*",  0, 0) != 0) {
				report_repeat("\nERROR: Can not find any getcwd() variant.\n\n");
				return 1;
			}

	if (want_fuse) {
		require("libs/sul/fuse/*", 0, 0);
		if (!istrue(get("libs/sul/fuse/presents"))) {
			report_repeat("WARNING: Since there's no fuse found, disabling export_vfs_fuse plugin...\n");
			hook_custom_arg("disable-export_vfs_fuse", NULL);
		}
	}

	if (want_stroke) {
		require("libs/gui/libstroke/presents", 0, 0);
		if (!istrue(get("libs/gui/libstroke/presents"))) {
			report_repeat("WARNING: Since there's no libstroke found, disabling the stroke plugin...\n");
			hook_custom_arg("disable-stroke", NULL);
		}
	}

	if (want_freetype2) {
		require("libs/sul/freetype2/presents", 0, 0);
		if (!istrue(get("libs/sul/freetype2/presents"))) {
			report_repeat("WARNING: Since there's no libfreetype2 found, disabling the import_ttf plugin...\n");
			hook_custom_arg("disable-import_ttf", NULL);
		}
	}

	if (want_gtk2) {
		require("libs/gui/gtk2/presents", 0, 0);
		if (istrue(get("libs/gui/gtk2/presents"))) {
			require("libs/gui/gtk2/key_prefix", 0, 1);
			require("libs/gui/gtk2gl/presents", 0, 0);
			if (!istrue(get("libs/gui/gtk2gl/presents"))) {
				report_repeat("WARNING: Since there's no gl support for gtk found, disabling the gl rendering...\n");
				hook_custom_arg("disable-hid_gtk2_gl", NULL);
			}
			need_gtklibs = 1;
			has_gtk2 = 1;
		}
		else {
			report_repeat("WARNING: Since there's no libgtk2 found, disabling hid_gtk2*...\n");
			hook_custom_arg("disable-hid_gtk2_gdk", NULL);
			hook_custom_arg("disable-hid_gtk2_gl", NULL);
		}
	}

	want_gl = plug_is_enabled("hid_gtk2_gl");
	if (want_gl) {
		require("libs/gui/glu/presents", 0, 0);
		if (!istrue(get("libs/gui/glu/presents"))) {
			report_repeat("WARNING: Since there's no GLU found, disabling the hid_gtk2_gl plugin...\n");
			goto disable_gl;
		}
		else
			put("/local/pcb/has_glu", strue);
	}
	else {
		disable_gl:;
		hook_custom_arg("disable-lib_hid_gl", NULL);
		hook_custom_arg("disable-hid_gtk2_gl", NULL);
	}

	/* libs/gui/gtkx is the version-independent set of flags in the XOR model */
	if (has_gtk2) {
		put("/target/libs/gui/gtkx/cflags", get("/target/libs/gui/gtk2/cflags"));
		put("/target/libs/gui/gtkx/ldflags", get("/target/libs/gui/gtk2/ldflags"));
	}

	if (!need_gtklibs) {
		report("No gtk support available, disabling lib_gtk_common...\n");
		hook_custom_arg("disable-lib_gtk_common", NULL);
	}

	if (want_xml2) {
		require("libs/sul/libxml2/presents", 0, 0);
		if (!istrue(get("libs/sul/libxml2/presents"))) {
			report("libxml2 is not available, disabling io_eagle...\n");
			report_repeat("WARNING: Since there's no libxml2 found, disabling the Eagle IO and pcbway order plugins...\n");
			hook_custom_arg("disable-io_eagle", NULL);
			hook_custom_arg("disable-order_pcbway", NULL);
		}
		put("/local/pcb/want_libxml2", strue);
	}
	else
		put("/local/pcb/want_libxml2", strue);


	if (plug_is_enabled("hid_lesstif")) {
		require("libs/gui/lesstif2/presents", 0, 0);
		if (istrue(get("libs/gui/lesstif2/presents"))) {
			require("libs/gui/xinerama/presents", 0, 0);
			require("libs/gui/xrender/presents", 0, 0);
		}
		else {
			report_repeat("WARNING: Since there's no lesstif2 found, disabling the lesstif HID and xinerama and xrender...\n");
			hook_custom_arg("disable-xinerama", NULL);
			hook_custom_arg("disable-xrender", NULL);
			hook_custom_arg("disable-hid_lesstif", NULL);
		}
	}
	else {
		hook_custom_arg("disable-xinerama", NULL);
		hook_custom_arg("disable-xrender", NULL);
	}

	if (want_gtk)
		want_glib = 1;

	if (plug_is_enabled("puller"))
		want_glib = 1;

	if (want_glib) {
		require("libs/sul/glib", 0, 0);
		if (!istrue(get("libs/sul/glib/presents"))) {
			if (want_gtk) {
				report_repeat("WARNING: Since GLIB is not found, disabling the GTK HID...\n");
				hook_custom_arg("disable-hid_gtk2_gdk", NULL);
				hook_custom_arg("disable-hid_gtk2_gl", NULL);
			}
			if (plug_is_enabled("puller")) {
				report_repeat("WARNING: Since GLIB is not found, disabling the puller...\n");
				hook_custom_arg("disable-puller", NULL);
			}
		}
	}
	else {
		report("No need for glib, skipping GLIB detection\n");
		put("libs/sul/glib/presents", "false");
		put("libs/sul/glib/cflags", "");
		put("libs/sul/glib/ldflags", "");
	}

	if (!istrue(get("libs/sul/glib/presents"))) {
		/* Makefile templates will still reference these variables, they should be empty */
		put("libs/sul/glib/cflags", "");
		put("libs/sul/glib/ldflags", "");
	}

	calc_dialog_deps();

	if (want_gd) {
		require("libs/gui/gd/presents", 0, 0);
		if (!istrue(get("libs/gui/gd/presents"))) {
			report_repeat("WARNING: Since there's no libgd, disabling gd based exports (png)...\n");
			hook_custom_arg("disable-gd-gif", NULL);
			hook_custom_arg("disable-gd-png", NULL);
			hook_custom_arg("disable-gd-jpg", NULL);
			hook_custom_arg("disable-export_png", NULL);
			hook_custom_arg("disable-import_pxm_gd", NULL);
			want_gd = 0;
			goto disable_gd_formats;
		}
		else {
			require("libs/gui/gd/gdImagePng/presents", 0, 0);
			require("libs/gui/gd/gdImageGif/presents", 0, 0);
			require("libs/gui/gd/gdImageJpeg/presents", 0, 0);
			require("libs/gui/gd/gdImageSetResolution/presents", 0, 0);
			if (!istrue(get("libs/gui/gd/gdImagePng/presents"))) {
				report_repeat("WARNING: libgd is installed, but its png code fails, some exporters will be compiled with reduced functionality\n");
			}
		}
	}
	else {
		put("libs/gui/gd/presents", sfalse);
		disable_gd_formats:;
		put("libs/gui/gd/gdImagePng/presents", sfalse);
		put("libs/gui/gd/gdImageGif/presents", sfalse);
		put("libs/gui/gd/gdImageJpeg/presents", sfalse);
	}

	if (!istrue(get("libs/script/fungw/presents"))) {
		if (plug_is_enabled("script"))
			report_repeat("WARNING: Since there's no suitable system-installed fungw, only limited scripting is available using libfawk - if you need more scripting languages, install fungw and reconfigure.\n");
		put("/local/pcb/fungw_system", sfalse);
	}
	else
		put("/local/pcb/fungw_system", strue);

	/* generic utils for Makefiles */
	require("sys/ext_exe", 0, 1);
	require("sys/sysid", 0, 1);

	/* options for config.h */
	require("sys/path_sep", 0, 1);
	require("sys/types/size/*", 0, 1);
	require("cc/rdynamic", 0, 0);
	require("cc/soname", 0, 0);
	require("cc/so_undefined", 0, 0);
	require("libs/snprintf", 0, 0);
	require("libs/vsnprintf", 0, 0);
	require("libs/fs/getcwd", 0, 0);
	require("libs/fs/stat/macros/*", 0, 0);

	if (istrue(get("/local/pcb/want_dmalloc"))) {
		require("libs/sul/dmalloc/*", 0, 1);
	}
	else {
		put("libs/sul/dmalloc/presents", sfalse);
		put("libs/sul/dmalloc/cflags", "");
		put("libs/sul/dmalloc/ldflags", "");
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
		report("Bison/flex are disabled, among with parser generation.\n");
		put("/local/pcb/want_parsgen", sfalse);
	}

	/* byaccoc - are we able to regenerate languages? */
	if (istrue(get("/local/pcb/want_byaccic"))) {
		require("parsgen/byaccic/*", 0, 0);
		require("parsgen/ureglex/*", 0, 0);
		if (!istrue(get("parsgen/byaccic/presents")) || !istrue(get("parsgen/ureglex/presents")))
			put("/local/pcb/want_parsgen_byaccic", sfalse);
		else
			put("/local/pcb/want_parsgen_byaccic", strue);
	}
	else {
		report("byaccic/ureglex are disabled, among with parser generation.\n");
		put("/local/pcb/want_parsgen_byaccic", sfalse);
	}

	if (get("cc/rdynamic") == NULL)
		put("cc/rdynamic", "");

	/* plugin dependencies */
	plugin_deps(1);

	rnd_hook_detect_coord_bits(); /* remove this after the librnd split */

	/* restore the original CFLAGS, without the effects of --debug, so Makefiles can decide when to use what cflag (c99 needs different ones) */
	/* this should be removed after the librnd split */
	put("/target/cc/cflags", get("/local/cc_flags_save"));

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

	logprintf(0, "scconfig generate version info: version='%s' rev='%s'\n", version, rev);
	put("/local/revision", rev);
	put("/local/version",  version);
	put("/local/version_major",  version_major);
	put("/local/apiver", apiver);
	put("/local/pup/sccbox", "../../scconfig/sccbox");

	printf("\n");

	printf("Generating Makefile.conf (%d)\n", generr |= tmpasm("..", "Makefile.conf.in", "Makefile.conf"));

	printf("Generating pcb/Makefile (%d)\n", generr |= tmpasm("../src", "Makefile.in", "Makefile"));

	printf("Generating librnd.mak (%d)\n", generr |= tmpasm("../src/librnd/core", "librnd.mak.in", "librnd.mak"));

	printf("Generating util/gsch2pcb-rnd/Makefile (%d)\n", generr |= tmpasm("../util", "gsch2pcb-rnd/Makefile.in", "gsch2pcb-rnd/Makefile"));
	printf("Generating util/bxl2txt/Makefile (%d)\n", generr |= tmpasm("../util", "bxl2txt/Makefile.in", "bxl2txt/Makefile"));

	printf("Generating librnd config.h (%d)\n", generr |= tmpasm("../src/librnd", "config.h.in", "config.h"));
	printf("Generating pcb-rnd config.h (%d)\n", generr |= tmpasm("..", "config.h.in", "config.h"));

	printf("Generating compat_inc.h (%d)\n", generr |= tmpasm("../src/librnd/core", "compat_inc.h.in", "compat_inc.h"));

	printf("Generating opengl.h (%d)\n", generr |= tmpasm("../src_plugins/lib_hid_gl", "opengl.h.in", "opengl.h"));

	printf("Generating tests/librnd/inc_all.h (%d)\n", generr |= tmpasm("../tests/librnd", "inc_all.h.in", "inc_all.h"));

	if (plug_is_enabled("export_vfs_fuse"))
		printf("Generating fuse_includes.h (%d)\n", generr |= tmpasm("../src_plugins/export_vfs_fuse", "fuse_includes.h.in", "fuse_includes.h"));

	generr |= pup_hook_generate("../src_3rd/puplug");

	if (!istrue(get("libs/script/fungw/presents")))
		generr |= fungw_hook_generate("../src_3rd/libfungw");


	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n");

	print_sum_setting("/local/pcb/want_parsgen",   "Regenerating languages with bison & flex");
	print_sum_setting("/local/pcb/debug",          "Compilation for debugging");
	print_sum_setting_or("/local/pcb/symbols",        "Include debug symbols", istrue(get("/local/pcb/debug")));
	print_sum_setting("libs/sul/dmalloc/presents", "Compile with dmalloc");
	print_sum_cfg_val("/local/pcb/coord_bits",     "Coordinate type bits");
	print_sum_cfg_val("/local/pcb/dot_pcb_rnd",    ".pcb_rnd config dir under $HOME");

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_, hidlib_) plugin3_stat(name, desc)
#define plugin_header(sect) printf(sect);
#define plugin_dep(plg, on, hidlib)
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

