/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Drawing primitive: polygons */


#include "config.h"

#include "board.h"
#include "data.h"
#include "undo.h"
#include "polygon.h"
#include <librnd/poly/polygon1_gen.h>
#include <librnd/poly/offset.h>
#include "rotate.h"
#include "search.h"
#include <librnd/core/hid_inlines.h>

#include "conf_core.h"

#include "obj_poly.h"
#include "obj_poly_op.h"
#include "obj_poly_list.h"
#include "obj_poly_draw.h"

#include "obj_subc_parent.h"
#include "obj_hash.h"

/* TODO: get rid of these: */
#include "draw.h"

#define	STEP_POLYGONPOINT	10
#define	STEP_POLYGONHOLEINDEX	10

#include "obj_poly_draw_helper.c"

/*** allocation ***/

void pcb_poly_reg(pcb_layer_t *layer, pcb_poly_t *poly)
{
	polylist_append(&layer->Polygon, poly);
	PCB_SET_PARENT(poly, layer, layer);

	if (layer->parent_type == PCB_PARENT_UI)
		return;

	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_obj_id_reg(layer->parent.data, poly);
}

void pcb_poly_unreg(pcb_poly_t *poly)
{
	pcb_layer_t *layer = poly->parent.layer;
	assert(poly->parent_type == PCB_PARENT_LAYER);
	polylist_remove(poly);
	if (layer->parent_type != PCB_PARENT_UI) {
		assert(layer->parent_type == PCB_PARENT_DATA);
		pcb_obj_id_del(layer->parent.data, poly);
	}
	PCB_SET_PARENT(poly, layer, NULL);
}

pcb_poly_t *pcb_poly_alloc_id(pcb_layer_t *layer, long int id)
{
	pcb_poly_t *new_obj;

	new_obj = calloc(sizeof(pcb_poly_t), 1);
	new_obj->ID = id;
	new_obj->type = PCB_OBJ_POLY;
	new_obj->Attributes.post_change = pcb_obj_attrib_post_change;

	pcb_poly_reg(layer, new_obj);

	return new_obj;
}

pcb_poly_t *pcb_poly_alloc(pcb_layer_t *layer)
{
	return pcb_poly_alloc_id(layer, pcb_create_ID_get());
}


void pcb_poly_free(pcb_poly_t *poly)
{
	if ((poly->parent.layer != NULL) && (poly->parent.layer->polygon_tree != NULL))
		pcb_r_delete_entry(poly->parent.layer->polygon_tree, (rnd_box_t *)poly);
	rnd_attribute_free(&poly->Attributes);
	pcb_poly_unreg(poly);
	free(poly);
}

/* gets the next slot for a point in a polygon struct, allocates memory if necessary */
pcb_point_t *pcb_poly_point_alloc(pcb_poly_t *Polygon)
{
	pcb_point_t *points = Polygon->Points;

	/* realloc new memory if necessary and clear it */
	if (Polygon->PointN >= Polygon->PointMax) {
		Polygon->PointMax += STEP_POLYGONPOINT;
		points = (pcb_point_t *) realloc(points, Polygon->PointMax * sizeof(pcb_point_t));
		Polygon->Points = points;
		memset(points + Polygon->PointN, 0, STEP_POLYGONPOINT * sizeof(pcb_point_t));
	}
	return (points + Polygon->PointN++);
}

/* gets the next slot for a point in a polygon struct, allocates memory if necessary */
pcb_cardinal_t *pcb_poly_holeidx_new(pcb_poly_t *Polygon)
{
	pcb_cardinal_t *holeindex = Polygon->HoleIndex;

	/* realloc new memory if necessary and clear it */
	if (Polygon->HoleIndexN >= Polygon->HoleIndexMax) {
		Polygon->HoleIndexMax += STEP_POLYGONHOLEINDEX;
		holeindex = (pcb_cardinal_t *) realloc(holeindex, Polygon->HoleIndexMax * sizeof(int));
		Polygon->HoleIndex = holeindex;
		memset(holeindex + Polygon->HoleIndexN, 0, STEP_POLYGONHOLEINDEX * sizeof(int));
	}
	return (holeindex + Polygon->HoleIndexN++);
}

/* frees memory used by a polygon */
void pcb_poly_free_fields(pcb_poly_t * polygon)
{
	pcb_parent_t parent;
	pcb_parenttype_t parent_type;

	if (polygon == NULL)
		return;

	free(polygon->Points);
	free(polygon->HoleIndex);

	if (polygon->Clipped)
		pcb_polyarea_free(&polygon->Clipped);
	pcb_poly_contours_free(&polygon->NoHoles);

	/* have to preserve parent info for unreg */
	parent = polygon->parent;
	parent_type = polygon->parent_type;
	reset_obj_mem(pcb_poly_t, polygon);
	polygon->parent = parent;
	polygon->parent_type = parent_type;
}

/*** utility ***/

/* rotates a polygon in 90 degree steps */
void pcb_poly_rotate90(pcb_poly_t *Polygon, rnd_coord_t X, rnd_coord_t Y, unsigned Number)
{
	pcb_layer_t *layer = Polygon->parent.layer;
	assert(Polygon->parent_type = PCB_PARENT_LAYER);
	PCB_POLY_POINT_LOOP(Polygon);
	{
		PCB_COORD_ROTATE90(point->X, point->Y, X, Y, Number);
	}
	PCB_END_LOOP;

	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_poly_init_clip(layer->parent.data, layer, Polygon);
	/* simply rotating the bounding box here won't work, because the box is 'closed' (increased by 1) */
	pcb_poly_bbox(Polygon);
}

void pcb_poly_rotate(pcb_layer_t *layer, pcb_poly_t *polygon, rnd_coord_t X, rnd_coord_t Y, double cosa, double sina)
{
	if (layer->polygon_tree != NULL)
		pcb_r_delete_entry(layer->polygon_tree, (rnd_box_t *) polygon);
	PCB_POLY_POINT_LOOP(polygon);
	{
		pcb_rotate(&point->X, &point->Y, X, Y, cosa, sina);
	}
	PCB_END_LOOP;
	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_poly_init_clip(layer->parent.data, layer, polygon);
	pcb_poly_bbox(polygon);
	if (layer->polygon_tree != NULL)
		pcb_r_insert_entry(layer->polygon_tree, (rnd_box_t *) polygon);
}

