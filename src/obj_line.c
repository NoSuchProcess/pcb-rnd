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

/* Drawing primitive: line segment */

#include "config.h"
#include <setjmp.h>

#include "undo.h"
#include "board.h"
#include "data.h"
#include "search.h"
#include "polygon.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "rotate.h"

#include "obj_line.h"
#include "obj_line_op.h"

#include "obj_subc_parent.h"

/* TODO: maybe remove this and move lines from draw here? */
#include "draw.h"
#include "obj_line_draw.h"
#include "obj_rat_draw.h"
#include "obj_pinvia_draw.h"


/**** allocation ****/

/* get next slot for a line, allocates memory if necessary */
pcb_line_t *pcb_line_alloc(pcb_layer_t * layer)
{
	pcb_line_t *new_obj;

	new_obj = calloc(sizeof(pcb_line_t), 1);
	new_obj->type = PCB_OBJ_LINE;
	PCB_SET_PARENT(new_obj, layer, layer);

	linelist_append(&layer->Line, new_obj);

	return new_obj;
}

void pcb_line_free(pcb_line_t * data)
{
	linelist_remove(data);
	free(data);
}


/**** utility ****/
struct line_info {
	pcb_coord_t X1, X2, Y1, Y2;
	pcb_coord_t Thickness;
	pcb_flag_t Flags;
	pcb_line_t test, *ans;
	jmp_buf env;
};

