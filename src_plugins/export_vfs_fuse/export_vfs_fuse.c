#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"

#include "hid.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid_nogui.h"

#include "../src_plugins/lib_vfs/lib_vfs.h"

const char *export_vfs_fuse_cookie = "export_vfs_fuse HID";

static pcb_hid_attribute_t export_vfs_fuse_options[] = {
	{"cmd", "mc command",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_export_vfs_fuse_cmd 0
};

#define NUM_OPTIONS (sizeof(export_vfs_fuse_options)/sizeof(export_vfs_fuse_options[0]))

PCB_REGISTER_ATTRIBUTES(export_vfs_fuse_options, export_vfs_fuse_cookie)

static pcb_hid_attr_val_t export_vfs_fuse_values[NUM_OPTIONS];

static pcb_hid_attribute_t *export_vfs_fuse_get_export_options(int *n)
{
	if (n)
		*n = NUM_OPTIONS;
	return export_vfs_fuse_options;
}

static void export_vfs_fuse_do_export(pcb_hid_attr_val_t * options)
{
	const char *cmd;
	int i;

	if (!options) {
		export_vfs_fuse_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			export_vfs_fuse_values[i] = export_vfs_fuse_options[i].default_val;
		options = export_vfs_fuse_values;
	}

	cmd = options[HA_export_vfs_fuse_cmd].str_value;
	TODO("Process cmd here");
}

static int export_vfs_fuse_usage(const char *topic)
{
	fprintf(stderr, "\nexport_vfs_fuse exporter command line arguments:\n\n");
	pcb_hid_usage(export_vfs_fuse_options, sizeof(export_vfs_fuse_options) / sizeof(export_vfs_fuse_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x export_vfs_fuse [export_vfs_fuse_options] foo.pcb\n\n");
	return 0;
}


static int export_vfs_fuse_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(export_vfs_fuse_options, sizeof(export_vfs_fuse_options) / sizeof(export_vfs_fuse_options[0]), export_vfs_fuse_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t export_vfs_fuse_hid;

int pplg_check_ver_export_vfs_fuse(int ver_needed) { return 0; }

void pplg_uninit_export_vfs_fuse(void)
{
	pcb_hid_remove_attributes_by_cookie(export_vfs_fuse_cookie);
}

int pplg_init_export_vfs_fuse(void)
{
	PCB_API_CHK_VER;

	memset(&export_vfs_fuse_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&export_vfs_fuse_hid);

	export_vfs_fuse_hid.struct_size = sizeof(pcb_hid_t);
	export_vfs_fuse_hid.name = "vfs_fuse";
	export_vfs_fuse_hid.description = "Handler of FUSE calls, serving board data";
	export_vfs_fuse_hid.exporter = 1;
	export_vfs_fuse_hid.hide_from_gui = 1;

	export_vfs_fuse_hid.get_export_options = export_vfs_fuse_get_export_options;
	export_vfs_fuse_hid.do_export = export_vfs_fuse_do_export;
	export_vfs_fuse_hid.parse_arguments = export_vfs_fuse_parse_arguments;

	export_vfs_fuse_hid.usage = export_vfs_fuse_usage;

	pcb_hid_register_hid(&export_vfs_fuse_hid);
	return 0;
}
