/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020,2023 Tibor 'Igor2' Palinkas
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

#include <genvector/vtd0.h>
#include <librnd/core/actions.h>
#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/hid/hid_nogui.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/poly/rtree.h>
#include "data.h"
#include "funchash_core.h"
#include "layer.h"
#include "obj_pstk_inlines.h"
#include "plug_io.h"
#include "conf_core.h"

#include "../lib_polyhelp/topoly.h"
#include "../lib_polyhelp/triangulate.h"

static rnd_hid_t stl_hid, amf_hid, proj_hid;
const char *stl_cookie = "export_stl HID";

static const rnd_export_opt_t stl_attribute_list[] = {
	{"outfile", "STL/AMF output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_stlfile 0

	{"models", "enable searching and inserting model files",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_models 1

	{"min-drill", "minimum circular hole diameter to render (smaller ones are not drawn)",
	 RND_HATT_COORD, 0, RND_COORD_MAX, {0, 0, 0}, 0},
#define HA_mindrill 2

	{"min-slot-line", "minimum thickness of padstakc slots specified as lines (smaller ones are not drawn)",
	 RND_HATT_COORD, 0, RND_COORD_MAX, {0, 0, 0}, 0},
#define HA_minline 3

	{"slot-poly", "draw cutouts for slots specified as padstack polygons",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_slotpoly 4

	{"cutouts", "draw cutouts drawn on 'route' layer groups (outline/slot mech layer groups)",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0},
#define HA_cutouts 5

	{"override-thickness", "override calculated board thickness (when non-zero)",
	 RND_HATT_COORD, 0, RND_COORD_MAX, {1, 0, 0}, 0},
#define HA_ovrthick 6

	{"z-center", "when true: z=0 is the center of board cross section, instead of being at the bottom side",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0},
#define HA_zcent 7

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_cam 8
};

#define NUM_OPTIONS (sizeof(stl_attribute_list)/sizeof(stl_attribute_list[0]))

static rnd_hid_attr_val_t stl_values_[NUM_OPTIONS];
static rnd_hid_attr_val_t amf_values_[NUM_OPTIONS];
static rnd_hid_attr_val_t proj_values_[NUM_OPTIONS];

static long pa_len(const rnd_polyarea_t *pa_start)
{
	long cnt = 0;
	const rnd_polyarea_t *pa = pa_start;

	do {
		rnd_pline_t *pl = pa->contours; /* consider the main contour only */
		rnd_vnode_t *n = pl->head;
		do {
			cnt++;
			n = n->next;
		} while(n != pl->head);
		pa = pa->f;
	} while(pa != pa_start);

	return cnt;
}

typedef struct stl_facet_s stl_facet_t;

struct stl_facet_s {
	double n[3];
	double vx[3], vy[3], vz[3];
	stl_facet_t *next;
};
RND_INLINE void v_transform(double dst[3], double src[3], double mx[16]);

static void stl_triangle_normal(double *dx, double *dy, double *dz, double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3)
{
	double len;

	*dx = (y2-y1)*(z3-z1) - (y3-y1)*(z2-z1);
	*dy = (z2-z1)*(x3-x1) - (x2-x1)*(z3-z1);
	*dz = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1);

	len = sqrt((*dx) * (*dx) + (*dy) * (*dy) + (*dz) * (*dz));
	if (len == 0) return;

	*dx /= len;
	*dy /= len;
	*dz /= len;
}

static void stl_facet_normal(stl_facet_t *t)
{
	stl_triangle_normal(
		&t->n[0], &t->n[1], &t->n[2],
		t->vx[0], t->vy[0], t->vz[0],
		t->vx[1], t->vy[1], t->vz[1],
		t->vx[2], t->vy[2], t->vz[2]
		);
}

#include "verthash.c"

typedef struct {
	/* output */
	const char *suffix;
	void (*print_horiz_tri)(FILE *f, fp2t_triangle_t *t, int up, rnd_coord_t z);
	void (*print_vert_tri)(FILE *f, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t z0, rnd_coord_t z1);
	void (*print_facet)(FILE *f, stl_facet_t *head, double mx[16], double mxn[16]);
	void (*new_obj)(float r, float g, float b);
	void (*print_header)(FILE *f);
	void (*print_footer)(FILE *f);

	/* model load */
	const char *attr_model_name;
	const char *attr_xlate, *attr_xlate_old;
	const char *attr_rotate, *attr_rotate_old;
	stl_facet_t *(*model_load)(rnd_design_t *hl, FILE *f, const char *fn);
} stl_fmt_t;

static const rnd_export_opt_t *stl_get_export_options_(rnd_hid_t *hid, int *n, const stl_fmt_t *fmt, rnd_hid_attr_val_t *values, rnd_design_t *dsg, void *appspec)
{
	const char *suffix = fmt->suffix;
	const char *val = values[HA_stlfile].str;

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &values[HA_stlfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return stl_attribute_list;
}

#include "exp_fmt_stl.c"
#include "exp_fmt_amf.c"
#include "exp_fmt_proj.c"

static const stl_fmt_t *fmt_all[] = {&fmt_amf, &fmt_stl, &fmt_proj, NULL};

#include "stl_models.c"
#include "model_load_stl.c"
#include "model_load_amf.c"


static const rnd_export_opt_t *stl_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	return stl_get_export_options_(hid, n, &fmt_stl, stl_values_, dsg, appspec);
}

static const rnd_export_opt_t *amf_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	return stl_get_export_options_(hid, n, &fmt_amf, amf_values_, dsg, appspec);
}

static const rnd_export_opt_t *proj_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	return stl_get_export_options_(hid, n, &fmt_proj, proj_values_, dsg, appspec);
}