static pcb_r_dir_t line_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct line_info *i = (struct line_info *) cl;

	if (line->Point1.X == i->X1 && line->Point2.X == i->X2 && line->Point1.Y == i->Y1 && line->Point2.Y == i->Y2) {
		i->ans = (pcb_line_t *) (-1);
		longjmp(i->env, 1);
	}
	/* check the other point order */
	if (line->Point1.X == i->X1 && line->Point2.X == i->X2 && line->Point1.Y == i->Y1 && line->Point2.Y == i->Y2) {
		i->ans = (pcb_line_t *) (-1);
		longjmp(i->env, 1);
	}
	if (line->Point2.X == i->X1 && line->Point1.X == i->X2 && line->Point2.Y == i->Y1 && line->Point1.Y == i->Y2) {
		i->ans = (pcb_line_t *) - 1;
		longjmp(i->env, 1);
	}
	/* remove unnecessary line points */
	if (line->Thickness == i->Thickness
			/* don't merge lines if the clear flags differ  */
			&& PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line) == PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, i)) {
		if (line->Point1.X == i->X1 && line->Point1.Y == i->Y1) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (pcb_is_point_on_line(i->X1, i->Y1, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X1 && line->Point2.Y == i->Y1) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (pcb_is_point_on_line(i->X1, i->Y1, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point1.X == i->X2 && line->Point1.Y == i->Y2) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (pcb_is_point_on_line(i->X2, i->Y2, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X2 && line->Point2.Y == i->Y2) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (pcb_is_point_on_line(i->X2, i->Y2, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
	}
	return PCB_R_DIR_NOT_FOUND;
}


/* creates a new line on a layer and checks for overlap and extension */
pcb_line_t *pcb_line_new_merge(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags)
{
	struct line_info info;
	pcb_box_t search;

	search.X1 = MIN(X1, X2);
	search.X2 = MAX(X1, X2);
	search.Y1 = MIN(Y1, Y2);
	search.Y2 = MAX(Y1, Y2);
	if (search.Y2 == search.Y1)
		search.Y2++;
	if (search.X2 == search.X1)
		search.X2++;
	info.X1 = X1;
	info.X2 = X2;
	info.Y1 = Y1;
	info.Y2 = Y2;
	info.Thickness = Thickness;
	info.Flags = Flags;
	info.test.Thickness = 0;
	info.test.Flags = pcb_no_flags();
	info.ans = NULL;
	/* prevent stacking of duplicate lines
	 * and remove needless intermediate points
	 * verify that the layer is on the board first!
	 */
	if (setjmp(info.env) == 0) {
		pcb_r_search(Layer->line_tree, &search, NULL, line_callback, &info, NULL);
		return pcb_line_new(Layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags);
	}

	if ((void *) info.ans == (void *) (-1))
		return NULL;								/* stacked line */
	/* remove unnecessary points */
	if (info.ans) {
		/* must do this BEFORE getting new line memory */
		pcb_undo_move_obj_to_remove(PCB_TYPE_LINE, Layer, info.ans, info.ans);
		X1 = info.test.Point1.X;
		X2 = info.test.Point2.X;
		Y1 = info.test.Point1.Y;
		Y2 = info.test.Point2.Y;
	}
	return pcb_line_new(Layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags);
}

pcb_line_t *pcb_line_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags)
{
	pcb_line_t *Line;

	Line = pcb_line_alloc(Layer);
	if (!Line)
		return (Line);
	Line->ID = pcb_create_ID_get();
	Line->Flags = Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_RAT, Line);
	Line->Thickness = Thickness;
	Line->Clearance = Clearance;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
	Line->Point1.ID = pcb_create_ID_get();
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = pcb_create_ID_get();
	pcb_add_line_on_layer(Layer, Line);
	return (Line);
}

static pcb_line_t *pcb_line_copy_meta(pcb_line_t *dst, pcb_line_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes, 0);
	if (src->Number != NULL)
		dst->Number = pcb_strdup(src->Number);
	return dst;
}

pcb_line_t *pcb_line_dup(pcb_layer_t *dst, pcb_line_t *src)
{
	pcb_line_t *l = pcb_line_new(dst, src->Point1.X, src->Point1.Y, src->Point2.X, src->Point2.Y, src->Thickness, src->Clearance, src->Flags);
	return pcb_line_copy_meta(l, src);
}

pcb_line_t *pcb_line_dup_at(pcb_layer_t *dst, pcb_line_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_line_t *l = pcb_line_new(dst, src->Point1.X + dx, src->Point1.Y + dy, src->Point2.X + dx, src->Point2.Y + dy, src->Thickness, src->Clearance, src->Flags);
	return pcb_line_copy_meta(l, src);
}

void pcb_add_line_on_layer(pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_bbox(Line);
	if (!Layer->line_tree)
		Layer->line_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
}

/* sets the bounding box of a line */
void pcb_line_bbox(pcb_line_t *Line)
{
	pcb_coord_t width = (Line->Thickness + Line->Clearance + 1) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *PCB_POLY_CIRC_RADIUS_ADJ + 0.5;

	Line->BoundingBox.X1 = MIN(Line->Point1.X, Line->Point2.X) - width;
	Line->BoundingBox.X2 = MAX(Line->Point1.X, Line->Point2.X) + width;
	Line->BoundingBox.Y1 = MIN(Line->Point1.Y, Line->Point2.Y) - width;
	Line->BoundingBox.Y2 = MAX(Line->Point1.Y, Line->Point2.Y) + width;
	pcb_close_box(&Line->BoundingBox);
	pcb_set_point_bounding_box(&Line->Point1);
	pcb_set_point_bounding_box(&Line->Point2);
}

int pcb_line_eq(const pcb_element_t *e1, const pcb_line_t *l1, const pcb_element_t *e2, const pcb_line_t *l2)
{
	if (pcb_field_neq(l1, l2, Thickness) || pcb_field_neq(l1, l2, Clearance)) return 0;
	if (pcb_element_neq_offsx(e1, l1, e2, l2, Point1.X) || pcb_element_neq_offsy(e1, l1, e2, l2, Point1.Y)) return 0;
	if (pcb_element_neq_offsx(e1, l1, e2, l2, Point2.X) || pcb_element_neq_offsy(e1, l1, e2, l2, Point2.Y)) return 0;
	if (pcb_neqs(l1->Number, l2->Number)) return 0;
	return 1;
}


unsigned int pcb_line_hash(const pcb_element_t *e, const pcb_line_t *l)
{
	return
		pcb_hash_coord(l->Thickness) ^ pcb_hash_coord(l->Clearance) ^
		pcb_hash_element_ox(e, l->Point1.X) ^ pcb_hash_element_oy(e, l->Point1.Y) ^
		pcb_hash_element_ox(e, l->Point2.X) ^ pcb_hash_element_oy(e, l->Point2.Y) ^
		pcb_hash_str(l->Number);
}


pcb_coord_t pcb_line_length(const pcb_line_t *line)
{
	pcb_coord_t dx = line->Point1.X - line->Point2.X;
	pcb_coord_t dy = line->Point1.Y - line->Point2.Y;
	return pcb_round(sqrt((double)dx*(double)dx + (double)dy*(double)dy));
}

double pcb_line_area(const pcb_line_t *line)
{
	return
		pcb_line_length(line) * (double)line->Thickness /* body */
		+ (double)line->Thickness * (double)line->Thickness * M_PI; /* cap circles */
}


/*** ops ***/
/* copies a line to buffer */
void *pcb_lineop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_t *line;
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, Layer)];
	line = pcb_line_new(layer, Line->Point1.X, Line->Point1.Y,
															Line->Point2.X, Line->Point2.Y,
															Line->Thickness, Line->Clearance, pcb_flag_mask(Line->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));
	return pcb_line_copy_meta(line, Line);
}

