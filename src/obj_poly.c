/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Drawing primitive: polygons */


#include "config.h"

#include "board.h"
#include "data.h"
#include "compat_nls.h"
#include "undo.h"
#include "polygon.h"
#include "rotate.h"
#include "search.h"

#include "conf_core.h"

#include "obj_poly.h"
#include "obj_poly_op.h"
#include "obj_poly_list.h"
#include "obj_poly_draw.h"

/* TODO: get rid of these: */
#include "draw.h"

#define	STEP_POLYGONPOINT	10
#define	STEP_POLYGONHOLEINDEX	10

/*** allocation ***/

/* get next slot for a polygon object, allocates memory if necessary */
pcb_polygon_t *pcb_poly_alloc(pcb_layer_t * layer)
{
	pcb_polygon_t *new_obj;

	new_obj = calloc(sizeof(pcb_polygon_t), 1);
	polylist_append(&layer->Polygon, new_obj);

	return new_obj;
}

void pcb_poly_free(pcb_polygon_t * data)
{
	polylist_remove(data);
	free(data);
}

/* gets the next slot for a point in a polygon struct, allocates memory if necessary */
pcb_point_t *pcb_poly_point_alloc(pcb_polygon_t *Polygon)
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
pcb_cardinal_t *pcb_poly_holeidx_new(pcb_polygon_t *Polygon)
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
void pcb_poly_free_fields(pcb_polygon_t * polygon)
{
	if (polygon == NULL)
		return;

	free(polygon->Points);
	free(polygon->HoleIndex);

	if (polygon->Clipped)
		pcb_polyarea_free(&polygon->Clipped);
	pcb_poly_contours_free(&polygon->NoHoles);

	reset_obj_mem(pcb_polygon_t, polygon);
}

/*** utility ***/

/* rotates a polygon in 90 degree steps */
void pcb_poly_rotate90(pcb_polygon_t *Polygon, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	PCB_POLY_POINT_LOOP(Polygon);
	{
		PCB_COORD_ROTATE90(point->X, point->Y, X, Y, Number);
	}
	END_LOOP;
	pcb_box_rotate90(&Polygon->BoundingBox, X, Y, Number);
}


/* sets the bounding box of a polygons */
void pcb_poly_bbox(pcb_polygon_t *Polygon)
{
	Polygon->BoundingBox.X1 = Polygon->BoundingBox.Y1 = PCB_MAX_COORD;
	Polygon->BoundingBox.X2 = Polygon->BoundingBox.Y2 = 0;
	PCB_POLY_POINT_LOOP(Polygon);
	{
		PCB_MAKE_MIN(Polygon->BoundingBox.X1, point->X);
		PCB_MAKE_MIN(Polygon->BoundingBox.Y1, point->Y);
		PCB_MAKE_MAX(Polygon->BoundingBox.X2, point->X);
		PCB_MAKE_MAX(Polygon->BoundingBox.Y2, point->Y);
	}
	/* boxes don't include the lower right corner */
	pcb_close_box(&Polygon->BoundingBox);
	END_LOOP;
}

/* creates a new polygon from the old formats rectangle data */
pcb_polygon_t *pcb_poly_new_from_rectangle(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_flag_t Flags)
{
	pcb_polygon_t *polygon = pcb_poly_new(Layer, Flags);
	if (!polygon)
		return (polygon);

	pcb_poly_point_new(polygon, X1, Y1);
	pcb_poly_point_new(polygon, X2, Y1);
	pcb_poly_point_new(polygon, X2, Y2);
	pcb_poly_point_new(polygon, X1, Y2);

	pcb_add_polygon_on_layer(Layer, polygon);
	return (polygon);
}

void pcb_add_polygon_on_layer(pcb_layer_t *Layer, pcb_polygon_t *polygon)
{
	pcb_poly_bbox(polygon);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) polygon, 0);
}