static int stl_hid_export_to_file(FILE *f, rnd_hid_attr_val_t *options, rnd_coord_t maxy, rnd_coord_t z0, rnd_coord_t z1, const stl_fmt_t *fmt)
{
	pcb_dynf_t df;
	pcb_poly_t *brdpoly;
	rnd_polyarea_t *cutoutpa;
	size_t mem_req;
	void *mem;
	fp2t_t tri;
	long cn_start, cn, n, cutout_points;
	rnd_layer_id_t lid = -1;
	vtd0_t contours = {0};
	long contlen;
	rnd_vnode_t *vn;
	rnd_pline_t *pl;
	int first_vert = 1;

	/* lib_polyhelp would give a less useful message, rather check this here */
	if ((pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_TOP, &lid, 1) != 1) && (pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &lid, 1) != 1)) {
		rnd_message(RND_MSG_ERROR, "A top or bottom copper layer is required for stl export\n");
		return -1;
	}

	df = pcb_dynflag_alloc("export_stl_map_contour");
	pcb_data_dynflag_clear(PCB->Data, df);
	brdpoly = pcb_topoly_1st_outline_with(PCB, PCB_TOPOLY_FLOATING, df);

	if (brdpoly == NULL) {
		rnd_message(RND_MSG_ERROR, "Contour/outline error: need closed loops\n(Hint: use the wireframe draw mode to see broken connections; use a coarse grid and snap to fix them up!)\n");
		pcb_dynflag_free(df);
		return -1;
	}

	/* collect cutouts **/
	{
		pcb_topoly_cutout_opts_t opts = {0};

		opts.pstk_omit_slot_poly = !options[HA_slotpoly].lng;
		opts.pstk_min_drill_dia = options[HA_mindrill].crd;
		opts.pstk_min_line_thick = options[HA_minline].crd;

		cutoutpa = pcb_topoly_cutouts_in(PCB, df, brdpoly, &opts);
	}
	cutout_points = 0;
	if (cutoutpa != NULL)
		cutout_points = pa_len(cutoutpa);

	contlen = pa_len(brdpoly->Clipped);
	mem_req = fp2t_memory_required(contlen + cutout_points);
	mem = calloc(mem_req, 1);
	if (!fp2t_init(&tri, mem, contlen + cutout_points)) {
		free(mem);
		pcb_poly_free(brdpoly);
		pcb_dynflag_free(df);
		return -1;
	}

	/* there are no holes in the brdpoly so this simple approach for determining
	   the number of contour points works (cutouts are applied separately on
	   the triangulation lib level) */
