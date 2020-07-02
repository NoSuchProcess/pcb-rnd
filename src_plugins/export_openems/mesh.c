/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include "mesh.h"
#include "layer.h"
#include "layer_ui.h"
#include <librnd/core/actions.h>
#include "board.h"
#include "data.h"
#include <librnd/core/hid_dad.h>
#include "event.h"
#include <librnd/core/safe_fs.h>
#include <librnd/core/rnd_printf.h>

static pcb_mesh_t mesh;
static const char *mesh_ui_cookie = "mesh ui layer cookie";

static const char *bnds[] = { "PEC", "PMC", "MUR", "PML_8", NULL };
static const char *bnd_names[] = { "xmin", "xmax", "ymin", "ymax", "zmin", "zmax" };
static const char *subslines[] =   { "0", "1", "3", "5", NULL };
static const int num_subslines[] = { 0,    1,   3,   5 };

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int dens_obj, dens_gap, min_space, smooth, hor, ver, noimpl;
	int bnd[6], pml, subslines, air_top, air_bot, dens_air, smoothz, max_air, def_subs_thick, def_copper_thick;
	unsigned active:1;
} mesh_dlg_t;
static mesh_dlg_t ia;

static void mesh2dlg()
{
	int n;
	rnd_coord_t subst_thick;

	subst_thick = pcb_board_thickness(PCB, "openems", PCB_BRDTHICK_PRINT_ERROR);
	if (subst_thick <= 0) {
		rnd_message(RND_MSG_ERROR, "Assuming 1.5mm thick substrate because of the missing thickness attributes.\nFeel free to change it in the mesh dialog or add the attributes to the substrate groups.");
		subst_thick = RND_MM_TO_COORD(1.5);
	}

	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.pml, lng, mesh.pml);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.dens_obj, crd, RND_MM_TO_COORD(0.15));
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.dens_gap, crd, RND_MM_TO_COORD(0.5));
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.min_space, crd, RND_MM_TO_COORD(0.1));
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.smooth, lng, 1);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.noimpl, lng, 0);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.hor, lng, 1);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.ver, lng, 1);
TODO("enum lookup");
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.subslines, lng, 3);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.def_subs_thick, crd, subst_thick);
/*	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.def_copper_thick, crd, RND_MM_TO_COORD(1.5));*/
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.air_top, lng, 1);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.air_bot, lng, 1);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.dens_air, crd, RND_MM_TO_COORD(0.7));
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.smoothz, lng, 1);
	RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.max_air, crd, RND_MM_TO_COORD(4));
	for(n = 0; n < 6; n++) {
		const char **a;
		int i;
		if (mesh.bnd[n] != NULL)
			for(i = 0, a = bnds; *a != NULL; a++,i++)
				if (strcmp(*a, mesh.bnd[n]) == 0)
					RND_DAD_SET_VALUE(ia.dlg_hid_ctx, ia.bnd[n], lng, i);
	}
}

static void dlg2mesh(void)
{
	int n;
	mesh.pml = ia.dlg[ia.pml].val.lng;
	mesh.dens_obj = ia.dlg[ia.dens_obj].val.crd;
	mesh.dens_gap = ia.dlg[ia.dens_gap].val.crd;
	mesh.min_space = ia.dlg[ia.min_space].val.crd;
	mesh.smooth = ia.dlg[ia.smooth].val.lng;
	mesh.noimpl = ia.dlg[ia.noimpl].val.lng;
	mesh.hor = ia.dlg[ia.hor].val.lng;
	mesh.ver = ia.dlg[ia.ver].val.lng;
TODO("enum lookup");
	mesh.subslines = ia.dlg[ia.subslines].val.lng;
	mesh.def_subs_thick = ia.dlg[ia.def_subs_thick].val.crd;
	mesh.def_copper_thick = ia.dlg[ia.def_copper_thick].val.crd;
	mesh.air_top = ia.dlg[ia.air_top].val.lng;
	mesh.air_bot = ia.dlg[ia.air_top].val.lng;
	mesh.dens_air = ia.dlg[ia.dens_air].val.crd;
	mesh.smoothz = ia.dlg[ia.smoothz].val.lng;
	mesh.max_air = ia.dlg[ia.max_air].val.crd;
	for(n = 0; n < 6; n++)
		mesh.bnd[n] = bnds[ia.dlg[ia.bnd[n]].val.lng];
}

#if 1
	static void mesh_trace(const char *fmt, ...) { }
#else
#	define mesh_trace pcb_trace
#endif

TODO("remove this once the function is moved and published in core")
extern const char *pcb_layergrp_thickness_attr(pcb_layergrp_t *grp, const char *namespace);

rnd_coord_t mesh_layergrp_thickness(pcb_layergrp_t *grp, rnd_coord_t fallback)
{
	rnd_coord_t th = fallback;
	const char *s = pcb_layergrp_thickness_attr(grp, "openems");
	if (s != NULL)
		th = rnd_get_value(s, NULL, NULL, NULL);
	else
		rnd_message(RND_MSG_ERROR, "openEMS: thickness of layer group '%s' is not available, using default for type\n(You should fix your layer group attributes!)\n", grp->name);
	return th;
}


TODO("reorder to avoid fwd decl")
static void mesh_auto_add_smooth(vtc0_t *v, rnd_coord_t c1, rnd_coord_t c2, rnd_coord_t d1, rnd_coord_t d, rnd_coord_t d2);

