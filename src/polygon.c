/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2010 Thomas Nau
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
 */

/* pcb-rnd specific polygon editing routines
   see doc/developer/polygon.html for more info */

#include "config.h"
#include "conf_core.h"

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <setjmp.h>

#include "board.h"
#include <librnd/core/box.h>
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "polygon.h"
#include "remove.h"
#include "search.h"
#include "thermal.h"
#include "undo.h"
#include "layer.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"
#include <librnd/poly/self_isc.h>
#include <librnd/poly/polygon1_gen.h>
#include "event.h"

#define UNSUBTRACT_BLOAT 10
#define SUBTRACT_PIN_VIA_BATCH_SIZE 100
#define SUBTRACT_PADSTACK_BATCH_SIZE 50
#define SUBTRACT_LINE_BATCH_SIZE 20

#define sqr(x) ((x)*(x))

#undef min
#undef max
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

static int Unsubtract(rnd_polyarea_t * np1, pcb_poly_t * p);

static const char *polygon_cookie = "core polygon";


void pcb_poly_layers_chg(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_layer_t *ly;
	pcb_data_t *data;

	if ((argc < 2) || (argv[1].type != RND_EVARG_PTR))
		return;
	ly = argv[1].d.p;
	if ((ly->is_bound) || (ly->parent_type != PCB_PARENT_DATA))
		return;

	data = ly->parent.data;
	pcb_data_clip_inhibit_inc(data);
	PCB_POLY_LOOP(ly); {
		polygon->clip_dirty = 1;
	}
	PCB_END_LOOP;
	pcb_data_clip_inhibit_dec(data, 1);
}

void pcb_polygon_init(void)
{
	rnd_event_bind(PCB_EVENT_LAYER_CHANGED_GRP, pcb_poly_layers_chg, NULL, polygon_cookie);
}

void pcb_polygon_uninit(void)
{
	rnd_event_unbind_allcookie(polygon_cookie);
}

rnd_cardinal_t pcb_poly_point_idx(pcb_poly_t *polygon, rnd_point_t *point)
{
	assert(point >= polygon->Points);
	assert(point <= polygon->Points + polygon->PointN);
	return ((char *) point - (char *) polygon->Points) / sizeof(rnd_point_t);
}

/* Find contour number: 0 for outer, 1 for first hole etc.. */
rnd_cardinal_t pcb_poly_contour_point(pcb_poly_t *polygon, rnd_cardinal_t point)
{
	rnd_cardinal_t i;
	rnd_cardinal_t contour = 0;

	for (i = 0; i < polygon->HoleIndexN; i++)
		if (point >= polygon->HoleIndex[i])
			contour = i + 1;
	return contour;
}

rnd_cardinal_t pcb_poly_contour_next_point(pcb_poly_t *polygon, rnd_cardinal_t point)
{
	rnd_cardinal_t contour;
	rnd_cardinal_t this_contour_start;
	rnd_cardinal_t next_contour_start;

	contour = pcb_poly_contour_point(polygon, point);

	this_contour_start = (contour == 0) ? 0 : polygon->HoleIndex[contour - 1];
	next_contour_start = (contour == polygon->HoleIndexN) ? polygon->PointN : polygon->HoleIndex[contour];

	/* Wrap back to the start of the contour we're in if we pass the end */
	if (++point == next_contour_start)
		point = this_contour_start;

	return point;
}

rnd_cardinal_t pcb_poly_contour_prev_point(pcb_poly_t *polygon, rnd_cardinal_t point)
{
	rnd_cardinal_t contour;
	rnd_cardinal_t prev_contour_end;
	rnd_cardinal_t this_contour_end;

	contour = pcb_poly_contour_point(polygon, point);

	prev_contour_end = (contour == 0) ? 0 : polygon->HoleIndex[contour - 1];
	this_contour_end = (contour == polygon->HoleIndexN) ? polygon->PointN - 1 : polygon->HoleIndex[contour] - 1;

	/* Wrap back to the start of the contour we're in if we pass the end */
	if (point == prev_contour_end)
		point = this_contour_end;
	else
		point--;

	return point;
}

static void add_noholes_polyarea(rnd_pline_t * pline, void *user_data)
{
	pcb_poly_t *poly = (pcb_poly_t *) user_data;

	/* Prepend the pline into the NoHoles linked list */
	pline->next = poly->NoHoles;
	poly->NoHoles = pline;
}

void pcb_poly_compute_no_holes(pcb_poly_t * poly)
{
	rnd_poly_contours_free(&poly->NoHoles);
	if (poly->Clipped)
		pcb_poly_no_holes_dicer(poly, NULL, add_noholes_polyarea, poly);
	else
		printf("Compute_noholes caught poly->Clipped = NULL\n");
	poly->NoHolesValid = 1;
}

static rnd_polyarea_t *biggest(rnd_polyarea_t * p)
{
	rnd_polyarea_t *n, *top = NULL;
	rnd_pline_t *pl;
	rnd_rtree_t *tree;
	double big = -1;
	if (!p)
		return NULL;
	n = p;
	do {
#if 0
		if (n->contours->area < conf_core.design.poly_isle_area) {
			n->b->f = n->f;
			n->f->b = n->b;
			rnd_poly_contour_del(&n->contours);
			if (n == p)
				p = n->f;
			n = n->f;
			if (!n->contours) {
				free(n);
				return NULL;
			}
		}
#endif
		if (n->contours->area > big) {
			top = n;
			big = n->contours->area;
		}
	}
	while ((n = n->f) != p);
	assert(top);
	if (top == p)
		return p;
	pl = top->contours;
	tree = top->contour_tree;
	top->contours = p->contours;
	top->contour_tree = p->contour_tree;
	p->contours = pl;
	p->contour_tree = tree;
	assert(pl);
	assert(p->f);
	assert(p->b);
	return p;
}

rnd_polyarea_t *pcb_poly_to_polyarea(pcb_poly_t *p, rnd_bool *need_full)
{
	rnd_pline_t *contour = NULL;
	rnd_polyarea_t *np1 = NULL, *np = NULL;
	rnd_cardinal_t n;
	rnd_vector_t v;
	int hole = 0;

	*need_full = rnd_false;

	np1 = np = rnd_polyarea_create();
	if (np == NULL)
		return NULL;

	/* first make initial polygon contour */
	for (n = 0; n < p->PointN; n++) {
		/* No current contour? Make a new one starting at point */
		/*   (or) Add point to existing contour */

		v[0] = p->Points[n].X;
		v[1] = p->Points[n].Y;
		if (contour == NULL) {
			if ((contour = rnd_poly_contour_new(v)) == NULL)
				return NULL;
		}
		else {
			rnd_poly_vertex_include(contour->head->prev, rnd_poly_node_create(v));
		}

		/* Is current point last in contour? If so process it. */
		if (n == p->PointN - 1 || (hole < p->HoleIndexN && n == p->HoleIndex[hole] - 1)) {
			rnd_poly_contour_pre(contour, rnd_true);

			/* make sure it is a positive contour (outer) or negative (hole) */
			if (contour->Flags.orient != (hole ? RND_PLF_INV : RND_PLF_DIR))
				rnd_poly_contour_inv(contour);
			assert(contour->Flags.orient == (hole ? RND_PLF_INV : RND_PLF_DIR));

			rnd_polyarea_contour_include(np, contour);
			contour = NULL;

TODO("multiple plines within the returned polyarea np does not really work\n");
#if 0
			if (!rnd_poly_valid(np)) {
				rnd_cardinal_t cnt = rnd_polyarea_split_selfint(np);
				rnd_message(RND_MSG_ERROR, "Had to split up self-intersecting polygon into %ld parts\n", (long)cnt);
				if (cnt > 1)
					*need_full = rnd_true;
				assert(rnd_poly_valid(np));
			}
#else
				assert(rnd_poly_valid(np));
#endif

			hole++;
		}
	}
	return biggest(np1);
}