/*	pn = contlen;*/
	pl = brdpoly->Clipped->contours;
	vn = pl->head;
	n = 0;
	do {
		fp2t_point_t *pt = fp2t_push_point(&tri);
		pt->X = vn->point[0];
		pt->Y = maxy - vn->point[1];
		vtd0_append(&contours, pt->X);
		vtd0_append(&contours, pt->Y);
		vn = vn->next;
		n++;
	} while(vn != pl->head);

	fp2t_add_edge(&tri);
	vtd0_append(&contours, HUGE_VAL);
	vtd0_append(&contours, HUGE_VAL);

	if (cutoutpa != NULL) {
		rnd_polyarea_t *pa = cutoutpa;

		do {
			/* consider poly island outline only */
			pl = pa->contours;
			vn = pl->head;
			do {
				fp2t_point_t *pt = fp2t_push_point(&tri);
				pt->X = vn->point[0];
				pt->Y = maxy - vn->point[1];
				vtd0_append(&contours, pt->X);
				vtd0_append(&contours, pt->Y);
				vn = vn->prev;
			} while(vn != pl->head);

			fp2t_add_hole(&tri);
			vtd0_append(&contours, HUGE_VAL);
			vtd0_append(&contours, HUGE_VAL);

			pa = pa->f;
		} while(pa != cutoutpa);
	}

	fp2t_triangulate(&tri);

	fmt->print_header(f);
	fmt->new_obj(0, 0.4, 0);

	/* write the top and bottom plane */
	for(n = 0; n < tri.TriangleCount; n++) {
		fmt->print_horiz_tri(f, tri.Triangles[n], 0, z0);
		fmt->print_horiz_tri(f, tri.Triangles[n], 1, z1);
	}

	/* write the vertical side */
	for(cn_start = 0, cn = 2; cn < contours.used; cn+=2) {
		if (contours.array[cn] == HUGE_VAL) {
			double cx, cy, px, py;
			long n, pn;
			for(n = cn-2; n >= cn_start; n-=2) {
				pn = n-2;
				if (pn < cn_start)
					pn = cn-2;
				px = contours.array[pn], py = contours.array[pn+1];
				cx = contours.array[n], cy = contours.array[n+1];
/*				rnd_trace(" [%ld <- %ld] c:%f;%f p:%f;%f\n", n, pn, cx/1000000.0, cy/1000000.0, px/1000000.0, py/1000000.0);*/
				if (first_vert)
					fmt->print_vert_tri(f, cx, cy, px, py, z0, z1);
				else
					fmt->print_vert_tri(f, cx, cy, px, py, z1, z0);
			}
			cn += 2;
			cn_start = cn;
			first_vert = 0;
		}
	}

	if (options[HA_models].lng)
		stl_models_print(PCB, f, maxy, z0, z1, fmt);

	fmt->print_footer(f);

	if (cutoutpa != NULL)
		rnd_polyarea_free(&cutoutpa);
	vtd0_uninit(&contours);
	free(mem);
	pcb_poly_free(brdpoly);
	pcb_dynflag_free(df);
	return 0;
}

