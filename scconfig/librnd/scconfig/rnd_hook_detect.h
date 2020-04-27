extern int want_coord_bits;

	/* figure coordinate bits */
static void rnd_hook_detect_coord_bits(void)
{
	int int_bits       = safe_atoi(get("sys/types/size/signed_int")) * 8;
	int long_bits      = safe_atoi(get("sys/types/size/signed_long_int")) * 8;
	int long_long_bits = safe_atoi(get("sys/types/size/signed_long_long_int")) * 8;
	int int64_bits     = safe_atoi(get("sys/types/size/uint64_t")) * 8;
	const char *chosen, *postfix;
	char tmp[64];
	int need_stdint = 0;

	if (want_coord_bits == int_bits)             { postfix="U";   chosen = "int";           }
	else if (want_coord_bits == long_bits)       { postfix="UL";  chosen = "long int";      }
	else if (want_coord_bits == int64_bits)      { postfix="ULL"; chosen = "int64_t";       need_stdint = 1; }
	else if (want_coord_bits == long_long_bits)  { postfix="ULL"; chosen = "long long int"; }
	else {
		report("ERROR: can't find a suitable integer type for coord to be %d bits wide\n", want_coord_bits);
		exit(1);
	}

	sprintf(tmp, "((1%s<<%d)-1)", postfix, want_coord_bits - 1);
	put("/local/pcb/coord_type", chosen);
	put("/local/pcb/coord_max", tmp);

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

static void rnd_hook_detect_cc(void)
{
	int need_inl = 0;
	const char *host_ansi, *host_ped, *target_ansi, *target_ped, *target_pg, *target_no_pie;
	const char *tmp, *fpic, *debug;

	require("cc/fpic",  0, 0);
	host_ansi = get("/host/cc/argstd/ansi");
	host_ped = get("/host/cc/argstd/pedantic");
	target_ansi = get("/target/cc/argstd/ansi");
	target_ped = get("/target/cc/argstd/pedantic");
	target_pg = get("/target/cc/argstd/pg");
	target_no_pie = get("/target/cc/argstd/no-pie");
	require("cc/pragma_message",  0, 0);
	require("fstools/ar",  0, 1);

	/* need to set debug flags here to make sure libs are detected with the modified cflags; -ansi matters in what #defines we need for some #includes */
	fpic = get("/target/cc/fpic");
	if (fpic == NULL) fpic = "";
	debug = get("/arg/debug");
	if (debug == NULL) debug = "";
	tmp = str_concat(" ", fpic, debug, NULL);
	put("/local/global_cflags", tmp);

	/* for --debug mode, use -ansi -pedantic for all detection */
	put("/local/cc_flags_save", get("/target/cc/cflags"));
	if (istrue(get("/local/pcb/debug"))) {
		append("/target/cc/cflags", " ");
		append("/target/cc/cflags", target_ansi);
		append("/target/cc/cflags", " ");
		append("/target/cc/cflags", target_ped);
	}
	if (istrue(get("/local/pcb/profile"))) {
		append("/target/cc/cflags", " ");
		append("/target/cc/cflags", target_pg);
		append("/target/cc/cflags", " ");
		append("/target/cc/cflags", target_no_pie);
	}

	/* set cflags for C89 */
	put("/local/pcb/c89flags", "");
	if (istrue(get("/local/pcb/debug"))) {

		if ((target_ansi != NULL) && (*target_ansi != '\0')) {
			append("/local/pcb/c89flags", " ");
			append("/local/pcb/c89flags", target_ansi);
			need_inl = 1;
		}
		if ((target_ped != NULL) && (*target_ped != '\0')) {
			append("/local/pcb/c89flags", " ");
			append("/local/pcb/c89flags", target_ped);
			need_inl = 1;
		}
	}

	if (istrue(get("/local/pcb/profile"))) {
		append("/local/pcb/cflags_profile", " ");
		append("/local/pcb/cflags_profile", target_pg);
		append("/local/pcb/cflags_profile", " ");
		append("/local/pcb/cflags_profile", target_no_pie);
	}

	if (!istrue(get("cc/inline")))
		need_inl = 1;

	if (need_inl) {
		/* disable inline for C89 */
		append("/local/pcb/c89flags", " ");
		append("/local/pcb/c89flags", "-Dinline= ");
	}
}

static int rnd_hook_detect_sys(void)
{
	int can_live_without_dynlib = 0; /* special optional exception for windows cross compilation for now */

	if (istrue(get("/target/pcb/can_live_without_dynlib")))
		can_live_without_dynlib = 1;

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

	if (require("libs/fs/getcwd/*",  0, 0) != 0) {
		if (require("libs/fs/_getcwd/*",  0, 0) != 0) {
			if (require("libs/fs/getwd/*",  0, 0) != 0) {
				report_repeat("\nERROR: Can not find any getcwd() variant.\n\n");
				return 1;
			}
		}
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

	if (get("cc/rdynamic") == NULL)
		put("cc/rdynamic", "");

	return 0;
}


static void rnd_hook_detect_hid(int want_glib)
{
	int need_gtklibs = 0, want_gtk, want_gtk2, has_gtk2 = 0, want_gl;

	want_gtk2   = plug_is_enabled("hid_gtk2_gdk") || plug_is_enabled("hid_gtk2_gl");
	want_gtk    = want_gtk2; /* plus |gtkN */

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

}


static int rnd_hook_detect_host(void)
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


