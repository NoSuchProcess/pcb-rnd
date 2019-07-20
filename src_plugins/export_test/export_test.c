#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "build_run.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_attrib.h"
#include "hid_cam.h"
#include "hid_init.h"

const char *export_test_cookie = "export_test HID";

static pcb_hid_attribute_t export_test_options[] = {
/* %start-doc options "8 export_test Creation"
@ftable @code
@item --export_testfile <string>
Name of the export_test output file. Use stdout if not specified.
@end ftable
%end-doc
*/
	{"export_testfile", "Name of the export_test output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_export_testfile 0
};

#define NUM_OPTIONS (sizeof(export_test_options)/sizeof(export_test_options[0]))

static pcb_hid_attr_val_t export_test_values[NUM_OPTIONS];

static const char *export_test_filename;

static pcb_hid_attribute_t *export_test_get_export_options(pcb_hid_t *hid, int *n)
{
	if ((PCB != NULL)  && (export_test_options[HA_export_testfile].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &export_test_options[HA_export_testfile], ".export_test");

	if (n)
		*n = NUM_OPTIONS;
	return export_test_options;
}

static int Printexport_test(void)
{
	return 0;
}

static void export_test_do_export(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_hid_attr_val_t *options)
{
	int i;

	if (!options) {
		export_test_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			export_test_values[i] = export_test_options[i].default_val;
		options = export_test_values;
	}

	export_test_filename = options[HA_export_testfile].str_value;
	if (!export_test_filename)
		export_test_filename = "pcb-out.export_test";
	else {
TODO(": set some FILE *fp to stdout")
	}

	Printexport_test();
}

static int export_test_usage(const char *topic)
{
	fprintf(stderr, "\nexport_test exporter command line arguments:\n\n");
	pcb_hid_usage(export_test_options, sizeof(export_test_options) / sizeof(export_test_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x export_test [export_test_options] foo.pcb\n\n");
	return 0;
}


static int export_test_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_hid_register_attributes(export_test_options, sizeof(export_test_options) / sizeof(export_test_options[0]), export_test_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}

pcb_hid_t export_test_hid;

int pplg_check_ver_export_test(int ver_needed) { return 0; }

void pplg_uninit_export_test(void)
{
}

int pplg_init_export_test(void)
{
	PCB_API_CHK_VER;

	memset(&export_test_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&export_test_hid);

	export_test_hid.struct_size = sizeof(pcb_hid_t);
	export_test_hid.name = "export_test";
	export_test_hid.description = "Exports a dump of HID calls";
	export_test_hid.exporter = 1;
	export_test_hid.hide_from_gui = 1;

	export_test_hid.get_export_options = export_test_get_export_options;
	export_test_hid.do_export = export_test_do_export;
	export_test_hid.parse_arguments = export_test_parse_arguments;

	export_test_hid.usage = export_test_usage;

	pcb_hid_register_hid(&export_test_hid);
	return 0;
}