static void stl_do_export_(rnd_hid_t *hid, rnd_hid_attr_val_t *options, const stl_fmt_t *fmt, rnd_hid_attr_val_t *values, rnd_design_t *dsg, void *appspec)
{
	const char *filename;
	pcb_cam_t cam;
	FILE *f;
	rnd_coord_t thick;

	if (!options) {
		stl_get_export_options_(hid, 0, fmt, values, dsg, appspec);
		options = values;
	}

	filename = options[HA_stlfile].str;
	if (!filename)
		filename = "pcb.stl";

	pcb_cam_begin_nolayer(PCB, &cam, NULL, options[HA_cam].str, &filename);

	f = rnd_fopen_askovr(&PCB->hidlib, filename, "wb", NULL);
	if (!f) {
		perror(filename);
		return;
	}

	/* determine sheet thickness */
	if (options[HA_ovrthick].crd > 0) thick = options[HA_ovrthick].crd;
	else thick = pcb_board_thickness(PCB, "stl", PCB_BRDTHICK_PRINT_ERROR);
	if (thick <= 0) {
		rnd_message(RND_MSG_WARNING, "STL: can not determine board thickness - falling back to hardwired 1.6mm\n");
		thick = RND_MM_TO_COORD(1.6);
	}

	if (options[HA_zcent].lng)
		stl_hid_export_to_file(f, options, PCB->hidlib.dwg.Y2, -thick/2, +thick/2, fmt);
	else
		stl_hid_export_to_file(f, options, PCB->hidlib.dwg.Y2, 0, thick, fmt);

	fclose(f);
	pcb_cam_end(&cam);
}

static void stl_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	stl_do_export_(hid, options, &fmt_stl, stl_values_, design, appspec);
}

static void amf_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	stl_do_export_(hid, options, &fmt_amf, amf_values_, design, appspec);
}

static void proj_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	stl_do_export_(hid, options, &fmt_proj, proj_values_, design, appspec);
}

static int stl_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, stl_attribute_list, sizeof(stl_attribute_list) / sizeof(stl_attribute_list[0]), stl_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}


static int stl_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nstl exporter command line arguments:\n\n");
	rnd_hid_usage(stl_attribute_list, sizeof(stl_attribute_list) / sizeof(stl_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x stl [stl options] foo.pcb\n\n");
	return 0;
}


int pplg_check_ver_export_stl(int ver_needed) { return 0; }

void pplg_uninit_export_stl(void)
{
	rnd_remove_actions_by_cookie(stl_cookie);
	rnd_hid_remove_hid(&stl_hid);
	rnd_hid_remove_hid(&amf_hid);
}

int pplg_init_export_stl(void)
{
	RND_API_CHK_VER;

	memset(&stl_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&stl_hid);

	stl_hid.struct_size = sizeof(rnd_hid_t);
	stl_hid.name = "stl";
	stl_hid.description = "export board outline in 3-dimensional STL";
	stl_hid.exporter = 1;

	stl_hid.get_export_options = stl_get_export_options;
	stl_hid.do_export = stl_do_export;
	stl_hid.parse_arguments = stl_parse_arguments;
	stl_hid.argument_array = stl_values_;

	stl_hid.usage = stl_usage;

	rnd_hid_register_hid(&stl_hid);
	rnd_hid_load_defaults(&stl_hid, stl_attribute_list, NUM_OPTIONS);


	memcpy(&amf_hid, &stl_hid, sizeof(rnd_hid_t));
	amf_hid.name = "amf";
	amf_hid.description = "export board outline in 3-dimensional AMF";
	amf_hid.get_export_options = amf_get_export_options;
	amf_hid.do_export = amf_do_export;
	amf_hid.argument_array = amf_values_;

	rnd_hid_register_hid(&amf_hid);
	rnd_hid_load_defaults(&amf_hid, stl_attribute_list, NUM_OPTIONS);


	memcpy(&proj_hid, &stl_hid, sizeof(rnd_hid_t));
	proj_hid.name = "projector";
	proj_hid.description = "export board outline as a projector(1) object for 3d rendering";
	proj_hid.get_export_options = proj_get_export_options;
	proj_hid.do_export = proj_do_export;
	proj_hid.argument_array = proj_values_;

	rnd_hid_register_hid(&proj_hid);
	rnd_hid_load_defaults(&proj_hid, stl_attribute_list, NUM_OPTIONS);

	return 0;
}