rnd_polyarea_t *pcb_poly_from_poly(pcb_poly_t * p)
{
	rnd_bool tmp;
	return pcb_poly_to_polyarea(p, &tmp);
}


rnd_polyarea_t *pcb_poly_from_pcb_line(pcb_line_t *L, rnd_coord_t thick)
{
	return rnd_poly_from_line(L->Point1.X, L->Point1.Y, L->Point2.X, L->Point2.Y, thick, PCB_FLAG_TEST(PCB_FLAG_SQUARE, L));
}

rnd_polyarea_t *pcb_poly_from_pcb_arc(pcb_arc_t *a, rnd_coord_t thick)
{
	return rnd_poly_from_arc(a->X, a->Y, a->Width, a->Height, a->StartAngle, a->Delta, thick);
}


/* clear np1 from the polygon - should be inline with -O3 */
static int Subtract(rnd_polyarea_t * np1, pcb_poly_t * p, rnd_bool fnp)
{
	rnd_polyarea_t *merged = NULL, *np = np1;
	int x;
	assert(np);
	assert(p);
	if (!p->Clipped) {
		if (fnp)
			rnd_polyarea_free(&np);
		return 1;
	}
	assert(rnd_poly_valid(p->Clipped));
	assert(rnd_poly_valid(np));
	if (fnp)
		x = rnd_polyarea_boolean_free(p->Clipped, np, &merged, RND_PBO_SUB);
	else {
		x = rnd_polyarea_boolean(p->Clipped, np, &merged, RND_PBO_SUB);
		rnd_polyarea_free(&p->Clipped);
	}
	assert(!merged || rnd_poly_valid(merged));
	if (x != rnd_err_ok) {
		fprintf(stderr, "Error while clipping RND_PBO_SUB: %d\n", x);
		rnd_polyarea_free(&merged);
		p->Clipped = NULL;
		if (p->NoHoles)
			printf("Just leaked in Subtract\n");
		p->NoHoles = NULL;
		return -1;
	}
	p->Clipped = biggest(merged);
	assert(!p->Clipped || rnd_poly_valid(p->Clipped));
	if (!p->Clipped)
		rnd_message(RND_MSG_WARNING, "Polygon #%ld cleared out of existence near (%$mm, %$mm)\n",
						p->ID, (p->BoundingBox.X1 + p->BoundingBox.X2) / 2, (p->BoundingBox.Y1 + p->BoundingBox.Y2) / 2);
	return 1;
}

int pcb_poly_subtract(rnd_polyarea_t *np1, pcb_poly_t *p, rnd_bool fnp)
{
	return Subtract(np1, p, fnp);
}

static rnd_polyarea_t *pcb_poly_from_box_bloated_(rnd_box_t * box, rnd_coord_t bloat, rnd_coord_t enforce_clr, rnd_coord_t obj_clr)
{
	if ((enforce_clr != 0) && (enforce_clr*2 > obj_clr)) /* in case of poly-side clearance, the bounding box has to be adjusted: original object side clearance removed, larger poly side clearance added */
		bloat += obj_clr - enforce_clr/2;
	return rnd_poly_from_rect(box->X1 - bloat, box->X2 + bloat, box->Y1 - bloat, box->Y2 + bloat);
}

rnd_polyarea_t *pcb_poly_from_box_bloated(rnd_box_t * box, rnd_coord_t bloat)
{
	return pcb_poly_from_box_bloated_(box, bloat, 0, 0);
}

/* remove the padstack clearance from the polygon */
static int SubtractPadstack(pcb_data_t *d, pcb_pstk_t *ps, pcb_layer_t *l, pcb_poly_t *p)
{
	rnd_polyarea_t *np;
	rnd_layer_id_t i;

	/* ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, ps))
		return 0;
	i = pcb_layer_id(d, l);
	np = pcb_thermal_area_pstk(pcb_data_get_top(d), ps, i, p);
	if (np == 0)
		return 0;

	return Subtract(np, p, rnd_true);
}

/* return the clearance polygon for a line */
static rnd_polyarea_t *line_clearance_poly(rnd_cardinal_t layernum, pcb_board_t *pcb, pcb_line_t *line, pcb_poly_t *in_poly)
{
	if (line->thermal & PCB_THERMAL_ON)
		return pcb_thermal_area_line(pcb, line, layernum, in_poly);
	return pcb_poly_from_pcb_line(line, line->Thickness + pcb_obj_clearance_p2(line, in_poly));
}

static int SubtractLine(pcb_line_t * line, pcb_poly_t * p)
{
	rnd_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(line))
		return 0;
	if (!(np = line_clearance_poly(-1, NULL, line, p)))
		return -1;
	return Subtract(np, p, rnd_true);
}

static int SubtractArc(pcb_arc_t * arc, pcb_poly_t * p)
{
	rnd_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(arc))
		return 0;
	if (!(np = pcb_poly_from_pcb_arc(arc, arc->Thickness + pcb_obj_clearance_p2(arc, p))))
		return -1;
	return Subtract(np, p, rnd_true);
}

typedef struct {
	pcb_poly_t *poly;
	rnd_coord_t clearance;
	unsigned sub:1;
	rnd_polyarea_t *pa; /* in case of sub==0 the result is accumulated here */
} poly_poly_text_t;

static void poly_sub_text_cb(void *ctx_, pcb_any_obj_t *obj)
{
	poly_poly_text_t *ctx = ctx_;
	pcb_line_t *line = (pcb_line_t *)obj;
	pcb_poly_t *poly = (pcb_poly_t *)obj;
	pcb_arc_t *arc = (pcb_arc_t *)obj;
	rnd_polyarea_t *np = NULL;
	rnd_bool need_full;

	switch(obj->type) {
		case PCB_OBJ_LINE: np = pcb_poly_from_pcb_line(line, line->Thickness + RND_MAX(ctx->clearance, ctx->poly->enforce_clearance)); break;
		case PCB_OBJ_ARC: np = pcb_poly_from_pcb_arc(arc, arc->Thickness + RND_MAX(ctx->clearance, ctx->poly->enforce_clearance)); break;
		case PCB_OBJ_POLY:
			poly->Clipped = pcb_poly_to_polyarea(poly, &need_full);
			np = pcb_poly_clearance_construct(poly, &ctx->clearance, ctx->poly);
			rnd_polyarea_free(&poly->Clipped);
			break;
		default:;
	}

	if (np != NULL) {
		if (ctx->sub) { /* subtract from ctx->poly directly (drawing clearance) */
			Subtract(np, ctx->poly, rnd_true);
			ctx->poly->NoHolesValid = 0;
		}
		else { /* add to ctx->pa (constructing the clearance polygon for other purposes) */
			if (ctx->pa != NULL) {
				rnd_polyarea_t *tmp;
				rnd_polyarea_boolean_free(ctx->pa, np, &tmp, RND_PBO_UNITE);
				ctx->pa = tmp;
			}
			else
				ctx->pa = np;
		}
	}
}

static pcb_draw_info_t tsub_info;
static rnd_xform_t tsub_xform;

static int SubtractText(pcb_text_t * text, pcb_poly_t * p)
{
	poly_poly_text_t sctx;
	int by_bbox = !text->tight_clearance;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return 0;

	if (by_bbox) {
		/* old method: clear by bounding box */
		const rnd_box_t *b = &text->BoundingBox;
		rnd_coord_t clr = RND_MAX(conf_core.design.bloat, p->enforce_clearance);
		rnd_polyarea_t *np;
		if (!(np = rnd_poly_from_round_rect(b->X1 + conf_core.design.bloat, b->X2 - conf_core.design.bloat, b->Y1 + conf_core.design.bloat, b->Y2 - conf_core.design.bloat, clr)))
			return -1;
		return Subtract(np, p, rnd_true);
	}

	/* new method: detailed clearance */
	sctx.poly = p;
	sctx.clearance = text->clearance + RND_MM_TO_COORD(0.5);
	sctx.sub = 1;
	tsub_info.xform = &tsub_xform;
	pcb_text_decompose_text(&tsub_info, text, poly_sub_text_cb, &sctx);
	return 1;
}

