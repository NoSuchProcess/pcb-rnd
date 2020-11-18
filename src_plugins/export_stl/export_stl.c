/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include "hid_cam.h"
#include <librnd/core/hid_nogui.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/plugins.h>
#include <librnd/core/compat_misc.h>
#include <librnd/poly/rtree.h>
#include <librnd/poly/rtree2_compat.h>
#include "data.h"
#include "funchash_core.h"
#include "layer.h"
#include "obj_pstk_inlines.h"
#include "plug_io.h"
#include "conf_core.h"

#include "../lib_polyhelp/topoly.h"
#include "../lib_polyhelp/triangulate.h"

static rnd_hid_t stl_hid;
const char *stl_cookie = "export_stl HID";

rnd_export_opt_t stl_attribute_list[] = {
	{"outfile", "STL output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_stlfile 0

	{"models", "enable searching and inserting model files",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_models 1

	{"min-drill", "minimum circular hole diameter to render (smaller ones are not drawn)",
	 RND_HATT_COORD, 0, 0, {0, 0, 0}, 0, 0},
#define HA_mindrill 2

	{"min-slot-line", "minimum thickness of padstakc slots specified as lines (smaller ones are not drawn)",
	 RND_HATT_COORD, 0, 0, {0, 0, 0}, 0, 0},
#define HA_minline 3

	{"slot-poly", "draw cutouts for slots specified as padstack polygons",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_slotpoly 4

	{"cutouts", "draw cutouts drawn on 'route' layer groups (outline/slot mech layer groups)",
	 RND_HATT_BOOL, 0, 0, {1, 0, 0}, 0, 0},
#define HA_cutouts 5

	{"override-thickness", "override calculated board thickness (when non-zero)",
	 RND_HATT_COORD, 0, 0, {1, 0, 0}, 0, 0},
#define HA_ovrthick 6

	{"z-center", "when true: z=0 is the center of board cross section, instead of being at the bottom side",
	 RND_HATT_BOOL, 0, 0, {0, 0, 0}, 0, 0},
#define HA_zcent 7

	{"cam", "CAM instruction",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 8
};

#define NUM_OPTIONS (sizeof(stl_attribute_list)/sizeof(stl_attribute_list[0]))

static rnd_hid_attr_val_t stl_values[NUM_OPTIONS];

static rnd_export_opt_t *stl_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".stl";
	char **val = stl_attribute_list[HA_stlfile].value;

	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &stl_attribute_list[HA_stlfile], suffix);

	if (n)
		*n = NUM_OPTIONS;
	return stl_attribute_list;
}

static void stl_print_horiz_tri(FILE *f, fp2t_triangle_t *t, int up, rnd_coord_t z)
{
	fprintf(f, "	facet normal 0 0 %d\n", up ? 1 : -1);
	fprintf(f, "		outer loop\n");

	if (up) {
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[0]->X, (rnd_coord_t)t->Points[0]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[1]->X, (rnd_coord_t)t->Points[1]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[2]->X, (rnd_coord_t)t->Points[2]->Y, z);
	}
	else {
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[2]->X, (rnd_coord_t)t->Points[2]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[1]->X, (rnd_coord_t)t->Points[1]->Y, z);
		rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", (rnd_coord_t)t->Points[0]->X, (rnd_coord_t)t->Points[0]->Y, z);
	}

	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");
}

static void stl_print_vert_tri(FILE *f, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t z0, rnd_coord_t z1)
{
	double vx, vy, nx, ny, len;

	vx = x2 - x1; vy = y2 - y1;
	len = sqrt(vx*vx + vy*vy);
	if (len == 0) return;
	vx /= len; vy /= len;
	nx = -vy; ny = vx;

	fprintf(f, "	facet normal %f %f 0\n", nx, ny);
	fprintf(f, "		outer loop\n");
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z1);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z1);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z0);
	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");

	fprintf(f, "	facet normal %f %f 0\n", nx, ny);
	fprintf(f, "		outer loop\n");
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z1);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x1, y1, z0);
	rnd_fprintf(f, "			vertex %.09mm %.09mm %.09mm\n", x2, y2, z0);
	fprintf(f, "		endloop\n");
	fprintf(f, "	endfacet\n");
}