int pcb_poly_is_valid(pcb_poly_t *p)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np1 = NULL, *np = NULL;
	pcb_cardinal_t n;
	pcb_vector_t v;
	int res = 1;

	np1 = np = pcb_polyarea_create();
	if (np == NULL)
		return 0;

	/* first make initial polygon contour */
	for (n = 0; n < p->PointN; n++) {
		/* No current contour? Make a new one starting at point */
		/*   (or) Add point to existing contour */

		v[0] = p->Points[n].X;
		v[1] = p->Points[n].Y;
		if (contour == NULL) {
			if ((contour = pcb_poly_contour_new(v)) == NULL)
				goto err;
		}
		else {
			pcb_poly_vertex_include(contour->head->prev, pcb_poly_node_create(v));
		}

		/* Is current point last in contour? If so process it. */
		if (n == p->PointN - 1) {
			pcb_poly_contour_pre(contour, pcb_true);

			if (contour->Count == 0) {
				pcb_poly_contours_free(&contour);
				goto err;
			}

			{ /* count number of not-on-the-same-line vertices to make sure there's more than 2*/
				pcb_vnode_t *cur;
				int r = 0;

				cur = contour->head;
				do {
					r++;
				} while ((cur = cur->next) != contour->head);
				if (r < 3) {
					pcb_poly_contours_free(&contour);
					goto err;
				}
			}

			/* make sure it is a positive contour (outer) or negative (hole) */
			if (contour->Flags.orient != PCB_PLF_DIR) {
				pcb_poly_contour_inv(contour);
			}

			pcb_polyarea_contour_include(np, contour);
			if (contour->Count == 0)
				goto err;
			contour = NULL;

			if (!pcb_poly_valid(np))
				res = 0;
		}
	}
	pcb_polyarea_free(&np1);
	return res;

	err:;
	pcb_polyarea_free(&np1);
	return 0;
}

/*** undoable mirror ***/

static const char core_poly_cookie[] = "core-poly";

typedef struct {
	pcb_poly_t *poly; /* it is safe to save the object pointer because it is persistent (through the removed object list) */
	rnd_coord_t y_offs;
} undo_poly_mirror_t;

static int undo_poly_mirror_swap(void *udata)
{
	undo_poly_mirror_t *g = udata;
	pcb_layer_t *layer = g->poly->parent.layer;

	if (layer->polygon_tree != NULL)
		pcb_r_delete_entry(layer->polygon_tree, (rnd_box_t *)g->poly);
	PCB_POLY_POINT_LOOP(g->poly);
	{
		point->X = PCB_SWAP_X(point->X);
		point->Y = PCB_SWAP_Y(point->Y) + g->y_offs;
	}
	PCB_END_LOOP;
	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_poly_init_clip(layer->parent.data, layer, g->poly);
	pcb_poly_bbox(g->poly);
	if (layer->polygon_tree != NULL)
		pcb_r_insert_entry(layer->polygon_tree, (rnd_box_t *)g->poly);

	return 0;
}

static void undo_poly_mirror_print(void *udata, char *dst, size_t dst_len)
{
	undo_poly_mirror_t *g = udata;
	pcb_snprintf(dst, dst_len, "poly mirror y=%$mm", g->y_offs);
}

static const uundo_oper_t undo_poly_mirror = {
	core_poly_cookie,
	NULL,
	undo_poly_mirror_swap,
	undo_poly_mirror_swap,
	undo_poly_mirror_print
};


void pcb_poly_mirror(pcb_poly_t *poly, rnd_coord_t y_offs, rnd_bool undoable)
{
	undo_poly_mirror_t gtmp, *g = &gtmp;

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(poly->parent.layer->parent.data), &undo_poly_mirror, sizeof(undo_poly_mirror_t));

	g->poly = poly;
	g->y_offs = y_offs;

	undo_poly_mirror_swap(g);
	if (undoable) pcb_undo_inc_serial();
}

void pcb_poly_flip_side(pcb_layer_t *layer, pcb_poly_t *polygon)
{
	pcb_r_delete_entry(layer->polygon_tree, (rnd_box_t *) polygon);
	PCB_POLY_POINT_LOOP(polygon);
	{
		point->X = PCB_SWAP_X(point->X);
		point->Y = PCB_SWAP_Y(point->Y);
	}
	PCB_END_LOOP;
	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_poly_init_clip(layer->parent.data, layer, polygon);
	pcb_poly_bbox(polygon);
	pcb_r_insert_entry(layer->polygon_tree, (rnd_box_t *) polygon);
	/* hmmm, how to handle clip */
}

void pcb_poly_scale(pcb_poly_t *poly, double sx, double sy)
{
	int onbrd = (poly->parent.layer != NULL) && (!poly->parent.layer->is_bound);

	if (onbrd)
		pcb_poly_pre(poly);
	PCB_POLY_POINT_LOOP(poly);
	{
		point->X = pcb_round((double)point->X * sx);
		point->Y = pcb_round((double)point->Y * sy);
	}
	PCB_END_LOOP;
	pcb_poly_bbox(poly);
	if (onbrd)
		pcb_poly_post(poly);
}


/* sets the bounding box of a polygons */
void pcb_poly_bbox(pcb_poly_t *Polygon)
{
	Polygon->bbox_naked.X1 = Polygon->bbox_naked.Y1 = PCB_MAX_COORD;
	Polygon->bbox_naked.X2 = Polygon->bbox_naked.Y2 = -PCB_MAX_COORD;

	PCB_POLY_POINT_LOOP(Polygon);
	{
		PCB_MAKE_MIN(Polygon->bbox_naked.X1, point->X);
		PCB_MAKE_MIN(Polygon->bbox_naked.Y1, point->Y);
		PCB_MAKE_MAX(Polygon->bbox_naked.X2, point->X);
		PCB_MAKE_MAX(Polygon->bbox_naked.Y2, point->Y);
	}
	PCB_END_LOOP;

	/* clearance is generally considered to be part of the bbox for all objects */
	{
		rnd_coord_t clr = PCB_POLY_HAS_CLEARANCE(Polygon) ? Polygon->Clearance/2 : 0;
		Polygon->BoundingBox.X1 = Polygon->bbox_naked.X1 - clr;
		Polygon->BoundingBox.Y1 = Polygon->bbox_naked.Y1 - clr;
		Polygon->BoundingBox.X2 = Polygon->bbox_naked.X2 + clr;
		Polygon->BoundingBox.Y2 = Polygon->bbox_naked.Y2 + clr;
	}

	/* boxes don't include the lower right corner */
	rnd_close_box(&Polygon->bbox_naked);
	rnd_close_box(&Polygon->BoundingBox);
}