rnd_polyarea_t *pcb_poly_construct_text_clearance(pcb_text_t *text)
{
	poly_poly_text_t sctx;
	pcb_poly_t dummy = {0};
	int by_bbox = !text->tight_clearance;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return NULL;

	if (by_bbox) {
		/* old method: clear by bounding box */
		const rnd_box_t *b = &text->BoundingBox;
		rnd_coord_t clr = conf_core.design.bloat;
		return rnd_poly_from_round_rect(b->X1 + conf_core.design.bloat, b->X2 - conf_core.design.bloat, b->Y1 + conf_core.design.bloat, b->Y2 - conf_core.design.bloat, clr);
	}

	/* new method: detailed clearance */
	sctx.poly = &dummy;
	sctx.pa = NULL;
	sctx.clearance = text->clearance + RND_MM_TO_COORD(0.5);
	sctx.sub = 0;
	tsub_info.xform = &tsub_xform;
	pcb_text_decompose_text(&tsub_info, text, poly_sub_text_cb, &sctx);
	return sctx.pa;
}


struct cpInfo {
	const rnd_box_t *other;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_poly_t *polygon;
	rnd_bool solder;
	rnd_polyarea_t *accumulate;
	int batch_size;
	jmp_buf env;
};

static void subtract_accumulated(struct cpInfo *info, pcb_poly_t *polygon)
{
	if (info->accumulate == NULL)
		return;
	Subtract(info->accumulate, polygon, rnd_true);
	info->accumulate = NULL;
	info->batch_size = 0;
}

static int pcb_poly_clip_noop = 0;
static void *pcb_poly_clip_prog_ctx;
static void (*pcb_poly_clip_prog)(void *ctx) = NULL;

/* call the progress report callback and return if no-op is set */
#define POLY_CLIP_PROG() \
do { \
	if (pcb_poly_clip_prog != NULL) \
		pcb_poly_clip_prog(pcb_poly_clip_prog_ctx); \
	if (pcb_poly_clip_noop) \
		return RND_R_DIR_FOUND_CONTINUE; \
} while(0)

static rnd_r_dir_t padstack_sub_callback(const rnd_box_t *b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	struct cpInfo *info = (struct cpInfo *)cl;
	pcb_poly_t *polygon;
	rnd_polyarea_t *np;
	rnd_polyarea_t *merged;
	rnd_layer_id_t i;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return RND_R_DIR_NOT_FOUND;
	polygon = info->polygon;

	/* ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, ps))
		return RND_R_DIR_NOT_FOUND;

	i = pcb_layer_id(info->data, info->layer);

	np = pcb_thermal_area_pstk(pcb_data_get_top(info->data), ps, i, polygon);
	if (np == 0)
			return RND_R_DIR_FOUND_CONTINUE;

	info->batch_size++;
	POLY_CLIP_PROG();

	rnd_polyarea_boolean_free(info->accumulate, np, &merged, RND_PBO_UNITE);
	info->accumulate = merged;

	if (info->batch_size == SUBTRACT_PADSTACK_BATCH_SIZE)
		subtract_accumulated(info, polygon);

	return RND_R_DIR_FOUND_CONTINUE;
}

static rnd_r_dir_t arc_sub_callback(const rnd_box_t * b, void *cl)
{
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return RND_R_DIR_NOT_FOUND;
	if (!PCB_NONPOLY_HAS_CLEARANCE(arc))
		return RND_R_DIR_NOT_FOUND;

	POLY_CLIP_PROG();

	polygon = info->polygon;

	if (SubtractArc(arc, polygon) < 0)
		longjmp(info->env, 1);
	return RND_R_DIR_FOUND_CONTINUE;
}

/* quick create a polygon area from a line, knowing the coords and width */
static rnd_polyarea_t *poly_sub_callback_line(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t width)
{
	static pcb_line_t lin;
	static int inited = 0;

	if (!inited) {
		lin.type = PCB_OBJ_LINE;
		lin.Flags = pcb_no_flags();
		PCB_FLAG_SET(PCB_FLAG_CLEARLINE, &lin);
		lin.Thickness = 0;
		inited = 1;
	}

	lin.Point1.X = x1;
	lin.Point1.Y = y1;
	lin.Point2.X = x2;
	lin.Point2.Y = y2;
	lin.Clearance = width;

	return pcb_poly_from_pcb_line(&lin, width);
}

#define pa_append(src) \
	do { \
		rnd_polyarea_boolean(*dst, src, &tmp, RND_PBO_UNITE); \
		rnd_polyarea_free(dst); \
		*dst = tmp; \
	} while(0)

void pcb_poly_pa_clearance_construct(rnd_polyarea_t **dst, pcb_poly_it_t *it, rnd_coord_t clearance)
{
	rnd_polyarea_t *tmp, *lin;
	rnd_coord_t x, y, px, py, x0, y0;
	rnd_pline_t *pl;
	int go;
	rnd_cardinal_t cnt;

	if (*dst != NULL)
		pa_append(it->pa);
	else
		rnd_polyarea_copy0(dst, it->pa);

	if (clearance == 0)
		return;

	clearance *= 2;

	/* care about the outer contours only */
	pl = pcb_poly_contour(it);
	if (pl != NULL) {
		/* iterate over the vectors of the contour */
		for(go = pcb_poly_vect_first(it, &x, &y), cnt = 0; go; go = pcb_poly_vect_next(it, &x, &y), cnt++) {
			if (cnt > 0) {
				lin = poly_sub_callback_line(px, py, x, y, clearance);
				pa_append(lin);
				rnd_polyarea_free(&lin);
			}
			else {
				x0 = x;
				y0 = y;
			}
			px = x;
			py = y;
		}
		if (cnt > 0) {
			lin = poly_sub_callback_line(px, py, x0, y0, clearance);
			pa_append(lin);
			rnd_polyarea_free(&lin);
		}
	}
}
#undef pa_append

/* Construct a poly area that represents the enlarged subpoly - so it can be
   subtracted from the parent poly to form the clearance for subpoly.
   If clr_override is not NULL, use that clearance value instead of the subpoly's */
rnd_polyarea_t *pcb_poly_clearance_construct(pcb_poly_t *subpoly, rnd_coord_t *clr_override, pcb_poly_t *in_poly)
{
	rnd_polyarea_t *ret = NULL, *pa;
	pcb_poly_it_t it;
	rnd_coord_t clr;

	if (clr_override != NULL)
		clr = *clr_override;
	else
		clr = subpoly->Clearance/2;

	if (in_poly != NULL)
		clr = RND_MAX(clr, in_poly->enforce_clearance);
	/* iterate over all islands of a polygon */
	for(pa = pcb_poly_island_first(subpoly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
		pcb_poly_pa_clearance_construct(&ret, &it, clr);

	return ret;
}

/* return the clearance polygon for a line */
static rnd_polyarea_t *poly_clearance_poly(rnd_cardinal_t layernum, pcb_board_t *pcb, pcb_poly_t *subpoly, pcb_poly_t *in_poly)
{
	if (subpoly->thermal & PCB_THERMAL_ON)
		return pcb_thermal_area_poly(pcb, subpoly, layernum, in_poly);
	return pcb_poly_clearance_construct(subpoly, NULL, in_poly);
}


static int SubtractPolyPoly(pcb_poly_t *subpoly, pcb_poly_t *frompoly)
{
	rnd_polyarea_t *pa;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, frompoly)) /* two clearing polys won't interact */
		return 0; /* but it's not an error, that'd kill other clearances in the same poly */

	pa = poly_clearance_poly(-1, NULL, subpoly, frompoly);
	if (pa == NULL)
		return -1;

	Subtract(pa, frompoly, rnd_true);
	frompoly->NoHolesValid = 0;
	return 0;
}


static int UnsubtractPolyPoly(pcb_poly_t *subpoly, pcb_poly_t *frompoly)
{
	rnd_polyarea_t *pa;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, frompoly)) /* two clearing polys won't interact */
		return 0; /* but it's not an error, that'd kill other clearances in the same poly */

	pa = poly_clearance_poly(-1, NULL, subpoly, frompoly);
	if (pa == NULL)
		return -1;

	if (!Unsubtract(pa, frompoly))
		return -1;
	frompoly->NoHolesValid = 0;
	return 0;
}

