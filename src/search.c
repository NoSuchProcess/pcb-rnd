/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */


/* search routines
 * some of the functions use dummy parameters
 */

#include "config.h"
#include "conf_core.h"

#include <math.h>

#include "actions_pcb.h"
#include "board.h"
#include "data.h"
#include <librnd/core/error.h>
#include <librnd/core/actions.h>
#include <librnd/poly/polyarea.h>
#include "find.h"
#include "polygon.h"
#include "search.h"
#include "select.h"
#include "obj_subc_parent.h"
#include "obj_pstk_inlines.h"

static double PosX, PosY;				/* search position for subroutines */
static rnd_coord_t SearchRadius;
static rnd_box_t SearchBox;
static pcb_layer_t *SearchLayer;

/* ---------------------------------------------------------------------------
 * The first parameter includes PCB_OBJ_LOCKED if we
 * want to include locked object in the search and PCB_OBJ_SUBC_PART if
 * objects that are part of a subcircuit should be found.
 */
static rnd_bool SearchLineByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_line_t **, pcb_line_t **);
static rnd_bool SearchArcByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_arc_t **, pcb_arc_t **);
static rnd_bool SearchGfxByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_gfx_t **, pcb_gfx_t **);
static rnd_bool SearchRatLineByLocation(unsigned long, unsigned long, pcb_rat_t **, pcb_rat_t **, pcb_rat_t **);
static rnd_bool SearchTextByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_text_t **, pcb_text_t **);
static rnd_bool SearchPolygonByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_poly_t **, pcb_poly_t **);
static rnd_bool SearchLinePointByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_line_t **, rnd_point_t **);
static rnd_bool SearchArcPointByLocation(unsigned long, unsigned long, pcb_layer_t **, pcb_arc_t **, int **);
static rnd_bool SearchPointByLocation(unsigned long, unsigned long, unsigned long, pcb_layer_t **, pcb_poly_t **, rnd_point_t **);
static rnd_bool SearchSubcByLocation(unsigned long, unsigned long, pcb_subc_t **, pcb_subc_t **, pcb_subc_t **, rnd_bool);

/* Return not-found for subc parts and locked items unless objst says otherwise
   obj is the object to be checked if part of subc; check lock on locked_obj
   Also return not-found if flags are required but the object doesn't have them */
