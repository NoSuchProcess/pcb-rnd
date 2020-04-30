#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "data.h"
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include <librnd/core/hid.h>
#include "../export_ps/ps.h"
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>

const char *lpr_cookie = "lpr HID";

static rnd_export_opt_t base_lpr_options[] = {

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
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_lprcommand 0
};

#define NUM_OPTIONS (sizeof(lpr_options)/sizeof(lpr_options[0]))

static rnd_export_opt_t *lpr_options = 0;
static int num_lpr_options = 0;
static rnd_hid_attr_val_t *lpr_values;

static rnd_export_opt_t *lpr_get_export_options(rnd_hid_t *hid, int *n)
{
	/*
	 * We initialize the default value in this manner because the GUI
	 * HID's may want to free() this string value and replace it with a
	 * new one based on how a user fills out a print dialog.
	 */
	if (base_lpr_options[HA_lprcommand].default_val.str == NULL) {
		base_lpr_options[HA_lprcommand].default_val.str = rnd_strdup("lpr");
	}

	if (lpr_options == 0) {
		rnd_export_opt_t *ps_opts = ps_hid.get_export_options(&ps_hid, &num_lpr_options);
		lpr_options = calloc(num_lpr_options, sizeof(rnd_hid_attribute_t));
		memcpy(lpr_options, ps_opts, num_lpr_options * sizeof(rnd_hid_attribute_t));
		memcpy(lpr_options, base_lpr_options, sizeof(base_lpr_options));
		lpr_values = (rnd_hid_attr_val_t *) calloc(num_lpr_options, sizeof(rnd_hid_attr_val_t));
	}
	if (n)
		*n = num_lpr_options;
	return lpr_options;
}

static void lpr_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	FILE *f;
	int i;
	const char *filename;

	if (!options) {
		lpr_get_export_options(hid, 0);
		for (i = 0; i < num_lpr_options; i++)
			lpr_values[i] = lpr_options[i].default_val;
		options = lpr_values;
	}

	filename = options[HA_lprcommand].str;

	printf("LPR: open %s\n", filename);
	f = pcb_popen(NULL, filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	ps_hid_export_to_file(f, options, NULL);

	pcb_pclose(f);
}

static int lpr_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	lpr_get_export_options(hid, 0);
	rnd_export_register_opts(lpr_options, num_lpr_options, lpr_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static void lpr_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	ps_calibrate_1(hid, xval, yval, 1);
}

static rnd_hid_t lpr_hid;

static int lpr_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nlpr exporter command line arguments:\n\n");
	rnd_hid_usage(base_lpr_options, sizeof(base_lpr_options) / sizeof(base_lpr_options[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x lpr [lpr options] foo.pcb\n\n");
	return 0;
}

int pplg_check_ver_export_lpr(int ver_needed) { return 0; }

void pplg_uninit_export_lpr(void)
{
	rnd_remove_actions_by_cookie(lpr_cookie);
}

int pplg_init_export_lpr(void)
{
	RND_API_CHK_VER;
	memset(&lpr_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&lpr_hid);
	ps_ps_init(&lpr_hid);

	lpr_hid.struct_size = sizeof(rnd_hid_t);
	lpr_hid.name = "lpr";
	lpr_hid.description = "Postscript print";
	lpr_hid.printer = 1;

	lpr_hid.get_export_options = lpr_get_export_options;
	lpr_hid.do_export = lpr_do_export;
	lpr_hid.parse_arguments = lpr_parse_arguments;
	lpr_hid.calibrate = lpr_calibrate;

	lpr_hid.usage = lpr_usage;

	rnd_hid_register_hid(&lpr_hid);

	return 0;
}