int pcb_poly_eq(const pcb_host_trans_t *tr1, const pcb_poly_t *p1, const pcb_host_trans_t *tr2, const pcb_poly_t *p2)
{
	if (p1->PointN != p2->PointN) return 0;
	if (pcb_field_neq(p1, p2, Clearance)) return 0;
	if (pcb_neqs(p1->term, p2->term)) return 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, p1) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, p2)) {
		pcb_cardinal_t n;
		for(n = 0; n < p1->PointN; n++) {
			rnd_coord_t x1, y1, x2, y2;
			pcb_hash_tr_coords(tr1, &x1, &y1, p1->Points[n].X, p1->Points[n].Y);
			pcb_hash_tr_coords(tr2, &x2, &y2, p2->Points[n].X, p2->Points[n].Y);
			if ((x1 != x2) || (y1 != y2)) return 0;
		}
	}
	return 1;
}

unsigned int pcb_poly_hash(const pcb_host_trans_t *tr, const pcb_poly_t *p)
{
	unsigned int crd = 0;
	pcb_cardinal_t n;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, p))
		for(n = 0; n < p->PointN; n++) {
			rnd_coord_t x, y;

			pcb_hash_tr_coords(tr, &x, &y, p->Points[n].X, p->Points[n].Y);
			crd ^= pcb_hash_coord(x) ^ pcb_hash_coord(y);
		}

	return pcb_hash_coord(p->Clearance) ^ pcb_hash_str(p->term) ^ crd;
}


/* creates a new polygon from the old formats rectangle data */
pcb_poly_t *pcb_poly_new_from_rectangle(pcb_layer_t *Layer, rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, rnd_coord_t Clearance, pcb_flag_t Flags)
{
	pcb_poly_t *polygon = pcb_poly_new(Layer, Clearance, Flags);
	if (!polygon)
		return polygon;

	pcb_poly_point_new(polygon, X1, Y1);
	pcb_poly_point_new(polygon, X2, Y1);
	pcb_poly_point_new(polygon, X2, Y2);
	pcb_poly_point_new(polygon, X1, Y2);

	pcb_add_poly_on_layer(Layer, polygon);
	return polygon;
}

pcb_poly_t *pcb_poly_new_from_poly(pcb_layer_t *Layer, pcb_poly_t *src, rnd_coord_t offs, rnd_coord_t Clearance, pcb_flag_t Flags)
{
	rnd_coord_t x, y;
	pcb_poly_it_t it;
	pcb_pline_t *pl;
	int go;
	pcb_poly_t *polygon = pcb_poly_new(Layer, Clearance, Flags);

	if (!polygon)
		return polygon;

	it.pa = src->Clipped;
	pcb_poly_island_first(src, &it);
	pl = pcb_poly_contour(&it);
	it.cntr = pcb_pline_dup_offset(pl, offs);
	for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y))
		pcb_poly_point_new(polygon, x, y);

	pcb_poly_contours_free(&it.cntr);

	pcb_add_poly_on_layer(Layer, polygon);
	return polygon;
}



void pcb_add_poly_on_layer(pcb_layer_t *Layer, pcb_poly_t *polygon)
{
	pcb_poly_bbox(polygon);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) polygon);
	PCB_SET_PARENT(polygon, layer, Layer);
	pcb_poly_clear_from_poly(Layer->parent.data, PCB_OBJ_POLY, Layer, polygon);
}

/* creates a new polygon on a layer */
pcb_poly_t *pcb_poly_new(pcb_layer_t *Layer, rnd_coord_t Clearance, pcb_flag_t Flags)
{
	pcb_poly_t *polygon = pcb_poly_alloc(Layer);

	/* copy values */
	polygon->Flags = Flags;

	polygon->Clearance = Clearance;
	polygon->Clipped = NULL;
	polygon->NoHoles = NULL;
	polygon->NoHolesValid = 0;
	return polygon;
}

static pcb_poly_t *pcb_poly_copy_meta(pcb_poly_t *dst, pcb_poly_t *src)
{
	if (dst == NULL)
		return NULL;
	rnd_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}

pcb_poly_t *pcb_poly_dup(pcb_layer_t *dst, pcb_poly_t *src)
{
	pcb_board_t *pcb;
	pcb_poly_t *p = pcb_poly_new(dst, src->Clearance, src->Flags);
	pcb_poly_copy(p, src, 0, 0);
	pcb_add_poly_on_layer(dst, p);
	pcb_poly_copy_meta(p, src);

	pcb = pcb_data_get_top(dst->parent.data);
	if (pcb != NULL)
		pcb_poly_init_clip(pcb->Data, dst, p);
	return p;
}

pcb_poly_t *pcb_poly_dup_at(pcb_layer_t *dst, pcb_poly_t *src, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_board_t *pcb;
	pcb_poly_t *p = pcb_poly_new(dst, src->Clearance, src->Flags);
	pcb_poly_copy(p, src, dx, dy);
	pcb_add_poly_on_layer(dst, p);
	pcb_poly_copy_meta(p, src);

	pcb = pcb_data_get_top(dst->parent.data);
	if (pcb != NULL)
		pcb_poly_init_clip(pcb->Data, dst, p);
	return p;
}

/* creates a new point in a polygon */
pcb_point_t *pcb_poly_point_new(pcb_poly_t *Polygon, rnd_coord_t X, rnd_coord_t Y)
{
	pcb_point_t *point = pcb_poly_point_alloc(Polygon);

	/* copy values */
	point->X = X;
	point->Y = Y;
TODO("ID: register points too")
	point->ID = pcb_create_ID_get();
	return point;
}

/* creates a new hole in a polygon */
pcb_poly_t *pcb_poly_hole_new(pcb_poly_t * Polygon)
{
	pcb_cardinal_t *holeindex = pcb_poly_holeidx_new(Polygon);
	*holeindex = Polygon->PointN;
	return Polygon;
}