static rnd_r_dir_t poly_sub_callback(const rnd_box_t *b, void *cl)
{
	pcb_poly_t *subpoly = (pcb_poly_t *)b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;

	polygon = info->polygon;

	/* don't do clearance in itself */
	if (subpoly == polygon)
		return RND_R_DIR_NOT_FOUND;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return RND_R_DIR_NOT_FOUND;
	if (!PCB_POLY_HAS_CLEARANCE(subpoly))
		return RND_R_DIR_NOT_FOUND;

	POLY_CLIP_PROG();

	if (SubtractPolyPoly(subpoly, polygon) < 0)
		longjmp(info->env, 1);

	return RND_R_DIR_FOUND_CONTINUE;
}

static rnd_r_dir_t line_sub_callback(const rnd_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;
	rnd_polyarea_t *np;
	rnd_polyarea_t *merged;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return RND_R_DIR_NOT_FOUND;
	if (!PCB_NONPOLY_HAS_CLEARANCE(line))
		return RND_R_DIR_NOT_FOUND;
	polygon = info->polygon;

	POLY_CLIP_PROG();

	np = line_clearance_poly(-1, NULL, line, polygon);
	if (!np)
		longjmp(info->env, 1);

	rnd_polyarea_boolean_free(info->accumulate, np, &merged, RND_PBO_UNITE);
	info->accumulate = merged;
	info->batch_size++;

	if (info->batch_size == SUBTRACT_LINE_BATCH_SIZE)
		subtract_accumulated(info, polygon);

	return RND_R_DIR_FOUND_CONTINUE;
}

static rnd_r_dir_t text_sub_callback(const rnd_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	struct cpInfo *info = (struct cpInfo *) cl;
	pcb_poly_t *polygon;

	/* don't subtract the object that was put back! */
	if (b == info->other)
		return RND_R_DIR_NOT_FOUND;
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return RND_R_DIR_NOT_FOUND;

	POLY_CLIP_PROG();

	polygon = info->polygon;
	if (SubtractText(text, polygon) < 0)
		longjmp(info->env, 1);
	return RND_R_DIR_FOUND_CONTINUE;
}

static rnd_cardinal_t clearPoly(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *polygon, const rnd_box_t *here, rnd_coord_t expand, int noop)
{
	rnd_cardinal_t r = 0;
	int seen;
	rnd_box_t region;
	struct cpInfo info;
	rnd_layergrp_id_t group;
	unsigned int gflg;
	pcb_layer_type_t lf;
	int old_noop;

	lf = pcb_layer_flags_(Layer);
	if (!(lf & PCB_LYT_COPPER)) { /* also handles lf == 0 */
		polygon->NoHolesValid = 0;
		return 0;
	}

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon))
		return 0;

	old_noop = pcb_poly_clip_noop;
	pcb_poly_clip_noop = noop;

	group = pcb_layer_get_group_(Layer);
	gflg = pcb_layergrp_flags(PCB, group);
	info.solder = (gflg & PCB_LYT_BOTTOM);
	info.data = Data;
	info.other = here;
	info.layer = Layer;
	info.polygon = polygon;
	if (here)
		region = rnd_clip_box(here, &polygon->BoundingBox);
	else
		region = polygon->BoundingBox;
	region = rnd_bloat_box(&region, expand);

	if (setjmp(info.env) == 0) {

		r = 0;
		info.accumulate = NULL;
		info.batch_size = 0;
		PCB_COPPER_GROUP_LOOP(Data, group);
		{
			rnd_r_search(layer->line_tree, &region, NULL, line_sub_callback, &info, &seen);
			r += seen;
			subtract_accumulated(&info, polygon);
			rnd_r_search(layer->arc_tree, &region, NULL, arc_sub_callback, &info, &seen);
			r += seen;
			rnd_r_search(layer->text_tree, &region, NULL, text_sub_callback, &info, &seen);
			r += seen;
			rnd_r_search(layer->polygon_tree, &region, NULL, poly_sub_callback, &info, &seen);
			r += seen;
		}
		PCB_END_LOOP;
		rnd_r_search(Data->padstack_tree, &region, NULL, padstack_sub_callback, &info, &seen);
		r += seen;
		subtract_accumulated(&info, polygon);
	}
	if (!noop)
		polygon->NoHolesValid = 0;
	pcb_poly_clip_noop = old_noop;
	return r;
}

static int Unsubtract(rnd_polyarea_t * np1, pcb_poly_t * p)
{
	rnd_polyarea_t *merged = NULL, *np = np1;
	rnd_polyarea_t *orig_poly, *clipped_np;
	int x;
	rnd_bool need_full;
	assert(np);
	assert(p); /* NOTE: p->clipped might be NULL if a poly is "cleared out of existence" and is now coming back */

	orig_poly = pcb_poly_to_polyarea(p, &need_full);

	x = rnd_polyarea_boolean_free(np, orig_poly, &clipped_np, RND_PBO_ISECT);
	if (x != rnd_err_ok) {
		fprintf(stderr, "Error while clipping RND_PBO_ISECT: %d\n", x);
		rnd_polyarea_free(&clipped_np);
		goto fail;
	}

	x = rnd_polyarea_boolean_free(p->Clipped, clipped_np, &merged, RND_PBO_UNITE);
	if (x != rnd_err_ok) {
		fprintf(stderr, "Error while clipping RND_PBO_UNITE: %d\n", x);
		rnd_polyarea_free(&merged);
		goto fail;
	}
	p->Clipped = biggest(merged);
	assert(!p->Clipped || rnd_poly_valid(p->Clipped));
	return 1;

fail:
	p->Clipped = NULL;
	if (p->NoHoles)
		printf("Just leaked in Unsubtract\n");
	p->NoHoles = NULL;
	return 0;
}

static int UnsubtractPadstack(pcb_data_t *data, pcb_pstk_t *ps, pcb_layer_t *l, pcb_poly_t *p)
{
	rnd_polyarea_t *np;

	/* overlap a bit to prevent gaps from rounding errors */
	np = pcb_poly_from_box_bloated_(&ps->BoundingBox, UNSUBTRACT_BLOAT * 400000, p->enforce_clearance, ps->Clearance);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;

	clearPoly(PCB->Data, l, p, (const rnd_box_t *)ps, 2 * UNSUBTRACT_BLOAT * 400000, 0);
	return 1;
}



static int UnsubtractArc(pcb_arc_t * arc, pcb_layer_t * l, pcb_poly_t * p)
{
	rnd_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(arc))
		return 0;

	/* overlap a bit to prevent gaps from rounding errors */
	np = pcb_poly_from_box_bloated_(&arc->BoundingBox, UNSUBTRACT_BLOAT, p->enforce_clearance, arc->Clearance);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const rnd_box_t *) arc, 2 * UNSUBTRACT_BLOAT, 0);
	return 1;
}