#define SAVE_INT(name) \
	rnd_append_printf(dst, "%s  " #name" = %d\n", prefix, (int)me->dlg[me->name].val.lng);
#define SAVE_COORD(name) \
	rnd_append_printf(dst, "%s  " #name" = %.08$$mm\n", prefix, (rnd_coord_t)me->dlg[me->name].val.crd);
void pcb_mesh_save(const mesh_dlg_t *me, gds_t *dst, const char *prefix)
{
	int n;

	if (prefix == NULL)
		prefix = "";

	rnd_append_printf(dst, "%sha:pcb-rnd-mesh-v1 {\n", prefix);
	SAVE_COORD(dens_obj);
	SAVE_COORD(dens_gap);
	SAVE_COORD(min_space);
	SAVE_INT(pml);
	SAVE_INT(smooth);
	SAVE_INT(hor);
	SAVE_INT(ver);
	SAVE_INT(noimpl);
	SAVE_INT(air_top);
	SAVE_INT(air_bot);
	SAVE_COORD(dens_air);
	SAVE_INT(smoothz);
	SAVE_COORD(max_air);
	SAVE_COORD(def_subs_thick);
	SAVE_COORD(def_copper_thick);
	rnd_append_printf(dst, "%s  li:boundary = {", prefix);
	for(n = 0; n < 6; n++) {
		int bidx = me->dlg[me->bnd[n]].val.lng;
		const char *bs;
		if ((bidx < 0) || (bidx >= sizeof(bnds) / sizeof(bnds[0])))
			bs = "invalid";
		else
			bs = bnds[bidx];
		gds_append_str(dst, bs);
		gds_append(dst, ';');
	}
	gds_append_str(dst, "}\n");

	{
		int sidx = me->dlg[me->subslines].val.lng;
		const char *bs;
		if ((sidx < 0) || (sidx >= sizeof(subslines) / sizeof(subslines[0])))
			bs = "invalid";
		else
			bs = subslines[sidx];
		rnd_append_printf(dst, "%s  subslines = %s\n", prefix, bs);
	}

	rnd_append_printf(dst, "%s}\n", prefix);
}
#undef SAVE_INT
#undef SAVE_COORD

#define LOAD_INT(name) \
do { \
	lht_node_t *n = lht_dom_hash_get(root, #name); \
	if (n != NULL) { \
		int v; \
		char *end; \
		if (n->type != LHT_TEXT) { \
			rnd_message(RND_MSG_ERROR, "Invalid mesh item: " #name " should be text\n"); \
			return -1; \
		} \
		v = strtol(n->data.text.value, &end, 10); \
		if (*end != '\0') { \
			rnd_message(RND_MSG_ERROR, "Invalid mesh integer: " #name "\n"); \
			return -1; \
		} \
		RND_DAD_SET_VALUE(me->dlg_hid_ctx, me->name, lng, v); \
	} \
} while(0)

#define LOAD_COORD(name) \
do { \
	lht_node_t *n = lht_dom_hash_get(root, #name); \
	if (n != NULL) { \
		double v; \
		rnd_bool succ; \
		if (n->type != LHT_TEXT) { \
			rnd_message(RND_MSG_ERROR, "Invalid mesh item: " #name " should be text\n"); \
			return -1; \
		} \
		v = rnd_get_value(n->data.text.value, NULL, NULL, &succ); \
		if (!succ) { \
			rnd_message(RND_MSG_ERROR, "Invalid mesh coord: " #name "\n"); \
			return -1; \
		} \
		RND_DAD_SET_VALUE(me->dlg_hid_ctx, me->name, crd, (rnd_coord_t)v); \
	} \
} while(0)

#define LOAD_ENUM_VAL(dst, name, node, arr) \
do { \
	if (node != NULL) { \
		int __found__ = 0, __n__; \
		const char **__a__; \
		if (node->type != LHT_TEXT) { \
			rnd_message(RND_MSG_ERROR, "Invalid mesh value: " #name " should be text\n"); \
			return -1; \
		} \
		if (strcmp(node->data.text.value, "invalid") == 0) break; \
		for(__n__ = 0, __a__ = arr; *__a__ != NULL; __a__++,__n__++) { \
			if (strcmp(node->data.text.value, *__a__) == 0) { \
				__found__ = 1; \
				break; \
			} \
		} \
		if (!__found__) { \
			rnd_message(RND_MSG_ERROR, "Invalid mesh value '%s' for " #name "\n", node->data.text.value); \
			return -1; \
		} \
		RND_DAD_SET_VALUE(me->dlg_hid_ctx, dst, lng, __n__); \
	} \
} while(0)

static int mesh_load_subtree(mesh_dlg_t *me, lht_node_t *root)
{
	lht_node_t *lst, *nd;

	if ((root->type != LHT_HASH) || (strcmp(root->name, "pcb-rnd-mesh-v1") != 0)) {
		rnd_message(RND_MSG_ERROR, "Input is not a valid mesh save - should be a ha:pcb-rnd-mesh subtree\n");
		return -1;
	}

	LOAD_COORD(dens_obj);
	LOAD_COORD(dens_gap);
	LOAD_COORD(min_space);
	LOAD_INT(pml);
	LOAD_INT(smooth);
	LOAD_INT(hor);
	LOAD_INT(ver);
	LOAD_INT(noimpl);
	LOAD_INT(air_top);
	LOAD_INT(air_bot);
	LOAD_COORD(dens_air);
	LOAD_INT(smoothz);
	LOAD_COORD(max_air);
	LOAD_COORD(def_subs_thick);
	LOAD_COORD(def_copper_thick);
	LOAD_ENUM_VAL(me->subslines, subslines, lht_dom_hash_get(root, "subslines"), subslines);

	lst = lht_dom_hash_get(root, "boundary");
	if (lst != NULL) {
		int n;
		if (lst->type != LHT_LIST) {
			rnd_message(RND_MSG_ERROR, "Boundary shall be a list\n");
			return -1;
		}
		for(n = 0, nd = lst->data.list.first; (n < 6) && (nd != NULL); n++,nd = nd->next)
			LOAD_ENUM_VAL(me->bnd[n], boundary, nd, bnds);
	}

	return 0;
}
#undef LOAD_INT
#undef LOAD_COORD

int mesh_load_file(mesh_dlg_t *me, FILE *f)
{
	int c, res;
	lht_doc_t *doc;

	doc = lht_dom_init();

	while((c = fgetc(f)) != EOF) {
		lht_err_t err = lht_dom_parser_char(doc, c);
		if ((err != LHTE_SUCCESS) && (err != LHTE_STOP)) {
			lht_dom_uninit(doc);
			return -1;
		}
	}
	res = mesh_load_subtree(me, doc->root);
	lht_dom_uninit(doc);
	return res;
}


static void mesh_add_edge(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t crd)
{
	vtc0_append(&mesh->line[dir].edge, crd);
}

static void mesh_add_result(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t crd)
{
	vtc0_append(&mesh->line[dir].result, crd);
}

static void mesh_add_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t c1, rnd_coord_t c2, rnd_coord_t dens)
{
	pcb_range_t *r = vtr0_alloc_append(&mesh->line[dir].dens, 1);
	r->begin = c1;
	r->end = c2;
	r->data[0].c = dens;
}

static void mesh_add_obj(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t c1, rnd_coord_t c2, int aligned)
{
	if (aligned) {
		mesh_add_edge(mesh, dir, c1 - mesh->dens_obj * 2 / 3);
		mesh_add_edge(mesh, dir, c1 + mesh->dens_obj * 1 / 3);
		mesh_add_edge(mesh, dir, c2 - mesh->dens_obj * 1 / 3);
		mesh_add_edge(mesh, dir, c2 + mesh->dens_obj * 2 / 3);
	}
	mesh_add_range(mesh, dir, c1, c2, mesh->dens_obj);
}

/* generate edges and ranges looking at objects on the given layer */
static int mesh_gen_obj(pcb_mesh_t *mesh, pcb_layer_t *layer, pcb_mesh_dir_t dir)
{
	pcb_data_t *data = layer->parent.data;
	pcb_line_t *line;
	pcb_line_t *arc;
	pcb_poly_t *poly;
	pcb_pstk_t *ps;
	gdl_iterator_t it;

	padstacklist_foreach(&data->padstack, &it, ps) {
		if (pcb_attribute_get(&ps->Attributes, "openems::vport") != 0) {
			switch(dir) {
				case PCB_MESH_HORIZONTAL: mesh_add_edge(mesh, dir, ps->y); break;
				case PCB_MESH_VERTICAL:   mesh_add_edge(mesh, dir, ps->x); break;
				default: break;
			}
		}
	}

	linelist_foreach(&layer->Line, &it, line) {
		rnd_coord_t x1 = line->Point1.X, y1 = line->Point1.Y, x2 = line->Point2.X, y2 = line->Point2.Y;
		int aligned = (x1 == x2) || (y1 == y2);

		switch(dir) {
			case PCB_MESH_HORIZONTAL:
				if (y1 < y2)
					mesh_add_obj(mesh, dir, y1 - line->Thickness/2, y2 + line->Thickness/2, aligned);
				else
					mesh_add_obj(mesh, dir, y2 - line->Thickness/2, y1 + line->Thickness/2, aligned);
				break;
			case PCB_MESH_VERTICAL:
				if (x1 < x2)
					mesh_add_obj(mesh, dir, x1 - line->Thickness/2, x2 + line->Thickness/2, aligned);
				else
					mesh_add_obj(mesh, dir, x2 - line->Thickness/2, x1 + line->Thickness/2, aligned);
				break;
			default: break;
		}
	}

	arclist_foreach(&layer->Arc, &it, arc) {
		/* no point in encorcinf 1/3 2/3 rule, just set the range */
		switch(dir) {
			case PCB_MESH_HORIZONTAL: mesh_add_range(mesh, dir, arc->BoundingBox.Y1 + arc->Clearance/2, arc->BoundingBox.Y2 - arc->Clearance/2, mesh->dens_obj); break;
			case PCB_MESH_VERTICAL:   mesh_add_range(mesh, dir, arc->BoundingBox.X1 + arc->Clearance/2, arc->BoundingBox.X2 - arc->Clearance/2, mesh->dens_obj); break;
			default: break;
		}
	}

TODO("mesh: text")

	polylist_foreach(&layer->Polygon, &it, poly) {
		pcb_poly_it_t it;
		rnd_polyarea_t *pa;

		for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
			rnd_coord_t x, y;
			rnd_pline_t *pl;
			int go;

			pl = pcb_poly_contour(&it);
			if (pl != NULL) {
				rnd_coord_t lx, ly, minx, miny, maxx, maxy;
				
				pcb_poly_vect_first(&it, &minx, &miny);
				maxx = minx;
				maxy = miny;
				pcb_poly_vect_peek_prev(&it, &lx, &ly);
				/* find axis aligned contour edges for the 2/3 1/3 rule */
				for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y)) {
					switch(dir) {
						case PCB_MESH_HORIZONTAL:
							if (y == ly) {
								int sign = (x > lx) ? +1 : -1;
								mesh_add_edge(mesh, dir, y - sign * mesh->dens_obj * 2 / 3);
								mesh_add_edge(mesh, dir, y + sign * mesh->dens_obj * 1 / 3);
							}
							break;
						case PCB_MESH_VERTICAL:
							if (x == lx) {
								int sign = (y < ly) ? +1 : -1;
								mesh_add_edge(mesh, dir, x - sign * mesh->dens_obj * 2 / 3);
								mesh_add_edge(mesh, dir, x + sign * mesh->dens_obj * 1 / 3);
							}
							break;
						default: break;
					}
					lx = x;
					ly = y;
					if (x < minx) minx = x;
					if (y < miny) miny = y;
					if (x > maxx) maxx = x;
					if (y > maxy) maxy = y;
				}
				switch(dir) {
					case PCB_MESH_HORIZONTAL: mesh_add_range(mesh, dir, miny, maxy, mesh->dens_obj); break;
					case PCB_MESH_VERTICAL:   mesh_add_range(mesh, dir, minx, maxx, mesh->dens_obj); break;
					default: break;
				}
				/* Note: holes can be ignored: holes are sorrunded by polygons, the grid is dense over them already */
			}
		}
	}
	return 0;
}

/* run mesh_gen_obj on all subc layers that match current board mesh layer */
static int mesh_gen_obj_subc(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	pcb_data_t *data = mesh->layer->parent.data;
	pcb_subc_t *sc;
	gdl_iterator_t it;

	subclist_foreach(&data->subc, &it, sc) {
		int n;
		pcb_layer_t *ly;
		for(n = 0, ly = sc->data->Layer; n < sc->data->LayerN; n++,ly++) {
			if (pcb_layer_get_real(ly) == mesh->layer) {
				if (mesh_gen_obj(mesh, ly, dir) != 0)
					return -1;
			}
		}
	}
	return 0;
}

static int cmp_coord(const void *v1, const void *v2)
{
	const rnd_coord_t *c1 = v1, *c2 = v2;
	return *c1 < *c2 ? -1 : +1;
}

static int cmp_range(const void *v1, const void *v2)
{
	const pcb_range_t *c1 = v1, *c2 = v2;
	return c1->begin < c2->begin ? -1 : +1;
}


typedef struct {
	rnd_coord_t min;
	rnd_coord_t max;
	int found;
} mesh_maybe_t;

static int cmp_maybe_add(const void *k, const void *v)
{
	const mesh_maybe_t *ctx = k;
	const rnd_coord_t *c = v;

	if ((*c >= ctx->min) && (*c <= ctx->max))
		return 0;
	if (*c < ctx->min)
		return +1;
	return -1;
}

static void mesh_maybe_add_edge(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t at, rnd_coord_t dist)
{
	mesh_maybe_t ctx;
	rnd_coord_t *c;
	ctx.min = at - dist;
	ctx.max = at + dist;
	ctx.found = 0;

	c = bsearch(&ctx, mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(rnd_coord_t), cmp_maybe_add);
	if (c == NULL) {
TODO(": optimization: run a second bsearch and insert instead of this; testing: 45 deg line (won't have axis aligned edge for the 2/3 1/3 rule)")
		vtc0_append(&mesh->line[dir].edge, at);
		qsort(mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(rnd_coord_t), cmp_coord);
	}
}

static int mesh_sort(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;
	pcb_range_t *r;

	if (vtr0_len(&mesh->line[dir].dens) < 1) {
		rnd_message(RND_MSG_ERROR, "There are not enough objects to do the meshing\n");
		return -1;
	}

	qsort(mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(rnd_coord_t), cmp_coord);
	qsort(mesh->line[dir].dens.array, vtr0_len(&mesh->line[dir].dens), sizeof(pcb_range_t), cmp_range);

	/* warn for edges too close */
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge)-1; n++) {
		rnd_coord_t c1 = mesh->line[dir].edge.array[n], c2 = mesh->line[dir].edge.array[n+1];
		if (c2 - c1 < mesh->min_space) {
			if ((c2 - c1) < RND_MM_TO_COORD(0.1)) {
				mesh->line[dir].edge.array[n] = (c1 + c2) / 2;
				vtc0_remove(&mesh->line[dir].edge, n+1, 1);
			}
			else
				rnd_message(RND_MSG_ERROR, "meshing error: invalid minimum spacing (%$mm) required: forced %s edges are closer than that around %$mm..%$mm; try decreasing your minimum spacing to below %$mm\n", mesh->min_space, dir == PCB_MESH_VERTICAL ? "vertical" : "horizonal", c1, c2, c2-c1);
		}
	}


	/* merge overlapping ranges of the same density */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens)-1; n++) {
		pcb_range_t *r1 = &mesh->line[dir].dens.array[n], *r2 = &mesh->line[dir].dens.array[n+1];
		if (r1->data[0].c != r2->data[0].c) continue;
		if (r2->begin < r1->end) {
			if (r2->end > r1->end)
				r1->end = r2->end;
			vtr0_remove(&mesh->line[dir].dens, n+1, 1);
			n--; /* make sure to check the next range against the current one, might be overlapping as well */
		}
	}

	/* continous ranges: fill in the gaps */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens)-1; n++) {
		pcb_range_t *r1 = &mesh->line[dir].dens.array[n], *r2 = &mesh->line[dir].dens.array[n+1];
		if (r1->end < r2->begin) {
			rnd_coord_t my_end = r2->begin; /* the insert will change r2 pointer */
			pcb_range_t *r = vtr0_alloc_insert(&mesh->line[dir].dens, n+1, 1);
			r->begin = r1->end;
			r->end = my_end;
			r->data[0].c = mesh->dens_gap;
			n++; /* no need to check the new block */
		}
	}

	/* make sure there's a forced mesh line at region transitions */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens); n++) {
		pcb_range_t *r = &mesh->line[dir].dens.array[n];
		if (n == 0)
			mesh_maybe_add_edge(mesh, dir, r->begin, mesh->dens_gap);
		mesh_maybe_add_edge(mesh, dir, r->end, mesh->dens_gap);
	}

	/* continous ranges: start and end */
	r = vtr0_alloc_insert(&mesh->line[dir].dens, 0, 1);
	r->begin = 0;
	r->end = mesh->line[dir].dens.array[1].begin;
	r->data[0].c = mesh->dens_gap;

	r = vtr0_alloc_append(&mesh->line[dir].dens, 1);
	r->begin = mesh->line[dir].dens.array[vtr0_len(&mesh->line[dir].dens)-2].end;
	r->end = (dir == PCB_MESH_HORIZONTAL) ? PCB->hidlib.size_y : PCB->hidlib.size_x;
	r->data[0].c = mesh->dens_gap;


	return 0;
}