/* copies data from one polygon to another; 'Dest' has to exist */
pcb_poly_t *pcb_poly_copy(pcb_poly_t *Dest, pcb_poly_t *Src, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_cardinal_t hole = 0;
	pcb_cardinal_t n;

	for (n = 0; n < Src->PointN; n++) {
		if (hole < Src->HoleIndexN && n == Src->HoleIndex[hole]) {
			pcb_poly_hole_new(Dest);
			hole++;
		}
		pcb_poly_point_new(Dest, Src->Points[n].X+dx, Src->Points[n].Y+dy);
	}
	pcb_poly_bbox(Dest);
	Dest->Flags = Src->Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, Dest);
	return Dest;
}

static double poly_area(pcb_point_t *points, pcb_cardinal_t n_points)
{
	double area = 0;
	int n;

	for(n = 1; n < n_points; n++)
		area += (double)(points[n-1].X - points[n].X) * (double)(points[n-1].Y + points[n].Y);
	area +=(double)(points[n_points-1].X - points[0].X) * (double)(points[n_points-1].Y + points[0].Y);

	if (area > 0)
		area /= 2.0;
	else
		area /= -2.0;

	return area;
}

double pcb_poly_area(const pcb_poly_t *poly)
{
	double area;

	if (poly->HoleIndexN > 0) {
		int h;
		area = poly_area(poly->Points, poly->HoleIndex[0]);
		for(h = 0; h < poly->HoleIndexN - 1; h++)
			area -= poly_area(poly->Points + poly->HoleIndex[h], poly->HoleIndex[h+1] - poly->HoleIndex[h]);
		area -= poly_area(poly->Points + poly->HoleIndex[h], poly->PointN - poly->HoleIndex[h]);
	}
	else
		area = poly_area(poly->Points, poly->PointN);

	return area;
}



/*** ops ***/

/* copies a polygon to buffer */
void *pcb_polyop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_poly_t *polygon;
	pcb_layer_id_t lid = pcb_layer_id(ctx->buffer.src, Layer);
	pcb_layer_t *layer;

	/* the buffer may not have the specified layer, e.g. on loose subc subc-aux layer */
	if (lid == -1)
		return NULL;

	layer = &ctx->buffer.dst->Layer[lid];

	polygon = pcb_poly_new(layer, Polygon->Clearance, Polygon->Flags);
	pcb_poly_copy(polygon, Polygon, 0, 0);
	pcb_poly_copy_meta(polygon, Polygon);
	if (ctx->buffer.keep_id) pcb_obj_change_id((pcb_any_obj_t *)polygon, Polygon->ID);

	/* Update the polygon r-tree. Unlike similarly named functions for
	 * other objects, CreateNewPolygon does not do this as it creates a
	 * skeleton polygon object, which won't have correct bounds.
	 */
	if (!layer->polygon_tree)
		layer->polygon_tree = pcb_r_create_tree();
	pcb_r_insert_entry(layer->polygon_tree, (rnd_box_t *) polygon);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND | ctx->buffer.extraflg, polygon);
	return polygon;
}


/* moves a polygon between board and buffer. Doesn't allocate memory for the points */
void *pcb_polyop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_poly_t *polygon)
{
	pcb_layer_t *srcly = polygon->parent.layer;

	assert(polygon->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) { /* auto layer in dst */
		pcb_layer_id_t lid = pcb_layer_id(ctx->buffer.src, srcly);
		if (lid < 0) {
			rnd_message(PCB_MSG_ERROR, "Internal error: can't resolve source layer ID in pcb_polyop_move_buffer\n");
			return NULL;
		}
		dstly = &ctx->buffer.dst->Layer[lid];
	}

	pcb_poly_pprestore(polygon);

	pcb_r_delete_entry(srcly->polygon_tree, (rnd_box_t *)polygon);

	pcb_poly_unreg(polygon);
	pcb_poly_reg(dstly, polygon);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, polygon);

	if (!dstly->polygon_tree)
		dstly->polygon_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dstly->polygon_tree, (rnd_box_t *)polygon);

	pcb_poly_ppclear(polygon);
	return polygon;
}

/* Handle attempts to change the clearance of a polygon. */
void *pcb_polyop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *poly)
{
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, poly)) {
		rnd_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : poly->Clearance + ctx->chgsize.value;

		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, poly))
			return NULL;

		value = MIN(PCB_MAX_THICKNESS, value);
		if (!ctx->chgsize.is_absolute && ctx->chgsize.value < 0)
			value = 0;
		if (value != poly->Clearance) {
			pcb_undo_add_obj_to_clear_size(PCB_OBJ_POLY, Layer, poly, poly);
			pcb_poly_restore_to_poly(ctx->chgsize.pcb->Data, PCB_OBJ_POLY, Layer, poly);
			pcb_poly_invalidate_erase(poly);
			pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *)poly);
			poly->Clearance = value;
			pcb_poly_bbox(poly);
			pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *)poly);
			pcb_poly_clear_from_poly(ctx->chgsize.pcb->Data, PCB_OBJ_POLY, Layer, poly);
			pcb_poly_invalidate_draw(Layer, poly);
			return poly;
		}

		return poly;
	}

	/* poly does not clear other polys */
	rnd_message(PCB_MSG_WARNING,
		"To change the clearance of objects in a polygon, change \nthe objects, not the polygon.\n"
		"Alternatively, set the clearpolypoly flag on the polygon to \nallow it to clear other polygons.\n"
		"Hint: To set a minimum clearance for a group of objects, \nselect them all then :MinClearGap(Selected,=10,mil)\n",
		"Ok", NULL);

	return NULL;
}

/* changes the CLEARPOLY flag of a polygon */
void *pcb_polyop_change_clear(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Polygon))
		return NULL;
	pcb_undo_add_obj_to_clear_poly(PCB_OBJ_POLY, Layer, Polygon, Polygon, pcb_true);
	pcb_undo_add_obj_to_flag(Polygon);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARPOLY, Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	pcb_poly_invalidate_draw(Layer, Polygon);
	return Polygon;
}