static int UnsubtractLine(pcb_line_t * line, pcb_layer_t * l, pcb_poly_t * p)
{
	rnd_polyarea_t *np;

	if (!PCB_NONPOLY_HAS_CLEARANCE(line))
		return 0;

	/* overlap a bit to prevent notches from rounding errors */
	np = pcb_poly_from_box_bloated_(&line->BoundingBox, UNSUBTRACT_BLOAT, p->enforce_clearance, line->Clearance);

	if (!np)
		return 0;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const rnd_box_t *) line, 2 * UNSUBTRACT_BLOAT, 0);
	return 1;
}

static int UnsubtractText(pcb_text_t * text, pcb_layer_t * l, pcb_poly_t * p)
{
	rnd_polyarea_t *np;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, text))
		return 0;

	/* overlap a bit to prevent notches from rounding errors */
	np = pcb_poly_from_box_bloated_(&text->BoundingBox, UNSUBTRACT_BLOAT, p->enforce_clearance, text->clearance);

	if (!np)
		return -1;
	if (!Unsubtract(np, p))
		return 0;
	clearPoly(PCB->Data, l, p, (const rnd_box_t *) text, 2 * UNSUBTRACT_BLOAT, 0);
	return 1;
}

static rnd_bool inhibit = rnd_false;

int pcb_poly_init_clip_prog(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p, void (*cb)(void *ctx), void *ctx, int force)
{
	pcb_board_t *pcb;
	rnd_bool need_full;
	void (*old_cb)(void *ctx);
	void *old_ctx;

	if (inhibit)
		return 0;

	if ((!force) && (Data->clip_inhibit > 0)) {
		p->clip_dirty = 1;
		return 0;
	}

	if (layer->is_bound)
		layer = layer->meta.bound.real;

	if (p->Clipped)
		rnd_polyarea_free(&p->Clipped);
	p->Clipped = pcb_poly_to_polyarea(p, &need_full);
	if (need_full && !PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, p)) {
		rnd_message(RND_MSG_WARNING, "Polygon #%ld was self intersecting; it had to be split up and\nthe full poly flag set.\n", (long)p->ID);
		PCB_FLAG_SET(PCB_FLAG_FULLPOLY, p);
	}
	rnd_poly_contours_free(&p->NoHoles);

	p->clip_dirty = 0;

	if (layer == NULL)
		return 0;

	if (!p->Clipped)
		return 0;
	assert(rnd_poly_valid(p->Clipped));

	/* calculate clipping for the real data (in case of subcircuits) */
	pcb = pcb_data_get_top(Data);
	if (pcb != NULL)
		Data = pcb->Data;

	old_cb = pcb_poly_clip_prog;
	old_ctx = pcb_poly_clip_prog_ctx;
	pcb_poly_clip_prog = cb;
	pcb_poly_clip_prog_ctx = ctx;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, p))
		clearPoly(Data, layer, p, NULL, 0, 0);
	else
		p->NoHolesValid = 0;

	pcb_poly_clip_prog = old_cb;
	pcb_poly_clip_prog_ctx = old_ctx;
	return 1;
}

int pcb_poly_init_clip(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p)
{
	return pcb_poly_init_clip_prog(Data, layer, p, NULL, NULL, 0);
}

int pcb_poly_init_clip_force(pcb_data_t *Data, pcb_layer_t *layer, pcb_poly_t *p)
{
	return pcb_poly_init_clip_prog(Data, layer, p, NULL, NULL, 1);
}

rnd_cardinal_t pcb_poly_num_clears(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon)
{
	rnd_cardinal_t res;
	void (*old_cb)(void *ctx);

	if (layer->is_bound)
		layer = layer->meta.bound.real;

	old_cb = pcb_poly_clip_prog;
	pcb_poly_clip_prog = NULL;

	res = clearPoly(data, layer, polygon, NULL, 0, 1);

	pcb_poly_clip_prog = old_cb;
	return res;
}

/* --------------------------------------------------------------------------
 * remove redundant polygon points. Any point that lies on the straight
 * line between the points on either side of it is redundant.
 * returns rnd_true if any points are removed
 */
rnd_bool pcb_poly_remove_excess_points(pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	rnd_point_t *p;
	rnd_cardinal_t n, prev, next;
	pcb_line_t line;
	rnd_bool changed = rnd_false;

	if (pcb_undoing())
		return rnd_false;

	for (n = 0; n < Polygon->PointN; n++) {
		prev = pcb_poly_contour_prev_point(Polygon, n);
		next = pcb_poly_contour_next_point(Polygon, n);
		p = &Polygon->Points[n];

		line.Point1 = Polygon->Points[prev];
		line.Point2 = Polygon->Points[next];
		line.Thickness = 0;
		if (pcb_is_point_on_line(p->X, p->Y, 0.0, &line)) {
			pcb_remove_object(PCB_OBJ_POLY_POINT, Layer, Polygon, p);
			changed = rnd_true;
		}
	}
	return changed;
}

/* returns the index of the polygon point which is the end
   point of the segment with the lowest distance to the passed coordinates */
rnd_cardinal_t pcb_poly_get_lowest_distance_point(pcb_poly_t *Polygon, rnd_coord_t X, rnd_coord_t Y)
{
	double mindistance = (double) RND_MAX_COORD * RND_MAX_COORD;
	rnd_point_t *ptr1, *ptr2;
	rnd_cardinal_t n, result = 0;

	/* we calculate the distance to each segment and choose the
	   shortest distance. If the closest approach between the
	   given point and the projected line (i.e. the segment extended)
	   is not on the segment, then the distance is the distance
	   to the segment end point.*/
	for (n = 0; n < Polygon->PointN; n++) {
		double u, dx, dy;
		ptr1 = &Polygon->Points[pcb_poly_contour_prev_point(Polygon, n)];
		ptr2 = &Polygon->Points[n];

		dx = ptr2->X - ptr1->X;
		dy = ptr2->Y - ptr1->Y;
		if (dx != 0.0 || dy != 0.0) {
			/* projected intersection is at P1 + u(P2 - P1) */
			u = ((X - ptr1->X) * dx + (Y - ptr1->Y) * dy) / (dx * dx + dy * dy);

			if (u < 0.0) {						/* ptr1 is closest point */
				u = RND_SQUARE(X - ptr1->X) + RND_SQUARE(Y - ptr1->Y);
			}
			else if (u > 1.0) {				/* ptr2 is closest point */
				u = RND_SQUARE(X - ptr2->X) + RND_SQUARE(Y - ptr2->Y);
			}
			else {										/* projected intersection is closest point */
				u = RND_SQUARE(X - ptr1->X * (1.0 - u) - u * ptr2->X) + RND_SQUARE(Y - ptr1->Y * (1.0 - u) - u * ptr2->Y);
			}
			if (u < mindistance) {
				mindistance = u;
				result = n;
			}
		}
	}
	return result;
}

/* go back to the previous point of the polygon (undo) */
void pcb_polygon_go_to_prev_point(void)
{
	switch (pcb_crosshair.AttachedPolygon.PointN) {
		/* do nothing if mode has just been entered */
	case 0:
		break;

		/* reset number of points line tool state */
	case 1:
		pcb_crosshair.AttachedPolygon.PointN = 0;
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
		break;

		/* back-up one point */
	default:
		{
			rnd_point_t *points = pcb_crosshair.AttachedPolygon.Points;
			rnd_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN - 2;

			pcb_crosshair.AttachedPolygon.PointN--;
			pcb_crosshair.AttachedLine.Point1.X = points[n].X;
			pcb_crosshair.AttachedLine.Point1.Y = points[n].Y;
			break;
		}
	}
}