static int cmp_range_at(const void *key_, const void *v_)
{
	const rnd_coord_t *key = key_;
	const pcb_range_t *v = v_;

	if ((*key >= v->begin) && (*key <= v->end))
		return 0;
	if (*key < v->begin) return -1;
	return +1;
}

static pcb_range_t *mesh_find_range(const vtr0_t *v, rnd_coord_t at, rnd_coord_t *dens, rnd_coord_t *dens_left, rnd_coord_t *dens_right)
{
	pcb_range_t *r;
	r = bsearch(&at, v->array, vtr0_len((vtr0_t *)v), sizeof(pcb_range_t), cmp_range_at);
	if (dens != NULL) {
		if (r == NULL)
			return NULL;
		*dens = r->data[0].c;
	}
	if (dens_left != NULL) {
		if (r == v->array)
			*dens_left = r->data[0].c;
		else
			*dens_left = r[-1].data[0].c;
	}
	if (dens_right != NULL) {
		if (r == v->array+v->used-1)
			*dens_right = r->data[0].c;
		else
			*dens_right = r[+1].data[0].c;
	}
	return r;
}

static int mesh_auto_z(pcb_mesh_t *mesh)
{
	rnd_layergrp_id_t gid;
	rnd_coord_t y = 0, ytop = 0, ybottom, top_dens, bottom_dens;
	int n, lns, first = 1;

	vtc0_truncate(&mesh->line[PCB_MESH_Z].result, 0);

	lns = num_subslines[ia.dlg[ia.subslines].val.lng];
	if (lns != 0) lns++;

	for(gid = 0; gid < PCB->LayerGroups.len; gid++) {
		pcb_layergrp_t *grp = &PCB->LayerGroups.grp[gid];
		if (grp->ltype & PCB_LYT_COPPER) {
			/* For now follow the old behavior and ignore the thickness of copper layers for now: copper sheets are modelled in 2d */
#if 0
			rnd_coord_t th = mesh_layergrp_thickness(grp, mesh->def_copper_thick);
			y += th;
			ybottom = y;
#endif
		}
		else if (grp->ltype & PCB_LYT_SUBSTRATE) {
			rnd_coord_t d, t = mesh_layergrp_thickness(grp, mesh->def_subs_thick);
			double dens = (double)t/(double)lns;
			bottom_dens = rnd_round(dens);
			if (lns != 0) {
				for(n = 0; n <= lns; n++) {
					if (n == 0) {
						if (first) {
							ytop = y;
							top_dens = rnd_round(dens);
							first = 0;
						}
						else
							continue;
					}
					d = rnd_round((double)y+dens*(double)n);
					mesh_add_result(mesh, PCB_MESH_Z, d);
				}
			}
			else {
				if (first) {
					ytop = y;
					first = 0;
					top_dens = mesh->def_subs_thick;
				}
				mesh_add_result(mesh, PCB_MESH_Z, y);
			}
			y += t;
			ybottom = y;
		}
	}

	if (ia.dlg[ia.air_top].val.lng) {
		if (ia.dlg[ia.smoothz].val.lng) {
			mesh_auto_add_smooth(&mesh->line[PCB_MESH_Z].result, ytop - ia.dlg[ia.max_air].val.crd, ytop,
				ia.dlg[ia.dens_air].val.crd, ia.dlg[ia.dens_air].val.crd, top_dens);
		}
		else {
			for(y = ytop; y > ytop - ia.dlg[ia.max_air].val.crd ; y -= ia.dlg[ia.dens_air].val.crd)
				mesh_add_result(mesh, PCB_MESH_Z, y);
		}
	}

	if (ia.dlg[ia.air_bot].val.lng) {
		if (ia.dlg[ia.smoothz].val.lng) {
			mesh_auto_add_smooth(&mesh->line[PCB_MESH_Z].result, ybottom, ybottom + ia.dlg[ia.max_air].val.crd,
				bottom_dens, ia.dlg[ia.dens_air].val.crd, ia.dlg[ia.dens_air].val.crd);
		}
		else {
			for(y = ybottom; y < ybottom + ia.dlg[ia.max_air].val.crd ; y += ia.dlg[ia.dens_air].val.crd)
				mesh_add_result(mesh, PCB_MESH_Z, y);
		}
	}

	mesh->z_bottom_copper = ybottom;

	return 0;
}