/* inserts a point into a polygon */
void *pcb_polyop_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_point_t save;
	pcb_cardinal_t n;
	pcb_line_t line;

	pcb_poly_pprestore(Polygon);

	if (!ctx->insert.forcible) {
		/*
		 * first make sure adding the point is sensible
		 */
		line.Thickness = 0;
		line.Point1 = Polygon->Points[pcb_poly_contour_prev_point(Polygon, ctx->insert.idx)];
		line.Point2 = Polygon->Points[ctx->insert.idx];
		if (pcb_is_point_on_line((float) ctx->insert.x, (float) ctx->insert.y, 0.0, &line))
			return NULL;
	}
	/*
	 * second, shift the points up to make room for the new point
	 */
	pcb_poly_invalidate_erase(Polygon);
	pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	save = *pcb_poly_point_new(Polygon, ctx->insert.x, ctx->insert.y);
	for (n = Polygon->PointN - 1; n > ctx->insert.idx; n--)
		Polygon->Points[n] = Polygon->Points[n - 1];

	/* Shift up indices of any holes */
	for (n = 0; n < Polygon->HoleIndexN; n++)
		if (Polygon->HoleIndex[n] > ctx->insert.idx || (ctx->insert.last && Polygon->HoleIndex[n] == ctx->insert.idx))
			Polygon->HoleIndex[n]++;

	Polygon->Points[ctx->insert.idx] = save;
	pcb_board_set_changed_flag(pcb_true);
	pcb_undo_add_obj_to_insert_point(PCB_OBJ_POLY_POINT, Layer, Polygon, &Polygon->Points[ctx->insert.idx]);

	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (ctx->insert.forcible || !pcb_poly_remove_excess_points(Layer, Polygon))
		pcb_poly_invalidate_draw(Layer, Polygon);

	pcb_poly_ppclear(Polygon);

	return (&Polygon->Points[ctx->insert.idx]);
}

/* changes the clearance flag of a polygon */
void *pcb_polyop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *poly)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, poly))
		return NULL;
	pcb_poly_invalidate_erase(poly);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, poly)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_POLY, Layer, poly, poly, pcb_false);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_POLY, Layer, poly);
	}
	pcb_undo_add_obj_to_flag(poly);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARPOLYPOLY, poly);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, poly)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_POLY, Layer, poly, poly, pcb_true);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_POLY, Layer, poly);
	}
	pcb_poly_invalidate_draw(Layer, poly);
	return poly;
}

/* low level routine to move a polygon */
void pcb_poly_move(pcb_poly_t *Polygon, rnd_coord_t DX, rnd_coord_t DY)
{
	PCB_POLY_POINT_LOOP(Polygon);
	{
		RND_MOVE_POINT(point->X, point->Y, DX, DY);
	}
	PCB_END_LOOP;
	RND_BOX_MOVE_LOWLEVEL(&Polygon->BoundingBox, DX, DY);
	RND_BOX_MOVE_LOWLEVEL(&Polygon->bbox_naked, DX, DY);
}

/* moves a polygon */
void *pcb_polyop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	if (Layer->meta.real.vis) {
		pcb_poly_invalidate_erase(Polygon);
	}
	pcb_poly_move(Polygon, ctx->move.dx, ctx->move.dy);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_draw(Layer, Polygon);
	return Polygon;
}

void *pcb_polyop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_pprestore(Polygon);
	pcb_polyop_move_noclip(ctx, Layer, Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_ppclear(Polygon);
	return Polygon;
}

void *pcb_polyop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	if (ctx->clip.restore) {
		pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
		pcb_poly_pprestore(Polygon);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
		pcb_poly_ppclear(Polygon);
	}
	return Polygon;
}
/* moves a polygon-point */
void *pcb_polyop_move_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon, pcb_point_t *Point)
{
	pcb_poly_pprestore(Polygon);
	if (Layer->meta.real.vis) {
		pcb_poly_invalidate_erase(Polygon);
	}
	pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	RND_MOVE_POINT(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_remove_excess_points(Layer, Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_draw(Layer, Polygon);
	pcb_poly_ppclear(Polygon);
	return Point;
}

/* moves a polygon between layers; lowlevel routines */
void *pcb_polyop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_poly_t * polygon, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->polygon_tree, (rnd_box_t *) polygon);

	pcb_poly_pprestore(polygon);
	pcb_poly_unreg(polygon);
	pcb_poly_reg(Destination, polygon);

	if (!Destination->polygon_tree)
		Destination->polygon_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Destination->polygon_tree, (rnd_box_t *) polygon);

	pcb_poly_ppclear(polygon);
	return polygon;
}

/* moves a polygon between layers */
void *pcb_polyop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_poly_t * Polygon)
{
	pcb_poly_t *newone;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Polygon)) {
		rnd_message(PCB_MSG_WARNING, "Sorry, polygon object is locked\n");
		return NULL;
	}
	if (Layer == ctx->move.dst_layer)
		return Polygon;
	pcb_undo_add_obj_to_move_to_layer(PCB_OBJ_POLY, Layer, Polygon, Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_erase(Polygon);

	newone = (struct pcb_poly_s *) pcb_polyop_move_to_layer_low(ctx, Layer, Polygon, ctx->move.dst_layer);
	pcb_poly_init_clip(PCB->Data, ctx->move.dst_layer, newone);
	if (ctx->move.dst_layer->meta.real.vis)
		pcb_poly_invalidate_draw(ctx->move.dst_layer, newone);
	return newone;
}


/* destroys a polygon from a layer */
void *pcb_polyop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_free_fields(Polygon);

	pcb_poly_free(Polygon);

	return NULL;
}

/* removes a polygon-point from a polygon and destroys the data */
void *pcb_polyop_destroy_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon, pcb_point_t *Point)
{
	pcb_cardinal_t point_idx;
	pcb_cardinal_t i;
	pcb_cardinal_t contour;
	pcb_cardinal_t contour_start, contour_end, contour_points;

	pcb_poly_pprestore(Polygon);
	point_idx = pcb_poly_point_idx(Polygon, Point);
	contour = pcb_poly_contour_point(Polygon, point_idx);
	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	if (contour_points <= 3)
		return pcb_polyop_remove_counter(ctx, Layer, Polygon, contour);

	pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);

	/* remove point from list, keep point order */
	for (i = point_idx; i < Polygon->PointN - 1; i++)
		Polygon->Points[i] = Polygon->Points[i + 1];
	Polygon->PointN--;

	/* Shift down indices of any holes */
	for (i = 0; i < Polygon->HoleIndexN; i++)
		if (Polygon->HoleIndex[i] > point_idx)
			Polygon->HoleIndex[i]--;

	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	pcb_poly_ppclear(Polygon);
	return Polygon;
}

