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

#define version "1.1.3"

#include "plugin_3state.h"

int want_intl = 0, want_coord_bits;

const arg_auto_set_t disable_libs[] = { /* list of --disable-LIBs and the subtree they affect */
	{"disable-xrender",   "libs/gui/xrender",             arg_lib_nodes, "$do not use xrender for lesstif"},
	{"disable-xinerama",  "libs/gui/xinerama",            arg_lib_nodes, "$do not use xinerama for lesstif"},
	{"disable-gd",        "libs/gui/gd",                  arg_lib_nodes, "$do not use gd (many exporters need it)"},
	{"disable-gd-gif",    "libs/gui/gd/gdImageGif",       arg_lib_nodes, "$no gif support in the png exporter"},
	{"disable-gd-png",    "libs/gui/gd/gdImagePng",       arg_lib_nodes, "$no png support in the png exporter"},
	{"disable-gd-jpg",    "libs/gui/gd/gdImageJpeg",      arg_lib_nodes, "$no jpeg support in the png exporter"},
	{"enable-bison",      "/local/pcb/want_bison",        arg_true,      "$enable generating language files"},
	{"disable-bison",     "/local/pcb/want_bison",        arg_false,     "$disable generating language files"},
	{"enable-dmalloc",    "/local/pcb/want_dmalloc",      arg_true,      "$compile with lib dmalloc"},
	{"disable-dmalloc",   "/local/pcb/want_dmalloc",      arg_false,     "$compile without lib dmalloc"},

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) plugin3_args(name, desc)
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"

	{NULL, NULL, NULL, NULL}
};

static void help1(void)
{
	printf("./configure: configure pcb-rnd.\n");
	printf("\n");
	printf("Usage: ./configure [options]\n");
	printf("\n");
	printf("options are:\n");
	printf(" --prefix=path              change installation prefix from /usr to path\n");
	printf(" --debug                    build full debug version (-g -O0, extra asserts)\n");
	printf(" --symbols                  include symbols (add -g, but no -O0 or extra asserts)\n");
	printf(" --coord=32|64              set coordinate integer type's width in bits\n");
	printf(" --dot_pcb_pcb=path         .pcb-rnd config path under $HOME/\n");
	printf(" --workaround-gtk-ctrl      enable GTK control key query workaround\n");
	printf(" --all=plugin               enable all working plugins for dynamic load\n");
	printf(" --all=buildin              enable all working plugins for static link\n");
	printf(" --all=disable              disable all plugins (compile core only)\n");
}

static void help2(void)
{
	printf("\n");
	printf("Some of the --disable options will make ./configure to skip detection of the given feature and mark them \"not found\".");
	printf("\n");
}

char *repeat = NULL;
#define report_repeat(msg) \
do { \
	report(msg); \
	if (repeat != NULL) { \
		char *old = repeat; \
		repeat = str_concat("", old, msg, NULL); \
		free(old); \
	} \
	else \
		repeat = strclone(msg); \
} while(0)

static void all_plugin_select(const char *state);

/* Runs when a custom command line argument is found
 returns true if no further argument processing should be done */