static long pa_len(const rnd_polyarea_t *pa)
{
	rnd_pline_t *pl;
	rnd_vnode_t *n;
	long cnt = 0;

	pl = pa->contours;
	n = pl->head;
	do {
		cnt++;
		n = n->next;
	} while(n != pl->head);
	return cnt;
}

static long poly_len(const pcb_poly_t *poly)
{
	return poly->PointN; /* assume no holes */
}

static int pstk_points(pcb_board_t *pcb, pcb_pstk_t *pstk, pcb_layer_t *layer, fp2t_t *tri, rnd_coord_t maxy, vtd0_t *contours, rnd_hid_attr_val_t *options)
{
	pcb_pstk_shape_t *shp, tmp;
	rnd_polyarea_t *pa = NULL;
	int segs = 0;

	shp = pcb_pstk_shape_mech_or_hole_at(pcb, pstk, layer, &tmp);
	if (shp == NULL)
		return 0;

	switch(shp->shape) {
		case PCB_PSSH_POLY:
			if (!options[HA_slotpoly].lng)
				return 0;
			segs = shp->data.poly.len;
			break;
		case PCB_PSSH_CIRC:
			if (shp->data.circ.dia < options[HA_mindrill].crd)
				return 0;
			segs = RND_COORD_TO_MM(shp->data.circ.dia)*8+6;
			break;
		case PCB_PSSH_HSHADOW: return 0;
		case PCB_PSSH_LINE:
			if (shp->data.line.thickness < options[HA_minline].crd)
				return 0;

			{
				pcb_line_t l = {0};
				l.Point1.X = pstk->x + shp->data.line.x1; l.Point1.Y = pstk->y + shp->data.line.y1;
				l.Point2.X = pstk->x + shp->data.line.x2; l.Point2.Y = pstk->y + shp->data.line.y2;
				l.Thickness = shp->data.line.thickness;
				if (shp->data.line.square)
					PCB_FLAG_SET(PCB_FLAG_SQUARE, &l);
				pa = pcb_poly_from_pcb_line(&l, l.Thickness);
				segs += pa_len(pa);
			}
			break;
	}

	if (tri != NULL) {
		switch(shp->shape) {
			case PCB_PSSH_POLY:
				{
					int n;
					for(n = 0; n < shp->data.poly.len; n++) {
						fp2t_point_t *pt = fp2t_push_point(tri);
						pt->X = pstk->x + shp->data.poly.x[n];
						pt->Y = maxy - (pstk->y + shp->data.poly.y[n]);
						vtd0_append(contours, pt->X);
						vtd0_append(contours, pt->Y);
					}
					fp2t_add_hole(tri);
					vtd0_append(contours, HUGE_VAL);
					vtd0_append(contours, HUGE_VAL);
				}
				break;
			case PCB_PSSH_LINE:
				{
					rnd_pline_t *pl = pa->contours;
					rnd_vnode_t *n = pl->head;
					do {
						fp2t_point_t *pt = fp2t_push_point(tri);
						pt->X = n->point[0];
						pt->Y = maxy - n->point[1];
						vtd0_append(contours, pt->X);
						vtd0_append(contours, pt->Y);
						n = n->next;
					} while(n != pl->head);
					fp2t_add_hole(tri);
					vtd0_append(contours, HUGE_VAL);
					vtd0_append(contours, HUGE_VAL);
				}
				break;
			case PCB_PSSH_CIRC:
				{
					double a, step = 2*M_PI/segs, r = (double)shp->data.circ.dia / 2.0;
					int n;
					for(n = 0, a = 0; n < segs; n++, a += step) {
						fp2t_point_t *pt = fp2t_push_point(tri);
						pt->X = pstk->x + shp->data.circ.x + rnd_round(cos(a) * r);
						pt->Y = maxy - (pstk->y + shp->data.circ.y + rnd_round(sin(a) * r));
						vtd0_append(contours, pt->X);
						vtd0_append(contours, pt->Y);
					}
					fp2t_add_hole(tri);
					vtd0_append(contours, HUGE_VAL);
					vtd0_append(contours, HUGE_VAL);
				}
			case PCB_PSSH_HSHADOW: return 0;
		}
	}

	if (pa != NULL)
		rnd_polyarea_free(&pa);

	return segs;
}