/* removes a polygon from a layer */
void *pcb_polyop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	/* erase from screen */
	pcb_poly_pprestore(Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_erase(Polygon);
	pcb_undo_move_obj_to_remove(PCB_OBJ_POLY, Layer, Polygon, Polygon);
	return NULL;
}

void *pcb_poly_remove(pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	res = pcb_polyop_remove(&ctx, Layer, Polygon);
	pcb_draw();
	return res;
}

/* Removes a contour from a polygon.
   If removing the outer contour, it removes the whole polygon. */
void *pcb_polyop_remove_counter(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon, pcb_cardinal_t contour)
{
	pcb_cardinal_t contour_start, contour_end, contour_points;
	pcb_cardinal_t i;

	if (contour == 0)
		return pcb_poly_remove(Layer, Polygon);

	pcb_poly_pprestore(Polygon);

	if (Layer->meta.real.vis)
		pcb_poly_invalidate_erase(Polygon);

	/* Copy the polygon to the undo list */
	pcb_undo_add_obj_to_remove_contour(PCB_OBJ_POLY, Layer, Polygon);

	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	/* remove points from list, keep point order */
	for (i = contour_start; i < Polygon->PointN - contour_points; i++)
		Polygon->Points[i] = Polygon->Points[i + contour_points];
	Polygon->PointN -= contour_points;

	/* remove hole from list and shift down remaining indices */
	for (i = contour; i < Polygon->HoleIndexN; i++)
		Polygon->HoleIndex[i - 1] = Polygon->HoleIndex[i] - contour_points;
	Polygon->HoleIndexN--;

	pcb_poly_init_clip(PCB->Data, Layer, Polygon);

	pcb_poly_ppclear(Polygon);

	/* redraw polygon if necessary */
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_draw(Layer, Polygon);
	return NULL;
}

/* removes a polygon-point from a polygon */
void *pcb_polyop_remove_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon, pcb_point_t *Point)
{
	pcb_cardinal_t point_idx;
	pcb_cardinal_t i;
	pcb_cardinal_t contour;
	pcb_cardinal_t contour_start, contour_end, contour_points;

	pcb_poly_pprestore(Polygon);

	point_idx = pcb_poly_point_idx(Polygon, Point);
	contour = pcb_poly_contour_point(Polygon, point_idx);
	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	if (contour_points <= 3)
		return pcb_polyop_remove_counter(ctx, Layer, Polygon, contour);

	if (Layer->meta.real.vis)
		pcb_poly_invalidate_erase(Polygon);

	/* insert the polygon-point into the undo list */
	pcb_undo_add_obj_to_remove_point(PCB_OBJ_POLY_POINT, Layer, Polygon, point_idx);
	pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);

	/* remove point from list, keep point order */
	for (i = point_idx; i < Polygon->PointN - 1; i++)
		Polygon->Points[i] = Polygon->Points[i + 1];
	Polygon->PointN--;

	/* Shift down indices of any holes */
	for (i = 0; i < Polygon->HoleIndexN; i++)
		if (Polygon->HoleIndex[i] > point_idx)
			Polygon->HoleIndex[i]--;

	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_remove_excess_points(Layer, Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);

	/* redraw polygon if necessary */
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_draw(Layer, Polygon);

	pcb_poly_ppclear(Polygon);
	return NULL;
}

/* copies a polygon */
void *pcb_polyop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_poly_t *polygon;

	polygon = pcb_poly_new(Layer, Polygon->Clearance, pcb_no_flags());
	if (ctx->copy.keep_id)
		polygon->ID = Polygon->ID;
	pcb_poly_copy(polygon, Polygon, ctx->copy.DeltaX, ctx->copy.DeltaY);
	pcb_poly_copy_meta(polygon, Polygon);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) polygon);
	pcb_poly_init_clip(PCB->Data, Layer, polygon);
	pcb_poly_invalidate_draw(Layer, polygon);
	pcb_undo_add_obj_to_create(PCB_OBJ_POLY, Layer, polygon, polygon);
	pcb_poly_ppclear(polygon);
	return polygon;
}

