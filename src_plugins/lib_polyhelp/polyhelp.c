/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <math.h>

#include "polyhelp.h"
#include "polygon.h"
#include <librnd/core/plugins.h>
#include <librnd/core/rnd_printf.h>
#include "obj_line.h"
#include <librnd/core/box.h>
#include <librnd/poly/offset.h>
#include <librnd/core/hid_dad.h>

#include "topoly.h"

/* for the action: */
static const char *polyhelp_cookie = "lib_polyhelp";
#include <string.h>
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/actions.h>

void pcb_pline_fprint_anim(FILE *f, const pcb_pline_t *pl)
{
	const pcb_vnode_t *v, *n;
	fprintf(f, "!pline start\n");
	v = pl->head;
	do {
		n = v->next;
		rnd_fprintf(f, "line %#mm %#mm %#mm %#mm\n", v->point[0], v->point[1], n->point[0], n->point[1]);
	}
	while((v = v->next) != pl->head);
	fprintf(f, "!pline end\n");
}

#if 0
/* debug helper */
static void cross(FILE *f, rnd_coord_t x, rnd_coord_t y)
{
	static rnd_coord_t cs = RND_MM_TO_COORD(0.2);
	rnd_fprintf(f, "line %#mm %#mm %#mm %#mm\n", x - cs, y, x + cs, y);
	rnd_fprintf(f, "line %#mm %#mm %#mm %#mm\n", x, y - cs, x, y + cs);
}
#endif

rnd_cardinal_t pcb_pline_to_lines(pcb_layer_t *dst, const pcb_pline_t *src, rnd_coord_t thickness, rnd_coord_t clearance, pcb_flag_t flags)
{
	rnd_cardinal_t cnt = 0;
	vtp0_t tracks;
	long i;

	vtp0_init(&tracks);
	rnd_pline_dup_offsets(&tracks, src, -((thickness/2)+1));

	for(i = 0; i < tracks.used; i++) {
		const pcb_vnode_t *v, *n;
		pcb_pline_t *track = tracks.array[i];

		v = track->head;
		do {
			n = v->next;
			pcb_line_new(dst, v->point[0], v->point[1], n->point[0], n->point[1], thickness, clearance, flags);
			cnt++;
		}
		while((v = v->next) != track->head);
		pcb_poly_contour_del(&track);
	}

	vtp0_uninit(&tracks);
	return cnt;
}

rnd_bool pcb_pline_is_aligned(const pcb_pline_t *src)
{
	const pcb_vnode_t *v, *n;

	v = src->head;
	do {
		n = v->next;
		if ((v->point[0] != n->point[0]) && (v->point[1] != n->point[1]))
			return rnd_false;
	}
	while((v = v->next) != src->head);
	return rnd_true;
}


rnd_bool pcb_cpoly_is_simple_rect(const pcb_poly_t *p)
{
	if (p->Clipped->f != p->Clipped)
		return rnd_false; /* more than one islands */
	if (p->Clipped->contours->next != NULL)
		return rnd_false; /* has holes */
	return pcb_pline_is_rectangle(p->Clipped->contours);
}

rnd_cardinal_t pcb_cpoly_num_corners(const pcb_poly_t *src)
{
	rnd_cardinal_t res = 0;
	pcb_poly_it_t it;
	rnd_polyarea_t *pa;


	for(pa = pcb_poly_island_first(src, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		pcb_pline_t *pl;

		pl = pcb_poly_contour(&it);
		if (pl != NULL) { /* we have a contour */
			res += pl->Count;
			for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it))
				res += pl->Count;
		}
	}

	return res;
}

static void add_track_seg(pcb_cpoly_edgetree_t *dst, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pcb_cpoly_edge_t *e = &dst->edges[dst->used++];
	rnd_rnd_box_t *b = &e->bbox;

	if (x1 <= x2) {
		b->X1 = x1;
		b->X2 = x2;
	}
	else {
		b->X1 = x2;
		b->X2 = x1;
	}
	if (y1 <= y2) {
		b->Y1 = y1;
		b->Y2 = y2;
	}
	else {
		b->Y1 = y2;
		b->Y2 = y1;
	}

	e->x1 = x1;
	e->y1 = y1;
	e->x2 = x2;
	e->y2 = y2;

	rnd_box_bump_box(&dst->bbox, b);
	pcb_r_insert_entry(dst->edge_tree, (rnd_rnd_box_t *)e);
}

static void add_track(pcb_cpoly_edgetree_t *dst, pcb_pline_t *track)
{
	int go, first = 1;
	rnd_coord_t x, y, px, py;
	pcb_poly_it_t it;

	it.cntr = track;
	for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y)) {
		if (!first)
			add_track_seg(dst, px, py, x, y);
		first = 0;
		px = x;
		py = y;
	}

	pcb_poly_vect_first(&it, &x, &y);
	add_track_seg(dst, px, py, x, y);
}