static void mesh_draw_line(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t at, rnd_coord_t aux1, rnd_coord_t aux2, rnd_coord_t thick)
{
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_line_new(mesh->ui_layer_xy, aux1, at, aux2, at, thick, 0, pcb_no_flags());
	else
		pcb_line_new(mesh->ui_layer_xy, at, aux1, at, aux2, thick, 0, pcb_no_flags());
}

static void mesh_draw_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t at1, rnd_coord_t at2, rnd_coord_t aux, rnd_coord_t thick)
{
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_line_new(mesh->ui_layer_xy, aux, at1, aux, at2, thick, 0, pcb_no_flags());
	else
		pcb_line_new(mesh->ui_layer_xy, at1, aux, at2, aux, thick, 0, pcb_no_flags());
}

static void mesh_draw_label(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, rnd_coord_t aux, const char *label)
{
	aux -= RND_MM_TO_COORD(0.6);
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_text_new(mesh->ui_layer_xy, pcb_font(PCB, 0, 0), aux, 0, 90, 75, 0, label, pcb_no_flags());
	else
		pcb_text_new(mesh->ui_layer_xy, pcb_font(PCB, 0, 0), 0, aux, 0, 75, 0, label, pcb_no_flags());

}

static int mesh_vis_xy(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;
	rnd_coord_t end;

	mesh_draw_label(mesh, dir, RND_MM_TO_COORD(0.1), "object edge");

	mesh_trace("%s edges:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge); n++) {
		mesh_trace(" %mm", mesh->line[dir].edge.array[n]);
		mesh_draw_line(mesh, dir, mesh->line[dir].edge.array[n], RND_MM_TO_COORD(0.1), RND_MM_TO_COORD(0.5), RND_MM_TO_COORD(0.1));
	}
	mesh_trace("\n");

	mesh_draw_label(mesh, dir, RND_MM_TO_COORD(2), "density ranges");

	mesh_trace("%s ranges:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens); n++) {
		pcb_range_t *r = &mesh->line[dir].dens.array[n];
		mesh_trace(" [%mm..%mm=%mm]", r->begin, r->end, r->data[0].c);
		mesh_draw_range(mesh, dir, r->begin, r->end, RND_MM_TO_COORD(2)+r->data[0].c/2, RND_MM_TO_COORD(0.05));
	}
	mesh_trace("\n");

	mesh_trace("%s result:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	end = (dir == PCB_MESH_HORIZONTAL) ? PCB->hidlib.size_x : PCB->hidlib.size_y;
	for(n = 0; n < vtc0_len(&mesh->line[dir].result); n++) {
		mesh_trace(" %mm", mesh->line[dir].result.array[n]);
		mesh_draw_line(mesh, dir, mesh->line[dir].result.array[n], 0, end, RND_MM_TO_COORD(0.03));
	}
	mesh_trace("\n");

	return 0;
}

static int mesh_vis_z(pcb_mesh_t *mesh)
{
	int n;
	rnd_layergrp_id_t gid;
	rnd_coord_t y0 = PCB->hidlib.size_y/3, y = y0, y2;
	rnd_coord_t xl = PCB->hidlib.size_x/5; /* board left */
	rnd_coord_t xr = PCB->hidlib.size_x/5*3; /* board right */
	rnd_coord_t spen = RND_MM_TO_COORD(0.3), cpen = RND_MM_TO_COORD(0.2), mpen = RND_MM_TO_COORD(0.03);
	int mag = 2;

	for(gid = 0; gid < PCB->LayerGroups.len; gid++) {
		pcb_layergrp_t *grp = &PCB->LayerGroups.grp[gid];
		if (grp->ltype & PCB_LYT_COPPER) {
			y2 = y + mesh->def_copper_thick * mag / 2;
			pcb_line_new(mesh->ui_layer_z, xr, y2, xr+RND_MM_TO_COORD(2), y2, cpen, 0, pcb_no_flags());
			pcb_text_new(mesh->ui_layer_z, pcb_font(PCB, 0, 0), xr+RND_MM_TO_COORD(3), y2 - RND_MM_TO_COORD(1), 0, 100, 0, grp->name, pcb_no_flags());
			y += mesh->def_copper_thick * mag;
		}
		else if (grp->ltype & PCB_LYT_SUBSTRATE) {
			y2 = y + mesh->def_subs_thick * mag;
			pcb_line_new(mesh->ui_layer_z, xl, y, xr, y, spen, 0, pcb_no_flags());
			pcb_line_new(mesh->ui_layer_z, xl, y2, xr, y2, spen, 0, pcb_no_flags());
			pcb_line_new(mesh->ui_layer_z, xl, y, xl, y2, spen, 0, pcb_no_flags());
			pcb_line_new(mesh->ui_layer_z, xr, y, xr, y2, spen, 0, pcb_no_flags());
			y = y2;
		}
	}

	mesh_trace("Z lines:\n");
	for(n = 0; n < vtc0_len(&mesh->line[PCB_MESH_Z].result); n++) {
		rnd_coord_t y = y0+mesh->line[PCB_MESH_Z].result.array[n]*mag;
		mesh_trace(" %mm", y);
		pcb_line_new(mesh->ui_layer_z, 0, y, PCB->hidlib.size_x, y, mpen, 0, pcb_no_flags());
	}
	mesh_trace("\n");
	return 0;
}


static void mesh_auto_add_even(vtc0_t *v, rnd_coord_t c1, rnd_coord_t c2, rnd_coord_t d)
{
	long num = (c2 - c1) / d;

	if (num < 1)
		return;

	d = (c2 - c1)/(num+1);
	if (d > 0) {
		c2 -= d/4; /* open on the right, minus rounding errors */
		for(; c1 < c2; c1 += d)
			vtc0_append(v, c1);
	}
}

static rnd_coord_t mesh_auto_add_interp(vtc0_t *v, rnd_coord_t c, rnd_coord_t d1, rnd_coord_t d2, rnd_coord_t dd)
{
	if (dd > 0) {
		for(; d1 <= d2; d1 += dd) {
			vtc0_append(v, c);
			c += d1;
		}
		return c;
	}
	else {
		for(; d1 <= d2; d1 -= dd) {
			c -= d1;
			vtc0_append(v, c);
		}
		return c;
	}

}

static void mesh_auto_add_smooth(vtc0_t *v, rnd_coord_t c1, rnd_coord_t c2, rnd_coord_t d1, rnd_coord_t d, rnd_coord_t d2)
{
	rnd_coord_t len = c2 - c1, begin = c1, end = c2, glen;
	int lines;

	/* ramp up (if there's room) */
	if (d > d1) {
		lines = (d / d1) + 0;
		if (lines > 0) {
			glen = lines * d;
			if (glen < len/4)
				begin = mesh_auto_add_interp(v, c1, d1, d, (d-d1)/lines);
		}
		else
			begin = c1;
	}

	/* ramp down (if there's room) */
	if (d > d2) {
		lines = (d / d2) + 0;
		if (lines > 0) {
			glen = lines * d;
			if (glen < len/4)
				end = mesh_auto_add_interp(v, c2, d2, d, -(d-d2)/lines);
		}
		else
			end = c2;
	}

	/* middle section: linear */
	mesh_auto_add_even(v, begin, end, d);
}

static int mesh_auto_build(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;
	rnd_coord_t c1, c2;
	rnd_coord_t d1, d, d2;

	mesh_trace("build:\n");

	/* left edge, before the first known line */
	if (!mesh->noimpl) {
		c1 = 0;
		c2 = mesh->line[dir].edge.array[0];
		mesh_find_range(&mesh->line[dir].dens, (c1+c2)/2, &d, &d1, &d2);
		if (mesh->smooth)
			mesh_auto_add_smooth(&mesh->line[dir].result, c1, c2, d1, d, d2);
		else
			mesh_auto_add_even(&mesh->line[dir].result, c1, c2, d);
	}

	/* normal, between known lines */
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge); n++) {
		c1 = mesh->line[dir].edge.array[n];
		c2 = mesh->line[dir].edge.array[n+1];

		vtc0_append(&mesh->line[dir].result, c1);

		if (c2 - c1 < mesh->dens_obj / 2)
			continue; /* don't attempt to insert lines where it won't fit */

		mesh_find_range(&mesh->line[dir].dens, (c1+c2)/2, &d, &d1, &d2);
		if (c2 - c1 < d * 2)
			continue; /* don't attempt to insert lines where it won't fit */

		mesh_trace(" %mm..%mm %mm,%mm,%mm\n", c1, c2, d1, d, d2);

		if (mesh->noimpl)
			continue;

		/* place mesh lines between c1 and c2 */
		if (mesh->smooth)
			mesh_auto_add_smooth(&mesh->line[dir].result, c1, c2, d1, d, d2);
		else
			mesh_auto_add_even(&mesh->line[dir].result, c1, c2, d);
	}

	/* right edge, after the last known line */
	if (!mesh->noimpl) {
		c1 = mesh->line[dir].edge.array[vtc0_len(&mesh->line[dir].edge)-1];
		c2 = (dir == PCB_MESH_HORIZONTAL) ? PCB->hidlib.size_y : PCB->hidlib.size_x;
		mesh_find_range(&mesh->line[dir].dens, (c1+c2)/2, &d, &d1, &d2);
		if (mesh->smooth)
			mesh_auto_add_smooth(&mesh->line[dir].result, c1, c2, d1, d, d2);
		else
			mesh_auto_add_even(&mesh->line[dir].result, c1, c2, d);

		vtc0_append(&mesh->line[dir].result, c2); /* ranges are open from the end, need to manually place the last */
	}

	mesh_trace("\n");
	return 0;
}