static void add_holes_pstk(fp2t_t *tri, pcb_board_t *pcb, pcb_layer_t *toply, rnd_coord_t maxy, vtd0_t *contours, rnd_hid_attr_val_t *options, pcb_dynf_t df)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;

	for(n = rnd_r_first(pcb->Data->padstack_tree, &it); n != NULL; n = rnd_r_next(&it)) {
		pcb_pstk_t *ps = (pcb_pstk_t *)n;
		if (!PCB_DFLAG_TEST(&ps->Flags, df)) /* if a padstack is marked, it is on the contour and it should already be subtracted from the contour poly, skip it */
			pstk_points(pcb, ps, toply, tri, maxy, contours, options);
	}
}

static long estimate_hole_pts_pstk(pcb_board_t *pcb, pcb_layer_t *toply, rnd_hid_attr_val_t *options)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;
	long cnt = 0;

	for(n = rnd_r_first(pcb->Data->padstack_tree, &it); n != NULL; n = rnd_r_next(&it)) {
		pcb_pstk_t *ps = (pcb_pstk_t *)n;
		cnt += pstk_points(pcb, ps, toply, NULL, 0, NULL, options);
	}

	return cnt;
}

static long estimate_cutout_pts(pcb_board_t *pcb, vtp0_t *cutouts, pcb_dynf_t df, rnd_hid_attr_val_t *options)
{
	rnd_layer_id_t lid;
	long cnt = 0;

	if (!options[HA_cutouts].lng)
		return 0;

	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		pcb_layer_type_t lyt = pcb_layer_flags(pcb, lid);
		int purpi = pcb_layer_purpose(pcb, lid, NULL);
		pcb_layer_t *layer = &pcb->Data->Layer[lid];
		pcb_poly_t *poly;

		if (!PCB_LAYER_IS_ROUTE(lyt, purpi)) continue;
/*		rnd_trace("Outline [%ld]\n", lid);*/
		PCB_LINE_LOOP(layer) {
			if (PCB_DFLAG_TEST(&line->Flags, df)) continue; /* object already found - either as outline or as a cutout */
			poly = pcb_topoly_conn_with(pcb, (pcb_any_obj_t *)line, PCB_TOPOLY_FLOATING, df);
			vtp0_append(cutouts, poly);
			cnt += poly_len(poly);
/*			rnd_trace(" line: %ld %d -> %p\n", line->ID, PCB_DFLAG_TEST(&line->Flags, df), poly);*/
		} PCB_END_LOOP;
		PCB_ARC_LOOP(layer) {
			if (PCB_DFLAG_TEST(&arc->Flags, df)) continue; /* object already found - either as outline or as a cutout */
			poly = pcb_topoly_conn_with(pcb, (pcb_any_obj_t *)arc, PCB_TOPOLY_FLOATING, df);
			vtp0_append(cutouts, poly);
			cnt += poly_len(poly);
/*			rnd_trace(" arc: %ld %d -> %p\n", arc->ID, PCB_DFLAG_TEST(&arc->Flags, df), poly);*/
		} PCB_END_LOOP;
	}
	return cnt;
}

static void add_holes_cutout(fp2t_t *tri, pcb_board_t *pcb, rnd_coord_t maxy, vtp0_t *cutouts, vtd0_t *contours, rnd_hid_attr_val_t *options)
{
	long np;

	if (!options[HA_cutouts].lng)
		return;

	for(np = 0; np < cutouts->used; np++) {
		pcb_poly_t *poly = cutouts->array[np];
		int n;

		for(n = 0; n < poly->PointN; n++) {
			fp2t_point_t *pt = fp2t_push_point(tri);
			pt->X = poly->Points[n].X;
			pt->Y = maxy - poly->Points[n].Y;
			vtd0_append(contours, pt->X);
			vtd0_append(contours, pt->Y);
		}

		fp2t_add_hole(tri);
		vtd0_append(contours, HUGE_VAL);
		vtd0_append(contours, HUGE_VAL);
	}
}