int hook_custom_arg(const char *key, const char *value)
{
	if (strcmp(key, "prefix") == 0) {
		report("Setting prefix to '%s'\n", value);
		put("/local/prefix", strclone(value));
		return 1;
	}
	if (strcmp(key, "debug") == 0) {
		put("/local/pcb/debug", strue);
		return 1;
	}
	if (strcmp(key, "symbols") == 0) {
		put("/local/pcb/symbols", strue);
		return 1;
	}
	if (strcmp(key, "coord") == 0) {
		int v = atoi(value);
		if ((v != 32) && (v != 64)) {
			report("ERROR: --coord needs to be 32 or 64.\n");
			exit(1);
		}
		put("/local/pcb/coord_bits", value);
		want_coord_bits = v;
		return 1;
	}
	if (strcmp(key, "all") == 0) {
		if ((strcmp(value, sbuildin) == 0) || (strcmp(value, splugin) == 0) || (strcmp(value, sdisable) == 0)) {
			all_plugin_select(value);
			return 1;
		}
		report("Error: unknown --all argument: %s\n", value);
		exit(1);
	}
	if (strcmp(key, "coord") == 0)
		put("/local/pcb/dot_pcb_rnd", value);
	if (strncmp(key, "workaround-", 11) == 0) {
		const char *what = key+11;
		if (strcmp(what, "gtk-ctrl") == 0) append("/local/pcb/workaround_defs", "\n#define PCB_WORKAROUND_GTK_CTRL 1");
		else {
			report("ERROR: unknown workaround '%s'\n", what);
			exit(1);
		}
		return 1;
	}
	if ((strcmp(key, "with-intl") == 0) || (strcmp(key, "enable-intl") == 0)) {
		want_intl = 1;
		return 1;
	}
	else if (strcmp(key, "help") == 0) {
		help1();
		arg_auto_print_options(stdout, " ", "                         ", disable_libs);
		help2();
		exit(0);
	}

	return arg_auto_set(key, value, disable_libs);
}

/* execute plugin dependency statements, depending on "require":
	require = 0 - attempt to mark any dep as buildin
	require = 1 - check if dependencies are met, disable plugins that have
	              unmet deps
*/
void plugin_dep1(int require, const char *plugin, const char *deps_on)
{
	char buff[1024];
	const char *st_plugin, *st_deps_on;

	sprintf(buff, "/local/pcb/%s/controls", plugin);
	st_plugin = get(buff);
	sprintf(buff, "/local/pcb/%s/controls", deps_on);
	st_deps_on = get(buff);

	if (require) {
		if ((strcmp(st_plugin, sbuildin) == 0) || (strcmp(st_plugin, splugin) == 0)) {
			if (strcmp(st_deps_on, sbuildin) != 0) {
				sprintf(buff, "WARNING: disabling %s because the %s is not enabled as a buildin...\n", plugin, deps_on);
				report_repeat(buff);
				sprintf(buff, "disable-%s", plugin);
				hook_custom_arg(buff, NULL);
			}
		}
	}
	else {
		if ((strcmp(st_plugin, sbuildin) == 0) || (strcmp(st_plugin, splugin) == 0))
			put(buff, sbuildin);
	}
}

static void all_plugin_select(const char *state)
{
	char buff[1024];

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) \
	if (all_) { \
		sprintf(buff, "/local/pcb/%s/controls", name); \
		put(buff, state); \
	}
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"
}

void plugin_deps(int require)
{
#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_)
#define plugin_header(sect)
#define plugin_dep(plg, on) plugin_dep1(require, plg, on);
#include "plugins.h"
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
	put("/local/prefix", "/usr/local");

#undef plugin_def
#undef plugin_header
#undef plugin_dep
#define plugin_def(name, desc, default_, all_) plugin3_default(name, default_)
#define plugin_header(sect)
#define plugin_dep(plg, on)
#include "plugins.h"

	put("/local/pcb/want_bison", sfalse);
	put("/local/pcb/debug", sfalse);
	put("/local/pcb/symbols", sfalse);
	put("/local/pcb/coord_bits", "32");
	want_coord_bits = 32;
	put("/local/pcb/dot_pcb_rnd", ".pcb-rnd");

	return 0;
}

/* Runs after all arguments are read and parsed */
int hook_postarg()
{
	plugin_deps(0);
	return 0;
}



/* Runs when things should be detected for the host system */
int hook_detect_host()
{
	require("fstools/ar",  0, 1);
	require("fstools/mkdir", 0, 1);
	require("fstools/rm",  0, 1);
	require("fstools/cp",  0, 1);
	require("fstools/ln",  0, 1);
	require("fstools/mkdir",  0, 1);

/* until we rewrite the generators in C */
	require("fstools/awk",  0, 1);

	if (istrue(get("/local/pcb/debug")))
		require("cc/argstd/*", 0, 0);

	require("cc/func_attr/unused/*", 0, 0);
	require("cc/inline", 0, 0);

	return 0;
}

