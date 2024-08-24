/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2024 Tibor 'Igor2' Palinkas
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

/* toolpath based hpgl export (derived from the gcode plugin) */

#include "../src_plugins/millpath/toolpath.h"

const char *pcb_export_hpgltp_cookie = "export_hpgltp plugin";

static rnd_hid_t hpgltp_hid;

typedef struct {
	pcb_cam_t cam;
	pcb_board_t *pcb;
	pcb_layergrp_t *grp; /* layer group being exported */
	long drawn_objs;
} hpgltp_t;

static hpgltp_t gctx;

#undef HA_cam

static const char def_layer_script[] = "setup_negative; trace_contour; fix_overcuts";
static const char def_mech_script[]  = "setup_positive; trace_contour; fix_overcuts";

static const rnd_export_opt_t hpgltp_attribute_list[] = {
	{"outfile", "file name prefix for non-cam",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_outfile 0

	{"dias", "tool diameters",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_template 1

	{"layer-script", "rendering script for layer graphics",
	 RND_HATT_STRING, 0, 0, {0, def_layer_script, 0}, 0},
#define HA_layer_script 2

	{"mech-script", "rendering script for boundary/mech/drill",
	 RND_HATT_STRING, 0, 0, {0, def_mech_script, 0}, 0},
#define HA_mech_script 3

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 4

};

#define NUM_OPTIONS (sizeof(hpgltp_attribute_list)/sizeof(hpgltp_attribute_list[0]))

static rnd_hid_attr_val_t hpgltp_values[NUM_OPTIONS];

static const rnd_export_opt_t *hpgltp_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	if (n)
		*n = NUM_OPTIONS;
	return hpgltp_attribute_list;
}

static void hpgltp_print_lines_(pcb_line_t *from, pcb_line_t *to)
{
	pcb_line_t *l;

	gctx.drawn_objs++;
	fprintf(f, "PU;PA%ld,%ld;PD;\n", TX(from->Point1.X), TY(from->Point1.Y));

	for(l = from; l != to; l = l->link.next)
		fprintf(f, "PA%ld,%ld;\n", TX(l->Point2.X), TY(l->Point2.Y));
	fprintf(f, "PA%ld,%ld;\n", TX(to->Point2.X), TY(to->Point2.Y));
}

static void hpgltp_print_lines(pcb_tlp_session_t *tctx, pcb_layergrp_t *grp, int thru)
{
	pcb_line_t *from = NULL, *to, *last_to = NULL;
	gdl_iterator_t it;
	rnd_coord_t lastx = RND_MAX_COORD, lasty = RND_MAX_COORD;

	if (tctx->res_path->Line.lst.length == 0) {
		rnd_fprintf(f, "(empty layer group: %s)\n", grp->name);
		return;
	}

	from = linelist_first(&tctx->res_path->Line);
	lastx = TX(from->Point2.X);
	lasty = TY(from->Point2.Y);
	linelist_foreach(&tctx->res_path->Line, &it, to) {
		rnd_coord_t x1 = TX(to->Point1.X), y1 = TY(to->Point1.Y), x2 = TX(to->Point2.X), y2 = TY(to->Point2.Y);
		if ((lastx != x1) && (lasty != y1)) {
			if (to->link.prev == NULL)
				hpgltp_print_lines_(from, from); /* corner case: first line is a stand-alone segment */
			else
				hpgltp_print_lines_(from, to->link.prev);
			from = to;
		}
		lastx = x2;
		lasty = y2;
		last_to = to;
	}
	hpgltp_print_lines_(from, last_to);
}

static int hpgltp_export_layer_group(rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, rnd_xform_t **xform)
{
	int script_ha, thru;
	const char *script;
	pcb_layergrp_t *grp = &gctx.pcb->LayerGroups.grp[group];
	static pcb_tlp_session_t tctx;
	static rnd_coord_t tool_dias[] = {
		RND_MM_TO_COORD(0.2),
	};
	static pcb_tlp_tools_t tools = { sizeof(tool_dias)/sizeof(tool_dias[0]), tool_dias};


	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&gctx.cam, group, purpose, purpi, flags, xform);

	if (!gctx.cam.active) {
		/* in direct export do only mechanical and copper layer groups */
		if (!(flags & PCB_LYT_COPPER) && !(flags & PCB_LYT_BOUNDARY) && !(flags & PCB_LYT_MECH))
			return 0;
	}

	if (!gctx.cam.active) {
		const char *base = hpgltp_values[HA_outfile].str;
		gds_t fn;

		if (base == NULL) base = gctx.pcb->hidlib.loadname;
		if (base == NULL) base = "unknown";

		gds_init(&fn);
		gds_append_str(&fn, base);
		gds_append(&fn, '.');
		pcb_layer_to_file_name_append(&fn, layer, flags, purpose, purpi, PCB_FNS_pcb_rnd);
		gds_append_str(&fn, ".hpgl");

		f = rnd_fopen_askovr(&gctx.pcb->hidlib, fn.array, "w", NULL);
		if (f != NULL)
			print_header();

		gds_uninit(&fn);
	}

	if (f == NULL)
		return 0;

	if (PCB_LAYER_IS_ROUTE(flags, purpi) || PCB_LAYER_IS_DRILL(flags, purpi)) {
		script_ha = HA_mech_script;
		script = def_mech_script;
		thru = 1;
	}
	else {
		script_ha = HA_layer_script;
		script = def_layer_script;
		thru = 0;
	}

	if (hpgltp_values[script_ha].str != NULL)
		script = hpgltp_values[script_ha].str;

	memset(&tctx, 0, sizeof(tctx));
	tctx.edge_clearance = RND_MM_TO_COORD(0.05);
	tctx.tools = &tools;
	pcb_tlp_mill_script(gctx.pcb, &tctx, grp, script);

	hpgltp_print_lines(&tctx, grp, thru);

	if (!gctx.cam.active) {
		print_footer();
		fclose(f);
	}
	return 0;
}