void *pcb_polyop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_poly_pprestore(Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_erase(Polygon);
	if (Layer->polygon_tree != NULL)
		pcb_r_delete_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_rotate90(Polygon, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	if (Layer->polygon_tree != NULL)
		pcb_r_insert_entry(Layer->polygon_tree, (rnd_box_t *) Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_draw(Layer, Polygon);
	pcb_poly_ppclear(Polygon);
	return Polygon;
}

void *pcb_polyop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_poly_pprestore(Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_erase(Polygon);
	pcb_poly_rotate(Layer, Polygon, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (Layer->meta.real.vis)
		pcb_poly_invalidate_draw(Layer, Polygon);
	pcb_poly_ppclear(Polygon);
	return Polygon;
}

void *pcb_polyop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	static pcb_flag_values_t pcb_poly_flags = 0;
	if (pcb_poly_flags == 0)
		pcb_poly_flags = pcb_obj_valid_flags(PCB_OBJ_POLY);

	if ((ctx->chgflag.flag & pcb_poly_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (Polygon->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(Polygon);


	if (ctx->chgflag.flag & (PCB_FLAG_CLEARPOLY | PCB_FLAG_CLEARPOLYPOLY))
		pcb_poly_restore_to_poly(ctx->chgflag.pcb->Data, PCB_OBJ_POLY, Polygon->parent.layer, Polygon);

	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, Polygon);

	if (ctx->chgflag.flag & PCB_FLAG_FULLPOLY)
		pcb_poly_init_clip(ctx->chgflag.pcb->Data, Polygon->parent.layer, Polygon);

	if (ctx->chgflag.flag & (PCB_FLAG_CLEARPOLY | PCB_FLAG_CLEARPOLYPOLY))
		pcb_poly_clear_from_poly(ctx->chgflag.pcb->Data, PCB_OBJ_POLY, Polygon->parent.layer, Polygon);

	return Polygon;
}

void pcb_poly_pre(pcb_poly_t *poly)
{
	pcb_layer_t *ly = pcb_layer_get_real(poly->parent.layer);
	if (ly == NULL)
		return;

	pcb_poly_pprestore(poly);
	if (ly->polygon_tree != NULL)
		pcb_r_delete_entry(ly->polygon_tree, (rnd_box_t *)poly);
}

void pcb_poly_post(pcb_poly_t *poly)
{
	pcb_layer_t *ly = pcb_layer_get_real(poly->parent.layer);
	if (ly == NULL)
		return;

	if (ly->polygon_tree != NULL)
		pcb_r_insert_entry(ly->polygon_tree, (rnd_box_t *)poly);
	pcb_poly_ppclear(poly);
}


/*** iteration helpers ***/
void pcb_poly_map_contours(pcb_poly_t *p, void *ctx, pcb_poly_map_cb_t *cb)
{
	pcb_polyarea_t *pa;
	pcb_pline_t *pl;

	pa = p->Clipped;
	do {
		int cidx;
		for(cidx = 0, pl = pa->contours; pl != NULL; cidx++, pl = pl->next) {
			pcb_vnode_t *v;
			cb(p, ctx, (cidx == 0 ? PCB_POLYEV_ISLAND_START : PCB_POLYEV_HOLE_START), 0, 0);
			v = pl->head->next;
			do {
				cb(p, ctx, (cidx == 0 ? PCB_POLYEV_ISLAND_POINT : PCB_POLYEV_HOLE_POINT), v->point[0], v->point[1]);
			} while ((v = v->next) != pl->head->next);

			cb(p, ctx, (cidx == 0 ? PCB_POLYEV_ISLAND_END : PCB_POLYEV_HOLE_END), 0, 0);
		}
		pa = pa->f;
	} while(pa != p->Clipped);
}

void *pcb_polyop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_poly_t *poly)
{
	pcb_poly_name_invalidate_draw(poly);
	return poly;
}

/*** draw ***/

static rnd_bool is_poly_term_vert(const pcb_poly_t *poly)
{
	rnd_coord_t dx, dy;

	dx = poly->BoundingBox.X2 - poly->BoundingBox.X1;
	if (dx < 0)
		dx = -dx;

	dy = poly->BoundingBox.Y2 - poly->BoundingBox.Y1;
	if (dy < 0)
		dy = -dy;

	return dx < dy;
}


void pcb_poly_name_invalidate_draw(pcb_poly_t *poly)
{
	if (poly->term != NULL) {
		pcb_term_label_invalidate((poly->BoundingBox.X1 + poly->BoundingBox.X2)/2, (poly->BoundingBox.Y1 + poly->BoundingBox.Y2)/2,
			100.0, is_poly_term_vert(poly), pcb_true, (pcb_any_obj_t *)poly);
	}
}

void pcb_poly_draw_label(pcb_draw_info_t *info, pcb_poly_t *poly)
{
	if (poly->term != NULL)
		pcb_term_label_draw(info, (poly->BoundingBox.X1 + poly->BoundingBox.X2)/2, (poly->BoundingBox.Y1 + poly->BoundingBox.Y2)/2,
			conf_core.appearance.term_label_size, is_poly_term_vert(poly), pcb_true, (pcb_any_obj_t *)poly);

	if (poly->noexport)
		pcb_obj_noexport_mark(poly, (poly->BoundingBox.X1 + poly->BoundingBox.X2)/2, (poly->BoundingBox.Y1 + poly->BoundingBox.Y2)/2);
}

void pcb_poly_draw_annotation(pcb_draw_info_t *info, pcb_poly_t *poly)
{
	pcb_cardinal_t n, np;

	if (!conf_core.editor.as_drawn_poly)
		return;

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, poly))
		pcb_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.selected);
	else
		pcb_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.pin_name);

	pcb_hid_set_line_width(pcb_draw_out.fgGC, -1);
	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);

	if (poly->HoleIndexN > 0)
		np = poly->HoleIndex[0];
	else
		np = poly->PointN;

	for(n = 1; n < np; n++)
		pcb_render->draw_line(pcb_draw_out.fgGC, poly->Points[n-1].X, poly->Points[n-1].Y, poly->Points[n].X, poly->Points[n].Y);
	pcb_render->draw_line(pcb_draw_out.fgGC, poly->Points[n-1].X, poly->Points[n-1].Y, poly->Points[0].X, poly->Points[0].Y);
}

static void pcb_poly_draw_tr_offs(pcb_poly_it_t *it, rnd_coord_t offs)
{
	int go;
	long len, n;
	rnd_coord_t x, y;
	pcb_polo_t *p, p_st[256];

	/* calculate length of the polyline */
	for(go = pcb_poly_vect_first(it, &x, &y), len = 0; go; go = pcb_poly_vect_next(it, &x, &y))
		len++;

	if (len >= sizeof(p_st) / sizeof(p_st[0]))
		p = malloc(sizeof(pcb_polo_t) * len);
	else
		p = p_st;

	for(go = pcb_poly_vect_first(it, &x, &y), n = 0; go; go = pcb_poly_vect_next(it, &x, &y), n++) {
		p[n].x = x;
		p[n].y = y;
	}

	pcb_polo_norms(p, len);
	pcb_polo_offs(offs, p, len);


	for(go = pcb_poly_vect_first(it, &x, &y), n = 0; go; go = pcb_poly_vect_next(it, &x, &y), n++) {
		it->v->point[0] = pcb_round(p[n].x);
		it->v->point[1] = pcb_round(p[n].y);
	}

	if (p != p_st)
		free(p);
}

static pcb_poly_t *pcb_poly_draw_tr(pcb_draw_info_t *info, pcb_poly_t *polygon)
{
	pcb_poly_t *np = pcb_poly_alloc(polygon->parent.layer);
	pcb_poly_it_t it;
	pcb_polyarea_t *pa;
	rnd_coord_t offs = info->xform->bloat / 2;

	pcb_poly_copy(np, polygon, 0, 0);
	pcb_polyarea_copy0(&np->Clipped, polygon->Clipped);

	/* iterate over all islands of a polygon */
	for(pa = pcb_poly_island_first(np, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		pcb_pline_t *pl;

		/* check if we have a contour for the given island */
		pl = pcb_poly_contour(&it);
		if (pl == NULL)
			continue;

		/* iterate over the vectors of the contour */
		pcb_poly_draw_tr_offs(&it, offs);

		/* iterate over all holes within this island */
		for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it))
			pcb_poly_draw_tr_offs(&it, offs);
	}

	return np;
}