/* creates a new polygon on a layer */
pcb_polygon_t *pcb_poly_new(pcb_layer_t *Layer, pcb_flag_t Flags)
{
	pcb_polygon_t *polygon = pcb_poly_alloc(Layer);

	/* copy values */
	polygon->Flags = Flags;
	polygon->ID = CreateIDGet();
	polygon->Clipped = NULL;
	polygon->NoHoles = NULL;
	polygon->NoHolesValid = 0;
	return (polygon);
}

/* creates a new point in a polygon */
pcb_point_t *pcb_poly_point_new(pcb_polygon_t *Polygon, pcb_coord_t X, pcb_coord_t Y)
{
	pcb_point_t *point = pcb_poly_point_alloc(Polygon);

	/* copy values */
	point->X = X;
	point->Y = Y;
	point->ID = CreateIDGet();
	return (point);
}

/* creates a new hole in a polygon */
pcb_polygon_t *pcb_poly_hole_new(pcb_polygon_t * Polygon)
{
	pcb_cardinal_t *holeindex = pcb_poly_holeidx_new(Polygon);
	*holeindex = Polygon->PointN;
	return Polygon;
}

/* copies data from one polygon to another; 'Dest' has to exist */
pcb_polygon_t *pcb_poly_copy(pcb_polygon_t *Dest, pcb_polygon_t *Src)
{
	pcb_cardinal_t hole = 0;
	pcb_cardinal_t n;

	for (n = 0; n < Src->PointN; n++) {
		if (hole < Src->HoleIndexN && n == Src->HoleIndex[hole]) {
			pcb_poly_hole_new(Dest);
			hole++;
		}
		pcb_poly_point_new(Dest, Src->Points[n].X, Src->Points[n].Y);
	}
	pcb_poly_bbox(Dest);
	Dest->Flags = Src->Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, Dest);
	return (Dest);
}


/*** ops ***/
/* copies a polygon to buffer */
void *AddPolygonToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];
	pcb_polygon_t *polygon;

	polygon = pcb_poly_new(layer, Polygon->Flags);
	pcb_poly_copy(polygon, Polygon);

	/* Update the polygon r-tree. Unlike similarly named functions for
	 * other objects, CreateNewPolygon does not do this as it creates a
	 * skeleton polygon object, which won't have correct bounds.
	 */
	if (!layer->polygon_tree)
		layer->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(layer->polygon_tree, (pcb_box_t *) polygon, 0);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND | ctx->buffer.extraflg, polygon);
	return (polygon);
}


/* moves a polygon to buffer. Doesn't allocate memory for the points */
void *MovePolygonToBuffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_polygon_t * polygon)
{
	pcb_layer_t *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	pcb_r_delete_entry(layer->polygon_tree, (pcb_box_t *) polygon);

	polylist_remove(polygon);
	polylist_append(&lay->Polygon, polygon);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, polygon);

	if (!lay->polygon_tree)
		lay->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(lay->polygon_tree, (pcb_box_t *) polygon, 0);
	return (polygon);
}

/* Handle attempts to change the clearance of a polygon. */
void *ChangePolygonClearSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *poly)
{
	static int shown_this_message = 0;
	if (!shown_this_message) {
		pcb_gui->confirm_dialog(_("To change the clearance of objects in a polygon, "
													"change the objects, not the polygon.\n"
													"Hint: To set a minimum clearance for a group of objects, "
													"select them all then :MinClearGap(Selected,=10,mil)"), "Ok", NULL);
		shown_this_message = 1;
	}

	return (NULL);
}

/* changes the CLEARPOLY flag of a polygon */
void *ChangePolyClear(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Polygon))
		return (NULL);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_POLYGON, Layer, Polygon, Polygon, pcb_true);
	pcb_undo_add_obj_to_flag(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARPOLY, Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	DrawPolygon(Layer, Polygon);
	return (Polygon);
}