#define TEST_OBJST_(objst, req_flag, locality, obj, locked_obj, reject) \
do { \
	if ((req_flag != 0) && (!PCB_FLAG_TEST(req_flag, obj))) \
		reject; \
	if (!(objst & PCB_OBJ_SUBC_PART) && (pcb_ ## locality ## obj_parent_subc(obj->parent_type, &obj->parent))) \
		reject; \
	if (!(objst & PCB_OBJ_LOCKED) && (PCB_FLAG_TEST(objst & PCB_FLAG_LOCK, locked_obj))) \
		reject; \
} while(0)

#define TEST_OBJST(objst, req_flag, locality, obj, locked_obj) \
	TEST_OBJST_(objst, req_flag, locality, obj, locked_obj, return RND_R_DIR_NOT_FOUND)

struct ans_info {
	void **ptr1, **ptr2, **ptr3;
	rnd_bool BackToo;
	double area;
	unsigned long objst, req_flag;
	int on_current;
	pcb_layer_type_t on_lyt;
};

/* ---------------------------------------------------------------------------
 * searches a padstack
 */
static rnd_r_dir_t padstack_callback(const rnd_box_t *box, void *cl)
{
	struct ans_info *i = (struct ans_info *) cl;
	pcb_pstk_t *ps = (pcb_pstk_t *) box;
	pcb_data_t *pdata;

	TEST_OBJST(i->objst, i->req_flag, g, ps, ps);

	if (!pcb_is_point_in_pstk(PosX, PosY, SearchRadius, ps, NULL))
		return RND_R_DIR_NOT_FOUND;
	if ((i->on_current) && (pcb_pstk_shape_at(PCB, ps, PCB_CURRLAYER(PCB)) == NULL) && (!pcb_pstk_bb_drills(PCB, ps, PCB_CURRLAYER(PCB)->meta.real.grp, NULL)))
		return RND_R_DIR_NOT_FOUND;
	if ((i->on_lyt != 0) && (pcb_pstk_shape(ps, i->on_lyt, 0) == NULL))
		return RND_R_DIR_NOT_FOUND;
	*i->ptr2 = *i->ptr3 = ps;
	assert(ps->parent_type == PCB_PARENT_DATA);
	pdata = ps->parent.data;
	if (pdata->parent_type == PCB_PARENT_SUBC)
		*i->ptr1 = pdata->parent.subc; /* padstack of a subcircuit */
	else
		*i->ptr1 = ps; /* padstack on a board (e.g. via) */
	return RND_R_DIR_CANCEL; /* found, stop searching */
}

static rnd_bool SearchPadstackByLocation(unsigned long objst, unsigned long req_flag, pcb_pstk_t **ps, pcb_pstk_t **Dummy1, pcb_pstk_t **Dummy2, int on_current, pcb_layer_type_t on_lyt)
{
	struct ans_info info;

	/* search only if via-layer is visible */
	if (!PCB->pstk_on)
		return rnd_false;

	info.ptr1 = (void **)ps;
	info.ptr2 = (void **)Dummy1;
	info.ptr3 = (void **)Dummy2;
	info.objst = objst;
	info.req_flag = req_flag;
	info.on_current = on_current;
	info.on_lyt = on_lyt;

	if (rnd_r_search(PCB->Data->padstack_tree, &SearchBox, NULL, padstack_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * searches ordinary line on the SearchLayer
 */

struct line_info {
	pcb_line_t **Line;
	rnd_point_t **Point;
	double least;
	unsigned long objst, req_flag;
};


static rnd_r_dir_t line_callback(const rnd_box_t * box, void *cl)
{
	struct line_info *i = (struct line_info *) cl;
	pcb_line_t *l = (pcb_line_t *) box;

	TEST_OBJST(i->objst, i->req_flag, l, l, l);

	if (!pcb_is_point_in_line(PosX, PosY, SearchRadius, (pcb_any_line_t *)l))
		return RND_R_DIR_NOT_FOUND;
	*i->Line = l;
	*i->Point = (rnd_point_t *) l;

	return RND_R_DIR_CANCEL; /* found what we were looking for */
}


static rnd_bool SearchLineByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_line_t ** Line, pcb_line_t ** Dummy)
{
	struct line_info info;

	info.Line = Line;
	info.Point = (rnd_point_t **) Dummy;
	info.objst = objst;
	info.req_flag = req_flag;

	*Layer = SearchLayer;
	if (rnd_r_search(SearchLayer->line_tree, &SearchBox, NULL, line_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;

	return rnd_false;
}

static rnd_r_dir_t rat_callback(const rnd_box_t * box, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) box;
	struct ans_info *i = (struct ans_info *) cl;

	TEST_OBJST(i->objst, i->req_flag, l, line, line);

	if (PCB_FLAG_TEST(PCB_FLAG_VIA, line) ?
			(rnd_distance(line->Point1.X, line->Point1.Y, PosX, PosY) <=
			 line->Thickness * 2 + SearchRadius) : pcb_is_point_on_line(PosX, PosY, SearchRadius, line)) {
		*i->ptr1 = *i->ptr2 = *i->ptr3 = line;
		return RND_R_DIR_CANCEL;
	}
	return RND_R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches rat lines if they are visible
 */
static rnd_bool SearchRatLineByLocation(unsigned long objst, unsigned long req_flag, pcb_rat_t ** Line, pcb_rat_t ** Dummy1, pcb_rat_t ** Dummy2)
{
	struct ans_info info;

	info.ptr1 = (void **) Line;
	info.ptr2 = (void **) Dummy1;
	info.ptr3 = (void **) Dummy2;
	info.objst = objst;
	info.req_flag = req_flag;

	if (rnd_r_search(PCB->Data->rat_tree, &SearchBox, NULL, rat_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * searches arc on the SearchLayer
 */
struct arc_info {
	pcb_arc_t **Arc, **Dummy;
	unsigned long objst, req_flag;
	int **arc_pt; /* 0=start, 1=end (start+delta) */
	double least;
};

static rnd_r_dir_t arc_callback(const rnd_box_t * box, void *cl)
{
	struct arc_info *i = (struct arc_info *) cl;
	pcb_arc_t *a = (pcb_arc_t *) box;

	TEST_OBJST(i->objst, i->req_flag, l, a, a);

	if (!pcb_is_point_on_arc(PosX, PosY, SearchRadius, a))
		return 0;
	*i->Arc = a;
	*i->Dummy = a;
	return RND_R_DIR_CANCEL; /* found */
}


static rnd_bool SearchArcByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_arc_t ** Arc, pcb_arc_t ** Dummy)
{
	struct arc_info info;

	info.Arc = Arc;
	info.Dummy = Dummy;
	info.objst = objst;
	info.req_flag = req_flag;

	*Layer = SearchLayer;
	if (rnd_r_search(SearchLayer->arc_tree, &SearchBox, NULL, arc_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * searches gfx on the SearchLayer
 */
struct gfx_info {
	pcb_gfx_t **Gfx, **Dummy;
	unsigned long objst, req_flag;
	double least;
};

static rnd_r_dir_t gfx_callback(const rnd_box_t *box, void *cl)
{
	struct gfx_info *i = (struct gfx_info *)cl;
	pcb_gfx_t *g = (pcb_gfx_t *)box;

	TEST_OBJST(i->objst, i->req_flag, l, g, g);

	if (!pcb_is_point_in_gfx(PosX, PosY, SearchRadius, g))
		return 0;
	*i->Gfx = g;
	*i->Dummy = g;
	return RND_R_DIR_CANCEL; /* found */
}

static rnd_bool SearchGfxByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t **Layer, pcb_gfx_t **gfx, pcb_gfx_t **Dummy)
{
	struct gfx_info info;

	info.Gfx = gfx;
	info.Dummy = Dummy;
	info.objst = objst;
	info.req_flag = req_flag;

	*Layer = SearchLayer;
	if (rnd_r_search(SearchLayer->gfx_tree, &SearchBox, NULL, gfx_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;
	return rnd_false;
}

static rnd_r_dir_t text_callback(const rnd_box_t * box, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) box;
	struct ans_info *i = (struct ans_info *) cl;

	TEST_OBJST(i->objst, i->req_flag, l, text, text);

	if (PCB_POINT_IN_BOX(PosX, PosY, &text->BoundingBox)) {
		*i->ptr2 = *i->ptr3 = text;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}

/* ---------------------------------------------------------------------------
 * searches text on the SearchLayer
 */
static rnd_bool SearchTextByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_text_t ** Text, pcb_text_t ** Dummy)
{
	struct ans_info info;

	*Layer = SearchLayer;
	info.ptr2 = (void **) Text;
	info.ptr3 = (void **) Dummy;
	info.objst = objst;
	info.req_flag = req_flag;

	if (rnd_r_search(SearchLayer->text_tree, &SearchBox, NULL, text_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;
	return rnd_false;
}

static rnd_r_dir_t polygon_callback(const rnd_box_t * box, void *cl)
{
	pcb_poly_t *polygon = (pcb_poly_t *) box;
	struct ans_info *i = (struct ans_info *) cl;

	TEST_OBJST(i->objst, i->req_flag, l, polygon, polygon);

	if (polygon->Clipped == NULL)
		return RND_R_DIR_NOT_FOUND; /* polygon cleared out of existence */

	if (pcb_poly_is_point_in_p(PosX, PosY, SearchRadius, polygon)) {
		*i->ptr2 = *i->ptr3 = polygon;
		return RND_R_DIR_CANCEL; /* found */
	}
	return RND_R_DIR_NOT_FOUND;
}


/* ---------------------------------------------------------------------------
 * searches a polygon on the SearchLayer
 */
static rnd_bool SearchPolygonByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_poly_t ** Polygon, pcb_poly_t ** Dummy)
{
	struct ans_info info;

	*Layer = SearchLayer;
	info.ptr2 = (void **) Polygon;
	info.ptr3 = (void **) Dummy;
	info.objst = objst;
	info.req_flag = req_flag;

	if (rnd_r_search(SearchLayer->polygon_tree, &SearchBox, NULL, polygon_callback, &info, NULL) != RND_R_DIR_NOT_FOUND)
		return rnd_true;
	return rnd_false;
}

static rnd_r_dir_t linepoint_callback(const rnd_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct line_info *i = (struct line_info *) cl;
	rnd_r_dir_t ret_val = RND_R_DIR_NOT_FOUND;
	double d;

	TEST_OBJST(i->objst, i->req_flag, l, line, line);

	/* some stupid code to check both points */
	d = rnd_distance(PosX, PosY, line->Point1.X, line->Point1.Y);
	if (d < i->least) {
		i->least = d;
		*i->Line = line;
		*i->Point = &line->Point1;
		ret_val = RND_R_DIR_FOUND_CONTINUE;
	}

	d = rnd_distance(PosX, PosY, line->Point2.X, line->Point2.Y);
	if (d < i->least) {
		i->least = d;
		*i->Line = line;
		*i->Point = &line->Point2;
		ret_val = RND_R_DIR_FOUND_CONTINUE;
	}
	return ret_val;
}

static rnd_r_dir_t arcpoint_callback(const rnd_box_t * b, void *cl)
{
	rnd_box_t ab;
	pcb_arc_t *arc = (pcb_arc_t *) b;
	struct arc_info *i = (struct arc_info *) cl;
	rnd_r_dir_t ret_val = RND_R_DIR_NOT_FOUND;
	double d;

	TEST_OBJST(i->objst, i->req_flag, l, arc, arc);

	/* some stupid code to check both points */
	pcb_arc_get_end(arc, 0, &ab.X1, &ab.Y1);
	pcb_arc_get_end(arc, 1, &ab.X2, &ab.Y2);

	d = rnd_distance(PosX, PosY, ab.X1, ab.Y1);
	if (d < i->least) {
		i->least = d;
		*i->Arc = arc;
		*i->arc_pt = pcb_arc_start_ptr;
		ret_val = RND_R_DIR_FOUND_CONTINUE;
	}

	d = rnd_distance(PosX, PosY, ab.X2, ab.Y2);
	if (d < i->least) {
		i->least = d;
		*i->Arc = arc;
		*i->arc_pt = pcb_arc_end_ptr;
		ret_val = RND_R_DIR_FOUND_CONTINUE;
	}
	return ret_val;
}

/* ---------------------------------------------------------------------------
 * searches a line-point on all the search layer
 */
static rnd_bool SearchLinePointByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_line_t ** Line, rnd_point_t ** Point)
{
	struct line_info info;
	*Layer = SearchLayer;
	info.Line = Line;
	info.Point = Point;
	*Point = NULL;
	info.least = RND_MAX_LINE_POINT_DISTANCE + SearchRadius;
	info.objst = objst;
	info.req_flag = req_flag;

	if (rnd_r_search(SearchLayer->line_tree, &SearchBox, NULL, linepoint_callback, &info, NULL))
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * searches a line-point on all the search layer
 */
static rnd_bool SearchArcPointByLocation(unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_arc_t ** Arc, int **Point)
{
	struct arc_info info;
	*Layer = SearchLayer;
	info.Arc = Arc;
	info.arc_pt = Point;
	*Point = NULL;
	info.least = RND_MAX_LINE_POINT_DISTANCE + SearchRadius;
	info.objst = objst;
	info.req_flag = req_flag;

	if (rnd_r_search(SearchLayer->arc_tree, &SearchBox, NULL, arcpoint_callback, &info, NULL))
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * searches a polygon-point on all layers that are switched on
 * in layerstack order
 */
typedef struct {
	double least;
	rnd_bool found;
	unsigned long Type;

	/* result */
	pcb_poly_t **Polygon;
	rnd_point_t **Point;
} ptcb_t;

static rnd_r_dir_t polypoint_callback(const rnd_box_t *box, void *cl)
{
	pcb_poly_t *polygon = (pcb_poly_t *)box;
	ptcb_t *ctx = (ptcb_t *)cl;
	double d;
	pcb_data_t *dt;

	dt = polygon->parent.layer->parent.data; /* polygon -> layer -> data */
	if ((dt != NULL) && (dt->parent_type == PCB_PARENT_SUBC)) {
		/* do not find subc part poly points if not explicitly requested */
		if (!(ctx->Type & PCB_OBJ_SUBC_PART))
			return RND_R_DIR_NOT_FOUND;

		/* don't find subc poly points even as subc part unless we are editing a subc */
		if (!PCB->loose_subc)
			return RND_R_DIR_NOT_FOUND;
	}

	PCB_POLY_POINT_LOOP(polygon);
	{
		d = rnd_distance2(point->X, point->Y, PosX, PosY);
		if (d < ctx->least) {
			ctx->least = d;
			*ctx->Polygon = polygon;
			*ctx->Point = point;
			ctx->found = rnd_true;
		}
	}
	PCB_END_LOOP;

	return RND_R_DIR_NOT_FOUND;
}

static rnd_bool SearchPointByLocation(unsigned long Type, unsigned long objst, unsigned long req_flag, pcb_layer_t ** Layer, pcb_poly_t ** Polygon, rnd_point_t ** Point)
{
	ptcb_t ctx;

	*Layer = SearchLayer;
	ctx.Type = Type;
	ctx.Polygon = Polygon;
	ctx.Point = Point;
	ctx.found = rnd_false;;
	ctx.least = SearchRadius + RND_MAX_POLYGON_POINT_DISTANCE;
	ctx.least = ctx.least * ctx.least;
	rnd_r_search(SearchLayer->polygon_tree, &SearchBox, NULL, polypoint_callback, &ctx, NULL);

	if (ctx.found)
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * searches a gfx-point on all layers that are switched on
 * in layerstack order
 */
typedef struct {
	double least;
	rnd_bool found;
	unsigned long Type;

	/* result */
	pcb_gfx_t **gfx;
	rnd_point_t **Point;
} gfxptcb_t;

static rnd_r_dir_t gfxpoint_callback(const rnd_box_t *box, void *cl)
{
	pcb_gfx_t *gfx = (pcb_gfx_t *)box;
	gfxptcb_t *ctx = (gfxptcb_t *)cl;
	double d;
	pcb_data_t *dt;
	int n;

	dt = gfx->parent.layer->parent.data; /* gfx -> layer -> data */
	if ((dt != NULL) && (dt->parent_type == PCB_PARENT_SUBC)) {
		/* do not find subc part poly points if not explicitly requested */
		if (!(ctx->Type & PCB_OBJ_SUBC_PART))
			return RND_R_DIR_NOT_FOUND;

		/* don't find subc poly points even as subc part unless we are editing a subc */
		if (!PCB->loose_subc)
			return RND_R_DIR_NOT_FOUND;
	}

	for(n = 0; n < 4; n++) {
		d = rnd_distance2(gfx->corner[n].X, gfx->corner[n].Y, PosX, PosY);
		if (d < ctx->least) {
			ctx->least = d;
			*ctx->gfx = gfx;
			*ctx->Point = &gfx->corner[n];
			ctx->found = rnd_true;
		}
	}

	return RND_R_DIR_NOT_FOUND;
}

static rnd_bool SearchGfxPointByLocation(unsigned long Type, unsigned long objst, unsigned long req_flag, pcb_layer_t **Layer, pcb_gfx_t **gfx, rnd_point_t **Point)
{
	gfxptcb_t ctx;

	*Layer = SearchLayer;
	ctx.Type = Type;
	ctx.gfx = gfx;
	ctx.Point = Point;
	ctx.found = rnd_false;;
	ctx.least = SearchRadius + RND_MAX_POLYGON_POINT_DISTANCE;
	ctx.least = ctx.least * ctx.least;
	rnd_r_search(SearchLayer->gfx_tree, &SearchBox, NULL, gfxpoint_callback, &ctx, NULL);

	if (ctx.found)
		return rnd_true;
	return rnd_false;
}


static rnd_r_dir_t subc_callback(const rnd_box_t *box, void *cl)
{
	pcb_subc_t *subc = (pcb_subc_t *) box;
	struct ans_info *i = (struct ans_info *) cl;
	double newarea;
	int subc_on_bottom = 0, front;

	pcb_subc_get_side(subc, &subc_on_bottom);
	front = subc_on_bottom == conf_core.editor.show_solder_side;

	TEST_OBJST(i->objst, i->req_flag, g, subc, subc);

	if ((front || i->BackToo) && PCB_POINT_IN_BOX(PosX, PosY, &subc->BoundingBox)) {
		rnd_coord_t ox, oy;
		if ((subc->extobj != NULL) && (pcb_subc_get_origin(subc, &ox, &oy) == 0)) {
			/* extended objects are special case: only the origin should be clickable
			   to avoid problems with large extended objects that cover the board */
			if ((PosX < ox) || (PosX > ox + PCB_SUBC_AUX_UNIT) || (PosY < oy) || (PosY > oy + PCB_SUBC_AUX_UNIT))
				return RND_R_DIR_NOT_FOUND;
		}
		/* use the subcircuit with the smallest bounding box */
		newarea = (subc->BoundingBox.X2 - subc->BoundingBox.X1) * (double) (subc->BoundingBox.Y2 - subc->BoundingBox.Y1);
		if (newarea < i->area) {
			i->area = newarea;
			*i->ptr1 = *i->ptr2 = *i->ptr3 = subc;
			return RND_R_DIR_FOUND_CONTINUE;
		}
	}
	return RND_R_DIR_NOT_FOUND;
}


/* ---------------------------------------------------------------------------
 * searches a subcircuit
 * if more than one subc matches, the smallest one is taken
 */
static rnd_bool
SearchSubcByLocation(unsigned long objst, unsigned long req_flag, pcb_subc_t **subc, pcb_subc_t ** Dummy1, pcb_subc_t ** Dummy2, rnd_bool BackToo)
{
	struct ans_info info;

	/* Both package layers have to be switched on */
	info.ptr1 = (void **) subc;
	info.ptr2 = (void **) Dummy1;
	info.ptr3 = (void **) Dummy2;
	info.area = RND_SQUARE(RND_MAX_COORD);
	info.BackToo = (BackToo && PCB->InvisibleObjectsOn);
	info.objst = objst;
	info.req_flag = req_flag;

	if (rnd_r_search(PCB->Data->subc_tree, &SearchBox, NULL, subc_callback, &info, NULL))
		return rnd_true;
	return rnd_false;
}

/* find the first floater on any layer */
static rnd_bool SearchSubcFloaterByLocation(unsigned long objst, unsigned long req_flag, pcb_subc_t **out_subc, pcb_text_t **out_txt, void **dummy, rnd_bool other_side)
{
	rnd_rtree_it_t it;
	void *obj;
	int n;
	pcb_layer_type_t my_side_lyt;

	if (other_side)
		my_side_lyt = conf_core.editor.show_solder_side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	else
		my_side_lyt = conf_core.editor.show_solder_side ? PCB_LYT_BOTTOM : PCB_LYT_TOP;

	for(n = 0; n < PCB->Data->LayerN; n++)  {
		pcb_layer_t *ly = &PCB->Data->Layer[n];
		pcb_layer_type_t lyt;

		if ((ly->text_tree == NULL) || (!ly->meta.real.vis))
			continue;

		lyt = pcb_layer_flags_(ly);
		if (!(lyt & my_side_lyt))
			continue;

		for(obj = rnd_rtree_first(&it, ly->text_tree, (rnd_rtree_box_t *)&SearchBox); obj != NULL; obj = rnd_rtree_next(&it)) {
			pcb_text_t *txt = (pcb_text_t *)obj;
			pcb_subc_t *subc;

			if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, txt) == 0)
				continue;

			subc = pcb_lobj_parent_subc(txt->parent_type, &txt->parent);
			if (subc == NULL)
				continue;

			TEST_OBJST_(objst, req_flag, l, txt, subc, continue);

			*out_subc = subc;
			*out_txt = txt;
			*dummy = txt;
			return rnd_true;
		}
	}

	return rnd_false;
}

/* for checking if a rat-line end is on a PV */
rnd_bool pcb_is_point_on_line_end(rnd_coord_t X, rnd_coord_t Y, pcb_rat_t *Line)
{
	if (((X == Line->Point1.X) && (Y == Line->Point1.Y)) || ((X == Line->Point2.X) && (Y == Line->Point2.Y)))
		return rnd_true;
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * Calculate the distance (squared) from a point to a line.
 *
 * let the point be (X,Y) and the line (X1,Y1)(X2,Y2)
 *
 * Calculate the position along the line that is closest to X,Y. The value 
 * ranges from 0 to 1 when the point is within the line boundaries with a 
 * value of 0 being at point X1,Y1 and a value of 1.0 being at point X2,Y2.
  *
 *   t = ((X-X1)*(X2-X1)) + ((Y-Y1)*(Y2-Y1))
 *       -----------------------------------
 *             (X2-X1)^2 + (Y2-Y1)^2
 *
 * let Q be the point of perpendicular projection of (X,Y) onto the line
 *
 *   QX = X1 + t * (X2-X1)
 *   QY = Y1 + t * (Y2-Y1)
 *
 * Calculate the distace (squared) using Pythagorean theorem.
 *
 *   distance^2 = (X-QX)^2 + (Y-QY)^2
 */

double pcb_point_line_dist2(rnd_coord_t X, rnd_coord_t Y, pcb_line_t *Line)
{
	const double abx = Line->Point2.X - Line->Point1.X;
	const double aby = Line->Point2.Y - Line->Point1.Y;
	const double apx = X - Line->Point1.X;
	const double apy = Y - Line->Point1.Y;
	double qx,qy,dx,dy;

	/* Calculate t, the normalised position along the line of the closest point 
	 * to (X,Y). A value between 0 and 1 indicates that the closest point on the 
	 * line is within the bounds of the end points. 
	 */
	double t = ( (apx * abx) + (apy * aby) ) / ( (abx * abx) + (aby * aby) );

	/* Clamp 't' to the line bounds */
	if(t < 0.0)		t = 0.0;
	if(t > 1.0)		t = 1.0;

	/* Calculate the point Q on the line that is closest to (X,Y) */
	qx = Line->Point1.X + (t * abx);
	qy = Line->Point1.Y + (t * aby);

	/* Return the distance from Q to (X,Y), squared */
	dx = X - qx;
	dy = Y - qy;

	return (dx * dx) + (dy * dy);
}

rnd_bool pcb_is_point_on_line(rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Radius, pcb_line_t *Line)
{
	double max = Radius + Line->Thickness / 2;

	return pcb_point_line_dist2(X, Y, Line) < (max * max);
}

static int is_point_on_line(rnd_coord_t px, rnd_coord_t py, rnd_coord_t lx1, rnd_coord_t ly1, rnd_coord_t lx2, rnd_coord_t ly2)
{
	/* ohh well... let's hope the optimizer does something clever with inlining... */
	pcb_line_t l;
	l.Point1.X = lx1;
	l.Point1.Y = ly1;
	l.Point2.X = lx2;
	l.Point2.Y = ly2;
	l.Thickness = 1;
	return pcb_is_point_on_line(px, py, 1, &l);
}

rnd_bool pcb_is_point_on_thinline( rnd_coord_t X, rnd_coord_t Y, rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2,rnd_coord_t Y2 )
{
	/* Calculate the cross product of the vector from the first line point to the test point
	 * and the vector from the first line point to the second line point. If the result is 
	 * not 0 then the point does not lie on the line.
	 */
	const rnd_coord_t lx = X2 - X1;
	const rnd_coord_t ly = Y2 - Y1;
	const rnd_coord_t cross = ((X-X1) * ly) - ((Y-Y1) * lx);

	if(cross != 0)
		return rnd_false;

	/* If the point does lie on the line then we do a simple check to see if the point is
	 * between the end points. Use the longest side of the line to perform the check.
	 */
	if(abs(lx) > abs(ly))
		return (X >= MIN(X1,X2)) && (X <= MAX(X1,X2)) ? rnd_true : rnd_false;

	return (Y >= MIN(Y1,Y2)) && (Y <= MAX(Y1,Y2)) ? rnd_true : rnd_false;
}

/* checks if a line crosses a rectangle or is within the rectangle */
rnd_bool pcb_is_line_in_rectangle(rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, pcb_line_t *Line)
{
	pcb_line_t line;

	/* first, see if point 1 is inside the rectangle */
	/* in case the whole line is inside the rectangle */
	if (X1 < Line->Point1.X && X2 > Line->Point1.X && Y1 < Line->Point1.Y && Y2 > Line->Point1.Y)
		return rnd_true;
	/* construct a set of dummy lines and check each of them */
	line.Thickness = 0;
	line.Flags = pcb_no_flags();

	/* upper-left to upper-right corner */
	line.Point1.Y = line.Point2.Y = Y1;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	/* upper-right to lower-right corner */
	line.Point1.X = X2;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	/* lower-right to lower-left corner */
	line.Point1.Y = Y2;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	/* lower-left to upper-left corner */
	line.Point2.X = X1;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	return rnd_false;
}

/*checks if a point (of null radius) is in a slanted rectangle */
static int IsPointInQuadrangle(rnd_point_t p[4], rnd_point_t *l)
{
	rnd_coord_t dx, dy, x, y;
	double prod0, prod1;

	dx = p[1].X - p[0].X;
	dy = p[1].Y - p[0].Y;
	x = l->X - p[0].X;
	y = l->Y - p[0].Y;
	prod0 = (double) x *dx + (double) y *dy;
	x = l->X - p[1].X;
	y = l->Y - p[1].Y;
	prod1 = (double) x *dx + (double) y *dy;
	if (prod0 * prod1 <= 0) {
		dx = p[1].X - p[2].X;
		dy = p[1].Y - p[2].Y;
		prod0 = (double) x *dx + (double) y *dy;
		x = l->X - p[2].X;
		y = l->Y - p[2].Y;
		prod1 = (double) x *dx + (double) y *dy;
		if (prod0 * prod1 <= 0)
			return rnd_true;
	}
	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * checks if a line crosses a quadrangle or is within the quadrangle: almost
 * copied from pcb_is_line_in_rectangle()
 * Note: actually this quadrangle is a slanted rectangle
 */
rnd_bool pcb_is_line_in_quadrangle(rnd_point_t p[4], pcb_line_t *Line)
{
	pcb_line_t line;

	/* first, see if point 1 is inside the rectangle */
	/* in case the whole line is inside the rectangle */
	if (IsPointInQuadrangle(p, &(Line->Point1)))
		return rnd_true;
	if (IsPointInQuadrangle(p, &(Line->Point2)))
		return rnd_true;
	/* construct a set of dummy lines and check each of them */
	line.Thickness = 0;
	line.Flags = pcb_no_flags();

	/* upper-left to upper-right corner */
	line.Point1.X = p[0].X;
	line.Point1.Y = p[0].Y;
	line.Point2.X = p[1].X;
	line.Point2.Y = p[1].Y;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	/* upper-right to lower-right corner */
	line.Point1.X = p[2].X;
	line.Point1.Y = p[2].Y;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	/* lower-right to lower-left corner */
	line.Point2.X = p[3].X;
	line.Point2.Y = p[3].Y;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	/* lower-left to upper-left corner */
	line.Point1.X = p[0].X;
	line.Point1.Y = p[0].Y;
	if (pcb_isc_line_line(pcb_find0, &line, Line))
		return rnd_true;

	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * checks if an arc crosses a rectangle (or arc is within the rectangle)
 */
rnd_bool pcb_is_arc_in_rectangle(rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, pcb_arc_t *Arc)
{
	pcb_line_t line;
	rnd_coord_t x, y;
	rnd_box_t box;
	
	/* check if any of arc endpoints is inside the rectangle */
	box.X1 = X1; box.Y1 = Y1;
	box.X2 = X2; box.Y2 = Y2;
	pcb_arc_get_end (Arc, 0, &x, &y);
	if (PCB_POINT_IN_BOX(x, y, &box))
		return rnd_true;
	pcb_arc_get_end (Arc, 1, &x, &y);
	if (PCB_POINT_IN_BOX(x, y, &box))
		return rnd_true;

	/* construct a set of dummy lines and check each of them */
	line.Thickness = 0;
	line.Flags = pcb_no_flags();

	/* upper-left to upper-right corner */
	line.Point1.Y = line.Point2.Y = Y1;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_isc_line_arc(pcb_find0, &line, Arc))
		return rnd_true;

	/* upper-right to lower-right corner */
	line.Point1.X = line.Point2.X = X2;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_isc_line_arc(pcb_find0, &line, Arc))
		return rnd_true;

	/* lower-right to lower-left corner */
	line.Point1.Y = line.Point2.Y = Y2;
	line.Point1.X = X1;
	line.Point2.X = X2;
	if (pcb_isc_line_arc(pcb_find0, &line, Arc))
		return rnd_true;

	/* lower-left to upper-left corner */
	line.Point1.X = line.Point2.X = X1;
	line.Point1.Y = Y1;
	line.Point2.Y = Y2;
	if (pcb_isc_line_arc(pcb_find0, &line, Arc))
		return rnd_true;

	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * checks if an gfx crosses a rectangle (or gfx is within the rectangle)
 */
rnd_bool pcb_is_gfx_in_rectangle(const rnd_box_t *b, const pcb_gfx_t *gfx)
{
	pcb_line_t l;
	int n, m;

	l.Thickness = 1;
	for(n = 0; n < 4; n++) {
		m = n+1;
		if (m == 4)
			m = 0;
		l.Point1.X = gfx->corner[n].X; l.Point1.Y = gfx->corner[n].Y;
		l.Point2.X = gfx->corner[m].X; l.Point2.Y = gfx->corner[m].Y;
		if (PCB_LINE_TOUCHES_BOX(&l, b))
			return rnd_true;
	}

	return rnd_false;
}

/* ---------------------------------------------------------------------------
 * Check if a circle of Radius with center at (X, Y) intersects a line.
 * Written to enable arbitrary line directions; for rounded/square lines, too.
 */
rnd_bool pcb_is_point_in_line(rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Radius, pcb_any_line_t *Pad)
{
	double r, Sin, Cos;
	rnd_coord_t x;

	/* Also used from line_callback with line type smaller than pad type;
	   use the smallest common subset; ->Thickness is still ok. */
	rnd_coord_t t2 = (Pad->Thickness + 1) / 2, range;
	pcb_any_line_t pad = *Pad;


	/* series of transforms saving range */
	/* move Point1 to the origin */
	X -= pad.Point1.X;
	Y -= pad.Point1.Y;

	pad.Point2.X -= pad.Point1.X;
	pad.Point2.Y -= pad.Point1.Y;
	/* so, pad.Point1.X = pad.Point1.Y = 0; */

	/* rotate round (0, 0) so that Point2 coordinates be (r, 0) */
	r = rnd_distance(0, 0, pad.Point2.X, pad.Point2.Y);
	if (r < .1) {
		Cos = 1;
		Sin = 0;
	}
	else {
		Sin = pad.Point2.Y / r;
		Cos = pad.Point2.X / r;
	}
	x = X;
	X = X * Cos + Y * Sin;
	Y = Y * Cos - x * Sin;
	/* now pad.Point2.X = r; pad.Point2.Y = 0; */

	/* take into account the ends */
	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad)) {
		r += Pad->Thickness;
		X += t2;
	}
	if (Y < 0)
		Y = -Y;											/* range value is evident now */

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad)) {
		if (X <= 0) {
			if (Y <= t2)
				range = -X;
			else
				return Radius > rnd_distance(0, t2, X, Y);
		}
		else if (X >= r) {
			if (Y <= t2)
				range = X - r;
			else
				return Radius > rnd_distance(r, t2, X, Y);
		}
		else
			range = Y - t2;
	}
	else {												/*Rounded pad: even more simple */

		if (X <= 0)
			return (Radius + t2) > rnd_distance(0, 0, X, Y);
		else if (X >= r)
			return (Radius + t2) > rnd_distance(r, 0, X, Y);
		else
			range = Y - t2;
	}
	return range < Radius;
}

rnd_bool pcb_is_point_in_box(rnd_coord_t X, rnd_coord_t Y, rnd_box_t *box, rnd_coord_t Radius)
{
	rnd_coord_t width, height, range;

	/* NB: Assumes box has point1 with numerically lower X and Y coordinates */

	/* Compute coordinates relative to Point1 */
	X -= box->X1;
	Y -= box->Y1;

	width = box->X2 - box->X1;
	height = box->Y2 - box->Y1;

	if (X <= 0) {
		if (Y < 0)
			return Radius > rnd_distance(0, 0, X, Y);
		else if (Y > height)
			return Radius > rnd_distance(0, height, X, Y);
		else
			range = -X;
	}
	else if (X >= width) {
		if (Y < 0)
			return Radius > rnd_distance(width, 0, X, Y);
		else if (Y > height)
			return Radius > rnd_distance(width, height, X, Y);
		else
			range = X - width;
	}
	else {
		if (Y < 0)
			range = -Y;
		else if (Y > height)
			range = Y - height;
		else
			return rnd_true;
	}

	return range < Radius;
}

rnd_bool pcb_arc_in_box(pcb_arc_t *arc, rnd_box_t *b)
{
	rnd_box_t ab = pcb_arc_mini_bbox(arc);
	return PCB_BOX_IN_BOX(&ab, b);
}

rnd_bool pcb_gfx_in_box(pcb_gfx_t *gfx, rnd_box_t *b)
{
	int n;

	for(n = 0; n < 4; n++)
		if (!PCB_POINT_IN_BOX(gfx->corner[n].X, gfx->corner[n].Y, b))
			return rnd_false;

	return rnd_true;
}

/* TODO: this code is BROKEN in the case of non-circular arcs,
 *       and in the case that the arc thickness is greater than
 *       the radius.
 */
rnd_bool pcb_is_point_on_arc(rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Radius, pcb_arc_t *Arc)
{
	/* Calculate angle of point from arc center */
	double p_dist = rnd_distance(X, Y, Arc->X, Arc->Y);
	double p_cos = (X - Arc->X) / p_dist;
	rnd_angle_t p_ang = acos(p_cos) * RND_RAD_TO_DEG;
	rnd_angle_t ang1, ang2;
#define angle_in_range(r1, r2, ang) (((ang) >= (r1)) && ((ang) <= (r2)))


	/* Convert StartAngle, Delta into bounding angles in [0, 720) */
	if (Arc->Delta > 0) {
		ang1 = rnd_normalize_angle(Arc->StartAngle);
		ang2 = rnd_normalize_angle(Arc->StartAngle + Arc->Delta);
	}
	else {
		ang1 = rnd_normalize_angle(Arc->StartAngle + Arc->Delta);
		ang2 = rnd_normalize_angle(Arc->StartAngle);
	}
	if (ang1 > ang2)
		ang2 += 360;
	/* Make sure full circles aren't treated as zero-length arcs */
	if (Arc->Delta == 360 || Arc->Delta == -360)
		ang2 = ang1 + 360;

	if (Y > Arc->Y)
		p_ang = -p_ang;
	p_ang += 180;


	/* Check point is outside arc range, check distance from endpoints
	   Need to check p_ang+360 too, because after the angle swaps above ang2
	   might be larger than 360 and that section of the arc shouldn't be missed
	   either. */
	if (!angle_in_range(ang1, ang2, p_ang) && !angle_in_range(ang1, ang2, p_ang+360)) {
		rnd_coord_t ArcX, ArcY;
		ArcX = Arc->X + Arc->Width * cos((Arc->StartAngle + 180) / RND_RAD_TO_DEG);
		ArcY = Arc->Y - Arc->Height * sin((Arc->StartAngle + 180) / RND_RAD_TO_DEG);
		if (rnd_distance(X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
			return rnd_true;

		ArcX = Arc->X + Arc->Width * cos((Arc->StartAngle + Arc->Delta + 180) / RND_RAD_TO_DEG);
		ArcY = Arc->Y - Arc->Height * sin((Arc->StartAngle + Arc->Delta + 180) / RND_RAD_TO_DEG);
		if (rnd_distance(X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
			return rnd_true;
		return rnd_false;
	}

	if ((Arc->Width == Arc->Height) || (fabs(Arc->Width - Arc->Height) < RND_MM_TO_COORD(0.1))) {
		/* Simple circular case: if point is inside the arc range, just compare it to the arc */
		return fabs(rnd_distance(X, Y, Arc->X, Arc->Y) - Arc->Width) < Radius + Arc->Thickness / 2;
	}
	else {
		/* elliptical case: guess where the arc would be in that point, by angle */
		double ang, dx, dy;
		rnd_coord_t ax, ay, d;

TODO(": elliptical arc: rewrite this, as it does not work properly on extreme cases")
		dy = (double)(Y - Arc->Y) / (double)Arc->Height;
		dx = (double)(X - Arc->X) / (double)Arc->Width;
		ang = -atan2(dy, dx);

		ax = Arc->X + cos(ang) * Arc->Width;
		ay = Arc->Y - sin(ang) * Arc->Height;

		d = fabs(rnd_distance(X, Y, ax, ay));

		return d < Arc->Thickness / 2;
	}
#undef angle_in_range
}

rnd_bool pcb_is_point_in_gfx(rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Radius, pcb_gfx_t *gfx)
{
	rnd_vector_t pt, a, b, c;
	int n, m;

	pt[0] = X; pt[1] = Y;
	a[0] = gfx->corner[0].X; a[1] = gfx->corner[0].Y;
	b[0] = gfx->corner[2].X; b[1] = gfx->corner[2].Y;

	c[0] = gfx->corner[1].X; c[1] = gfx->corner[1].Y;
	if (rnd_point_in_triangle(a, b, c, pt))
		return rnd_true;

	c[0] = gfx->corner[3].X; c[1] = gfx->corner[3].Y;
	if (rnd_point_in_triangle(a, b, c, pt))
		return rnd_true;

	/* the above triangle checks will miss points that are exactly on the triangle edges */
	for(n = 0; n < 4; n++) {
		m = n+1;
		if (m == 4)
			m = 0;
		if (pcb_is_point_on_thinline(X, Y, gfx->corner[n].X, gfx->corner[n].Y, gfx->corner[m].X, gfx->corner[m].Y))
			return rnd_true;
	}
	if (pcb_is_point_on_thinline(X, Y, gfx->corner[0].X, gfx->corner[0].Y, gfx->corner[2].X, gfx->corner[2].Y))
		return rnd_true;
	if (pcb_is_point_on_thinline(X, Y, gfx->corner[1].X, gfx->corner[1].Y, gfx->corner[3].X, gfx->corner[3].Y))
		return rnd_true;

	return rnd_false;
}


pcb_line_t *pcb_line_center_cross_point(pcb_layer_t *layer, rnd_coord_t x, rnd_coord_t y, rnd_angle_t *ang, rnd_bool no_subc_part, rnd_bool no_term)
{
	rnd_rtree_it_t it;
	rnd_rtree_box_t pt;
	pcb_line_t *l;

/* TODO: set pt coords to x and y */

	for(l = rnd_rtree_first(&it, layer->line_tree, &pt); l != NULL; l = rnd_rtree_next(&it)) {
		/* check if line needs to be ignored and "continue;" if so:
		   l->term is non-NULL if it's a terminal,
		   pcb_lobj_parent_subc(pcb_parenttype_t pt, pcb_parent_t *p) returns non-NULL if it's in a subc
		 */
		/* if the line shouldn't be ignored, check if x;y is on the centerline and
		   "continue;" if bad */
		/* check if angle is required and is correct; "continue;" on mismatch */
		return l;
	}
	return NULL; /* found nothing */
}

pcb_layer_type_t pstk_vis_layers(pcb_board_t *pcb, pcb_layer_type_t material)
{
	pcb_layer_type_t res = 0, lyt;
	rnd_layer_id_t lid;
	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		pcb_layer_t *ly = &pcb->Data->Layer[lid];
		lyt = pcb_layer_flags_(ly);
		if ((ly->meta.real.vis) && (lyt & material))
			res |= (lyt & PCB_LYT_ANYWHERE);
	}
	return res | material;
}

static int pcb_search_obj_by_loc_layer(unsigned long Type, void **Result1, void **Result2, void **Result3, unsigned long req_flag, pcb_layer_t *SearchLayer, int HigherAvail, double HigherBound, int objst)
{
	if (SearchLayer->meta.real.vis) {
		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 &&
				Type & PCB_OBJ_GFX_POINT &&
				SearchGfxPointByLocation(Type, objst, req_flag, (pcb_layer_t **) Result1, (pcb_gfx_t **) Result2, (rnd_point_t **) Result3))
			return PCB_OBJ_GFX_POINT;

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 &&
				Type & PCB_OBJ_POLY_POINT &&
				SearchPointByLocation(Type, objst, req_flag, (pcb_layer_t **) Result1, (pcb_poly_t **) Result2, (rnd_point_t **) Result3))
			return PCB_OBJ_POLY_POINT;

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 &&
				Type & PCB_OBJ_LINE_POINT &&
				SearchLinePointByLocation(objst, req_flag, (pcb_layer_t **) Result1, (pcb_line_t **) Result2, (rnd_point_t **) Result3))
			return PCB_OBJ_LINE_POINT;

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 &&
				Type & PCB_OBJ_ARC_POINT &&
				SearchArcPointByLocation(objst, req_flag, (pcb_layer_t **) Result1, (pcb_arc_t **) Result2, (int **) Result3))
			return PCB_OBJ_ARC_POINT;

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 && Type & PCB_OBJ_LINE
				&& SearchLineByLocation(objst, req_flag, (pcb_layer_t **) Result1, (pcb_line_t **) Result2, (pcb_line_t **) Result3))
			return PCB_OBJ_LINE;

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 && Type & PCB_OBJ_ARC &&
				SearchArcByLocation(objst, req_flag, (pcb_layer_t **) Result1, (pcb_arc_t **) Result2, (pcb_arc_t **) Result3))
			return PCB_OBJ_ARC;

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 && Type & PCB_OBJ_TEXT
				&& SearchTextByLocation(objst, req_flag, (pcb_layer_t **) Result1, (pcb_text_t **) Result2, (pcb_text_t **) Result3))
			return PCB_OBJ_TEXT;

		if (Type & PCB_OBJ_POLY &&
				SearchPolygonByLocation(objst, req_flag, (pcb_layer_t **) Result1, (pcb_poly_t **) Result2, (pcb_poly_t **) Result3)) {
			if (HigherAvail) {
				rnd_box_t *box = &(*(pcb_poly_t **) Result2)->BoundingBox;
				double area = (double) (box->X2 - box->X1) * (double) (box->X2 - box->X1);
				if (HigherBound < area)
					return -1;
				else
					return PCB_OBJ_POLY;
			}
			else
				return PCB_OBJ_POLY;
		}

		if ((HigherAvail & (PCB_OBJ_PSTK)) == 0 && Type & PCB_OBJ_GFX &&
				SearchGfxByLocation(objst, req_flag, (pcb_layer_t **)Result1, (pcb_gfx_t **)Result2, (pcb_gfx_t **)Result3))
			return PCB_OBJ_GFX;

	}
	return 0;
}

static int pcb_search_obj_by_loc_group(unsigned long Type, void **Result1, void **Result2, void **Result3, unsigned long req_flag, const pcb_layergrp_t *grp, int HigherAvail, double HigherBound, int objst)
{
	rnd_cardinal_t n;
	for(n = 0; n < grp->len; n++) {
		int found;
		pcb_layer_t *oldly, *layer = pcb_get_layer(PCB->Data, grp->lid[n]);
		if (layer == NULL)
			continue;
		oldly = SearchLayer;
		SearchLayer = layer;
		found = pcb_search_obj_by_loc_layer(Type, Result1, Result2, Result3, req_flag, layer, HigherAvail, HigherBound, objst);
		SearchLayer = oldly;
		if (found != 0)
			return found;
	}
	return 0;
}

static int pcb_search_obj_by_loc_group_id(unsigned long Type, void **Result1, void **Result2, void **Result3, unsigned long req_flag, const rnd_layergrp_id_t gid, int HigherAvail, double HigherBound, int objst)
{
	pcb_layergrp_t *grp = pcb_get_layergrp(PCB, gid);
	if (grp == NULL)
		return 0;
	return pcb_search_obj_by_loc_group(Type, Result1, Result2, Result3, req_flag, grp, HigherAvail, HigherBound, objst);
}


/* ---------------------------------------------------------------------------
 * searches for any kind of object or for a set of object types
 * the calling routine passes two pointers to allocated memory for storing
 * the results.
 * A type value is returned too which is PCB_OBJ_VOID if no objects has been found.
 * A set of object types is passed in.
 * The object is located by it's position.
 *
 * The layout is checked in the following order:
 *   polygon-point, padstack, line, text, polygon, subcircuit
 *
 * Note that if Type includes PCB_OBJ_LOCKED, then the search includes
 * locked items.  Otherwise, locked items are ignored.

 * Note that if Type includes PCB_OBJ_SUBC_PART, then the search includes
 * objects that are part of subcircuits, else they are ignored.
 */
static int pcb_search_obj_by_location_(unsigned long Type, void **Result1, void **Result2, void **Result3, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Radius, unsigned long req_flag)
{
	void *r1, *r2, *r3;
	void **pr1 = &r1, **pr2 = &r2, **pr3 = &r3;
	int i;
	double HigherBound = 0;
	int HigherAvail = PCB_OBJ_VOID;
	int objst = Type & (PCB_OBJ_LOCKED | PCB_OBJ_SUBC_PART);

	/* setup variables used by local functions */
	PosX = X;
	PosY = Y;
	SearchRadius = Radius;
	if (Radius) {
		SearchBox.X1 = X - Radius;
		SearchBox.Y1 = Y - Radius;
		SearchBox.X2 = X + Radius;
		SearchBox.Y2 = Y + Radius;
	}
	else {
		SearchBox = rnd_point_box(X, Y);
	}

	if (conf_core.editor.only_names) {
		Type &= PCB_OBJ_TEXT;
	}

	if (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly) {
		Type &= ~PCB_OBJ_POLY;
	}

	if (Type & PCB_OBJ_RAT && PCB->RatOn &&
			SearchRatLineByLocation(objst, req_flag, (pcb_rat_t **) Result1, (pcb_rat_t **) Result2, (pcb_rat_t **) Result3))
		return PCB_OBJ_RAT;

	if (Type & PCB_OBJ_PSTK && SearchPadstackByLocation(objst, req_flag, (pcb_pstk_t **) Result1, (pcb_pstk_t **) Result2, (pcb_pstk_t **) Result3, 1, 0))
		return PCB_OBJ_PSTK;

	if (Type & PCB_OBJ_PSTK && SearchPadstackByLocation(objst, req_flag, (pcb_pstk_t **) Result1, (pcb_pstk_t **) Result2, (pcb_pstk_t **) Result3, 0, PCB_LYT_VISIBLE_SIDE() | PCB_LYT_COPPER))
		return PCB_OBJ_PSTK;

	if (Type & PCB_OBJ_PSTK && SearchPadstackByLocation(objst, req_flag, (pcb_pstk_t **) Result1, (pcb_pstk_t **) Result2, (pcb_pstk_t **) Result3, 0, (PCB_LYT_VISIBLE_SIDE() | PCB_LYT_ANYTHING) & pstk_vis_layers(PCB, PCB_LYT_ANYTHING)))
		return PCB_OBJ_PSTK;

	if (Type & PCB_OBJ_PSTK && SearchPadstackByLocation(objst, req_flag, (pcb_pstk_t **) Result1, (pcb_pstk_t **) Result2, (pcb_pstk_t **) Result3, 0, pstk_vis_layers(PCB, PCB_LYT_MASK)))
		return PCB_OBJ_PSTK;

	if (Type & PCB_OBJ_PSTK && SearchPadstackByLocation(objst, req_flag, (pcb_pstk_t **) Result1, (pcb_pstk_t **) Result2, (pcb_pstk_t **) Result3, 0, pstk_vis_layers(PCB, PCB_LYT_COPPER)))
		return PCB_OBJ_PSTK;

	if (Type & PCB_OBJ_PSTK && SearchPadstackByLocation(objst, req_flag, (pcb_pstk_t **) Result1, (pcb_pstk_t **) Result2, (pcb_pstk_t **) Result3, 0, pstk_vis_layers(PCB, PCB_LYT_PASTE)))
		return PCB_OBJ_PSTK;

	if (!HigherAvail && (Type & PCB_OBJ_FLOATER) && (Type & PCB_OBJ_TEXT) &&
			SearchSubcFloaterByLocation(objst, req_flag, (pcb_subc_t **)pr1, (pcb_text_t **) pr2, pr3, rnd_false)) {
		*Result1 = ((pcb_text_t *)r2)->parent.layer;
		*Result2 = r2;
		*Result3 = r3;
		return PCB_OBJ_TEXT;
	}

	if (!HigherAvail && Type & PCB_OBJ_SUBC && PCB->SubcOn &&
			SearchSubcByLocation(objst, req_flag, (pcb_subc_t **) pr1, (pcb_subc_t **) pr2, (pcb_subc_t **) pr3, rnd_false)) {
		rnd_box_t *box = &((pcb_subc_t *) r1)->BoundingBox;
		HigherBound = (double) (box->X2 - box->X1) * (double) (box->Y2 - box->Y1);
		HigherAvail = PCB_OBJ_SUBC;
	}

	{ /* search the front silk layer first */
		int found;
		rnd_layergrp_id_t front_silk_id = conf_core.editor.show_solder_side ? pcb_layergrp_get_bottom_silk() : pcb_layergrp_get_top_silk();
		found = pcb_search_obj_by_loc_group_id(Type, Result1, Result2, Result3, req_flag, front_silk_id, HigherAvail, HigherBound, objst);
		if (found > 0)
			return found;
	}

	for(i = 0; i < pcb_max_layer(PCB); i++) {
		int found;
		if (pcb_layer_flags(PCB, i) & PCB_LYT_SILK) /* special order: silks are searched before/after this loop, if we meet them elsewhere, skip */
			continue;
		SearchLayer = PCB_STACKLAYER(PCB, i);

		found = pcb_search_obj_by_loc_layer(Type, Result1, Result2, Result3, req_flag, SearchLayer, HigherAvail, HigherBound, objst);
		if (found < 0)
			break;
		if (found != 0)
			return found;
	}

	if (PCB->InvisibleObjectsOn) { /* search the back silk layer last, unless it's invisible */
		int found;
		rnd_layergrp_id_t back_silk_id = conf_core.editor.show_solder_side ? pcb_layergrp_get_top_silk() : pcb_layergrp_get_bottom_silk();
		found = pcb_search_obj_by_loc_group_id(Type, Result1, Result2, Result3, req_flag, back_silk_id, HigherAvail, HigherBound, objst);
		if (found > 0)
			return found;
	}


	/* return any previously found objects */
	if (HigherAvail & PCB_OBJ_PSTK) {
		*Result1 = r1;
		*Result2 = r2;
		*Result3 = r3;
		return PCB_OBJ_PSTK;
	}

	if (HigherAvail & PCB_OBJ_SUBC) {
		*Result1 = r1;
		*Result2 = r2;
		*Result3 = r3;
		return PCB_OBJ_SUBC;
	}

	/* search the 'invisible objects' last */
	if (!PCB->InvisibleObjectsOn)
		return PCB_OBJ_VOID;

	if ((Type & PCB_OBJ_FLOATER) && (Type & PCB_OBJ_TEXT) &&
			SearchSubcFloaterByLocation(objst, req_flag, (pcb_subc_t **)pr1, (pcb_text_t **) pr2, pr3, rnd_true)) {
		*Result1 = ((pcb_text_t *)r2)->parent.layer;
		*Result2 = r2;
		*Result3 = r3;
		return PCB_OBJ_TEXT;
	}

	if (Type & PCB_OBJ_SUBC && PCB->SubcOn &&
			SearchSubcByLocation(objst, req_flag, (pcb_subc_t **) Result1, (pcb_subc_t **) Result2, (pcb_subc_t **) Result3, rnd_true))
		return PCB_OBJ_SUBC;

	return PCB_OBJ_VOID;
}

int pcb_search_obj_by_location(unsigned long Type, void **Result1, void **Result2, void **Result3, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Radius)
{
	int res;

	if ((conf_core.editor.lock_names) || (conf_core.editor.hide_names))
		Type &= ~ PCB_OBJ_FLOATER;

	res = pcb_search_obj_by_location_(Type, Result1, Result2, Result3, X, Y, Radius, 0);
	if ((res != PCB_OBJ_VOID) && (res != PCB_OBJ_SUBC))
		return res;

	/* found a subc because it was still the best option over all the plain
	   objects; if floaters can be found, repeat the search on them, they
	   have higher prio than subc, but lower prio than plain objects */
	if ((!conf_core.editor.lock_names) && (!conf_core.editor.hide_names)) {
		int fres;
		void *fr1, *fr2, *fr3;
		Type &= ~PCB_OBJ_SUBC;
		Type |= PCB_OBJ_SUBC_PART;
		fres = pcb_search_obj_by_location_(Type, &fr1, &fr2, &fr3, X, Y, Radius, PCB_FLAG_FLOATER);
		if (fres != PCB_OBJ_VOID) {
			*Result1 = fr1;
			*Result2 = fr2;
			*Result3 = fr3;
			return fres;
		}
	}
	
	return res;
}


/* ---------------------------------------------------------------------------
 * searches for a object by it's unique ID. It doesn't matter if
 * the object is visible or not. The search is performed on a PCB, a
 * buffer or on the remove list.
 * The calling routine passes two pointers to allocated memory for storing
 * the results.
 * A type value is returned too which is PCB_OBJ_VOID if no objects has been found.
 */
int pcb_search_obj_by_id_(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type)
{
	if (type == PCB_OBJ_LINE || type == PCB_OBJ_LINE_POINT) {
		PCB_LINE_ALL_LOOP(Base);
		{
			if (line->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) line;
				return PCB_OBJ_LINE;
			}
			if (line->Point1.ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point1;
				return PCB_OBJ_LINE_POINT;
			}
			if (line->Point2.ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point2;
				return PCB_OBJ_LINE_POINT;
			}
		}
		PCB_ENDALL_LOOP;
	}
	if (type == PCB_OBJ_ARC || type == PCB_OBJ_ARC_POINT) {
		PCB_ARC_ALL_LOOP(Base);
		{
			if (arc->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) arc;
				return PCB_OBJ_ARC;
			}
		}
		PCB_ENDALL_LOOP;
	}

	if (type == PCB_OBJ_TEXT) {
		PCB_TEXT_ALL_LOOP(Base);
		{
			if (text->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) text;
				return PCB_OBJ_TEXT;
			}
		}
		PCB_ENDALL_LOOP;
	}

	if (type == PCB_OBJ_POLY || type == PCB_OBJ_POLY_POINT) {
		PCB_POLY_ALL_LOOP(Base);
		{
			if (polygon->ID == ID) {
				*Result1 = (void *) layer;
				*Result2 = *Result3 = (void *) polygon;
				return PCB_OBJ_POLY;
			}
			if (type == PCB_OBJ_POLY_POINT)
				PCB_POLY_POINT_LOOP(polygon);
			{
				if (point->ID == ID) {
					*Result1 = (void *) layer;
					*Result2 = (void *) polygon;
					*Result3 = (void *) point;
					return PCB_OBJ_POLY_POINT;
				}
			}
			PCB_END_LOOP;
		}
		PCB_ENDALL_LOOP;
	}

	if ((type == PCB_OBJ_GFX) || (type == PCB_OBJ_GFX_POINT)) {
		PCB_GFX_ALL_LOOP(Base);
		{
			if (gfx->ID == ID) {
				*Result1 = (void *)layer;
				*Result2 = *Result3 = (void *)gfx;
				return PCB_OBJ_GFX;
			}
			if (type == PCB_OBJ_GFX_POINT) {
				TODO("gfx: id on point?");
			}
		}
		PCB_ENDALL_LOOP;
	}

	if (type == PCB_OBJ_PSTK) {
		PCB_PADSTACK_LOOP(Base);
		{
			if (padstack->ID == ID) {
				*Result1 = *Result2 = *Result3 = (void *)padstack;
				return PCB_OBJ_PSTK;
			}
		}
		PCB_END_LOOP;
	}

	if (type == PCB_OBJ_RAT || type == PCB_OBJ_LINE_POINT) {
		PCB_RAT_LOOP(Base);
		{
			if (line->ID == ID) {
				*Result1 = *Result2 = *Result3 = (void *) line;
				return PCB_OBJ_RAT;
			}
			if (line->Point1.ID == ID) {
				*Result1 = (void *) NULL;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point1;
				return PCB_OBJ_LINE_POINT;
			}
			if (line->Point2.ID == ID) {
				*Result1 = (void *) NULL;
				*Result2 = (void *) line;
				*Result3 = (void *) &line->Point2;
				return PCB_OBJ_LINE_POINT;
			}
		}
		PCB_END_LOOP;
	}

TODO("subc: once elements are gone, rewrite these to search the rtree instead of recursion")
	PCB_SUBC_LOOP(Base);
	{
		int res;
		if (type == PCB_OBJ_SUBC) {
			if (subc->ID == ID) {
				*Result1 = *Result2 = *Result3 = (void *)subc;
				return PCB_OBJ_SUBC;
			}
		}

		res = pcb_search_obj_by_id_(subc->data, Result1, Result2, Result3, ID, type);
		if (res != PCB_OBJ_VOID)
			return res;
	}
	PCB_END_LOOP;

	return PCB_OBJ_VOID;
}

int pcb_search_obj_by_id(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type)
{
	int res = pcb_search_obj_by_id_(Base, Result1, Result2, Result3, ID, type);
	if (res == PCB_OBJ_VOID)
		rnd_message(RND_MSG_ERROR, "pcb_search_obj_by_id(): internal error, search for ID %d failed\n", ID);
	return res;
}

int pcb_search_obj_by_id_buf2(pcb_data_t *Base, void **Result1, void **Result2, void **Result3, int ID, int type)
{
	int bid = 0;
	long res;

	res = pcb_search_obj_by_id_(Base, Result1, Result2, Result3, ID, type);
	if (res != PCB_OBJ_VOID)
		return res;

	while(res == PCB_OBJ_VOID) {
		res = pcb_search_obj_by_id_(pcb_buffers[bid].Data, Result1, Result2, Result3, ID, type);
		if (res != PCB_OBJ_VOID)
			return res;
		bid++;
		if (bid >= PCB_MAX_BUFFER)
			break;
	}

	rnd_message(RND_MSG_ERROR, "pcb_search_obj_by_id_buf2(): internal error, search for ID %d failed\n", ID);
	return PCB_OBJ_VOID;
}

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type
 */
int pcb_search_screen(rnd_coord_t X, rnd_coord_t Y, int Type, void **Result1, void **Result2, void **Result3)
{
	return pcb_search_obj_by_location(Type, Result1, Result2, Result3, X, Y, PCB_SLOP * rnd_pixel_slop);
}

/* ---------------------------------------------------------------------------
 * searches the cursor position for the type
 */
int pcb_search_grid_slop(rnd_coord_t X, rnd_coord_t Y, int Type, void **Result1, void **Result2, void **Result3)
{
	int ans, n;

	for(n = 8; n >= 2; n = n/2) {
		ans = pcb_search_obj_by_location(Type, Result1, Result2, Result3, X, Y, PCB->hidlib.grid / n);
		if (ans != PCB_OBJ_VOID) return ans;
	}

	return ans;
}

vtp0_t pcb_obj_list_vect;
int pcb_search_screen_selector(rnd_coord_t X, rnd_coord_t Y, int Type, void **Result1, void **Result2, void **Result3)
{
	rnd_box_t box;
	rnd_coord_t radius;
	pcb_any_obj_t *obj;

	if ((!conf_core.editor.click_objlist) || (!RND_HAVE_GUI_ATTR_DLG))
		return PCB_OBJ_VOID;

	radius = PCB_SLOP * rnd_pixel_slop;
	box.X2 = X - radius;
	box.Y2 = Y - radius;
	box.X1 = X + radius;
	box.Y1 = Y + radius;

	vtp0_init(&pcb_obj_list_vect);
	pcb_list_block_cb(PCB, &box, (void *(*)(void *, pcb_any_obj_t *))vtp0_append, &pcb_obj_list_vect);

	if (pcb_obj_list_vect.used == 1) {
		obj = pcb_obj_list_vect.array[0];
		goto found;
	}

	if (pcb_obj_list_vect.used > 1) {
		long idx = rnd_actionva(&PCB->hidlib, "dlg_obj_list", NULL);
		if ((idx >= 0) && (idx < pcb_obj_list_vect.used)) {
			obj = pcb_obj_list_vect.array[idx];
			goto found;
		}
	}

	vtp0_uninit(&pcb_obj_list_vect);
	return PCB_OBJ_VOID;

	found:;
	vtp0_uninit(&pcb_obj_list_vect);
	*Result1 = obj->parent.any;
	*Result2 = *Result3 = obj;
	return obj->type;
}

int pcb_search_screen_maybe_selector(rnd_coord_t X, rnd_coord_t Y, int Type, void **Result1, void **Result2, void **Result3)
{
	int sr = pcb_search_screen_selector(X, Y, Type, Result1, Result2, Result3);
	if (sr != PCB_OBJ_VOID) return sr;
	return pcb_search_screen(X, Y, Type, Result1, Result2, Result3);
}


int pcb_lines_intersect_at(rnd_coord_t ax1, rnd_coord_t ay1, rnd_coord_t ax2, rnd_coord_t ay2, rnd_coord_t bx1, rnd_coord_t by1, rnd_coord_t bx2, rnd_coord_t by2, rnd_coord_t *iscx, rnd_coord_t *iscy)
{
/* TODO: this should be long double if rnd_coord_t is 64 bits */
	double ua, xi, yi, X1, Y1, X2, Y2, X3, Y3, X4, Y4, tmp;
	int is_a_pt, is_b_pt;

	/* degenerate cases: a line is actually a point */
	is_a_pt = (ax1 == ax2) && (ay1 == ay2);
	is_b_pt = (bx1 == bx2) && (by1 == by2);

	if (is_a_pt && is_b_pt)
		return (ax1 == bx1) && (ay1 == by1);

	if (is_a_pt)
		return is_point_on_line(ax1, ay1, bx1, by1, bx2, by2);
	if (is_b_pt)
		return is_point_on_line(bx1, by1, ax1, ay1, ax2, ay2);

	/* maths from http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/ */

	X1 = ax1;
	X2 = ax2;
	X3 = bx1;
	X4 = bx2;
	Y1 = ay1;
	Y2 = ay2;
	Y3 = by1;
	Y4 = by2;

	tmp = ((Y4 - Y3) * (X2 - X1) - (X4 - X3) * (Y2 - Y1));

	if (tmp == 0) {
		/* Corner case: parallel lines; intersect only if the endpoint of either line
		   is on the other line */
		return
			is_point_on_line(ax1, ay1,  bx1, by1, bx2, by2) ||
			is_point_on_line(ax2, ay2,  bx1, by1, bx2, by2) ||
			is_point_on_line(bx1, by1,  ax1, ay1, ax2, ay2) ||
			is_point_on_line(bx2, by2,  ax1, ay1, ax2, ay2);
	}


	ua = ((X4 - X3) * (Y1 - Y3) - (Y4 - Y3) * (X1 - X3)) / tmp;
/*	ub = ((X2 - X1) * (Y1 - Y3) - (Y2 - Y1) * (X1 - X3)) / tmp;*/
	xi = X1 + ua * (X2 - X1);
	yi = Y1 + ua * (Y2 - Y1);

	if (iscx != NULL)
		*iscx = xi;
	if (iscy != NULL)
		*iscy = yi;

#define check(e1, e2, i) \
	if (e1 < e2) { \
		if ((i < e1) || (i > e2)) \
			return 0; \
	} \
	else { \
		if ((i > e1) || (i < e2)) \
			return 0; \
	}

	check(ax1, ax2, xi);
	check(bx1, bx2, xi);
	check(ay1, ay2, yi);
	check(by1, by2, yi);
	return 1;
}

int pcb_lines_intersect(rnd_coord_t ax1, rnd_coord_t ay1, rnd_coord_t ax2, rnd_coord_t ay2, rnd_coord_t bx1, rnd_coord_t by1, rnd_coord_t bx2, rnd_coord_t by2)
{
	return pcb_lines_intersect_at(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2, NULL, NULL);
}

rnd_r_dir_t pcb_search_data_by_loc(pcb_data_t *data, pcb_objtype_t type, const rnd_box_t *query_box, rnd_r_dir_t (*cb_)(void *closure, pcb_any_obj_t *obj, void *box), void *closure)
{
	pcb_layer_t *ly;
	rnd_layer_id_t lid;
	rnd_r_dir_t res;
	const rnd_rtree_box_t *query = (const rnd_rtree_box_t *)query_box;
	rnd_rtree_dir_t (*cb)(void *, void *, const rnd_rtree_box_t *) = (rnd_rtree_dir_t(*)(void *, void *, const rnd_rtree_box_t *))cb_;

	if ((type & PCB_OBJ_PSTK) && (data->padstack_tree != NULL)) {
		res = rnd_rtree_search_any(data->padstack_tree, query, NULL, cb, closure, NULL);
		if (res & rnd_RTREE_DIR_STOP)
			return res;
	}

	if ((type & PCB_OBJ_SUBC) && (data->subc_tree != NULL)) {
		res = rnd_rtree_search_any(data->subc_tree, query, NULL, cb, closure, NULL);
		if (res & rnd_RTREE_DIR_STOP)
			return res;
	}

	if ((type & PCB_OBJ_RAT) && (data->rat_tree != NULL)) {
		res = rnd_rtree_search_any(data->rat_tree, query, NULL, cb, closure, NULL);
		if (res & rnd_RTREE_DIR_STOP)
			return res;
	}

	for(lid = 0, ly = data->Layer; lid < data->LayerN; lid++,ly++) {

		if ((type & PCB_OBJ_ARC) && (ly->arc_tree != NULL)) {
			res = rnd_rtree_search_any(ly->arc_tree, query, NULL, cb, closure, NULL);
			if (res & rnd_RTREE_DIR_STOP)
				return res;
		}
		if ((type & PCB_OBJ_LINE) && (ly->line_tree != NULL)) {
			res = rnd_rtree_search_any(ly->line_tree, query, NULL, cb, closure, NULL);
			if (res & rnd_RTREE_DIR_STOP)
				return res;
		}
		if ((type & PCB_OBJ_POLY) && (ly->polygon_tree != NULL)) {
			res = rnd_rtree_search_any(ly->polygon_tree, query, NULL, cb, closure, NULL);
			if (res & rnd_RTREE_DIR_STOP)
				return res;
		}
		if ((type & PCB_OBJ_TEXT) && (ly->text_tree != NULL)) {
			res = rnd_rtree_search_any(ly->text_tree, query, NULL, cb, closure, NULL);
			if (res & rnd_RTREE_DIR_STOP)
				return res;
		}
		if ((type & PCB_OBJ_GFX) && (ly->gfx_tree != NULL)) {
			res = rnd_rtree_search_any(ly->gfx_tree, query, NULL, cb, closure, NULL);
			if (res & rnd_RTREE_DIR_STOP)
				return res;
		}

	}
	return 0;
}