/* go forward to the next point of the polygon (redo) */
void pcb_polygon_go_to_next_point(void)
{
	if ((pcb_crosshair.AttachedPolygon.PointN > 0) && (pcb_crosshair.AttachedPolygon.PointN < pcb_crosshair.AttachedPolygon_pts)) {
		rnd_point_t *points = pcb_crosshair.AttachedPolygon.Points;
		rnd_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

		pcb_crosshair.AttachedPolygon.PointN++;
		pcb_crosshair.AttachedLine.Point1.X = points[n].X;
		pcb_crosshair.AttachedLine.Point1.Y = points[n].Y;
	}
}

/* close polygon if possible */
void pcb_polygon_close_poly(void)
{
	rnd_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

	/* check number of points */
	if (n >= 3) {
		/* if 45 degree lines are what we want do a quick check
		   if closing the polygon makes sense */
		if (!conf_core.editor.all_direction_lines) {
			rnd_coord_t dx, dy;

			dx = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].X - pcb_crosshair.AttachedPolygon.Points[0].X);
			dy = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].Y - pcb_crosshair.AttachedPolygon.Points[0].Y);
			if (!(dx == 0 || dy == 0 || dx == dy)) {
				rnd_message(RND_MSG_ERROR, "Cannot close polygon because 45 degree lines are requested.\n");
				return;
			}
		}
		pcb_polygon_copy_attached_to_layer();
		pcb_draw();
	}
	else
		rnd_message(RND_MSG_ERROR, "A polygon has to have at least 3 points\n");
}

static void poly_copy_data(pcb_poly_t *dst, pcb_poly_t *src)
{
	pcb_poly_t p;
	void *old_parent = dst->parent.any;
	pcb_parenttype_t old_pt = dst->parent_type;

	memcpy(dst, src, ((char *)&p.link - (char *)&p));
	dst->type = PCB_OBJ_POLY;
	dst->parent.any = old_parent;
	dst->parent_type = old_pt;
}

/* ---------------------------------------------------------------------------
 * moves the data of the attached (new) polygon to the current layer
 */
void pcb_polygon_copy_attached_to_layer(void)
{
	pcb_layer_t *layer = pcb_loose_subc_layer(PCB, PCB_CURRLAYER(PCB), rnd_true);
	pcb_poly_t *polygon;
	int saveID;

	/* move data to layer and clear attached struct */
	polygon = pcb_poly_new(layer, 0, pcb_no_flags());
	saveID = polygon->ID;
	poly_copy_data(polygon, &pcb_crosshair.AttachedPolygon);
	polygon->Clearance = 2 * conf_core.design.clearance;
	polygon->ID = saveID;
	PCB_FLAG_SET(PCB_FLAG_CLEARPOLY, polygon);
	if (conf_core.editor.full_poly)
		PCB_FLAG_SET(PCB_FLAG_FULLPOLY, polygon);
	if (conf_core.editor.clear_polypoly)
		PCB_FLAG_SET(PCB_FLAG_CLEARPOLYPOLY, polygon);

	memset(&pcb_crosshair.AttachedPolygon, 0, sizeof(pcb_poly_t));

	pcb_add_poly_on_layer(layer, polygon);

	pcb_poly_init_clip(PCB->Data, layer, polygon);
	pcb_poly_invalidate_draw(layer, polygon);
	pcb_board_set_changed_flag(PCB, rnd_true);

	/* reset state of attached line */
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;

	/* add to undo list */
	pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, polygon, polygon);
	pcb_undo_inc_serial();
}

/* ---------------------------------------------------------------------------
 * close polygon hole if possible
 */
void pcb_polygon_close_hole(void)
{
	rnd_cardinal_t n = pcb_crosshair.AttachedPolygon.PointN;

	/* check number of points */
	if (n >= 3) {
		/* if 45 degree lines are what we want do a quick check
		 * if closing the polygon makes sense
		 */
		if (!conf_core.editor.all_direction_lines) {
			rnd_coord_t dx, dy;

			dx = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].X - pcb_crosshair.AttachedPolygon.Points[0].X);
			dy = coord_abs(pcb_crosshair.AttachedPolygon.Points[n - 1].Y - pcb_crosshair.AttachedPolygon.Points[0].Y);
			if (!(dx == 0 || dy == 0 || dx == dy)) {
				rnd_message(RND_MSG_ERROR, "Cannot close polygon hole because 45 degree lines are requested.\n");
				return;
			}
		}
		pcb_polygon_hole_create_from_attached();
		pcb_draw();
	}
	else
		rnd_message(RND_MSG_ERROR, "A polygon hole has to have at least 3 points\n");
}

/* ---------------------------------------------------------------------------
 * creates a hole of attached polygon shape in the underneath polygon
 */