/* moves a line to buffer */
void *pcb_lineop_move_to_buffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_line_t * line)
{
	pcb_layer_t *lay = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, layer)];

	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_LINE, layer, line);
	pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);

	linelist_remove(line);
	linelist_append(&(lay->Line), line);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, line);

	if (!lay->line_tree)
		lay->line_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(lay->line_tree, (pcb_box_t *) line, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_LINE, lay, line);

	PCB_SET_PARENT(line, layer, lay);

	return (line);
}

/* changes the size of a line */
void *pcb_lineop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Line->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line))
		return (NULL);
	if (value <= PCB_MAX_LINESIZE && value >= PCB_MIN_LINESIZE && value != Line->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_LINE, Layer, Line, Line);
		pcb_line_invalidate_erase(Line);
		pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		Line->Thickness = value;
		pcb_line_bbox(Line);
		pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		pcb_line_invalidate_draw(Layer, Line);
		return (Line);
	}
	return (NULL);
}

/*changes the clearance size of a line */
void *pcb_lineop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Line->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Line->Clearance) {
		pcb_undo_add_obj_to_clear_size(PCB_TYPE_LINE, Layer, Line, Line);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		pcb_line_invalidate_erase(Line);
		pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
		Line->Clearance = value;
		if (Line->Clearance == 0) {
			PCB_FLAG_CLEAR(PCB_FLAG_CLEARLINE, Line);
			Line->Clearance = PCB_MIL_TO_COORD(10);
		}
		pcb_line_bbox(Line);
		pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		pcb_line_invalidate_draw(Layer, Line);
		return (Line);
	}
	return (NULL);
}

/* changes the name of a line */
void *pcb_lineop_change_name(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	char *old = Line->Number;

	Layer = Layer;
	Line->Number = ctx->chgname.new_name;
	return (old);
}

/* changes the clearance flag of a line */
void *pcb_lineop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line))
		return (NULL);
	pcb_line_invalidate_erase(Line);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line)) {
		pcb_undo_add_obj_to_clear_poly(PCB_TYPE_LINE, Layer, Line, Line, pcb_false);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	}
	pcb_undo_add_obj_to_flag(PCB_TYPE_LINE, Layer, Line, Line);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Line);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line)) {
		pcb_undo_add_obj_to_clear_poly(PCB_TYPE_LINE, Layer, Line, Line, pcb_true);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	}
	pcb_line_invalidate_draw(Layer, Line);
	return (Line);
}

/* sets the clearance flag of a line */
void *pcb_lineop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line) || PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	return pcb_lineop_change_join(ctx, Layer, Line);
}

/* clears the clearance flag of a line */
void *pcb_lineop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	return pcb_lineop_change_join(ctx, Layer, Line);
}

/* copies a line */
void *pcb_lineop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_t *line;

	line = pcb_line_new_merge(Layer, Line->Point1.X + ctx->copy.DeltaX,
																Line->Point1.Y + ctx->copy.DeltaY,
																Line->Point2.X + ctx->copy.DeltaX,
																Line->Point2.Y + ctx->copy.DeltaY, Line->Thickness, Line->Clearance, pcb_flag_mask(Line->Flags, PCB_FLAG_FOUND));
	if (!line)
		return (line);
	pcb_line_copy_meta(line, Line);
	pcb_line_invalidate_draw(Layer, line);
	pcb_undo_add_obj_to_create(PCB_TYPE_LINE, Layer, line, line);
	return (line);
}

