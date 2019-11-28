/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_cam.h"
#include "safe_fs.h"
#include "plugins.h"

#include "../lib_polyhelp/topoly.h"

static pcb_hid_t stl_hid;
const char *stl_cookie = "export_stl HID";

pcb_export_opt_t stl_attribute_list[] = {
	{"outfile", "STL output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_stlfile 0

	{"models", "enable searching and inserting model files",
	 PCB_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_models 1

	{"drill", "enable drilling holes",
	 PCB_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_drill 2

	{"cam", "CAM instruction",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 3
};

#define NUM_OPTIONS (sizeof(stl_attribute_list)/sizeof(stl_attribute_list[0]))

static pcb_hid_attr_val_t stl_values[NUM_OPTIONS];

static pcb_export_opt_t *stl_get_export_options(pcb_hid_t *hid, int *n)
{
	const char *suffix = ".stl";

	if ((PCB != NULL)  && (stl_attribute_list[HA_stlfile].default_val.str == NULL))
		pcb_derive_default_filename(PCB->hidlib.filename, &stl_attribute_list[HA_stlfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return stl_attribute_list;
}

void stl_hid_export_to_file(FILE *f, pcb_hid_attr_val_t *options)
{
}

static void stl_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	const char *filename;
	int i;
	pcb_cam_t cam;
	FILE *f;

	if (!options) {
		stl_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			stl_values[i] = stl_attribute_list[i].default_val;
		options = stl_values;
	}

	filename = options[HA_stlfile].str;
	if (!filename)
		filename = "pcb.stl";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &filename);

	f = pcb_fopen_askovr(&PCB->hidlib, filename, "wb", NULL);
	if (!f) {
		perror(filename);
		return;
	}
	stl_hid_export_to_file(f, options);

	fclose(f);
	pcb_cam_end(&cam);
}

static int stl_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_export_register_opts(stl_attribute_list, sizeof(stl_attribute_list) / sizeof(stl_attribute_list[0]), stl_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}


static int stl_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nstl exporter command line arguments:\n\n");
	pcb_hid_usage(stl_attribute_list, sizeof(stl_attribute_list) / sizeof(stl_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x stl [stl options] foo.pcb\n\n");
	return 0;
}


int pplg_check_ver_export_stl(int ver_needed) { return 0; }

void pplg_uninit_export_stl(void)
{
	pcb_remove_actions_by_cookie(stl_cookie);
}

int pplg_init_export_stl(void)
{
	PCB_API_CHK_VER;

	memset(&stl_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&stl_hid);

	stl_hid.struct_size = sizeof(pcb_hid_t);
	stl_hid.name = "stl";
	stl_hid.description = "export board outline in 3-dimensional STL";
	stl_hid.exporter = 1;

	stl_hid.get_export_options = stl_get_export_options;
	stl_hid.do_export = stl_do_export;
	stl_hid.parse_arguments = stl_parse_arguments;

	stl_hid.usage = stl_usage;

	pcb_hid_register_hid(&stl_hid);

	return 0;
}
