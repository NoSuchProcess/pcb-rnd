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
#include "../lib_polyhelp/triangulate.h"

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

	{"override-thickness", "override calculated board thickness (when non-zero)",
	 PCB_HATT_COORD, 0, 0, {1, 0, 0}, 0, 0},
#define HA_ovrthick 3

	{"z-center", "when true: z=0 is the center of board cross section, instead of being at the bottom side",
	 PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_zcent 4

	{"cam", "CAM instruction",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 5
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

static void stl_print_horiz_tri(FILE *f, fp2t_triangle_t *t, int up, pcb_coord_t z)
{
	fprintf(f, "	facet normal 0 0 %d\n", up ? 1 : -1);
	fprintf(f, "		outer loop\n");

	if (up) {
		pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (pcb_coord_t)t->Points[0]->X, (pcb_coord_t)t->Points[0]->Y, z);
		pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (pcb_coord_t)t->Points[1]->X, (pcb_coord_t)t->Points[1]->Y, z);
		pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (pcb_coord_t)t->Points[2]->X, (pcb_coord_t)t->Points[2]->Y, z);
	}
	else {
		pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (pcb_coord_t)t->Points[2]->X, (pcb_coord_t)t->Points[2]->Y, z);
		pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (pcb_coord_t)t->Points[1]->X, (pcb_coord_t)t->Points[1]->Y, z);
		pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (pcb_coord_t)t->Points[0]->X, (pcb_coord_t)t->Points[0]->Y, z);
	}

	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");
}

static void stl_print_vert_tri(FILE *f, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t z0, pcb_coord_t z1)
{
	double vx, vy, nx, ny, len;

	vx = x2 - x1; vy = y2 - y1;
	len = sqrt(vx*vx + vy*vy);
	if (len == 0) return;
	vx /= len; vy /= len;
	nx = -vy; ny = vx;

	fprintf(f, "	facet normal %f %f 0\n", nx, ny);
	fprintf(f, "		outer loop\n");
	pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z1);
	pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z1);
	pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z0);
	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");

	fprintf(f, "	facet normal %f %f 0\n", nx, ny);
	fprintf(f, "		outer loop\n");
	pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z1);
	pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z0);
	pcb_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z0);
	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");
}

int stl_hid_export_to_file(FILE *f, pcb_hid_attr_val_t *options, pcb_coord_t maxy, pcb_coord_t z0, pcb_coord_t z1)
{
	pcb_poly_t *poly = pcb_topoly_1st_outline(PCB, PCB_TOPOLY_FLOATING);
	size_t mem_req = fp2t_memory_required(poly->PointN);
	void *mem = calloc(mem_req, 1);
	fp2t_t tri;
	long n, pn, nn;

	if (!fp2t_init(&tri, mem, poly->PointN)) {
		free(mem);
		pcb_poly_free(poly);
		return -1;
	}

	TODO("this is less if there are holes:");
	pn = poly->PointN;

	for(n = pn-1; n >= 0; n--) {
		fp2t_point_t *pt = fp2t_push_point(&tri);
		pt->X = poly->Points[n].X;
		pt->Y = maxy - poly->Points[n].Y;
	}

	fp2t_add_edge(&tri);

	TODO("add holes");
	fp2t_triangulate(&tri);

	fprintf(f, "solid pcb\n");

	/* write the top and bottom plane */
	for(n = 0; n < tri.TriangleCount; n++) {
		stl_print_horiz_tri(f, tri.Triangles[n], 0, z0);
		stl_print_horiz_tri(f, tri.Triangles[n], 1, z1);
	}

	/* write the side */
	for(n = pn-1; n >= 0; n--) {
		nn = n-1;
		if (nn < 0)
			nn = pn-1;
		stl_print_vert_tri(f, poly->Points[n].X, maxy - poly->Points[n].Y, poly->Points[nn].X, maxy - poly->Points[nn].Y, z0, z1);
	}


	fprintf(f, "endsolid\n");

	free(mem);
	pcb_poly_free(poly);
	return 0;
}

TODO("this is the same code as in g-code and probably openscad has a version too - centralize");
static pcb_coord_t pcb_board_thickness(pcb_board_t *pcb)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *grp;
	pcb_coord_t curr, total = 0;

	for(gid = 0, grp = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,grp++) {
		const char *s;

		if (!(grp->ltype & PCB_LYT_COPPER) && !(grp->ltype & PCB_LYT_SUBSTRATE))
			continue;
		s = pcb_attribute_get(&grp->Attributes, "stl::thickness");
		if (s == NULL)
			s = pcb_attribute_get(&grp->Attributes, "thickness");

		curr = 0;
		if (s != NULL)
			curr = pcb_get_value(s, NULL, NULL, NULL);
		if (curr <= 0) {
			if (grp->ltype & PCB_LYT_SUBSTRATE)
				pcb_message(PCB_MSG_ERROR, "stl: can not determine substrate thickness on layer group %ld - total board thickness is probably wrong\n", (long)gid);
			continue;
		}
		total += curr;
	}
	return curr;
}


static void stl_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	const char *filename;
	int i;
	pcb_cam_t cam;
	FILE *f;
	pcb_coord_t thick;

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

	/* determine sheet thickness */
	if (options[HA_ovrthick].crd > 0) thick = options[HA_ovrthick].crd;
	else thick = pcb_board_thickness(PCB);
	if (thick <= 0) {
		pcb_message(PCB_MSG_WARNING, "STL: can not determine board thickness - falling back to hardwired 1.6mm\n");
		thick = PCB_MM_TO_COORD(1.6);
	}

	if (options[HA_zcent].lng)
		stl_hid_export_to_file(f, options, PCB->hidlib.size_y, -thick/2, +thick/2);
	else
		stl_hid_export_to_file(f, options, PCB->hidlib.size_y, 0, thick);

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