int safe_atoi(const char *s)
{
	if (s == NULL)
		return 0;
	return atoi(s);
}

/* Runs when things should be detected for the target system */
int hook_detect_target()
{
	int want_glib = 0, want_gtk, want_gd, want_stroke, need_inl = 0;

	want_gtk    = plug_is_enabled("hid_gtk");
	want_gd     = plug_is_enabled("export_png") ||  plug_is_enabled("export_nelma") ||  plug_is_enabled("export_gcode");
	want_stroke = plug_is_enabled("stroke");

	require("cc/fpic",  0, 1);
	require("signal/names/*",  0, 0);
	require("libs/fs/mkdtemp/*",  0, 0);
	require("libs/fs/realpath/*",  0, 0);
	require("libs/fs/readdir/*",  0, 1);
	require("libs/math/rint/*",  0, 0);
	require("libs/math/round/*",  0, 0);
	require("libs/userpass/getpwuid/*",  0, 0);

	if (require("libs/ldl",  0, 0) != 0) {
		if (require("libs/LoadLibrary",  0, 0) != 0) {
			report_repeat("\nERROR: no dynamic linking found on your system. Can not compile pcb-rnd.\n\n");
			return 1;
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

	if (want_intl) {
		require("libs/sul/gettext/presents", 0, 0);
		if (!istrue(get("libs/sul/gettext/presents"))) {
			report_repeat("\nERROR: intl support explicitly requested but gettext is not found on your system.\n\n");
			return 1;
		}
		put("/local/pcb/want_nls", strue);
	}
	else
		put("/local/pcb/want_nls", sfalse);

	if (want_stroke) {
		require("libs/gui/libstroke/presents", 0, 0);
		if (!istrue(get("libs/gui/libstroke/presents"))) {
			report_repeat("WARNING: Since there's no libstroke found, disabling the stroke plugin...\n");
			hook_custom_arg("disable-stroke", NULL);
		}
	}

	if (want_gtk) {
		require("libs/gui/gtk2/presents", 0, 0);
		if (!istrue(get("libs/gui/gtk2/presents"))) {
			report_repeat("WARNING: Since there's no libgtk2 found, disabling the gtk hid...\n");
			hook_custom_arg("disable-hid_gtk", NULL);
		}
	}

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

	if (plug_is_enabled("toporouter"))
		want_glib = 1;

	if (plug_is_enabled("export_dsn"))
		want_glib = 1;

	if (plug_is_enabled("puller"))
		want_glib = 1;

	if (want_glib) {
		require("libs/sul/glib", 0, 0);
		if (!istrue(get("libs/sul/glib/presents"))) {
			if (want_gtk) {
				report_repeat("WARNING: Since GLIB is not found, disabling the GTK HID...\n");
				hook_custom_arg("disable-gtk", NULL);
			}
			if (plug_is_enabled("toporouter")) {
				report_repeat("WARNING: Since GLIB is not found, disabling the toporouter...\n");
				hook_custom_arg("disable-toporouter", NULL);
			}
			if (plug_is_enabled("puller")) {
				report_repeat("WARNING: Since GLIB is not found, disabling the puller...\n");
				hook_custom_arg("disable-puller", NULL);
			}
			if (plug_is_enabled("export_dsn")) {
				report_repeat("WARNING: Since GLIB is not found, disabling the dsn exporter...\n");
				hook_custom_arg("disable-export_dsn", NULL);
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

	if (want_gd) {
		require("libs/gui/gd/presents", 0, 0);
		if (!istrue(get("libs/gui/gd/presents"))) {
			report_repeat("WARNING: Since there's no libgd, disabling gd based exports (png, nelma, gcode)...\n");
			hook_custom_arg("disable-gd-gif", NULL);
			hook_custom_arg("disable-gd-png", NULL);
			hook_custom_arg("disable-gd-jpg", NULL);
			hook_custom_arg("disable-export_png", NULL);
			hook_custom_arg("disable-export_nelma", NULL);
			hook_custom_arg("disable-export_gcode", NULL);
			want_gd = 0;
			goto disable_gd_formats;
		}
		else {
			require("libs/gui/gd/gdImagePng/presents", 0, 0);
			require("libs/gui/gd/gdImageGif/presents", 0, 0);
			require("libs/gui/gd/gdImageJpeg/presents", 0, 0);
			if (!istrue(get("libs/gui/gd/gdImagePng/presents"))) {
				report_repeat("WARNING: libgd is installed, but its png code fails, some exporters will be compiled with reduced functionality; exporters affected: export_nelma, export_gcode\n");
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

	/* generic utils for Makefiles */
	require("sys/ext_exe", 0, 1);
	require("sys/sysid", 0, 1);

	/* options for config.h */
	require("sys/path_sep", 0, 1);
	require("sys/types/size/*", 0, 1);
	require("cc/rdynamic", 0, 0);
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


	if (get("cc/rdynamic") == NULL)
		put("cc/rdynamic", "");

	{
		const char *tmp, *fpic, *debug;
		fpic = get("/target/cc/fpic");
		if (fpic == NULL) fpic = "";
		debug = get("/arg/debug");
		if (debug == NULL) debug = "";
		tmp = str_concat(" ", fpic, debug, NULL);
		put("/local/global_cflags", tmp);
	}


	/* plugin dependencies */
	if (plug_is_enabled("dbus")) {
		require("libs/sul/dbus/presents", 0, 0);
		if (!istrue(get("libs/sul/dbus/presents"))) {
			report_repeat("WARNING: disabling the DBUS interface plugin because libdbus is not found or not configured...\n");
			hook_custom_arg("disable-dbus", NULL);
		}
	}

	plugin_deps(1);

	if (plug_is_enabled("gpmi")) {
		require("libs/script/gpmi/presents", 0, 0);
		if (!istrue(get("libs/script/gpmi/presents"))) {
			report_repeat("WARNING: disabling the gpmi scripting because libgpmi is not found or not configured...\n");
			hook_custom_arg("disable-gpmi", NULL);
		}
	}

	/* figure coordinate bits */
	{
		int int_bits       = safe_atoi(get("sys/types/size/signed_int")) * 8;
		int long_bits      = safe_atoi(get("sys/types/size/signed_long_int")) * 8;
		int long_long_bits = safe_atoi(get("sys/types/size/signed_long_long_int")) * 8;
		int int64_bits     = safe_atoi(get("sys/types/size/uint64_t")) * 8;
		const char *chosen, *abs_name, *postfix;
		char tmp[64];
		int need_stdint = 0;

		if (want_coord_bits == int_bits)             { postfix="U";   chosen = "int";           abs_name="abs"; }
		else if (want_coord_bits == long_bits)       { postfix="UL";  chosen = "long int";      abs_name="labs"; }
		else if (want_coord_bits == int64_bits)      { postfix="ULL"; chosen = "int64_t";       abs_name="llabs"; need_stdint = 1; }
		else if (want_coord_bits == long_long_bits)  { postfix="ULL"; chosen = "long long int"; abs_name="llabs"; }
		else {
			report("ERROR: can't find a suitable integer type for coord to be %d bits wide\n", want_coord_bits);
			exit(1);
		}

		sprintf(tmp, "((1%s<<%d)-1)", postfix, want_coord_bits - 1);
		put("/local/pcb/coord_type", chosen);
		put("/local/pcb/coord_max", tmp);
		put("/local/pcb/coord_abs", abs_name);

		chosen = NULL;
		if (istrue(get("/local/pcb/debug"))) { /* debug: c89 */
			if (int64_bits >= 64) {
				/* to suppress warnings on systems that support c99 but are forced to compile in c89 mode */
				chosen = "int64_t";
				need_stdint = 1;
			}
		}

		if (chosen == NULL) { /* non-debug, thus non-c89 */
			if (long_long_bits >= 64) chosen = "long long int";
			else if (long_bits >= 64) chosen = "long int";
			else chosen = "double";
		}
		put("/local/pcb/long64", chosen);
		if (need_stdint)
			put("/local/pcb/include_stdint", "#include <stdint.h>");
	}

	/* set cflags for C89 */
	put("/local/pcb/c89flags", "");
	if (istrue(get("/local/pcb/debug"))) {
		const char *ansi = get("/host/cc/argstd/ansi");
		const char *ped = get("/host/cc/argstd/pedantic");

		if ((ansi != NULL) && (*ansi != '\0')) {
			append("/local/pcb/c89flags", " ");
			append("/local/pcb/c89flags", ansi);
			need_inl = 1;
		}
		if ((ped != NULL) && (*ped != '\0')) {
			append("/local/pcb/c89flags", " ");
			append("/local/pcb/c89flags", ped);
			need_inl = 1;
		}
	}

	if (!istrue(get("cc/inline")))
		need_inl = 1;

	if (need_inl) {
		/* disable inline for C89 */
		append("/local/pcb/c89flags", " ");
		append("/local/pcb/c89flags", "-Dinline= ");
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
static void plugin_stat(const char *header, const char *path, const char *name)
{
	const char *val = get(path);

	if (*header == '#') /* don't print hidden plugins */
		return;

	printf(" %-32s", header);

	if (val == NULL)
		printf("??? (NULL)   ");
	else if (strcmp(val, sbuildin) == 0)
		printf("yes, buildin ");
	else if (strcmp(val, splugin) == 0)
		printf("yes, PLUGIN  ");
	else
		printf("no           ");

	printf("   [%s]\n", name);
}

static void print_sum_setting(const char *node, const char *desc)
{
	const char *res, *state;
	state = get(node);
	if (istrue(state))
		res = "enabled";
	else if (isfalse(state))
		res = "disabled";
	else
		res = "UNKNOWN - disabled?";
	printf("%-55s %s\n", desc, res);
}

static void print_sum_cfg_val(const char *node, const char *desc)
{
	const char *state = get(node);
	printf("%-55s %s\n", desc, state);
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
	printf("Generating pcb/buildin  (%d)\n", generr |= tmpasm("../src", "buildin.c.in", "buildin.c"));

	printf("Generating util/gsch2pcb-rnd/Makefile (%d)\n", generr |= tmpasm("../util", "gsch2pcb-rnd/Makefile.in", "gsch2pcb-rnd/Makefile"));

	printf("Generating config.h (%d)\n", generr |= tmpasm("..", "config.h.in", "config.h"));

	printf("Generating compat_inc.h (%d)\n", generr |= tmpasm("../src", "compat_inc.h.in", "compat_inc.h"));

	if (plug_is_enabled("gpmi"))
		gpmi_config();

	if (!generr) {
	printf("\n\n");
	printf("=====================\n");
	printf("Configuration summary\n");
	printf("=====================\n");

	print_sum_setting("/local/pcb/want_parsgen",   "Regenerating languages with bison & flex");
	print_sum_setting("/local/pcb/want_nls",       "Internationalization with gettext");
	print_sum_setting("/local/pcb/debug",          "Compilation for debugging");
	print_sum_setting("/local/pcb/symbols",        "Include debug symbols");
	print_sum_setting("libs/sul/dmalloc/presents", "Compile with dmalloc");
	print_sum_cfg_val("/local/pcb/coord_bits",     "Coordinate type bits");
	print_sum_cfg_val("/local/pcb/dot_pcb_rnd",    ".pcb_rnd config dir under $HOME");

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

	{
		FILE *f;
		f = fopen("Rev.stamp", "w");
		fprintf(f, "%d", myrev);
		fclose(f);
	}

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