/* moves a line */
void *pcb_lineop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (Layer->meta.real.vis)
		pcb_line_invalidate_erase(Line);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	pcb_line_move(Line, ctx->move.dx, ctx->move.dy);
	pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	if (Layer->meta.real.vis) {
		pcb_line_invalidate_draw(Layer, Line);
		pcb_draw();
	}
	return (Line);
}

/* moves one end of a line */
void *pcb_lineop_move_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	if (Layer) {
		if (Layer->meta.real.vis)
			pcb_line_invalidate_erase(Line);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		pcb_r_delete_entry(Layer->line_tree, &Line->BoundingBox);
		PCB_MOVE(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
		pcb_line_bbox(Line);
		pcb_r_insert_entry(Layer->line_tree, &Line->BoundingBox, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		if (Layer->meta.real.vis) {
			pcb_line_invalidate_draw(Layer, Line);
			pcb_draw();
		}
		return (Line);
	}
	else {												/* must be a rat */

		if (PCB->RatOn)
			pcb_rat_invalidate_erase((pcb_rat_t *) Line);
		pcb_r_delete_entry(PCB->Data->rat_tree, &Line->BoundingBox);
		PCB_MOVE(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
		pcb_line_bbox(Line);
		pcb_r_insert_entry(PCB->Data->rat_tree, &Line->BoundingBox, 0);
		if (PCB->RatOn) {
			pcb_rat_invalidate_draw((pcb_rat_t *) Line);
			pcb_draw();
		}
		return (Line);
	}
}

/* moves one end of a line */
void *pcb_lineop_move_point_with_route(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	if ((conf_core.editor.move_linepoint_uses_route == 0) || !Layer) {
		pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT, Layer, Line, Point, ctx->move.dx, ctx->move.dy);
		return pcb_lineop_move_point(ctx, Layer, Line, Point);
	}
	else {
		/* Move with Route Code */
		pcb_route_t route;
		pcb_point_t point1 = (&Line->Point1 == Point ? Line->Point2 : Line->Point1);
		pcb_point_t point2 = *Point;

		point2.X += ctx->move.dx;
		point2.Y += ctx->move.dy;
	
		/* Calculate the new line route and add apply it */
		pcb_route_init(&route);
		pcb_route_calculate(PCB,
												&route,
												&point1,
												&point2,
												pcb_layer_id(PCB->Data,Layer),
												Line->Thickness,
												Line->Clearance,
												Line->Flags,
												pcb_gui->shift_is_pressed(),
												pcb_gui->control_is_pressed() );
		pcb_route_apply_to_line(&route,Layer,Line);
		pcb_route_destroy(&route);

		pcb_draw();
	}
	return NULL;
}

/* moves a line between layers; lowlevel routines */
void *pcb_lineop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_line_t * line, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->line_tree, (pcb_box_t *) line);

	linelist_remove(line);
	linelist_append(&(Destination->Line), line);

	if (!Destination->line_tree)
		Destination->line_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Destination->line_tree, (pcb_box_t *) line, 0);

	PCB_SET_PARENT(line, layer, Destination);

	return line;
}

/* ---------------------------------------------------------------------------
 * moves a line between layers
 */
struct via_info {
	pcb_coord_t X, Y;
	jmp_buf env;
};

static pcb_r_dir_t moveline_callback(const pcb_box_t * b, void *cl)
{
	struct via_info *i = (struct via_info *) cl;
	pcb_pin_t *via;

	if ((via =
			 pcb_via_new(PCB->Data, i->X, i->Y,
										conf_core.design.via_thickness, 2 * conf_core.design.clearance, PCB_FLAG_NO, conf_core.design.via_drilling_hole, NULL, pcb_no_flags())) != NULL) {
		pcb_undo_add_obj_to_create(PCB_TYPE_VIA, via, via, via);
		pcb_via_invalidate_draw(via);
	}
	longjmp(i->env, 1);
}

