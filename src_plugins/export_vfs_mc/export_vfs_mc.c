#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include "plug_io.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_nogui.h>

#include "../src_plugins/lib_vfs/lib_vfs.h"

const char *export_vfs_mc_cookie = "export_vfs_mc HID";

static rnd_export_opt_t export_vfs_mc_options[] = {
	{"cmd", "mc command",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_export_vfs_mc_cmd 0

	{"qpath", "query path",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_export_vfs_mc_qpath 1
};

#define NUM_OPTIONS (sizeof(export_vfs_mc_options)/sizeof(export_vfs_mc_options[0]))

static rnd_hid_attr_val_t export_vfs_mc_values[NUM_OPTIONS];

static rnd_export_opt_t *export_vfs_mc_get_export_options(rnd_hid_t *hid, int *n)
{
	if (n)
		*n = NUM_OPTIONS;
	return export_vfs_mc_options;
}

static void mc_list_cb(void *ctx, const char *path, int isdir)
{
	const char *attrs = isdir ? "drwxr-xr-x" : "-rw-r--r--";
	printf("%s 1 pcb-rnd pcb-rnd 0 01/01/01 01:01 %s\n", attrs, path);
}

static void mc_list(void)
{
	pcb_vfs_list(PCB, mc_list_cb, NULL);
}

static void mc_copyout(const char *path)
{
	gds_t res;

	gds_init(&res);
	if (pcb_vfs_access(PCB, path, &res, 0, NULL) == 0) {
		printf("%s", res.array);
	}
	else {
		fprintf(stderr, "Can not access that field for read\n");
		exit(1);
	}
	gds_uninit(&res);
}

static void mc_copyin(const char *path)
{
	gds_t inp;
	char buf[256];
	int len;

	gds_init(&inp);
	while((len = read(0, buf, 256)) > 0) {
		gds_append_len(&inp, buf, len);
		if (inp.used > 1024*1024) {
			fprintf(stderr, "Data too large\n");
			exit(1);
		}
	}

	if ((inp.used > 0) && (inp.array[inp.used-1] == '\n')) {
		inp.used--; /* remove exactly one trailing newline */
		inp.array[inp.used] = '\0';
	}

	if (pcb_vfs_access(PCB, path, &inp, 1, NULL) != 0) {
		fprintf(stderr, "Can not access that field for write\n");
		exit(1);
	}
	gds_uninit(&inp);
	if (pcb_save_pcb(PCB->hidlib.filename, NULL) != 0) {
		fprintf(stderr, "Failed to save the modified board file\n");
		exit(1);
	}
}

static void export_vfs_mc_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *cmd;

	if (!options) {
		export_vfs_mc_get_export_options(hid, 0);
		options = export_vfs_mc_values;
	}

	cmd = options[HA_export_vfs_mc_cmd].str;
	if (strcmp(cmd, "list") == 0) mc_list();
	else if (strcmp(cmd, "copyout") == 0) mc_copyout(options[HA_export_vfs_mc_qpath].str);
	else if (strcmp(cmd, "copyin") == 0) mc_copyin(options[HA_export_vfs_mc_qpath].str);
	else {
		fprintf(stderr, "Unknown vfs_mc command: '%s'\n", cmd);
		exit(1);
	}
}

static int export_vfs_mc_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nexport_vfs_mc exporter command line arguments:\n\n");
	rnd_hid_usage(export_vfs_mc_options, sizeof(export_vfs_mc_options) / sizeof(export_vfs_mc_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x export_vfs_mc [export_vfs_mc_options] foo.pcb\n\n");
	return 0;
}


static int export_vfs_mc_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, export_vfs_mc_options, sizeof(export_vfs_mc_options) / sizeof(export_vfs_mc_options[0]), export_vfs_mc_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

rnd_hid_t export_vfs_mc_hid;

int pplg_check_ver_export_vfs_mc(int ver_needed) { return 0; }

void pplg_uninit_export_vfs_mc(void)
{
	rnd_export_remove_opts_by_cookie(export_vfs_mc_cookie);
	rnd_hid_remove_hid(&export_vfs_mc_hid);
}

int pplg_init_export_vfs_mc(void)
{
	RND_API_CHK_VER;

	memset(&export_vfs_mc_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&export_vfs_mc_hid);

	export_vfs_mc_hid.struct_size = sizeof(rnd_hid_t);
	export_vfs_mc_hid.name = "vfs_mc";
	export_vfs_mc_hid.description = "Handler of mc VFS calls, serving board data";
	export_vfs_mc_hid.exporter = 1;
	export_vfs_mc_hid.hide_from_gui = 1;

	export_vfs_mc_hid.get_export_options = export_vfs_mc_get_export_options;
	export_vfs_mc_hid.do_export = export_vfs_mc_do_export;
	export_vfs_mc_hid.parse_arguments = export_vfs_mc_parse_arguments;
	export_vfs_mc_hid.argument_array = export_vfs_mc_values;

	export_vfs_mc_hid.usage = export_vfs_mc_usage;

	rnd_hid_register_hid(&export_vfs_mc_hid);
	return 0;
}
