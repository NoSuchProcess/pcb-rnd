/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf.h"
#include "data.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "hid-logger.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_nogui.h"

static const char *loghid_cookie = "loghid plugin";

static pcb_hid_t loghid_gui;
static pcb_hid_t loghid_exp;

pcb_hid_attribute_t loghid_attribute_list[] = {
	{"target-hid", "the real GUI or export HID to relay calls to",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0}
#define HA_target_hid 0
};
#define NUM_OPTIONS sizeof(loghid_attribute_list) / sizeof(loghid_attribute_list[0])

static int loghid_parse_arguments_real(pcb_hid_t *hid, int *argc, char ***argv, int is_gui)
{
	pcb_hid_t *target, *me;
	const char *target_name;

	pcb_hid_register_attributes(loghid_attribute_list, NUM_OPTIONS, loghid_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);

	target_name = loghid_attribute_list[HA_target_hid].default_val.str_value;

	if (is_gui) {
		target = pcb_hid_find_gui(target_name);
		me = &loghid_gui;
	}
	else {
		target = pcb_hid_find_exporter(target_name);
		me = &loghid_exp;
	}

	create_log_hid(stdout, me, target);
	return target->parse_arguments(target, argc, argv);
}

static int loghid_parse_arguments_gui(pcb_hid_t *hid, int *argc, char ***argv)
{
	return loghid_parse_arguments_real(hid, argc, argv, 1);
}

static int loghid_parse_arguments_exp(pcb_hid_t *hid, int *argc, char ***argv)
{
	return loghid_parse_arguments_real(hid, argc, argv, 0);
}


static int loghid_usage(const char *topic)
{
	fprintf(stderr, "\nloghid command line arguments:\n\n");
	pcb_hid_usage(loghid_attribute_list, NUM_OPTIONS);
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: pcb-rnd [generic_options] --gui loghid-gui --target-hid gtk2_gdk foo.pcb\n");
	fprintf(stderr, "Usage: pcb-rnd [generic_options] --x loghid-exp --target-hid png foo.pcb\n");
	fprintf(stderr, "\n");
	return 0;
}

PCB_REGISTER_ATTRIBUTES(loghid_attribute_list, loghid_cookie)

static pcb_hid_attribute_t *loghid_get_export_options(int *n)
{
/*	loghid_attribute_list[HA_psfile] = pcb_strdup("default?");*/

	if (n)
		*n = (sizeof(loghid_attribute_list)/sizeof(loghid_attribute_list[0]));
	return loghid_attribute_list;
}


#include "dolists.h"

int pplg_check_ver_loghid(int ver_needed) { return 0; }

void pplg_uninit_loghid(void)
{
	pcb_hid_remove_attributes_by_cookie(loghid_cookie);
}

int pplg_init_loghid(void)
{
	PCB_API_CHK_VER;

	pcb_hid_nogui_init(&loghid_gui);
	pcb_hid_nogui_init(&loghid_exp);

	/* gui version */
	loghid_gui.struct_size = sizeof(pcb_hid_t);
	loghid_gui.name = "loghid-gui";
	loghid_gui.description = "log GUI HID calls";
	loghid_gui.gui = 1;

	loghid_gui.usage = loghid_usage;
	loghid_gui.parse_arguments = loghid_parse_arguments_gui;

	pcb_hid_register_hid(&loghid_gui);

	/* export version */
	loghid_exp.struct_size = sizeof(pcb_hid_t);
	loghid_exp.name = "loghid-exp";
	loghid_exp.description = "log export HID calls";
	loghid_exp.exporter = 1;
	loghid_exp.hide_from_gui = 1;

	loghid_exp.usage = loghid_usage;
	loghid_exp.parse_arguments = loghid_parse_arguments_exp;

	pcb_hid_register_hid(&loghid_exp);

	return 0;
}