void *pcb_lineop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * Layer, pcb_line_t * Line)
{
	struct via_info info;
	pcb_box_t sb;
	pcb_line_t *newone;
	void *ptr1, *ptr2, *ptr3;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line)) {
		pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->meta.real.vis) {
		pcb_line_invalidate_draw(Layer, Line);
		pcb_draw();
	}
	if (((long int) ctx->move.dst_layer == -1) || ctx->move.dst_layer == Layer)
		return (Line);

	pcb_undo_add_obj_to_move_to_layer(PCB_TYPE_LINE, Layer, Line, Line);
	if (Layer->meta.real.vis)
		pcb_line_invalidate_erase(Line);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	newone = (pcb_line_t *) pcb_lineop_move_to_layer_low(ctx, Layer, Line, ctx->move.dst_layer);
	Line = NULL;
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, ctx->move.dst_layer, newone);
	if (ctx->move.dst_layer->meta.real.vis)
		pcb_line_invalidate_draw(ctx->move.dst_layer, newone);
	pcb_draw();
	if (!PCB->ViaOn || ctx->move.more_to_come ||
			pcb_layer_get_group_(Layer) ==
			pcb_layer_get_group_(ctx->move.dst_layer) || !(pcb_layer_flags(PCB, pcb_layer_id(PCB->Data, Layer)) & PCB_LYT_COPPER) || !(pcb_layer_flags_(PCB, ctx->move.dst_layer) & PCB_LYT_COPPER))
		return (newone);
	/* consider via at Point1 */
	sb.X1 = newone->Point1.X - newone->Thickness / 2;
	sb.X2 = newone->Point1.X + newone->Thickness / 2;
	sb.Y1 = newone->Point1.Y - newone->Thickness / 2;
	sb.Y2 = newone->Point1.Y + newone->Thickness / 2;
	if ((pcb_search_obj_by_location(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
															newone->Point1.X, newone->Point1.Y, conf_core.design.via_thickness / 2) == PCB_TYPE_NONE)) {
		info.X = newone->Point1.X;
		info.Y = newone->Point1.Y;
		if (setjmp(info.env) == 0)
			pcb_r_search(Layer->line_tree, &sb, NULL, moveline_callback, &info, NULL);
	}
	/* consider via at Point2 */
	sb.X1 = newone->Point2.X - newone->Thickness / 2;
	sb.X2 = newone->Point2.X + newone->Thickness / 2;
	sb.Y1 = newone->Point2.Y - newone->Thickness / 2;
	sb.Y2 = newone->Point2.Y + newone->Thickness / 2;
	if ((pcb_search_obj_by_location(PCB_TYPEMASK_PIN, &ptr1, &ptr2, &ptr3,
															newone->Point2.X, newone->Point2.Y, conf_core.design.via_thickness / 2) == PCB_TYPE_NONE)) {
		info.X = newone->Point2.X;
		info.Y = newone->Point2.Y;
		if (setjmp(info.env) == 0)
			pcb_r_search(Layer->line_tree, &sb, NULL, moveline_callback, &info, NULL);
	}
	pcb_draw();
	return (newone);
}

/* destroys a line from a layer */
void *pcb_lineop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	free(Line->Number);

	pcb_line_free(Line);
	return NULL;
}

/* remove point... */
struct rlp_info {
	jmp_buf env;
	pcb_line_t *line;
	pcb_point_t *point;
};
static pcb_r_dir_t remove_point(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct rlp_info *info = (struct rlp_info *) cl;
	if (line == info->line)
		return PCB_R_DIR_NOT_FOUND;
	if ((line->Point1.X == info->point->X)
			&& (line->Point1.Y == info->point->Y)) {
		info->line = line;
		info->point = &line->Point1;
		longjmp(info->env, 1);
	}
	else if ((line->Point2.X == info->point->X)
					 && (line->Point2.Y == info->point->Y)) {
		info->line = line;
		info->point = &line->Point2;
		longjmp(info->env, 1);
	}
	return PCB_R_DIR_NOT_FOUND;
}

/* removes a line point, or a line if the selected point is the end */
void *pcb_lineop_remove_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	pcb_point_t other;
	struct rlp_info info;
	if (&Line->Point1 == Point)
		other = Line->Point2;
	else
		other = Line->Point1;
	info.line = Line;
	info.point = Point;
	if (setjmp(info.env) == 0) {
		pcb_r_search(Layer->line_tree, (const pcb_box_t *) Point, NULL, remove_point, &info, NULL);
		return pcb_lineop_remove(ctx, Layer, Line);
	}
	pcb_move_obj(PCB_TYPE_LINE_POINT, Layer, info.line, info.point, other.X - Point->X, other.Y - Point->Y);
	return (pcb_lineop_remove(ctx, Layer, Line));
}