/* inserts a point into a polygon */
void *InsertPointIntoPolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_point_t save;
	pcb_cardinal_t n;
	pcb_line_t line;

	if (!ctx->insert.forcible) {
		/*
		 * first make sure adding the point is sensible
		 */
		line.Thickness = 0;
		line.Point1 = Polygon->Points[pcb_poly_contour_prev_point(Polygon, ctx->insert.idx)];
		line.Point2 = Polygon->Points[ctx->insert.idx];
		if (pcb_is_point_on_line((float) ctx->insert.x, (float) ctx->insert.y, 0.0, &line))
			return (NULL);
	}
	/*
	 * second, shift the points up to make room for the new point
	 */
	ErasePolygon(Polygon);
	pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	save = *pcb_poly_point_new(Polygon, ctx->insert.x, ctx->insert.y);
	for (n = Polygon->PointN - 1; n > ctx->insert.idx; n--)
		Polygon->Points[n] = Polygon->Points[n - 1];

	/* Shift up indices of any holes */
	for (n = 0; n < Polygon->HoleIndexN; n++)
		if (Polygon->HoleIndex[n] > ctx->insert.idx || (ctx->insert.last && Polygon->HoleIndex[n] == ctx->insert.idx))
			Polygon->HoleIndex[n]++;

	Polygon->Points[ctx->insert.idx] = save;
	pcb_board_set_changed_flag(pcb_true);
	pcb_undo_add_obj_to_insert_point(PCB_TYPE_POLYGON_POINT, Layer, Polygon, &Polygon->Points[ctx->insert.idx]);

	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (ctx->insert.forcible || !pcb_poly_remove_excess_points(Layer, Polygon)) {
		DrawPolygon(Layer, Polygon);
		pcb_draw();
	}
	return (&Polygon->Points[ctx->insert.idx]);
}

/* low level routine to move a polygon */
void pcb_poly_move(pcb_polygon_t *Polygon, pcb_coord_t DX, pcb_coord_t DY)
{
	PCB_POLY_POINT_LOOP(Polygon);
	{
		PCB_MOVE(point->X, point->Y, DX, DY);
	}
	END_LOOP;
	PCB_BOX_MOVE_LOWLEVEL(&Polygon->BoundingBox, DX, DY);
}

/* moves a polygon */
void *MovePolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	if (Layer->On) {
		ErasePolygon(Polygon);
	}
	pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	pcb_poly_move(Polygon, ctx->move.dx, ctx->move.dy);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		pcb_draw();
	}
	return (Polygon);
}

/* moves a polygon-point */
void *MovePolygonPoint(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon, pcb_point_t *Point)
{
	if (Layer->On) {
		ErasePolygon(Polygon);
	}
	pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	PCB_MOVE(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);
	pcb_poly_remove_excess_points(Layer, Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		pcb_draw();
	}
	return (Point);
}

/* moves a polygon between layers; lowlevel routines */
void *MovePolygonToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_polygon_t * polygon, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->polygon_tree, (pcb_box_t *) polygon);

	polylist_remove(polygon);
	polylist_append(&Destination->Polygon, polygon);

	if (!Destination->polygon_tree)
		Destination->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Destination->polygon_tree, (pcb_box_t *) polygon, 0);

	return polygon;
}

struct mptlc {
	pcb_cardinal_t snum, dnum;
	int type;
	pcb_polygon_t *polygon;
} mptlc;

pcb_r_dir_t mptl_pin_callback(const pcb_box_t * b, void *cl)
{
	struct mptlc *d = (struct mptlc *) cl;
	pcb_pin_t *pin = (pcb_pin_t *) b;
	if (!PCB_FLAG_THERM_TEST(d->snum, pin) || !pcb_poly_is_point_in_p(pin->X, pin->Y, pin->Thickness + pin->Clearance + 2, d->polygon))
		return R_DIR_NOT_FOUND;
	if (d->type == PCB_TYPE_PIN)
		pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, pin->Element, pin, pin);
	else
		pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, pin, pin, pin);
	PCB_FLAG_THERM_ASSIGN(d->dnum, PCB_FLAG_THERM_GET(d->snum, pin), pin);
	PCB_FLAG_THERM_CLEAR(d->snum, pin);
	return R_DIR_FOUND_CONTINUE;
}

