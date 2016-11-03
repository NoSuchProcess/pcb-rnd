/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "hid_init.h"
#include "hid_attrib.h"

static const char *loghid_cookie = "loghid plugin";

HID_Attribute loghid_attribute_list[] = {
	{"target-hid", "the real GUI or export HID to relay calls to",
	 HID_String, 0, 0, {0, 0, 0}, 0, 0}
#define HA_target_hid 0
};
#define NUM_OPTIONS sizeof(loghid_attribute_list) / sizeof(loghid_attribute_list[0])

static void loghid_parse_arguments_real(int *argc, char ***argv, int is_gui)
{
	HID *target;
	const char *target_name;

	hid_register_attributes(loghid_attribute_list, NUM_OPTIONS, loghid_cookie, 0);
	hid_parse_command_line(argc, argv);

	target_name = loghid_attribute_list[HA_target_hid].default_val.str_value;

	if (is_gui)
		target = hid_find_gui(target_name);
	else
		target = hid_find_exporter(target_name);

#warning TODO:
	fprintf(stderr, "Initialize for delegatee: '%s' -> %p\n", target_name, (void *)target);
	exit(1); /* to avoid a segfault due to uninitialized hid fields now */
}

static void loghid_parse_arguments_gui(int *argc, char ***argv)
{
	loghid_parse_arguments_real(argc, argv, 1);
}

static void loghid_parse_arguments_exp(int *argc, char ***argv)
{
	loghid_parse_arguments_real(argc, argv, 0);
}


static int loghid_usage(const char *topic)
{
	fprintf(stderr, "\nhidlog command line arguments:\n\n");
	hid_usage(loghid_attribute_list, NUM_OPTIONS);
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: pcb-rnd [generic_options] --gui hidlog-gui --target-hid gtk foo.pcb\n");
	fprintf(stderr, "Usage: pcb-rnd [generic_options] --x hidlog-exp --target-hid png foo.pcb\n");
	fprintf(stderr, "\n");
	return 0;
}

REGISTER_ATTRIBUTES(loghid_attribute_list, loghid_cookie)

static HID_Attribute *loghid_get_export_options(int *n)
{
/*	loghid_attribute_list[HA_psfile] = pcb_strdup("default?");*/

	if (n)
		*n = (sizeof(loghid_attribute_list)/sizeof(loghid_attribute_list[0]));
	return loghid_attribute_list;
}


#include "dolists.h"

static void hid_loghid_uninit(void)
{
}

static HID loghid_gui;
static HID loghid_exp;

pcb_uninit_t hid_loghid_init(void)
{
	memset(&loghid_gui, 0, sizeof(HID));
	memset(&loghid_exp, 0, sizeof(HID));

	/* gui version */
	loghid_gui.struct_size = sizeof(HID);
	loghid_gui.name = "loghid-gui";
	loghid_gui.description = "log GUI HID calls";
	loghid_gui.gui = 1;

	loghid_gui.usage = loghid_usage;
	loghid_gui.parse_arguments = loghid_parse_arguments_gui;

	hid_register_hid(&loghid_gui);

	/* export version */
	loghid_exp.struct_size = sizeof(HID);
	loghid_exp.name = "loghid-exp";
	loghid_exp.description = "log export HID calls";
	loghid_exp.exporter = 1;

	loghid_exp.usage = loghid_usage;
	loghid_exp.parse_arguments = loghid_parse_arguments_exp;

	hid_register_hid(&loghid_exp);

	return hid_loghid_uninit;
}