void pcb_poly_draw_(pcb_draw_info_t *info, pcb_poly_t *polygon, int allow_term_gfx)
{
	pcb_poly_t *trpoly = NULL;

	if (delayed_terms_enabled && (polygon->term != NULL)) {
		pcb_draw_delay_obj_add((pcb_any_obj_t *)polygon);
		return;
	}

	if (conf_core.editor.as_drawn_poly)
		pcb_draw_annotation_add((pcb_any_obj_t *)polygon);

	if ((info != NULL) && (info->xform != NULL) && (info->xform->bloat != 0)) {
		/* Slow dupping and recalculation every time; if we ever need this on-screen, we should cache */
		trpoly = pcb_poly_draw_tr(info, polygon);
		polygon = trpoly;
	}

	if ((info->xform->thin_draw || info->xform->thin_draw_poly || info->xform->wireframe))
	{
		pcb_dhlp_thindraw_pcb_polygon(pcb_draw_out.fgGC, polygon, info->drawn_area);
	}
	else {
		if ((allow_term_gfx) && pcb_draw_term_need_gfx(polygon) && pcb_draw_term_hid_permission()) {
			pcb_vnode_t *n, *head;
			int i;
			pcb_dhlp_fill_pcb_polygon(pcb_draw_out.active_padGC, polygon, info->drawn_area);
			head = polygon->Clipped->contours->head;
			pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_square);
			for(n = head, i = 0; (n != head) || (i == 0); n = n->next, i++) {
				rnd_coord_t x, y, r;
				x = (n->prev->point[0] + n->point[0] + n->next->point[0]) / 3;
				y = (n->prev->point[1] + n->point[1] + n->next->point[1]) / 3;

TODO("subc: check if x;y is within the poly, but use a cheaper method than the official")
				r = PCB_DRAW_TERM_GFX_WIDTH;
				pcb_hid_set_line_width(pcb_draw_out.fgGC, r);
				pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_square);
				pcb_render->draw_line(pcb_draw_out.fgGC, x, y, x, y);
			}
		}
		else
			pcb_dhlp_fill_pcb_polygon(pcb_draw_out.fgGC, polygon, info->drawn_area);
	}

	/* If checking planes, thin-draw any pieces which have been clipped away */
	if (info->xform->check_planes && !PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, polygon)) {
		pcb_poly_t poly = *polygon;

		for (poly.Clipped = polygon->Clipped->f; poly.Clipped != polygon->Clipped; poly.Clipped = poly.Clipped->f)
			pcb_dhlp_thindraw_pcb_polygon(pcb_draw_out.fgGC, &poly, info->drawn_area);
	}

	if (polygon->term != NULL) {
		if ((pcb_draw_force_termlab) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, polygon))
			pcb_draw_delay_label_add((pcb_any_obj_t *)polygon);
	}

	if (trpoly != NULL)
		pcb_poly_free(trpoly);
}

static void pcb_poly_draw(pcb_draw_info_t *info, pcb_poly_t *polygon, int allow_term_gfx)
{
	static const pcb_color_t *color;
	pcb_color_t buf;
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(polygon->parent.layer);

	pcb_obj_noexport(info, polygon, return);

	if (layer == NULL) /* if the layer is inbound, e.g. in preview, fall back using the layer recipe */
		layer = polygon->parent.layer;

	if (info->xform->flag_color && PCB_FLAG_TEST(PCB_FLAG_WARN, polygon))
		color = &conf_core.appearance.color.warn;
	else if (info->xform->flag_color && PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon)) {
		if (layer->is_bound)
			PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
		else
			color = &conf_core.appearance.color.selected;
	}
	else if (info->xform->flag_color && PCB_FLAG_TEST(PCB_FLAG_FOUND, polygon))
		color = &conf_core.appearance.color.connected;
	else if (info->xform->flag_color && PCB_FLAG_TEST(PCB_FLAG_ONPOINT, polygon)) {
		pcb_lighten_color(color, &buf, 1.75);
		color = &buf;
	}
	else if (PCB_HAS_COLOROVERRIDE(polygon)) {
		color = polygon->override_color;
	}
	else if (layer->is_bound)
		PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 0);
	else
		color = &layer->meta.real.color;
	pcb_render->set_color(pcb_draw_out.fgGC, color);

	pcb_poly_draw_(info, polygon, allow_term_gfx);
}

pcb_r_dir_t pcb_poly_draw_callback(const rnd_box_t * b, void *cl)
{
	pcb_draw_info_t *i = cl;
	pcb_poly_t *polygon = (pcb_poly_t *) b;

	if (pcb_hidden_floater((pcb_any_obj_t*)b, i) || pcb_partial_export((pcb_any_obj_t*)b, i))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!polygon->Clipped)
		return PCB_R_DIR_NOT_FOUND;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(polygon->parent_type, &polygon->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_poly_draw(i, polygon, 0);

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_poly_draw_term_callback(const rnd_box_t * b, void *cl)
{
	pcb_draw_info_t *i = cl;
	pcb_poly_t *polygon = (pcb_poly_t *) b;

	if (pcb_hidden_floater((pcb_any_obj_t*)b, i) || pcb_partial_export((pcb_any_obj_t*)b, i))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (polygon->Clipped == NULL) {
		/* if poly is cleared out of existence, it may still have some annotations
		   visible. Do it the cheap way: if it has term, it's not cleared, so the
		   only thing affected is the as-drawn contour */
		if (conf_core.editor.as_drawn_poly)
			pcb_draw_annotation_add((pcb_any_obj_t *)polygon);
		return PCB_R_DIR_NOT_FOUND;
	}

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(polygon->parent_type, &polygon->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_poly_draw(i, polygon, 1);

	return PCB_R_DIR_FOUND_CONTINUE;
}

/* erases a polygon on a layer */
void pcb_poly_invalidate_erase(pcb_poly_t *Polygon)
{
	pcb_draw_invalidate(Polygon);
	pcb_flag_erase(&Polygon->Flags);
}

void pcb_poly_invalidate_draw(pcb_layer_t *Layer, pcb_poly_t *Polygon)
{
	pcb_draw_invalidate(Polygon);
}

void pcb_poly_uninit(void)
{
	pcb_dhlp_uninit();
}
