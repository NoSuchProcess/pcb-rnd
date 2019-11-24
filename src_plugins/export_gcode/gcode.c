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
#include "plugins.h"
#include "actions.h"
#include "board.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_nogui.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_cam.h"

const char *pcb_export_gcode_cookie = "export_gcode plugin";

static pcb_hid_t gcode_hid;

typedef struct {
	pcb_cam_t cam;
	pcb_board_t *pcb;
	FILE *f; /* output file */
} gcode_t;

static gcode_t gctx;


pcb_export_opt_t gcode_attribute_list[] = {
	{"outfile", "file name prefix for non-cam",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_outfile 0

	{"dias", "tool diameters",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_template 1

	{"script", "rendering script",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_script 2

	{"cam", "CAM instruction",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 3

};

#define NUM_OPTIONS (sizeof(gcode_attribute_list)/sizeof(gcode_attribute_list[0]))

static pcb_hid_attr_val_t gcode_values[NUM_OPTIONS];

static pcb_export_opt_t *gcode_get_export_options(pcb_hid_t *hid, int *n)
{
	if (n)
		*n = NUM_OPTIONS;
	return gcode_attribute_list;
}

static void gcode_export_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, pcb_xform_t **xform)
{
	if (flags & PCB_LYT_UI)
		return;

	pcb_cam_set_layer_group(&gctx.cam, group, purpose, purpi, flags, xform);

	if (!gctx.cam.active) {
		/* in direct export do only mechanical and copper layer groups */
		if (!(flags & PCB_LYT_COPPER) && !(flags & PCB_LYT_BOUNDARY) && !(flags & PCB_LYT_MECH))
			return;
	}

	if (!gctx.cam.active) {
		const char *base = gcode_values[HA_outfile].str;
		gds_t fn;

		if (base == NULL) base = gctx.pcb->hidlib.filename;
		if (base == NULL) base = "unknown";

		gds_init(&fn);
		gds_append_str(&fn, base);
		gds_append(&fn, '.');
		pcb_layer_to_file_name_append(&fn, layer, flags, purpose, purpi, PCB_FNS_pcb_rnd);
		gds_append_str(&fn, ".cnc");

		gctx.f = pcb_fopen_askovr(&gctx.pcb->hidlib, fn.array, "w", NULL);

		gds_uninit(&fn);
	}

	if (gctx.f == NULL)
		return;

	if (!gctx.cam.active)
		fclose(gctx.f);
}

static void gcode_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	int i;
	pcb_layergrp_id_t gid;
	pcb_xform_t xform;
	pcb_board_t *pcb = PCB;

	if (!options) {
		gcode_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			gcode_values[i] = gcode_attribute_list[i].default_val;
		options = gcode_values;
	}

	gctx.pcb = pcb;
	gctx.f = NULL;
	pcb_cam_begin(pcb, &gctx.cam, &xform, options[HA_cam].str, gcode_attribute_list, NUM_OPTIONS, options);


	if (gctx.cam.active)
		gctx.f = pcb_fopen_askovr(&pcb->hidlib, gctx.cam.fn, "w", NULL);

	if (!gctx.cam.active || (gctx.f != NULL)) {
		for(gid = 0; gid < pcb->LayerGroups.len; gid++) {
			pcb_layergrp_t *grp = &pcb->LayerGroups.grp[gid];
			pcb_xform_t *xf = &xform;
			gcode_export_layer_group(gid, grp->purpose, grp->purpi, grp->lid[0], grp->ltype, &xf);
		}

		if (gctx.cam.active)
			fclose(gctx.f);
	}

	if (pcb_cam_end(&gctx.cam) == 0)
		if (!gctx.cam.okempty)
			pcb_message(PCB_MSG_ERROR, "gcode cam export for '%s' failed to produce any content\n", options[HA_cam].str);
}

static int gcode_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_export_register_opts(gcode_attribute_list, sizeof(gcode_attribute_list) / sizeof(gcode_attribute_list[0]), pcb_export_gcode_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}


static int gcode_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\ngcode exporter command line arguments:\n\n");
	pcb_hid_usage(gcode_attribute_list, sizeof(gcode_attribute_list) / sizeof(gcode_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x gcode [gcode options] foo.pcb\n\n");
	return 0;
}



int pplg_check_ver_export_gcode(int ver_needed) { return 0; }

void pplg_uninit_export_gcode(void)
{
}


int pplg_init_export_gcode(void)
{
	PCB_API_CHK_VER;

	memset(&gcode_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&gcode_hid);

	gcode_hid.struct_size = sizeof(pcb_hid_t);
	gcode_hid.name = "gcode";
	gcode_hid.description = "router g-code for removing copper, drilling and routing board outline";
	gcode_hid.exporter = 1;

	gcode_hid.get_export_options = gcode_get_export_options;
	gcode_hid.do_export = gcode_do_export;
	gcode_hid.parse_arguments = gcode_parse_arguments;

	gcode_hid.usage = gcode_usage;

	pcb_hid_register_hid(&gcode_hid);


	return 0;
}