int mesh_auto(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	vtc0_truncate(&mesh->line[dir].edge, 0);
	vtr0_truncate(&mesh->line[dir].dens, 0);
	vtc0_truncate(&mesh->line[dir].result, 0);

	if (mesh_gen_obj(mesh, mesh->layer, dir) != 0)
		return -1;
	if (mesh_gen_obj_subc(mesh, dir) != 0)
		return -1;
	if (mesh_sort(mesh, dir) != 0)
		return -1;
	if (mesh_auto_build(mesh, dir) != 0)
		return -1;

	if (mesh->ui_layer_xy != NULL)
		mesh_vis_xy(mesh, dir);

	return 0;
}

static void mesh_layer_reset()
{
	static rnd_color_t clr;

	if (clr.str[0] != '#')
		rnd_color_load_str(&clr, "#007733");

	if (mesh.ui_layer_xy != NULL)
		pcb_uilayer_free(mesh.ui_layer_xy);
	if (mesh.ui_layer_z != NULL)
		pcb_uilayer_free(mesh.ui_layer_z);
	mesh.ui_layer_xy = pcb_uilayer_alloc(mesh_ui_cookie, "mesh xy", &clr);
	mesh.ui_layer_z = pcb_uilayer_alloc(mesh_ui_cookie, "mesh z", &clr);
}