static void hpgltp_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	rnd_layergrp_id_t gid;
	rnd_xform_t xform;
	pcb_board_t *pcb = PCB;

	if (!options) {
		hpgltp_get_export_options(hid, 0, design, appspec);
		options = hpgltp_values;
	}

	gctx.pcb = pcb;
	f = NULL;
	gctx.drawn_objs = 0;
	pcb_cam_begin(pcb, &gctx.cam, &xform, options[HA_cam].str, hpgltp_attribute_list, NUM_OPTIONS, options);


	if (gctx.cam.active) {
		f = rnd_fopen_askovr(&pcb->hidlib, gctx.cam.fn, "w", NULL);
		if (f != NULL)
			print_header();
	}

	if (!gctx.cam.active || (f != NULL)) {
		for(gid = 0; gid < pcb->LayerGroups.len; gid++) {
			pcb_layergrp_t *grp = &pcb->LayerGroups.grp[gid];
			rnd_xform_t *xf = &xform;
			gctx.grp = grp;
			hpgltp_export_layer_group(gid, grp->purpose, grp->purpi, grp->lid[0], grp->ltype, &xf);
			gctx.grp = NULL;
		}

		if (gctx.cam.active) {
			print_footer();
			fclose(f);
		}
	}

	if (!gctx.cam.active) gctx.cam.okempty_content = 1; /* never warn in direct export */

	if (pcb_cam_end(&gctx.cam) == 0) {
		if (!gctx.cam.okempty_group)
			rnd_message(RND_MSG_ERROR, "hpgltp cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	}
	else if (gctx.drawn_objs == 0) {
		if (!gctx.cam.okempty_content)
			rnd_message(RND_MSG_ERROR, "hpgltp cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}
}

static int hpgltp_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, hpgltp_attribute_list, sizeof(hpgltp_attribute_list) / sizeof(hpgltp_attribute_list[0]), pcb_export_hpgltp_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}


static int hpgltp_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nhpgltp exporter command line arguments:\n\n");
	rnd_hid_usage(hpgltp_attribute_list, sizeof(hpgltp_attribute_list) / sizeof(hpgltp_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x hpgltp [hpgltp options] foo.pcb\n\n");
	return 0;
}


static void hpgl_toolpath_uninit(void)
{
	rnd_hid_remove_hid(&hpgltp_hid);
}


static int hpgl_toolpath_init(void)
{
	RND_API_CHK_VER;

	memset(&hpgltp_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&hpgltp_hid);

	hpgltp_hid.struct_size = sizeof(rnd_hid_t);
	hpgltp_hid.name = "hpgltp";
	hpgltp_hid.description = "export toolpath in HP-GL (\"paint remover\")";
	hpgltp_hid.exporter = 1;

	hpgltp_hid.get_export_options = hpgltp_get_export_options;
	hpgltp_hid.do_export = hpgltp_do_export;
	hpgltp_hid.parse_arguments = hpgltp_parse_arguments;
	hpgltp_hid.argument_array = hpgltp_values;

	hpgltp_hid.usage = hpgltp_usage;

	rnd_hid_register_hid(&hpgltp_hid);
	rnd_hid_load_defaults(&hpgltp_hid, hpgltp_attribute_list, NUM_OPTIONS);

	return 0;
}
