/* for popen() */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "data.h"
#include "plugins.h"
#include "compat_misc.h"

#include "hid.h"
#include "../export_ps/ps.h"
#include "hid_nogui.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_actions.h"

const char *lpr_cookie = "lpr HID";

static HID_Attribute base_lpr_options[] = {

/* %start-doc options "98 lpr Printing Options"
@ftable @code
@item --lprcommand <string>
Command to use for printing. Defaults to @code{lpr}. This can be used to produce
PDF output with a virtual PDF printer. Example: @*
@code{--lprcommand "lp -d CUPS-PDF-Printer"}.
@end ftable
@noindent In addition, all @ref{Postscript Export} options are valid.
%end-doc
*/
	{"lprcommand", "Command to use for printing",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_lprcommand 0
};

#define NUM_OPTIONS (sizeof(lpr_options)/sizeof(lpr_options[0]))

static HID_Attribute *lpr_options = 0;
static int num_lpr_options = 0;
static HID_Attr_Val *lpr_values;

static HID_Attribute *lpr_get_export_options(int *n)
{
	/*
	 * We initialize the default value in this manner because the GUI
	 * HID's may want to free() this string value and replace it with a
	 * new one based on how a user fills out a print dialog.
	 */
	if (base_lpr_options[HA_lprcommand].default_val.str_value == NULL) {
		base_lpr_options[HA_lprcommand].default_val.str_value = pcb_strdup("lpr");
	}

	if (lpr_options == 0) {
		HID_Attribute *ps_opts = ps_hid.get_export_options(&num_lpr_options);
		lpr_options = (HID_Attribute *) calloc(num_lpr_options, sizeof(HID_Attribute));
		memcpy(lpr_options, ps_opts, num_lpr_options * sizeof(HID_Attribute));
		memcpy(lpr_options, base_lpr_options, sizeof(base_lpr_options));
		lpr_values = (HID_Attr_Val *) calloc(num_lpr_options, sizeof(HID_Attr_Val));
	}
	if (n)
		*n = num_lpr_options;
	return lpr_options;
}

static void lpr_do_export(HID_Attr_Val * options)
{
	FILE *f;
	int i;
	const char *filename;

	if (!options) {
		lpr_get_export_options(0);
		for (i = 0; i < num_lpr_options; i++)
			lpr_values[i] = lpr_options[i].default_val;
		options = lpr_values;
	}

	filename = options[HA_lprcommand].str_value;

	printf("LPR: open %s\n", filename);
	f = popen(filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	ps_hid_export_to_file(f, options);

	fclose(f);
}

static void lpr_parse_arguments(int *argc, char ***argv)
{
	lpr_get_export_options(0);
	hid_register_attributes(lpr_options, num_lpr_options, lpr_cookie, 0);
	hid_parse_command_line(argc, argv);
}

static void lpr_calibrate(double xval, double yval)
{
	ps_calibrate_1(xval, yval, 1);
}

static HID lpr_hid;

static int lpr_usage(const char *topic)
{
	fprintf(stderr, "\nlpr exporter command line arguments:\n\n");
	hid_usage(base_lpr_options, sizeof(base_lpr_options) / sizeof(base_lpr_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x lpr foo.pcb [lpr options]\n\n");
	return 0;
}

static void plugin_lpr_uninit(void)
{
	hid_remove_actions_by_cookie(lpr_cookie);
}

pcb_uninit_t hid_export_lpr_init()
{
	memset(&lpr_hid, 0, sizeof(HID));

	common_nogui_init(&lpr_hid);
	ps_ps_init(&lpr_hid);

	lpr_hid.struct_size = sizeof(HID);
	lpr_hid.name = "lpr";
	lpr_hid.description = "Postscript print";
	lpr_hid.printer = 1;
	lpr_hid.poly_before = 1;

	lpr_hid.get_export_options = lpr_get_export_options;
	lpr_hid.do_export = lpr_do_export;
	lpr_hid.parse_arguments = lpr_parse_arguments;
	lpr_hid.calibrate = lpr_calibrate;

	lpr_hid.usage = lpr_usage;

	hid_register_hid(&lpr_hid);

	return plugin_lpr_uninit;
}