/* removes a line from a layer */
void *pcb_lineop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	/* erase from screen */
	if (Layer->meta.real.vis) {
		pcb_line_invalidate_erase(Line);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	pcb_undo_move_obj_to_remove(PCB_TYPE_LINE, Layer, Line, Line);
	PCB_CLEAR_PARENT(Line);
	return NULL;
}

void *pcb_line_destroy(pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return pcb_lineop_remove(&ctx, Layer, Line);
}

/* rotates a line in 90 degree steps */
void pcb_line_rotate90(pcb_line_t *Line, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	PCB_COORD_ROTATE90(Line->Point1.X, Line->Point1.Y, X, Y, Number);
	PCB_COORD_ROTATE90(Line->Point2.X, Line->Point2.Y, X, Y, Number);
	/* keep horizontal, vertical Point2 > Point1 */
	if (Line->Point1.X == Line->Point2.X) {
		if (Line->Point1.Y > Line->Point2.Y) {
			pcb_coord_t t;
			t = Line->Point1.Y;
			Line->Point1.Y = Line->Point2.Y;
			Line->Point2.Y = t;
		}
	}
	else if (Line->Point1.Y == Line->Point2.Y) {
		if (Line->Point1.X > Line->Point2.X) {
			pcb_coord_t t;
			t = Line->Point1.X;
			Line->Point1.X = Line->Point2.X;
			Line->Point2.X = t;
		}
	}
	/* instead of rotating the bounding box, the call updates both end points too */
	pcb_line_bbox(Line);
}

void pcb_line_rotate(pcb_layer_t *layer, pcb_line_t *line, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina)
{
	pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);
	pcb_rotate(&line->Point1.X, &line->Point1.Y, X, Y, cosa, sina);
	pcb_rotate(&line->Point2.X, &line->Point2.Y, X, Y, cosa, sina);
	pcb_line_bbox(line);
	pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line, 0);
}

void pcb_line_mirror(pcb_layer_t *layer, pcb_line_t *line, pcb_coord_t y_offs)
{
	if (layer->line_tree != NULL)
		pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);
	line->Point1.X = PCB_SWAP_X(line->Point1.X);
	line->Point1.Y = PCB_SWAP_Y(line->Point1.Y) + y_offs;
	line->Point2.X = PCB_SWAP_X(line->Point2.X);
	line->Point2.Y = PCB_SWAP_Y(line->Point2.Y) + y_offs;
	pcb_line_bbox(line);
	if (layer->line_tree != NULL)
		pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line, 0);
}

void pcb_line_flip_side(pcb_layer_t *layer, pcb_line_t *line)
{
	pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);
	line->Point1.X = PCB_SWAP_X(line->Point1.X);
	line->Point1.Y = PCB_SWAP_Y(line->Point1.Y);
	line->Point2.X = PCB_SWAP_X(line->Point2.X);
	line->Point2.Y = PCB_SWAP_Y(line->Point2.Y);
	pcb_line_bbox(line);
	pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line, 0);
}

static void rotate_line1(pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_invalidate_erase(Line);
	if (Layer) {
		if (PCB_LAYER_IS_REAL(Layer))
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		if (Layer->line_tree != NULL)
			pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	}
	else
		pcb_r_delete_entry(PCB->Data->rat_tree, (pcb_box_t *) Line);
}

static void rotate_line2(pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_bbox(Line);
	if (Layer) {
		if (Layer->line_tree != NULL)
			pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
		if (PCB_LAYER_IS_REAL(Layer))
			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		pcb_line_invalidate_draw(Layer, Line);
	}
	else {
		pcb_r_insert_entry(PCB->Data->rat_tree, (pcb_box_t *) Line, 0);
		pcb_rat_invalidate_draw((pcb_rat_t *) Line);
	}
	pcb_draw();
}

/* rotates a line's point */
void *pcb_lineop_rotate90_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	pcb_point_t *center;

	rotate_line1(Layer, Line);

	if ((Point->X == Line->Point1.X) && (Point->Y == Line->Point1.Y))
		center = &Line->Point2;
	else
		center = &Line->Point1;

	pcb_point_rotate90(Point, center->X, center->Y, ctx->rotate.number);

	rotate_line2(Layer, Line);
	return Line;
}