/* moves a polygon between layers */
void *MovePolygonToLayer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_polygon_t * Polygon)
{
	pcb_polygon_t *newone;
	struct mptlc d;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Polygon)) {
		pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (((long int) ctx->move.dst_layer == -1) || (Layer == ctx->move.dst_layer))
		return (Polygon);
	pcb_undo_add_obj_to_move_to_layer(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	if (Layer->On)
		ErasePolygon(Polygon);
	/* Move all of the thermals with the polygon */
	d.snum = GetLayerNumber(PCB->Data, Layer);
	d.dnum = GetLayerNumber(PCB->Data, ctx->move.dst_layer);
	d.polygon = Polygon;
	d.type = PCB_TYPE_PIN;
	pcb_r_search(PCB->Data->pin_tree, &Polygon->BoundingBox, NULL, mptl_pin_callback, &d, NULL);
	d.type = PCB_TYPE_VIA;
	pcb_r_search(PCB->Data->via_tree, &Polygon->BoundingBox, NULL, mptl_pin_callback, &d, NULL);
	newone = (struct pcb_polygon_s *) MovePolygonToLayerLowLevel(ctx, Layer, Polygon, ctx->move.dst_layer);
	pcb_poly_init_clip(PCB->Data, ctx->move.dst_layer, newone);
	if (ctx->move.dst_layer->On) {
		DrawPolygon(ctx->move.dst_layer, newone);
		pcb_draw();
	}
	return (newone);
}


/* destroys a polygon from a layer */
void *DestroyPolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	pcb_poly_free_fields(Polygon);

	pcb_poly_free(Polygon);

	return NULL;
}

/* removes a polygon-point from a polygon and destroys the data */
void *DestroyPolygonPoint(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon, pcb_point_t *Point)
{
	pcb_cardinal_t point_idx;
	pcb_cardinal_t i;
	pcb_cardinal_t contour;
	pcb_cardinal_t contour_start, contour_end, contour_points;

	point_idx = pcb_poly_point_idx(Polygon, Point);
	contour = pcb_poly_contour_point(Polygon, point_idx);
	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	if (contour_points <= 3)
		return RemovePolygonContour(ctx, Layer, Polygon, contour);

	pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);

	/* remove point from list, keep point order */
	for (i = point_idx; i < Polygon->PointN - 1; i++)
		Polygon->Points[i] = Polygon->Points[i + 1];
	Polygon->PointN--;

	/* Shift down indices of any holes */
	for (i = 0; i < Polygon->HoleIndexN; i++)
		if (Polygon->HoleIndex[i] > point_idx)
			Polygon->HoleIndex[i]--;

	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);
	return (Polygon);
}