void pcb_polygon_hole_create_from_attached(void)
{
	rnd_polyarea_t *original, *new_hole, *result;
	pcb_flag_t Flags;
	
	/* Create pcb_polyarea_ts from the original polygon
	 * and the new hole polygon */
	original = pcb_poly_from_poly((pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2);
	new_hole = pcb_poly_from_poly(&pcb_crosshair.AttachedPolygon);

	/* Subtract the hole from the original polygon shape */
	rnd_polyarea_boolean_free(original, new_hole, &result, RND_PBO_SUB);

	/* Convert the resulting polygon(s) into a new set of nodes
	 * and place them on the page. Delete the original polygon.
	 */
	pcb_undo_save_serial();
	Flags = ((pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2)->Flags;
	pcb_poly_to_polygons_on_layer(PCB->Data, (pcb_layer_t *) pcb_crosshair.AttachedObject.Ptr1, result, Flags);
	pcb_remove_object(PCB_OBJ_POLY,
							 pcb_crosshair.AttachedObject.Ptr1, pcb_crosshair.AttachedObject.Ptr2, pcb_crosshair.AttachedObject.Ptr3);
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();

	/* reset state of attached line */
	memset(&pcb_crosshair.AttachedPolygon, 0, sizeof(pcb_poly_t));
	pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
}

/* find polygon holes in range, then call the callback function for
 * each hole. If the callback returns non-zero, stop
 * the search.
 */
int
pcb_poly_holes(pcb_poly_t * polygon, const rnd_box_t * range, int (*callback) (rnd_pline_t * contour, void *user_data), void *user_data)
{
	rnd_polyarea_t *pa = polygon->Clipped;
	rnd_pline_t *pl;
	/* If this hole is so big the polygon doesn't exist, then it's not
	 * really a hole.
	 */
	if (!pa)
		return 0;
	for (pl = pa->contours->next; pl; pl = pl->next) {
		if (range != NULL && (pl->xmin > range->X2 || pl->xmax < range->X1 || pl->ymin > range->Y2 || pl->ymax < range->Y1))
			continue;
		if (callback(pl, user_data)) {
			return 1;
		}
	}
	return 0;
}

struct plow_info {
	int type;
	void *ptr1, *ptr2;
	pcb_layer_t *layer;
	pcb_data_t *data;
	void *user_data;
	rnd_r_dir_t (*callback)(pcb_data_t *, pcb_layer_t *, pcb_poly_t *, int, void *, void *, void *user_data);
};

static rnd_r_dir_t subtract_plow(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	if (!Polygon->Clipped)
		return 0;
	switch (type) {
	case PCB_OBJ_PSTK:
		SubtractPadstack(Data, (pcb_pstk_t *) ptr2, Layer, Polygon);
		Polygon->NoHolesValid = 0;
		return RND_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_LINE:
		SubtractLine((pcb_line_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return RND_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_ARC:
		SubtractArc((pcb_arc_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return RND_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_POLY:
		if (ptr2 != Polygon) {
			SubtractPolyPoly((pcb_poly_t *) ptr2, Polygon);
			Polygon->NoHolesValid = 0;
			return RND_R_DIR_FOUND_CONTINUE;
		}
		break;
	case PCB_OBJ_TEXT:
		SubtractText((pcb_text_t *) ptr2, Polygon);
		Polygon->NoHolesValid = 0;
		return RND_R_DIR_FOUND_CONTINUE;
	}
	return RND_R_DIR_NOT_FOUND;
}

static rnd_r_dir_t add_plow(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	switch (type) {
	case PCB_OBJ_PSTK:
		UnsubtractPadstack(Data, (pcb_pstk_t *) ptr2, Layer, Polygon);
		return RND_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_LINE:
		UnsubtractLine((pcb_line_t *) ptr2, Layer, Polygon);
		return RND_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_ARC:
		UnsubtractArc((pcb_arc_t *) ptr2, Layer, Polygon);
		return RND_R_DIR_FOUND_CONTINUE;
	case PCB_OBJ_POLY:
		if (ptr2 != Polygon) {
			UnsubtractPolyPoly((pcb_poly_t *) ptr2, Polygon);
			return RND_R_DIR_FOUND_CONTINUE;
		}
		break;
	case PCB_OBJ_TEXT:
		UnsubtractText((pcb_text_t *) ptr2, Layer, Polygon);
		return RND_R_DIR_FOUND_CONTINUE;
	}
	return RND_R_DIR_NOT_FOUND;
}

int pcb_poly_sub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj)
{
	if (subtract_plow(Data, Layer, Polygon, type, NULL, obj, NULL) == RND_R_DIR_FOUND_CONTINUE)
		return 0;
	return -1;
}

int pcb_poly_unsub_obj(pcb_data_t *Data, pcb_layer_t *Layer, pcb_poly_t *Polygon, int type, void *obj)
{
	if (add_plow(Data, Layer, Polygon, type, NULL, obj, NULL) == RND_R_DIR_FOUND_CONTINUE)
		return 0;
	return -1;
}


static rnd_r_dir_t plow_callback(const rnd_box_t * b, void *cl)
{
	struct plow_info *plow = (struct plow_info *) cl;
	pcb_poly_t *polygon = (pcb_poly_t *) b;

	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLY, polygon))
		return plow->callback(plow->data, plow->layer, polygon, plow->type, plow->ptr1, plow->ptr2, plow->user_data);
	return RND_R_DIR_NOT_FOUND;
}

/* poly plows while clipping inhibit is on: mark any potentail polygon dirty */
static rnd_r_dir_t poly_plows_inhibited_cb(pcb_data_t *data, pcb_layer_t *lay, pcb_poly_t *poly, int type, void *ptr1, void *ptr2, void *user_data)
{
	poly->clip_dirty = 1;
	return RND_R_DIR_FOUND_CONTINUE;
}


int pcb_poly_plows(pcb_data_t *Data, int type, void *ptr1, void *ptr2,
	rnd_r_dir_t (*call_back) (pcb_data_t *data, pcb_layer_t *lay, pcb_poly_t *poly, int type, void *ptr1, void *ptr2, void *user_data),
	void *user_data)
{
	rnd_box_t sb = ((pcb_any_obj_t *) ptr2)->BoundingBox;
	int r = 0, seen;
	struct plow_info info;

	if (Data->clip_inhibit > 0)
		call_back = poly_plows_inhibited_cb;

	info.type = type;
	info.ptr1 = ptr1;
	info.ptr2 = ptr2;
	info.data = Data;
	info.user_data = user_data;
	info.callback = call_back;
	switch (type) {
	case PCB_OBJ_PSTK:
		if (Data->parent_type != PCB_PARENT_BOARD)
			return 0;
		if (ptr1 == NULL) { /* no layer specified: run on all layers */
			LAYER_LOOP(Data, pcb_max_layer(PCB));
			{
				if (!(pcb_layer_flags_(layer) & PCB_LYT_COPPER))
					continue;
				r += pcb_poly_plows(Data, type, layer, ptr2, call_back, NULL);
			}
			PCB_END_LOOP;
			return r;
		}

		/* run on the specified layer (ptr1), if there's any need for clearing */
	/* ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
		if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, (pcb_pstk_t *)ptr2))
			return 0;
		goto doit;
	case PCB_OBJ_POLY:
		if (!PCB_POLY_HAS_CLEARANCE((pcb_poly_t *) ptr2))
			return 0;
		goto doit;

	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
		/* the cast works equally well for lines and arcs */
		if (!PCB_NONPOLY_HAS_CLEARANCE((pcb_line_t *) ptr2))
			return 0;
		goto doit;

	case PCB_OBJ_TEXT:
		/* text has no clearance property */
		if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, (pcb_line_t *) ptr2))
			return 0;
		/* non-copper (e.g. silk, outline) doesn't plow */
TODO(": use pcb_layer_flags_ here - but what PCB?")
		if (!(pcb_layer_flags(PCB, pcb_layer_id(Data, (pcb_layer_t *) ptr1) & PCB_LYT_COPPER)))
			return 0;
		doit:;
		PCB_COPPER_GROUP_LOOP(Data, pcb_layer_get_group(PCB, pcb_layer_id(Data, ((pcb_layer_t *) ptr1))));
		{
			info.layer = layer;
			rnd_r_search(layer->polygon_tree, &sb, NULL, plow_callback, &info, &seen);
			r += seen;
		}
		PCB_END_LOOP;
		break;
	}
	return r;
}

void pcb_poly_restore_to_poly(pcb_data_t * Data, int type, void *ptr1, void *ptr2)
{
	pcb_data_t *dt = Data;
	while((dt != NULL) && (dt->parent_type == PCB_PARENT_SUBC))
		dt = dt->parent.subc->parent.data;
	if ((dt == NULL) || (dt->parent_type != PCB_PARENT_BOARD)) /* clear/restore only on boards */
		return;
	if (type == PCB_OBJ_POLY)
		pcb_poly_init_clip(dt, (pcb_layer_t *) ptr1, (pcb_poly_t *) ptr2);
	pcb_poly_plows(dt, type, ptr1, ptr2, add_plow, NULL);
}

void pcb_poly_clear_from_poly(pcb_data_t * Data, int type, void *ptr1, void *ptr2)
{
	pcb_data_t *dt = Data;
	while((dt != NULL) && (dt->parent_type == PCB_PARENT_SUBC))
		dt = dt->parent.subc->parent.data;
	if ((dt == NULL) || (dt->parent_type != PCB_PARENT_BOARD)) /* clear/restore only on boards */
		return;
	if (type == PCB_OBJ_POLY)
		pcb_poly_init_clip(dt, (pcb_layer_t *) ptr1, (pcb_poly_t *) ptr2);
	pcb_poly_plows(dt, type, ptr1, ptr2, subtract_plow, NULL);
}

rnd_bool pcb_poly_isects_poly(rnd_polyarea_t * a, pcb_poly_t *p, rnd_bool fr)
{
	rnd_polyarea_t *x;
	rnd_bool ans;
	ans = rnd_polyarea_touching(a, p->Clipped);
	/* argument may be register, so we must copy it */
	x = a;
	if (fr)
		rnd_polyarea_free(&x);
	return ans;
}


rnd_bool pcb_poly_is_point_in_p(rnd_coord_t X, rnd_coord_t Y, rnd_coord_t r, pcb_poly_t *p)
{
	rnd_polyarea_t *c;
	rnd_vector_t v;
	v[0] = X;
	v[1] = Y;
	if (rnd_polyarea_contour_inside(p->Clipped, v))
		return rnd_true;

	if (PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, p)) {
		pcb_poly_t tmp = *p;

		/* Check all clipped-away regions that are now drawn because of full-poly */
		for (tmp.Clipped = p->Clipped->f; tmp.Clipped != p->Clipped; tmp.Clipped = tmp.Clipped->f)
			if (rnd_polyarea_contour_inside(tmp.Clipped, v))
				return rnd_true;
	}

	if (r < 1)
		return rnd_false;
	if (!(c = rnd_poly_from_circle(X, Y, r)))
		return rnd_false;
	return pcb_poly_isects_poly(c, p, rnd_true);
}