#include "stl_models.c"

int stl_hid_export_to_file(FILE *f, rnd_hid_attr_val_t *options, rnd_coord_t maxy, rnd_coord_t z0, rnd_coord_t z1)
{
	pcb_dynf_t df;
	pcb_poly_t *brdpoly;
	size_t mem_req;
	void *mem;
	fp2t_t tri;
	long cn_start, cn, n, pstk_points, cutout_points;
	rnd_layer_id_t lid = -1;
	pcb_layer_t *toply;
	vtd0_t contours = {0};
	vtp0_t cutouts = {0};
	long contlen;
	rnd_vnode_t *vn;
	rnd_pline_t *pl;

	if ((pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_TOP, &lid, 1) != 1) && (pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &lid, 1) != 1)) {
		rnd_message(RND_MSG_ERROR, "A top or bottom copper layer is required for stl export\n");
		return -1;
	}
	toply = pcb_get_layer(PCB->Data, lid);


	df = pcb_dynflag_alloc("export_stl_map_contour");
	pcb_data_dynflag_clear(PCB->Data, df);
	brdpoly = pcb_topoly_1st_outline_with(PCB, PCB_TOPOLY_FLOATING, df);

	pstk_points = estimate_hole_pts_pstk(PCB, toply, options);
	cutout_points = estimate_cutout_pts(PCB, &cutouts, df, options);

	contlen = pa_len(brdpoly->Clipped);
	mem_req = fp2t_memory_required(contlen + pstk_points + cutout_points);
	mem = calloc(mem_req, 1);
	if (!fp2t_init(&tri, mem, contlen + pstk_points)) {
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

	add_holes_pstk(&tri, PCB, toply, maxy, &contours, options, df);
	add_holes_cutout(&tri, PCB, maxy, &cutouts, &contours, options);

	fp2t_triangulate(&tri);

	fprintf(f, "solid pcb\n");

	/* write the top and bottom plane */
	for(n = 0; n < tri.TriangleCount; n++) {
		stl_print_horiz_tri(f, tri.Triangles[n], 0, z0);
		stl_print_horiz_tri(f, tri.Triangles[n], 1, z1);
	}

	/* write the vertical side */
	for(cn_start = 0, cn = 2; cn < contours.used; cn+=2) {
		if (contours.array[cn] == HUGE_VAL) {
			double cx, cy, px, py;
			long n, pn;
/*			rnd_trace("contour: %ld..%ld\n", cn_start, cn);*/
			for(n = cn-2; n >= cn_start; n-=2) {
				pn = n-2;
				if (pn < cn_start)
					pn = cn-2;
				px = contours.array[pn], py = contours.array[pn+1];
				cx = contours.array[n], cy = contours.array[n+1];
/*				rnd_trace(" [%ld <- %ld] c:%f;%f p:%f;%f\n", n, pn, cx/1000000.0, cy/1000000.0, px/1000000.0, py/1000000.0);*/
				stl_print_vert_tri(f, cx, cy, px, py, z0, z1);
			}
			cn += 2;
			cn_start = cn;
		}
	}

	if (options[HA_models].lng)
		stl_models_print(PCB, f, maxy, z0, z1);

	fprintf(f, "endsolid\n");

	vtp0_uninit(&cutouts);
	for(n = 0; n < cutouts.used; n++)
		pcb_poly_free((pcb_poly_t *)cutouts.array[n]);
	vtd0_uninit(&contours);
	free(mem);
	pcb_poly_free(brdpoly);
	pcb_dynflag_free(df);
	return 0;
}

static void stl_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *filename;
	pcb_cam_t cam;
	FILE *f;
	rnd_coord_t thick;

	if (!options) {
		stl_get_export_options(hid, 0);
		options = stl_values;
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
		stl_hid_export_to_file(f, options, PCB->hidlib.size_y, -thick/2, +thick/2);
	else
		stl_hid_export_to_file(f, options, PCB->hidlib.size_y, 0, thick);

	fclose(f);
	pcb_cam_end(&cam);
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
	stl_hid.argument_array = stl_values;

	stl_hid.usage = stl_usage;

	rnd_hid_register_hid(&stl_hid);

	return 0;
}
