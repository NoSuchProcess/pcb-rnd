 /*
  *                            COPYRIGHT
  *
  *  PCB, interactive printed circuit board design
  *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
  *
  *  This program is free software; you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation; either version 2 of the License, or
  *  (at your option) any later version.
  *
  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program; if not, write to the Free Software
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
  */

/*
 *  Based on the png exporter by Dan McMahill
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "plugins.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "plug_io.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_helper.h"
#include "hid_flags.h"


static pcb_hid_t fidocadj_hid;

const char *fidocadj_cookie = "fidocadj HID";

pcb_hid_attribute_t fidocadj_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Output file name",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0},
#define HA_fidocadjfile 0
};

#define NUM_OPTIONS (sizeof(fidocadj_attribute_list)/sizeof(fidocadj_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(fidocadj_attribute_list, fidocadj_cookie)

static pcb_hid_attr_val_t fidocadj_values[NUM_OPTIONS];

static pcb_hid_attribute_t *fidocadj_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".fcd";

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &fidocadj_attribute_list[HA_fidocadjfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return fidocadj_attribute_list;
}

static void fidocadj_do_export(pcb_hid_attr_val_t * options)
{
	FILE *f;
	const char *filename;
	int n;

	if (!options) {
		fidocadj_get_export_options(0);
		for (n = 0; n < NUM_OPTIONS; n++)
			fidocadj_values[n] = fidocadj_attribute_list[n].default_val;
		options = fidocadj_values;
	}

	filename = options[HA_fidocadjfile].str_value;
	if (!filename)
		filename = "pcb-rnd-default.fcd";

	f = fopen(filename, "w");
	if (!f) {
		perror(filename);
		return;
	}

	fprintf(f, "[FIDOCAD]\n");

	fclose(f);
}

static void fidocadj_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(fidocadj_attribute_list, sizeof(fidocadj_attribute_list) / sizeof(fidocadj_attribute_list[0]), fidocadj_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int fidocadj_usage(const char *topic)
{
	fprintf(stderr, "\nfidocadj exporter command line arguments:\n\n");
	pcb_hid_usage(fidocadj_attribute_list, sizeof(fidocadj_attribute_list) / sizeof(fidocadj_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x fidocadj foo.pcb [fidocadj options]\n\n");
	return 0;
}

#include "dolists.h"

pcb_uninit_t hid_export_fidocadj_init()
{
	memset(&fidocadj_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&fidocadj_hid);
	pcb_dhlp_draw_helpers_init(&fidocadj_hid);

	fidocadj_hid.struct_size = sizeof(pcb_hid_t);
	fidocadj_hid.name = "fidocadj";
	fidocadj_hid.description = "export board in FidoCadJ .fcd format";
	fidocadj_hid.exporter = 1;

	fidocadj_hid.get_export_options = fidocadj_get_export_options;
	fidocadj_hid.do_export = fidocadj_do_export;
	fidocadj_hid.parse_arguments = fidocadj_parse_arguments;

	fidocadj_hid.usage = fidocadj_usage;

	pcb_hid_register_hid(&fidocadj_hid);

	return NULL;
}