static void ia_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	RND_DAD_FREE(ia.dlg);
	memset(&ia, 0, sizeof(ia));
}

static char *default_file = NULL;
static void ia_save_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char *fname = NULL;
	FILE *f;
	gds_t tmp;

	fname = rnd_gui->fileselect(rnd_gui, "Save mesh settings...",
															"Picks file for saving mesh settings.\n",
															default_file, ".lht", NULL, "mesh", RND_HID_FSD_MAY_NOT_EXIST, NULL);
	if (fname == NULL)
		return; /* cancel */

	if (default_file != NULL) {
		free(default_file);
		default_file = rnd_strdup(fname);
	}

	f = rnd_fopen_askovr(&PCB->hidlib, fname, "w", NULL);
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can not open '%s' for write\n", fname);
		return;
	}

	gds_init(&tmp);
	pcb_mesh_save(&ia, &tmp, NULL);
	fprintf(f, "%s", tmp.array);
	gds_uninit(&tmp);
	free(fname);
	fclose(f);
}

static int ia_load_file(const char *fname)
{
	FILE *f;
	int res;

	f = rnd_fopen(&PCB->hidlib, fname, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "Can not open '%s' for read\n", fname);
		return -1;
	}

	res = mesh_load_file(&ia, f);
	if (res != 0)
		rnd_message(RND_MSG_ERROR, "Loading mesh settings from '%s' failed.\n", fname);
	fclose(f);
	return res;
}