/* rotates a line */
void *pcb_lineop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	rotate_line1(Layer, Line);
	pcb_point_rotate90(&Line->Point1, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	pcb_point_rotate90(&Line->Point2, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	rotate_line2(Layer, Line);
	return Line;
}

/* inserts a point into a line */
void *pcb_lineop_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_t *line;
	pcb_coord_t X, Y;

	if (((Line->Point1.X == ctx->insert.x) && (Line->Point1.Y == ctx->insert.y)) ||
			((Line->Point2.X == ctx->insert.x) && (Line->Point2.Y == ctx->insert.y)))
		return (NULL);
	X = Line->Point2.X;
	Y = Line->Point2.Y;
	pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT, Layer, Line, &Line->Point2, ctx->insert.x - X, ctx->insert.y - Y);
	pcb_line_invalidate_erase(Line);
	pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	Line->Point2.X = ctx->insert.x;
	Line->Point2.Y = ctx->insert.y;
	pcb_line_bbox(Line);
	pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	pcb_line_invalidate_draw(Layer, Line);
	/* we must create after playing with Line since creation may
	 * invalidate the line pointer
	 */
	if ((line = pcb_line_new_merge(Layer, ctx->insert.x, ctx->insert.y, X, Y, Line->Thickness, Line->Clearance, Line->Flags))) {
		pcb_line_copy_meta(line, Line);
		pcb_undo_add_obj_to_create(PCB_TYPE_LINE, Layer, line, line);
		pcb_line_invalidate_draw(Layer, line);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_LINE, Layer, line);
		/* creation call adds it to the rtree */
	}
	pcb_draw();
	return (line);
}

#define PCB_LINE_FLAGS (PCB_FLAG_FOUND | PCB_FLAG_RAT | PCB_FLAG_CLEARLINE | PCB_FLAG_SELECTED | PCB_FLAG_AUTO | PCB_FLAG_RUBBEREND | PCB_FLAG_LOCK | PCB_FLAG_VISIT)
void *pcb_lineop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if ((ctx->chgflag.flag & PCB_LINE_FLAGS) != ctx->chgflag.flag)
		return NULL;
	pcb_undo_add_obj_to_flag(PCB_TYPE_LINE, Line, Line, Line);
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, Line);
	return Line;
}

void *pcb_lineop_invalidate_label(pcb_opctx_t *ctx, pcb_line_t *line)
{
	pcb_line_name_invalidate_draw(line);
	return line;
}


/*** draw ***/
void pcb_line_name_invalidate_draw(pcb_line_t *lineline)
{
#warning term TODO: need to get label sizes
}

void pcb_line_draw_name(pcb_line_t *line)
{
	if (line->term != NULL)
		pcb_term_label_draw((line->Point1.X + line->Point2.X)/2, (line->Point1.Y + line->Point2.Y)/2,
			100.0, 0, line->term);
}


void pcb_line_draw_(pcb_line_t * line)
{
	PCB_DRAW_BBOX(line);
	pcb_gui->set_line_cap(Output.fgGC, Trace_Cap);
	if (conf_core.editor.thin_draw)
		pcb_gui->set_line_width(Output.fgGC, 0);
	else
		pcb_gui->set_line_width(Output.fgGC, line->Thickness);

	pcb_gui->draw_line(Output.fgGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);

	if (pcb_draw_doing_pinout)
		pcb_line_draw_name(line);
}

void pcb_line_draw(pcb_layer_t * layer, pcb_line_t * line)
{
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if (PCB_FLAG_TEST(PCB_FLAG_WARN, line))
		color = conf_core.appearance.color.warn;
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, line)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
			color = layer->meta.real.selected_color;
		else
			color = conf_core.appearance.color.connected;
	}
	else
		color = layer->meta.real.color;

	if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, line)) {
		assert(color != NULL);
		pcb_lighten_color(color, buf, 1.75);
		color = buf;
	}

	pcb_gui->set_color(Output.fgGC, color);
	pcb_line_draw_(line);
}

pcb_r_dir_t pcb_line_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *)b;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(line->parent_type, &line->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_line_draw((pcb_layer_t *)cl, line);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* erases a line on a layer */
void pcb_line_invalidate_erase(pcb_line_t *Line)
{
	pcb_draw_invalidate(Line);
	pcb_flag_erase(&Line->Flags);
}

void pcb_line_invalidate_draw(pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_draw_invalidate(Line);
}