/* collect all edge lines (contour and holes) in an rtree, calculate the bbox */
pcb_cpoly_edgetree_t *pcb_cpoly_edgetree_create(const pcb_poly_t *src, rnd_coord_t offs)
{
	pcb_poly_it_t it;
	rnd_polyarea_t *pa;
	pcb_cpoly_edgetree_t *res;
	rnd_cardinal_t alloced = pcb_cpoly_num_corners(src) * sizeof(pcb_cpoly_edge_t);

	res = malloc(sizeof(pcb_cpoly_edgetree_t) + alloced);

	res->alloced = alloced;
	res->used = 0;
	res->edge_tree = pcb_r_create_tree();
	res->bbox.X1 = res->bbox.Y1 = RND_MAX_COORD;
	res->bbox.X2 = res->bbox.Y2 = -RND_MAX_COORD;

	for(pa = pcb_poly_island_first(src, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		pcb_pline_t *pl, *track;

		pl = pcb_poly_contour(&it);
		if (pl != NULL) { /* we have a contour */
			track = rnd_pline_dup_offset(pl, -offs);
			add_track(res, track);
			pcb_poly_contour_del(&track);

			for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it)) {
				track = rnd_pline_dup_offset(pl, -offs);
				add_track(res, track);
				pcb_poly_contour_del(&track);
			}
		}
	}

	return res;
}

void pcb_cpoly_edgetree_destroy(pcb_cpoly_edgetree_t *etr)
{
	pcb_r_destroy_tree(&etr->edge_tree);
	free(etr);
}

typedef struct {
	int used;
	rnd_coord_t at;
	rnd_coord_t coord[1];
} intersect_t;