static void ia_load_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	char *fname = NULL;

	fname = rnd_gui->fileselect(rnd_gui, "Load mesh settings...",
															"Picks file for loading mesh settings from.\n",
															default_file, ".lht", NULL, "mesh", RND_HID_FSD_READ, NULL);
	if (fname == NULL)
		return; /* cancel */

	if (default_file != NULL) {
		free(default_file);
		default_file = rnd_strdup(fname);
	}
	ia_load_file(fname);
}

static void mesh_sort_uniq(pcb_mesh_lines_t *l)
{
		qsort(l->result.array, vtc0_len(&l->result), sizeof(rnd_coord_t), cmp_coord);
}

static void mesh_sort_uniq_all(pcb_mesh_t *mesh)
{
	mesh_sort_uniq(&mesh->line[PCB_MESH_HORIZONTAL]);
	mesh_sort_uniq(&mesh->line[PCB_MESH_VERTICAL]);
	mesh_sort_uniq(&mesh->line[PCB_MESH_Z]);
}

static void ia_gen(void)
{
	mesh_layer_reset();
	mesh.layer = PCB_CURRLAYER(PCB);

	if (ia.dlg[ia.hor].val.lng)
		mesh_auto(&mesh, PCB_MESH_HORIZONTAL);
	if (ia.dlg[ia.ver].val.lng)
		mesh_auto(&mesh, PCB_MESH_VERTICAL);

	mesh_auto_z(&mesh);
	if (mesh.ui_layer_z != NULL)
		mesh_vis_z(&mesh);

	mesh_sort_uniq_all(&mesh);

	free(mesh.ui_name_xy);
	free((char *)mesh.ui_layer_xy->name); /* we have strdup'd it */
	mesh.ui_name_xy = rnd_strdup_printf("mesh 0: %s", mesh.layer->name);
	mesh.ui_layer_xy->name = rnd_strdup(mesh.ui_name_xy);
	rnd_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);

	rnd_gui->invalidate_all(rnd_gui);
}