rnd_bool pcb_poly_is_point_in_p_ignore_holes(rnd_coord_t X, rnd_coord_t Y, pcb_poly_t *p)
{
	rnd_vector_t v;
	v[0] = X;
	v[1] = Y;
	return rnd_poly_contour_inside(p->Clipped->contours, v);
}

rnd_bool pcb_poly_is_rect_in_p(rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, pcb_poly_t *p)
{
	rnd_polyarea_t *s;
	if (!(s = rnd_poly_from_rect(min(X1, X2), max(X1, X2), min(Y1, Y2), max(Y1, Y2))))
		return rnd_false;
	return pcb_poly_isects_poly(s, p, rnd_true);
}

void pcb_poly_no_holes_dicer(pcb_poly_t *p, const rnd_box_t *clip, void (*emit)(rnd_pline_t *, void *), void *user_data)
{
	rnd_polyarea_t *main_contour;

	main_contour = rnd_polyarea_create();
	/* copy the main poly only */
	rnd_polyarea_copy1(main_contour, p->Clipped);

	if (clip == NULL)
		rnd_polyarea_no_holes_dicer(main_contour, 0, 0, 0, 0, emit, user_data);
	else
		rnd_polyarea_no_holes_dicer(main_contour, clip->X1, clip->Y1, clip->X2, clip->Y2, emit, user_data);
}

/* make a polygon split into multiple parts into multiple polygons */
rnd_bool pcb_poly_morph(pcb_layer_t *layer, pcb_poly_t *poly)
{
	rnd_polyarea_t *p, *start;
	rnd_bool many = rnd_false;
	pcb_flag_t flags;

	if (!poly->Clipped || PCB_FLAG_TEST(PCB_FLAG_LOCK, poly))
		return rnd_false;
	if (poly->Clipped->f == poly->Clipped)
		return rnd_false;
	pcb_poly_invalidate_erase(poly);
	start = p = poly->Clipped;
	/* This is ugly. The creation of the new polygons can cause
	 * all of the polygon pointers (including the one we're called
	 * with to change if there is a realloc in pcb_poly_alloc().
	 * That will invalidate our original "poly" argument, potentially
	 * inside the loop. We need to steal the Clipped pointer and
	 * hide it from the Remove call so that it still exists while
	 * we do this dirty work.
	 */
	poly->Clipped = NULL;
	if (poly->NoHoles)
		printf("Just leaked in MorpyPolygon\n");
	poly->NoHoles = NULL;
	flags = poly->Flags;
	pcb_poly_remove(layer, poly);
	inhibit = rnd_true;
	do {
		rnd_vnode_t *v;
		pcb_poly_t *newone;

		if (p->contours->area > conf_core.design.poly_isle_area) {
			newone = pcb_poly_new(layer, poly->Clearance, flags);
			if (!newone)
				return rnd_false;
			many = rnd_true;
			v = p->contours->head;
			pcb_poly_point_new(newone, v->point[0], v->point[1]);
			for (v = v->next; v != p->contours->head; v = v->next)
				pcb_poly_point_new(newone, v->point[0], v->point[1]);
			newone->BoundingBox.X1 = p->contours->xmin;
			newone->BoundingBox.X2 = p->contours->xmax + 1;
			newone->BoundingBox.Y1 = p->contours->ymin;
			newone->BoundingBox.Y2 = p->contours->ymax + 1;
			pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, newone, newone);
			newone->Clipped = p;
			p = p->f;									/* go to next pline */
			newone->Clipped->b = newone->Clipped->f = newone->Clipped;	/* unlink from others */
			rnd_r_insert_entry(layer->polygon_tree, (rnd_box_t *) newone);
			pcb_poly_invalidate_draw(layer, newone);
		}
		else {
			rnd_polyarea_t *t = p;

			p = p->f;
			rnd_poly_contour_del(&t->contours);
			free(t);
		}
	}
	while (p != start);
	inhibit = rnd_false;
	pcb_undo_inc_serial();
	return many;
}

void debug_pline(rnd_pline_t * pl)
{
	rnd_vnode_t *v;
	rnd_fprintf(stderr, "\txmin %#mS xmax %#mS ymin %#mS ymax %#mS\n", pl->xmin, pl->xmax, pl->ymin, pl->ymax);
	v = pl->head;
	while (v) {
		rnd_fprintf(stderr, "\t\tvnode: %#mD\n", v->point[0], v->point[1]);
		v = v->next;
		if (v == pl->head)
			break;
	}
}

void debug_polyarea(rnd_polyarea_t * p)
{
	rnd_pline_t *pl;

	fprintf(stderr, "rnd_polyarea_t %p\n", (void *)p);
	for (pl = p->contours; pl; pl = pl->next)
		debug_pline(pl);
}

void debug_polygon(pcb_poly_t * p)
{
	rnd_cardinal_t i;
	rnd_polyarea_t *pa;
	fprintf(stderr, "POLYGON %p  %d pts\n", (void *)p, p->PointN);
	for (i = 0; i < p->PointN; i++)
		rnd_fprintf(stderr, "\t%d: %#mD\n", i, p->Points[i].X, p->Points[i].Y);
	if (p->HoleIndexN) {
		fprintf(stderr, "%d holes, starting at indices\n", p->HoleIndexN);
		for (i = 0; i < p->HoleIndexN; i++)
			fprintf(stderr, "\t%d: %d\n", i, p->HoleIndex[i]);
	}
	else
		fprintf(stderr, "it has no holes\n");
	pa = p->Clipped;
	while (pa) {
		debug_polyarea(pa);
		pa = pa->f;
		if (pa == p->Clipped)
			break;
	}
}

/* Convert a rnd_polyarea_t (and all linked rnd_polyarea_t) to
 * raw PCB polygons on the given layer.
 */
void pcb_poly_to_polygons_on_layer(pcb_data_t * Destination, pcb_layer_t * Layer, rnd_polyarea_t * Input, pcb_flag_t Flags)
{
	pcb_poly_t *Polygon;
	rnd_polyarea_t *pa;
	rnd_pline_t *pline;
	rnd_vnode_t *node;
	rnd_bool outer;

	if (Input == NULL)
		return;

	pa = Input;
	do {
		Polygon = pcb_poly_new(Layer, 2 * conf_core.design.clearance, Flags);

		pline = pa->contours;
		outer = rnd_true;

		do {
			if (!outer)
				pcb_poly_hole_new(Polygon);
			outer = rnd_false;

			node = pline->head;
			do {
				pcb_poly_point_new(Polygon, node->point[0], node->point[1]);
			}
			while ((node = node->next) != pline->head);

		}
		while ((pline = pline->next) != NULL);

		pcb_poly_init_clip(Destination, Layer, Polygon);
		pcb_poly_bbox(Polygon);
		if (!Layer->polygon_tree)
			Layer->polygon_tree = rnd_r_create_tree();
		rnd_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);

		pcb_poly_invalidate_draw(Layer, Polygon);
		/* add to undo list */
		pcb_undo_add_obj_to_create(PCB_OBJ_POLY, Layer, Polygon, Polygon);
	}
	while ((pa = pa->f) != Input);

	pcb_board_set_changed_flag(PCB, rnd_true);
}

rnd_bool pcb_pline_is_rectangle(rnd_pline_t *pl)
{
	int n;
	rnd_coord_t x[4], y[4];
	rnd_vnode_t *v;

	v = pl->head->next;
	n = 0;
	do {
		x[n] = v->point[0];
		y[n] = v->point[1];
		n++;
		v = v->next;
	} while((n < 4) && (v != pl->head->next));
	
	if (n != 4)
		return rnd_false;

	if (sqr(x[0] - x[2]) + sqr(y[0] - y[2]) == sqr(x[1] - x[3]) + sqr(y[1] - y[3]))
		return rnd_true;

	return rnd_false;
}