/* removes a polygon from a layer */
void *RemovePolygon_op(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	/* erase from screen */
	if (Layer->On) {
		ErasePolygon(Polygon);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	pcb_undo_move_obj_to_remove(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	return NULL;
}

void *pcb_poly_remove(pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemovePolygon_op(&ctx, Layer, Polygon);
}

/* Removes a contour from a polygon.
   If removing the outer contour, it removes the whole polygon. */
void *RemovePolygonContour(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon, pcb_cardinal_t contour)
{
	pcb_cardinal_t contour_start, contour_end, contour_points;
	pcb_cardinal_t i;

	if (contour == 0)
		return pcb_poly_remove(Layer, Polygon);

	if (Layer->On) {
		ErasePolygon(Polygon);
		if (!ctx->remove.bulk)
			pcb_draw();
	}

	/* Copy the polygon to the undo list */
	pcb_undo_add_obj_to_remove_contour(PCB_TYPE_POLYGON, Layer, Polygon);

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
	/* redraw polygon if necessary */
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	return NULL;
}

/* removes a polygon-point from a polygon */
void *RemovePolygonPoint(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon, pcb_point_t *Point)
{
	pcb_cardinal_t point_idx;
	pcb_cardinal_t i;
	pcb_cardinal_t contour;
	pcb_cardinal_t contour_start, contour_end, contour_points;

	point_idx = pcb_poly_point_idx(Polygon, Point);
	contour = pcb_poly_contour_point(Polygon, point_idx);
	contour_start = (contour == 0) ? 0 : Polygon->HoleIndex[contour - 1];
	contour_end = (contour == Polygon->HoleIndexN) ? Polygon->PointN : Polygon->HoleIndex[contour];
	contour_points = contour_end - contour_start;

	if (contour_points <= 3)
		return RemovePolygonContour(ctx, Layer, Polygon, contour);

	if (Layer->On)
		ErasePolygon(Polygon);

	/* insert the polygon-point into the undo list */
	pcb_undo_add_obj_to_remove_point(PCB_TYPE_POLYGON_POINT, Layer, Polygon, point_idx);
	pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);

	/* remove point from list, keep point order */
	for (i = point_idx; i < Polygon->PointN - 1; i++)
		Polygon->Points[i] = Polygon->Points[i + 1];
	Polygon->PointN--;

	/* Shift down indices of any holes */
	for (i = 0; i < Polygon->HoleIndexN; i++)
		if (Polygon->HoleIndex[i] > point_idx)
			Polygon->HoleIndex[i]--;

	pcb_poly_bbox(Polygon);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);
	pcb_poly_remove_excess_points(Layer, Polygon);
	pcb_poly_init_clip(PCB->Data, Layer, Polygon);

	/* redraw polygon if necessary */
	if (Layer->On) {
		DrawPolygon(Layer, Polygon);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	return NULL;
}

/* copies a polygon */
void *CopyPolygon(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_polygon_t *polygon;

	polygon = pcb_poly_new(Layer, pcb_no_flags());
	pcb_poly_copy(polygon, Polygon);
	pcb_poly_move(polygon, ctx->copy.DeltaX, ctx->copy.DeltaY);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) polygon, 0);
	pcb_poly_init_clip(PCB->Data, Layer, polygon);
	DrawPolygon(Layer, polygon);
	pcb_undo_add_obj_to_create(PCB_TYPE_POLYGON, Layer, polygon, polygon);
	return (polygon);
}

/*** draw ***/
pcb_r_dir_t draw_poly_callback(const pcb_box_t * b, void *cl)
{
	struct pcb_draw_poly_info_s *i = cl;
	pcb_polygon_t *polygon = (pcb_polygon_t *) b;
	static const char *color;
	char buf[sizeof("#XXXXXX")];

	if (!polygon->Clipped)
		return R_DIR_NOT_FOUND;

	if (PCB_FLAG_TEST(PCB_FLAG_WARN, polygon))
		color = PCB->WarnColor;
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
		color = i->layer->SelectedColor;
	else if (PCB_FLAG_TEST(PCB_FLAG_FOUND, polygon))
		color = PCB->ConnectedColor;
	else if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, polygon)) {
		assert(color != NULL);
		pcb_lighten_color(color, buf, 1.75);
		color = buf;
	}
	else
		color = i->layer->Color;
	pcb_gui->set_color(Output.fgGC, color);

	if ((pcb_gui->thindraw_pcb_polygon != NULL) && (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly))
		pcb_gui->thindraw_pcb_polygon(Output.fgGC, polygon, i->drawn_area);
	else
		pcb_gui->fill_pcb_polygon(Output.fgGC, polygon, i->drawn_area);

	/* If checking planes, thin-draw any pieces which have been clipped away */
	if (pcb_gui->thindraw_pcb_polygon != NULL && conf_core.editor.check_planes && !PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, polygon)) {
		pcb_polygon_t poly = *polygon;

		for (poly.Clipped = polygon->Clipped->f; poly.Clipped != polygon->Clipped; poly.Clipped = poly.Clipped->f)
			pcb_gui->thindraw_pcb_polygon(Output.fgGC, &poly, i->drawn_area);
	}

	return R_DIR_FOUND_CONTINUE;
}

/* erases a polygon on a layer */
void ErasePolygon(pcb_polygon_t *Polygon)
{
	pcb_draw_invalidate(Polygon);
	pcb_flag_erase(&Polygon->Flags);
}

void DrawPolygon(pcb_layer_t *Layer, pcb_polygon_t *Polygon)
{
	pcb_draw_invalidate(Polygon);
}