static void ia_gen_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{

	dlg2mesh();
	ia_gen();
}

pcb_mesh_t *pcb_mesh_get(const char *name)
{
	return &mesh;
}

int pcb_mesh_interactive(void)
{
	int n;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (ia.active)
		return 0;

	RND_DAD_BEGIN_VBOX(ia.dlg);
		RND_DAD_BEGIN_HBOX(ia.dlg);
			RND_DAD_BEGIN_VBOX(ia.dlg);
				RND_DAD_COMPFLAG(ia.dlg, RND_HATF_FRAME);
				RND_DAD_LABEL(ia.dlg, "XY-mesh");
				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.dens_obj = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "copper dens.");
					RND_DAD_HELP(ia.dlg, "mesh density over copper");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.dens_gap = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "gap dens.");
					RND_DAD_HELP(ia.dlg, "mesh density over gaps");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.min_space = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "min. spacing");
					RND_DAD_HELP(ia.dlg, "minimum distance between mesh lines");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.smooth = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "smooth mesh");
					RND_DAD_HELP(ia.dlg, "avoid jumps between different mesh densities,\nuse smooth (gradual) changes");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.hor = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "horizontal");
					RND_DAD_HELP(ia.dlg, "enable adding horizontal mesh lines");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.ver = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "vertical");
					RND_DAD_HELP(ia.dlg, "enable adding vertical mesh lines");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.noimpl = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "omit implicit");
					RND_DAD_HELP(ia.dlg, "add only the mesh lines for boundaries,\nomit in-material meshing");
				RND_DAD_END(ia.dlg);
			RND_DAD_END(ia.dlg);

			RND_DAD_BEGIN_VBOX(ia.dlg);
				RND_DAD_COMPFLAG(ia.dlg, RND_HATF_FRAME);
				RND_DAD_LABEL(ia.dlg, "Z-mesh");

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_ENUM(ia.dlg, subslines);
						ia.subslines = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "num in substrate");
					RND_DAD_HELP(ia.dlg, "number of mesh lines in substrate");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.def_subs_thick = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "def. subst. thick");
					RND_DAD_HELP(ia.dlg, "default substrate thickness\n(for substrate layer groups without\nthickness specified in attribute)");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.def_copper_thick = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "def. copper thick");
					RND_DAD_HELP(ia.dlg, "default copper thickness\n(for copper layer groups without\nthickness specified in attribute)");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.air_top = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "in air top");
					RND_DAD_HELP(ia.dlg, "add mesh lines in air above the top of the board");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.air_bot = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "in air bottom");
					RND_DAD_HELP(ia.dlg, "add mesh lines in air below the bottom of the board");
				RND_DAD_END(ia.dlg);


				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.dens_air = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "dens. air");
					RND_DAD_HELP(ia.dlg, "mesh line density (spacing) in air");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_COORD(ia.dlg);
						ia.max_air = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, RND_MM_TO_COORD(5));
					RND_DAD_LABEL(ia.dlg, "air thickness");
					RND_DAD_HELP(ia.dlg, "how far out to mesh in air");
				RND_DAD_END(ia.dlg);

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_BOOL(ia.dlg);
						ia.smoothz = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "smooth mesh");
					RND_DAD_HELP(ia.dlg, "avoid jumps between different mesh densities,\nuse smooth (gradual) changes");
				RND_DAD_END(ia.dlg);

			RND_DAD_END(ia.dlg);
		RND_DAD_END(ia.dlg);

		RND_DAD_BEGIN_HBOX(ia.dlg);
			RND_DAD_BEGIN_VBOX(ia.dlg);
				RND_DAD_COMPFLAG(ia.dlg, RND_HATF_FRAME);
				RND_DAD_LABEL(ia.dlg, "Boundary");
				for(n = 0; n < 6; n+=2) {
					char name[64];
					sprintf(name, "%s %s", bnd_names[n], bnd_names[n+1]);
					RND_DAD_BEGIN_HBOX(ia.dlg);
						RND_DAD_ENUM(ia.dlg, bnds);
							ia.bnd[n] = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_LABEL(ia.dlg, name);
						RND_DAD_ENUM(ia.dlg, bnds);
							ia.bnd[n+1] = RND_DAD_CURRENT(ia.dlg);
					RND_DAD_END(ia.dlg);
				}

				RND_DAD_BEGIN_HBOX(ia.dlg);
					RND_DAD_LABEL(ia.dlg, "PML cells:");
					RND_DAD_INTEGER(ia.dlg);
						ia.pml = RND_DAD_CURRENT(ia.dlg);
						RND_DAD_MINMAX(ia.dlg, 0, 32);
						RND_DAD_DEFAULT_NUM(ia.dlg, 8);
				RND_DAD_END(ia.dlg);
			RND_DAD_END(ia.dlg);

			RND_DAD_BEGIN_VBOX(ia.dlg);
				RND_DAD_BUTTON(ia.dlg, "Save to file");
					RND_DAD_CHANGE_CB(ia.dlg, ia_save_cb);
				RND_DAD_BUTTON(ia.dlg, "Load from file");
					RND_DAD_CHANGE_CB(ia.dlg, ia_load_cb);
				RND_DAD_BUTTON(ia.dlg, "Generate mesh!");
					RND_DAD_CHANGE_CB(ia.dlg, ia_gen_cb);
			RND_DAD_END(ia.dlg);
		RND_DAD_END(ia.dlg);
		RND_DAD_BUTTON_CLOSES(ia.dlg, clbtn);
	RND_DAD_END(ia.dlg);

	RND_DAD_NEW("mesh", ia.dlg, "mesher", &ia, 0, ia_close_cb);
	ia.active = 1;

	mesh2dlg();
	return 0;
}

const char pcb_acts_mesh[] = "mesh()";
const char pcb_acth_mesh[] = "generate a mesh for simulation";
fgw_error_t pcb_act_mesh(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_mesh_interactive();
	RND_ACT_IRES(0);
	return 0;
}