static pcb_r_dir_t pcb_cploy_hatch_edge_hor(const rnd_rnd_box_t *region, void *cl)
{
	intersect_t *is = (intersect_t *)cl;
	pcb_cpoly_edge_t *e = (pcb_cpoly_edge_t *)region;

	if (e->y1 != e->y2) {
		/* consider only non-horizontal edges */
		if (e->x1 != e->x2) {
			double y = ((double)e->x2 - (double)e->x1) / ((double)e->y2 - (double)e->y1) * ((double)is->at - (double)e->y1) + (double)e->x1;
			is->coord[is->used] = y;
		}
		else
			is->coord[is->used] = e->x1; /* faster method for vertical */
		is->used++;
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t pcb_cploy_hatch_edge_ver(const rnd_rnd_box_t *region, void *cl)
{
	intersect_t *is = (intersect_t *)cl;
	pcb_cpoly_edge_t *e = (pcb_cpoly_edge_t *)region;

	if (e->x1 != e->x2) {
		/* consider only non-vertical edges */
		if (e->y1 != e->y2) {
			double x = ((double)e->y2 - (double)e->y1) / ((double)e->x2 - (double)e->x1) * ((double)is->at - (double)e->x1) + (double)e->y1;
			is->coord[is->used] = x;
		}
		else
			is->coord[is->used] = e->y1; /* faster method for vertical */
		is->used++;
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

static int coord_cmp(const void *p1, const void *p2)
{
	const rnd_coord_t *c1 = p1, *c2 = p2;
	if (*c1 < *c2)
		return -1;
	return 1;
}


void pcb_cpoly_hatch(const pcb_poly_t *src, pcb_cpoly_hatchdir_t dir, rnd_coord_t offs, rnd_coord_t period, void *ctx, void (*cb)(void *ctx, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2))
{
	pcb_cpoly_edgetree_t *etr;
	rnd_rnd_box_t scan;
	int n;
	intersect_t *is;

	if (dir == 0)
		return;

	etr = pcb_cpoly_edgetree_create(src, offs);

	is = malloc(sizeof(intersect_t) + sizeof(rnd_coord_t) * etr->alloced);

	if (dir & PCB_CPOLY_HATCH_HORIZONTAL) {
		rnd_coord_t y;

		for(y = etr->bbox.Y1; y <= etr->bbox.Y2; y += period) {
			scan.X1 = -RND_MAX_COORD;
			scan.X2 = RND_MAX_COORD;
			scan.Y1 = y;
			scan.Y2 = y+1;

			is->used = 0;
			is->at = y;
			pcb_r_search(etr->edge_tree, &scan, NULL, pcb_cploy_hatch_edge_hor, is, NULL);
			qsort(is->coord, is->used, sizeof(rnd_coord_t), coord_cmp);
			for(n = 1; n < is->used; n+=2) /* call the callback for the odd scan lines */
				cb(ctx, is->coord[n-1], y, is->coord[n], y);
		}
	}

	if (dir & PCB_CPOLY_HATCH_VERTICAL) {
		rnd_coord_t x;

		for(x = etr->bbox.X1; x <= etr->bbox.X2; x += period) {
			scan.Y1 = -RND_MAX_COORD;
			scan.Y2 = RND_MAX_COORD;
			scan.X1 = x;
			scan.X2 = x+1;

			is->used = 0;
			is->at = x;
			pcb_r_search(etr->edge_tree, &scan, NULL, pcb_cploy_hatch_edge_ver, is, NULL);
			qsort(is->coord, is->used, sizeof(rnd_coord_t), coord_cmp);
			for(n = 1; n < is->used; n+=2) /* call the callback for the odd scan lines */
				cb(ctx, x, is->coord[n-1], x, is->coord[n]);
		}
	}

	free(is);
	pcb_cpoly_edgetree_destroy(etr);
}


typedef struct {
	pcb_layer_t *dst;
	rnd_coord_t thickness, clearance;
	pcb_flag_t flags;
} hatch_ctx_t;

static void hatch_cb(void *ctx_, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	hatch_ctx_t *ctx = (hatch_ctx_t *)ctx_;
	pcb_line_new(ctx->dst, x1, y1, x2, y2, ctx->thickness, ctx->clearance, ctx->flags);
}

void pcb_cpoly_hatch_lines(pcb_layer_t *dst, const pcb_poly_t *src, pcb_cpoly_hatchdir_t dir, rnd_coord_t period, rnd_coord_t thickness, rnd_coord_t clearance, pcb_flag_t flags)
{
	hatch_ctx_t ctx;


	ctx.dst = dst;
	ctx.thickness = thickness;
	ctx.clearance = clearance;
	ctx.flags = flags;

	pcb_cpoly_hatch(src, dir, (thickness/2)+1, period, &ctx, hatch_cb);
}

static int polyhatch_gui(rnd_coord_t *period, pcb_cpoly_hatchdir_t *dir, pcb_flag_t *flg, int *want_contour)
{
	int wspc, wcont, whor, wver, wclr;
	rnd_hid_dad_buttons_t clbtn[] = {{"Cancel", -1}, {"ok", 0}, {NULL, 0}};
	RND_DAD_DECL(dlg);

	RND_DAD_BEGIN_VBOX(dlg);
		RND_DAD_BEGIN_TABLE(dlg, 2);

			RND_DAD_LABEL(dlg, "Spacing");
			RND_DAD_COORD(dlg, "");
				RND_DAD_HELP(dlg, "Distance between centerlines of adjacent hatch lines for vertical and horizontal hatching");
				RND_DAD_DEFAULT_NUM(dlg, conf_core.design.line_thickness * 2);
				RND_DAD_MINMAX(dlg, 1, RND_MM_TO_COORD(100));
				wspc = RND_DAD_CURRENT(dlg);

			RND_DAD_LABEL(dlg, "Draw contour");
			RND_DAD_BOOL(dlg, "");
				RND_DAD_HELP(dlg, "Draw the contour of the polygon");
				wcont = RND_DAD_CURRENT(dlg);

			RND_DAD_LABEL(dlg, "Draw horizontal hatch");
			RND_DAD_BOOL(dlg, "");
				RND_DAD_HELP(dlg, "Draw evenly spaced horizontal hatch lines");
				whor = RND_DAD_CURRENT(dlg);

			RND_DAD_LABEL(dlg, "Draw vertical hatch");
			RND_DAD_BOOL(dlg, "");
				RND_DAD_HELP(dlg, "Draw evenly spaced vertical hatch lines");
				wver = RND_DAD_CURRENT(dlg);

			RND_DAD_LABEL(dlg, "Clear-line");
			RND_DAD_BOOL(dlg, "");
				RND_DAD_HELP(dlg, "Hatch lines have clearance");
				wclr = RND_DAD_CURRENT(dlg);

		RND_DAD_END(dlg);
		RND_DAD_BUTTON_CLOSES(dlg, clbtn);
	RND_DAD_END(dlg);

	RND_DAD_NEW("poly_hatch", dlg, "Polygon hatch", NULL, rnd_true, NULL);
	if (RND_DAD_RUN(dlg) != 0) {
	/* cancel */
		RND_DAD_FREE(dlg);
		return -1;
	}

	*period = dlg[wspc].val.crd;
	if (dlg[wcont].val.lng) *want_contour = 1;

	*dir = 0;
	if (dlg[whor].val.lng) *dir |= PCB_CPOLY_HATCH_HORIZONTAL;
	if (dlg[wver].val.lng) *dir |= PCB_CPOLY_HATCH_VERTICAL;

	*flg = pcb_flag_make(dlg[wclr].val.lng ? PCB_FLAG_CLEARLINE : 0);

	RND_DAD_FREE(dlg);
	return 0;
}

static const char pcb_acts_PolyHatch[] = "PolyHatch([spacing], [hvcp])\nPolyHatch(interactive)\n";
static const char pcb_acth_PolyHatch[] = "hatch the selected polygon(s) with lines of the current style; lines are drawn on the current layer; flags are h:horizontal, v:vertical, c:contour, p:poly";
static fgw_error_t pcb_act_PolyHatch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op, *arg = NULL;
	rnd_coord_t period = 0;
	pcb_cpoly_hatchdir_t dir = 0;
	pcb_flag_t flg;
	int want_contour = 0, want_poly = 0, cont_specd = 0;

	RND_ACT_CONVARG(1, FGW_STR, PolyHatch, op = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, PolyHatch, arg = argv[2].val.str);

	if (rnd_strcasecmp(op, "interactive") == 0) {
		if (polyhatch_gui(&period, &dir, &flg, &want_contour) != 0) {
			RND_ACT_IRES(1);
			return 0;
		}
	}
	else {
		if (op != NULL) {
			rnd_bool succ;
			period = rnd_get_value(op, NULL, NULL, &succ);
			if (!succ) {
				rnd_message(RND_MSG_ERROR, "Invalid spacing value - must be a coordinate\n");
				return -1;
			}
		}

		if (arg != NULL) {
			if (strchr(arg, 'c')) want_contour = 1;
			if (strchr(arg, 'p')) want_poly = 1;
			if (strchr(arg, 'h')) dir |= PCB_CPOLY_HATCH_HORIZONTAL;
			if (strchr(arg, 'v')) dir |= PCB_CPOLY_HATCH_VERTICAL;
			cont_specd = 1;
		}

		if (cont_specd == 0) {
			dir = PCB_CPOLY_HATCH_HORIZONTAL | PCB_CPOLY_HATCH_VERTICAL;
			want_contour = 1;
		}
		if (period == 0)
			period = conf_core.design.line_thickness * 2;

		flg = pcb_flag_make(conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0);
	}

	PCB_POLY_ALL_LOOP(PCB->Data); {
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
			continue;
		if (want_contour) {
			pcb_poly_it_t it;
			rnd_polyarea_t *pa;
			for(pa = pcb_poly_island_first(polygon, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
				pcb_pline_t *pl = pcb_poly_contour(&it);
				if (pl != NULL) { /* we have a contour */
					pcb_pline_to_lines(PCB_CURRLAYER(PCB), pl, conf_core.design.line_thickness, conf_core.design.line_thickness * 2, flg);
					for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it))
						pcb_pline_to_lines(PCB_CURRLAYER(PCB), pl, conf_core.design.line_thickness, conf_core.design.line_thickness * 2, flg);
				}
			}
		}
		if (want_poly) {
			pcb_poly_t *p = pcb_poly_new_from_poly(PCB_CURRLAYER(PCB), polygon, period, polygon->Clearance, polygon->Flags);
			PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, p);
		}
		pcb_cpoly_hatch_lines(PCB_CURRLAYER(PCB), polygon, dir, period, conf_core.design.line_thickness, conf_core.design.line_thickness * 2, flg);
	} PCB_ENDALL_LOOP;

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PolyOffs[] = "PolyOffs(offset)\n";
static const char pcb_acth_PolyOffs[] = "replicate the outer contour of the selected polygon(s) with growing or shrinking them by offset; the new polygon is drawn on the current layer";
static fgw_error_t pcb_act_PolyOffs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t offs;

	RND_ACT_CONVARG(1, FGW_COORD, PolyOffs, offs = fgw_coord(&argv[1]));

	PCB_POLY_ALL_LOOP(PCB->Data); {
		pcb_poly_t *p;
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
			continue;

		p = pcb_poly_new_from_poly(PCB_CURRLAYER(PCB), polygon, offs, polygon->Clearance, polygon->Flags);
		PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, p);
	} PCB_ENDALL_LOOP;

	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t polyhelp_action_list[] = {
	{"PolyHatch", pcb_act_PolyHatch, pcb_acth_PolyHatch, pcb_acts_PolyHatch},
	{"PolyOffs", pcb_act_PolyOffs, pcb_acth_PolyOffs, pcb_acts_PolyOffs},
	{"ToPoly", pcb_act_topoly, pcb_acth_topoly, pcb_acts_topoly}
};

int pplg_check_ver_lib_polyhelp(int ver_needed) { return 0; }

void pplg_uninit_lib_polyhelp(void)
{
	rnd_remove_actions_by_cookie(polyhelp_cookie);
}

int pplg_init_lib_polyhelp(void)
{
	RND_API_CHK_VER;

	RND_REGISTER_ACTIONS(polyhelp_action_list, polyhelp_cookie);
	return 0;
}
