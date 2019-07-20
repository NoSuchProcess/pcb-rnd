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

/* Drawing primitive: line segment */

#include "config.h"
#include <setjmp.h>

#include "undo.h"
#include "board.h"
#include "data.h"
#include "move.h"
#include "search.h"
#include "polygon.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "rotate.h"
#include "hid_inlines.h"

#include "obj_line.h"
#include "obj_line_op.h"

#include "obj_subc_parent.h"
#include "obj_hash.h"

/* TODO: maybe remove this and move lines from draw here? */
#include "draw.h"
#include "draw_wireframe.h"
#include "obj_line_draw.h"
#include "obj_rat_draw.h"
#include "obj_pstk_draw.h"

TODO("padstack: remove this when via is removed and the padstack is created from style directly")
#include "src_plugins/lib_compat_help/pstk_compat.h"


/**** allocation ****/

void pcb_line_reg(pcb_layer_t *layer, pcb_line_t *line)
{
	linelist_append(&layer->Line, line);
	PCB_SET_PARENT(line, layer, layer);

	if (layer->parent_type == PCB_PARENT_UI)
		return;

	assert(layer->parent_type == PCB_PARENT_DATA);
	pcb_obj_id_reg(layer->parent.data, line);
}

void pcb_line_unreg(pcb_line_t *line)
{
	pcb_layer_t *layer = line->parent.layer;
	assert(line->parent_type == PCB_PARENT_LAYER);
	linelist_remove(line);
	if (layer->parent_type != PCB_PARENT_UI) {
		assert(layer->parent_type == PCB_PARENT_DATA);
		pcb_obj_id_del(layer->parent.data, line);
	}
	PCB_CLEAR_PARENT(line);
}

pcb_line_t *pcb_line_alloc_id(pcb_layer_t *layer, long int id)
{
	pcb_line_t *new_obj;

	new_obj = calloc(sizeof(pcb_line_t), 1);
	new_obj->ID = id;
	new_obj->type = PCB_OBJ_LINE;
	new_obj->Attributes.post_change = pcb_obj_attrib_post_change;

	pcb_line_reg(layer, new_obj);

	return new_obj;
}

pcb_line_t *pcb_line_alloc(pcb_layer_t *layer)
{
	return pcb_line_alloc_id(layer, pcb_create_ID_get());
}

void pcb_line_free(pcb_line_t *line)
{
	pcb_line_unreg(line);
	free(line);
}


/**** utility ****/
struct line_info {
	pcb_coord_t X1, X2, Y1, Y2;
	pcb_coord_t Thickness, Clearance;
	pcb_flag_t Flags;
	pcb_line_t test, *ans;
	jmp_buf env;
};

static pcb_r_dir_t line_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *) b;
	struct line_info *i = (struct line_info *) cl;

	/* do not merge to subc parts or terminals */
	if ((pcb_obj_parent_subc((pcb_any_obj_t *)line) != NULL) || (line->term != NULL))
		return PCB_R_DIR_NOT_FOUND;

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
			/* don't merge lines if the clear flags or clearance differ  */
			&& PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, line) == PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, i)
			&& line->Clearance == i->Clearance) {
		if (line->Point1.X == i->X1 && line->Point1.Y == i->Y1) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (pcb_is_point_on_line(i->X1, i->Y1, 1, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X1 && line->Point2.Y == i->Y1) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (pcb_is_point_on_line(i->X1, i->Y1, 1, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point1.X == i->X2 && line->Point1.Y == i->Y2) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (pcb_is_point_on_line(i->X2, i->Y2, 1, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X2 && line->Point2.Y == i->Y2) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (pcb_is_point_on_line(i->X2, i->Y2, 1, &i->test)) {
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
	info.Clearance = Clearance;
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
		pcb_undo_move_obj_to_remove(PCB_OBJ_LINE, Layer, info.ans, info.ans);
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
		return Line;
	Line->Flags = Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_RAT, Line);
	Line->Thickness = Thickness;
	Line->Clearance = Clearance;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
TODO("ID: register points too")
	Line->Point1.ID = pcb_create_ID_get();
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = pcb_create_ID_get();
	pcb_add_line_on_layer(Layer, Line);
	return Line;
}

static pcb_line_t *pcb_line_copy_meta(pcb_line_t *dst, pcb_line_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
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
		Layer->line_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
}

static void pcb_line_bbox_(const pcb_line_t *Line, pcb_box_t *dst, int mini)
{
	pcb_coord_t width = mini ? (Line->Thickness + 1) / 2 : (Line->Thickness + Line->Clearance + 1) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *PCB_POLY_CIRC_RADIUS_ADJ + 0.5;

	dst->X1 = MIN(Line->Point1.X, Line->Point2.X) - width;
	dst->X2 = MAX(Line->Point1.X, Line->Point2.X) + width;
	dst->Y1 = MIN(Line->Point1.Y, Line->Point2.Y) - width;
	dst->Y2 = MAX(Line->Point1.Y, Line->Point2.Y) + width;
	pcb_close_box(dst);
}

/* sets the bounding box of a line */
void pcb_line_bbox(pcb_line_t *Line)
{
	pcb_line_bbox_(Line, &Line->BoundingBox, 0);
	pcb_line_bbox_(Line, &Line->bbox_naked, 1);
	pcb_set_point_bounding_box(&Line->Point1);
	pcb_set_point_bounding_box(&Line->Point2);
}

int pcb_line_eq(const pcb_host_trans_t *tr1, const pcb_line_t *l1, const pcb_host_trans_t *tr2, const pcb_line_t *l2)
{
	if (pcb_field_neq(l1, l2, Thickness) || pcb_field_neq(l1, l2, Clearance)) return 0;
	if (pcb_neqs(l1->term, l2->term)) return 0;
	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, l1) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, l2)) {
		if (pcb_neq_tr_coords(tr1, l1->Point1.X, l1->Point1.Y, tr2, l2->Point1.X, l2->Point1.Y)) goto swap;
		if (pcb_neq_tr_coords(tr1, l1->Point2.X, l1->Point2.Y, tr2, l2->Point2.X, l2->Point2.Y)) return 0;
	}
	return 1;

	swap:; /* check the line with swapped endpoints */
	if (pcb_neq_tr_coords(tr1, l1->Point2.X, l1->Point2.Y, tr2, l2->Point1.X, l2->Point1.Y)) return 0;
	if (pcb_neq_tr_coords(tr1, l1->Point1.X, l1->Point1.Y, tr2, l2->Point2.X, l2->Point2.Y)) return 0;
	return 1;
}


unsigned int pcb_line_hash(const pcb_host_trans_t *tr, const pcb_line_t *l)
{
	unsigned int crd = 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, l)) {
		pcb_coord_t x1, y1, x2, y2;
		pcb_hash_tr_coords(tr, &x1, &y1, l->Point1.X, l->Point1.Y);
		pcb_hash_tr_coords(tr, &x2, &y2, l->Point2.X, l->Point2.Y);
		crd = pcb_hash_coord(x1) ^ pcb_hash_coord(y1) ^ pcb_hash_coord(x2) ^ pcb_hash_coord(y2);
	}

	return pcb_hash_coord(l->Thickness) ^ pcb_hash_coord(l->Clearance) ^
		pcb_hash_str(l->term) ^ crd;
}


pcb_coord_t pcb_line_length(const pcb_line_t *line)
{
	pcb_coord_t dx = line->Point1.X - line->Point2.X;
	pcb_coord_t dy = line->Point1.Y - line->Point2.Y;
TODO(": use the distance func instead")
	return pcb_round(sqrt((double)dx*(double)dx + (double)dy*(double)dy));
}

double pcb_line_area(const pcb_line_t *line)
{
	return
		pcb_line_length(line) * (double)line->Thickness /* body */
		+ (double)line->Thickness * (double)line->Thickness * M_PI; /* cap circles */
}

void pcb_sqline_to_rect(const pcb_line_t *line, pcb_coord_t *x, pcb_coord_t *y)
{
	double l, vx, vy, nx, ny, width, x1, y1, x2, y2, dx, dy;

	x1 = line->Point1.X;
	y1 = line->Point1.Y;
	x2 = line->Point2.X;
	y2 = line->Point2.Y;

	width = (double)((line->Thickness + 1) / 2);
	dx = x2-x1;
	dy = y2-y1;

	if ((dx == 0) && (dy == 0))
		dx = 1;

	l = sqrt((double)dx*(double)dx + (double)dy*(double)dy);

	vx = dx / l;
	vy = dy / l;
	nx = -vy;
	ny = vx;

	x[0] = (pcb_coord_t)pcb_round(x1 - vx * width + nx * width);
	y[0] = (pcb_coord_t)pcb_round(y1 - vy * width + ny * width);
	x[1] = (pcb_coord_t)pcb_round(x1 - vx * width - nx * width);
	y[1] = (pcb_coord_t)pcb_round(y1 - vy * width - ny * width);
	x[2] = (pcb_coord_t)pcb_round(x2 + vx * width - nx * width);
	y[2] = (pcb_coord_t)pcb_round(y2 + vy * width - ny * width);
	x[3] = (pcb_coord_t)pcb_round(x2 + vx * width + nx * width);
	y[3] = (pcb_coord_t)pcb_round(y2 + vy * width + ny * width);
}

void pcb_line_pre(pcb_line_t *line)
{
	pcb_layer_t *ly = pcb_layer_get_real(line->parent.layer);
	if (ly == NULL)
		return;
	if (ly->line_tree != NULL)
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)line);
	pcb_poly_restore_to_poly(ly->parent.data, PCB_OBJ_LINE, ly, line);
}

void pcb_line_post(pcb_line_t *line)
{
	pcb_layer_t *ly = pcb_layer_get_real(line->parent.layer);
	if (ly == NULL)
		return;
	if (ly->line_tree != NULL)
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)line);
	pcb_poly_clear_from_poly(ly->parent.data, PCB_OBJ_LINE, ly, line);
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

/* moves a line between board and buffer */
void *pcb_lineop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_line_t *line)
{
	pcb_layer_t *srcly = line->parent.layer;

	assert(line->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) /* auto layer in dst */
		dstly = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, srcly)];

	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_OBJ_LINE, srcly, line);
	pcb_r_delete_entry(srcly->line_tree, (pcb_box_t *)line);

	pcb_line_unreg(line);
	pcb_line_reg(dstly, line);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, line);

	if (!dstly->line_tree)
		dstly->line_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dstly->line_tree, (pcb_box_t *)line);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_OBJ_LINE, dstly, line);

	return line;
}

/* changes the size of a line */
void *pcb_lineop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Line->Thickness + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line))
		return NULL;
	if (value <= PCB_MAX_LINESIZE && value >= PCB_MIN_LINESIZE && value != Line->Thickness) {
		pcb_undo_add_obj_to_size(PCB_OBJ_LINE, Layer, Line, Line);
		pcb_line_invalidate_erase(Line);
		pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
		pcb_poly_restore_to_poly(ctx->chgsize.pcb->Data, PCB_OBJ_LINE, Layer, Line);
		Line->Thickness = value;
		pcb_line_bbox(Line);
		pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
		pcb_poly_clear_from_poly(ctx->chgsize.pcb->Data, PCB_OBJ_LINE, Layer, Line);
		pcb_line_invalidate_draw(Layer, Line);
		return Line;
	}
	return NULL;
}

/*changes the clearance size of a line */
void *pcb_lineop_change_clear_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Line->Clearance + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line))
		return NULL;
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, value);
	if (!ctx->chgsize.is_absolute && ctx->chgsize.value < 0 && value < conf_core.design.bloat * 2)
		value = 0;
	if (ctx->chgsize.value > 0 && value < conf_core.design.bloat * 2)
		value = conf_core.design.bloat * 2 + 2;
	if (value != Line->Clearance) {
		pcb_undo_add_obj_to_clear_size(PCB_OBJ_LINE, Layer, Line, Line);
		pcb_poly_restore_to_poly(ctx->chgsize.pcb->Data, PCB_OBJ_LINE, Layer, Line);
		pcb_line_invalidate_erase(Line);
		pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
		Line->Clearance = value;
		pcb_line_bbox(Line);
		pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
		pcb_poly_clear_from_poly(ctx->chgsize.pcb->Data, PCB_OBJ_LINE, Layer, Line);
		pcb_line_invalidate_draw(Layer, Line);
		return Line;
	}
	return NULL;
}

/* changes the clearance flag of a line */
void *pcb_lineop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line))
		return NULL;
	pcb_line_invalidate_erase(Line);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_LINE, Layer, Line, Line, pcb_false);
		pcb_poly_restore_to_poly(ctx->chgsize.pcb->Data, PCB_OBJ_LINE, Layer, Line);
	}
	pcb_undo_add_obj_to_flag(Line);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Line);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_LINE, Layer, Line, Line, pcb_true);
		pcb_poly_clear_from_poly(ctx->chgsize.pcb->Data, PCB_OBJ_LINE, Layer, Line);
	}
	pcb_line_invalidate_draw(Layer, Line);
	return Line;
}

/* sets the clearance flag of a line */
void *pcb_lineop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line) || PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line))
		return NULL;
	return pcb_lineop_change_join(ctx, Layer, Line);
}

/* clears the clearance flag of a line */
void *pcb_lineop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Line) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Line))
		return NULL;
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
		return line;
	pcb_line_copy_meta(line, Line);
	pcb_line_invalidate_draw(Layer, line);
	pcb_undo_add_obj_to_create(PCB_OBJ_LINE, Layer, line, line);
	return line;
}

/* moves a line */
void *pcb_lineop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (Layer->meta.real.vis)
		pcb_line_invalidate_erase(Line);
	pcb_line_move(Line, ctx->move.dx, ctx->move.dy);
	if (Layer->meta.real.vis)
		pcb_line_invalidate_draw(Layer, Line);
	return Line;
}

void *pcb_lineop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (Layer->line_tree != NULL)
		pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	if (ctx->move.pcb != NULL)
		pcb_poly_restore_to_poly(ctx->move.pcb->Data, PCB_OBJ_LINE, Layer, Line);
	pcb_lineop_move_noclip(ctx, Layer, Line);
	if (Layer->line_tree != NULL)
		pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
	if (ctx->move.pcb != NULL)
		pcb_poly_clear_from_poly(ctx->move.pcb->Data, PCB_OBJ_LINE, Layer, Line);
	return Line;
}

void *pcb_lineop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	if (ctx->clip.restore) {
		pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
		if (ctx->clip.pcb != NULL)
			pcb_poly_restore_to_poly(ctx->clip.pcb->Data, PCB_OBJ_LINE, Layer, Line);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
		if (ctx->clip.pcb != NULL)
			pcb_poly_clear_from_poly(ctx->clip.pcb->Data, PCB_OBJ_LINE, Layer, Line);
	}
	return Line;
}

/* moves one end of a line */
void *pcb_lineop_move_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	if (Layer) {
		if (Layer->meta.real.vis)
			pcb_line_invalidate_erase(Line);
		pcb_poly_restore_to_poly(ctx->move.pcb->Data, PCB_OBJ_LINE, Layer, Line);
		pcb_r_delete_entry(Layer->line_tree, &Line->BoundingBox);
		PCB_MOVE_POINT(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
		pcb_line_bbox(Line);
		pcb_r_insert_entry(Layer->line_tree, &Line->BoundingBox);
		pcb_poly_clear_from_poly(ctx->move.pcb->Data, PCB_OBJ_LINE, Layer, Line);
		if (Layer->meta.real.vis)
			pcb_line_invalidate_draw(Layer, Line);
		return Line;
	}
	else {												/* must be a rat */

		if (ctx->move.pcb->RatOn)
			pcb_rat_invalidate_erase((pcb_rat_t *) Line);
		pcb_r_delete_entry(ctx->move.pcb->Data->rat_tree, &Line->BoundingBox);
		PCB_MOVE_POINT(Point->X, Point->Y, ctx->move.dx, ctx->move.dy);
		pcb_line_bbox(Line);
		pcb_r_insert_entry(ctx->move.pcb->Data->rat_tree, &Line->BoundingBox);
		if (ctx->move.pcb->RatOn)
			pcb_rat_invalidate_draw((pcb_rat_t *) Line);
		return Line;
	}
}

/* moves one end of a line */
void *pcb_lineop_move_point_with_route(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	if ((conf_core.editor.move_linepoint_uses_route == 0) || !Layer) {
		pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, Layer, Line, Point, ctx->move.dx, ctx->move.dy);
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
												pcb_layer_id(ctx->move.pcb->Data,Layer),
												Line->Thickness,
												Line->Clearance,
												Line->Flags,
												pcb_gui->shift_is_pressed(pcb_gui),
												pcb_gui->control_is_pressed(pcb_gui) );
		pcb_route_apply_to_line(&route,Layer,Line);
		pcb_route_destroy(&route);
	}
	return NULL;
}

/* moves a line between layers; lowlevel routines */
void *pcb_lineop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_line_t * line, pcb_layer_t * Destination)
{
	pcb_r_delete_entry(Source->line_tree, (pcb_box_t *) line);

	pcb_line_unreg(line);
	pcb_line_reg(Destination, line);

	if (!Destination->line_tree)
		Destination->line_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Destination->line_tree, (pcb_box_t *) line);

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
	pcb_pstk_t *ps;

TODO("pdstk TODO #21: do not work in comp mode, use a pstk proto - scconfig also has TODO #21, fix it there too")
	if ((ps = pcb_pstk_new_compat_via(PCB->Data, -1, i->X, i->Y, conf_core.design.via_drilling_hole, conf_core.design.via_thickness, conf_core.design.clearance, 0, PCB_PSTK_COMPAT_ROUND, pcb_true)) != NULL) {
		pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, ps, ps, ps);
		pcb_pstk_invalidate_draw(ps);
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
		pcb_message(PCB_MSG_WARNING, "Sorry, line object is locked\n");
		return NULL;
	}
	if (ctx->move.dst_layer == Layer && Layer->meta.real.vis)
		pcb_line_invalidate_draw(Layer, Line);
	if (ctx->move.dst_layer == Layer)
		return Line;

	pcb_undo_add_obj_to_move_to_layer(PCB_OBJ_LINE, Layer, Line, Line);
	if (Layer->meta.real.vis)
		pcb_line_invalidate_erase(Line);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_LINE, Layer, Line);
	newone = (pcb_line_t *) pcb_lineop_move_to_layer_low(ctx, Layer, Line, ctx->move.dst_layer);
	Line = NULL;
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, ctx->move.dst_layer, newone);
	if (ctx->move.dst_layer->meta.real.vis)
		pcb_line_invalidate_draw(ctx->move.dst_layer, newone);
	if (!conf_core.editor.auto_via || !PCB->pstk_on || ctx->move.more_to_come ||
			pcb_layer_get_group_(Layer) ==
			pcb_layer_get_group_(ctx->move.dst_layer) || !(pcb_layer_flags(PCB, pcb_layer_id(PCB->Data, Layer)) & PCB_LYT_COPPER) || !(pcb_layer_flags_(ctx->move.dst_layer) & PCB_LYT_COPPER))
		return newone;
	/* consider via at Point1 */
	sb.X1 = newone->Point1.X - newone->Thickness / 2;
	sb.X2 = newone->Point1.X + newone->Thickness / 2;
	sb.Y1 = newone->Point1.Y - newone->Thickness / 2;
	sb.Y2 = newone->Point1.Y + newone->Thickness / 2;
	if ((pcb_search_obj_by_location(PCB_OBJ_CLASS_PIN, &ptr1, &ptr2, &ptr3,
															newone->Point1.X, newone->Point1.Y, conf_core.design.via_thickness / 2) == PCB_OBJ_VOID)) {
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
	if ((pcb_search_obj_by_location(PCB_OBJ_CLASS_PIN, &ptr1, &ptr2, &ptr3,
															newone->Point2.X, newone->Point2.Y, conf_core.design.via_thickness / 2) == PCB_OBJ_VOID)) {
		info.X = newone->Point2.X;
		info.Y = newone->Point2.Y;
		if (setjmp(info.env) == 0)
			pcb_r_search(Layer->line_tree, &sb, NULL, moveline_callback, &info, NULL);
	}
	return newone;
}

/* destroys a line from a layer */
void *pcb_lineop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);

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
	pcb_move_obj(PCB_OBJ_LINE_POINT, Layer, info.line, info.point, other.X - Point->X, other.Y - Point->Y);
	return (pcb_lineop_remove(ctx, Layer, Line));
}

/* removes a line from a layer */
void *pcb_lineop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	/* erase from screen */
	if (Layer->meta.real.vis)
		pcb_line_invalidate_erase(Line);
	pcb_undo_move_obj_to_remove(PCB_OBJ_LINE, Layer, Line, Line);
	return NULL;
}

void *pcb_line_destroy(pcb_layer_t *Layer, pcb_line_t *Line)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	res = pcb_lineop_remove(&ctx, Layer, Line);
	pcb_draw();
	return res;
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
	if (layer->line_tree != NULL)
		pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);
	pcb_rotate(&line->Point1.X, &line->Point1.Y, X, Y, cosa, sina);
	pcb_rotate(&line->Point2.X, &line->Point2.Y, X, Y, cosa, sina);
	pcb_line_bbox(line);
	if (layer->line_tree != NULL)
		pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line);
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
		pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line);
}

void pcb_line_scale(pcb_line_t *line, double sx, double sy, double sth)
{
	int onbrd = (line->parent.layer != NULL) && (!line->parent.layer->is_bound);

	if (onbrd)
		pcb_line_pre(line);

	if (sx != 1.0) {
		line->Point1.X = pcb_round((double)line->Point1.X * sx);
		line->Point2.X = pcb_round((double)line->Point2.X * sx);
	}

	if (sy != 1.0) {
		line->Point1.Y = pcb_round((double)line->Point1.Y * sy);
		line->Point2.Y = pcb_round((double)line->Point2.Y * sy);
	}

	if (sth != 1.0)
		line->Thickness = pcb_round((double)line->Thickness * sth);

	pcb_line_bbox(line);

	if (onbrd)
		pcb_line_post(line);
}

void pcb_line_flip_side(pcb_layer_t *layer, pcb_line_t *line)
{
	pcb_r_delete_entry(layer->line_tree, (pcb_box_t *) line);
	line->Point1.X = PCB_SWAP_X(line->Point1.X);
	line->Point1.Y = PCB_SWAP_Y(line->Point1.Y);
	line->Point2.X = PCB_SWAP_X(line->Point2.X);
	line->Point2.Y = PCB_SWAP_Y(line->Point2.Y);
	pcb_line_bbox(line);
	pcb_r_insert_entry(layer->line_tree, (pcb_box_t *) line);
}

static void rotate_line1(pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_invalidate_erase(Line);
	if (Layer) {
		if (!Layer->is_bound)
			pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_LINE, Layer, Line);
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
			pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
		if (!Layer->is_bound)
			pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, Layer, Line);
		pcb_line_invalidate_draw(Layer, Line);
	}
	else {
		pcb_r_insert_entry(PCB->Data->rat_tree, (pcb_box_t *) Line);
		pcb_rat_invalidate_draw((pcb_rat_t *) Line);
	}
}

/* rotates a line's point */
void *pcb_lineop_rotate90_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line, pcb_point_t *Point)
{
	rotate_line1(Layer, Line);

	pcb_point_rotate90(Point, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);

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

void *pcb_lineop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_rotate(Layer, Line, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina);
	return Line;
}

/* inserts a point into a line */
void *pcb_lineop_insert_point(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	pcb_line_t *line;
	pcb_coord_t X, Y;

	if (((Line->Point1.X == ctx->insert.x) && (Line->Point1.Y == ctx->insert.y)) ||
			((Line->Point2.X == ctx->insert.x) && (Line->Point2.Y == ctx->insert.y)))
		return NULL;
	X = Line->Point2.X;
	Y = Line->Point2.Y;
	pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, Layer, Line, &Line->Point2, ctx->insert.x - X, ctx->insert.y - Y);
	pcb_line_invalidate_erase(Line);
	pcb_r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_LINE, Layer, Line);
	Line->Point2.X = ctx->insert.x;
	Line->Point2.Y = ctx->insert.y;
	pcb_line_bbox(Line);
	pcb_r_insert_entry(Layer->line_tree, (pcb_box_t *) Line);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, Layer, Line);
	pcb_line_invalidate_draw(Layer, Line);
	/* we must create after playing with Line since creation may
	 * invalidate the line pointer
	 */
	if ((line = pcb_line_new_merge(Layer, ctx->insert.x, ctx->insert.y, X, Y, Line->Thickness, Line->Clearance, Line->Flags))) {
		pcb_line_copy_meta(line, Line);
		pcb_undo_add_obj_to_create(PCB_OBJ_LINE, Layer, line, line);
		pcb_line_invalidate_draw(Layer, line);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_LINE, Layer, line);
		/* creation call adds it to the rtree */
	}
	return line;
}

void *pcb_lineop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_line_t *Line)
{
	static pcb_flag_values_t pcb_line_flags = 0;
	if (pcb_line_flags == 0)
		pcb_line_flags = pcb_obj_valid_flags(PCB_OBJ_LINE);

	if ((ctx->chgflag.flag & pcb_line_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (Line->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(Line);

	if (ctx->chgflag.flag & PCB_FLAG_CLEARLINE)
		pcb_poly_restore_to_poly(ctx->chgflag.pcb->Data, PCB_OBJ_LINE, Line->parent.layer, Line);

	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, Line);

	if (ctx->chgflag.flag & PCB_FLAG_CLEARLINE)
		pcb_poly_clear_from_poly(ctx->chgflag.pcb->Data, PCB_OBJ_LINE, Line->parent.layer, Line);

	return Line;
}

void *pcb_lineop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_line_t *line)
{
	pcb_line_name_invalidate_draw(line);
	return line;
}


/*** draw ***/

static pcb_bool is_line_term_vert(const pcb_line_t *line)
{
	pcb_coord_t dx, dy;

	dx = line->Point1.X - line->Point2.X;
	if (dx < 0)
		dx = -dx;

	dy = line->Point1.Y - line->Point2.Y;
	if (dy < 0)
		dy = -dy;

	return dx < dy;
}

void pcb_line_name_invalidate_draw(pcb_line_t *line)
{
	if (line->term != NULL) {
		pcb_term_label_invalidate((line->Point1.X + line->Point2.X)/2, (line->Point1.Y + line->Point2.Y)/2,
			100.0, is_line_term_vert(line), pcb_true, (pcb_any_obj_t *)line);
	}
}

void pcb_line_draw_label(pcb_draw_info_t *info, pcb_line_t *line)
{
	if (line->term != NULL)
		pcb_term_label_draw(info, (line->Point1.X + line->Point2.X)/2, (line->Point1.Y + line->Point2.Y)/2,
			conf_core.appearance.term_label_size, is_line_term_vert(line), pcb_true, (pcb_any_obj_t *)line);
}


void pcb_line_draw_(pcb_draw_info_t *info, pcb_line_t *line, int allow_term_gfx)
{
	pcb_coord_t thickness = line->Thickness;

	if (delayed_terms_enabled && (line->term != NULL)) {
		pcb_draw_delay_obj_add((pcb_any_obj_t *)line);
		return;
	}

	if ((info != NULL) && (info->xform != NULL) && (info->xform->bloat != 0)) {
		thickness += info->xform->bloat;
		if (thickness < 1)
			thickness = 1;
	}

	PCB_DRAW_BBOX(line);
	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
	if (!conf_core.editor.thin_draw && !conf_core.editor.wireframe_draw) {
		if ((allow_term_gfx) && pcb_draw_term_need_gfx(line) && pcb_draw_term_hid_permission()) {
			pcb_hid_set_line_cap(pcb_draw_out.active_padGC, pcb_cap_round);
			pcb_hid_set_line_width(pcb_draw_out.active_padGC, thickness);
			pcb_gui->draw_line(pcb_draw_out.active_padGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
			pcb_hid_set_line_width(pcb_draw_out.fgGC, PCB_DRAW_TERM_GFX_WIDTH);
		}
		else
			pcb_hid_set_line_width(pcb_draw_out.fgGC, thickness);
		pcb_gui->draw_line(pcb_draw_out.fgGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
	}
	else {
		if(conf_core.editor.thin_draw) {
			pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
			pcb_gui->draw_line(pcb_draw_out.fgGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
		}

		if(conf_core.editor.wireframe_draw) { 
			pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
			pcb_draw_wireframe_line(pcb_draw_out.fgGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, thickness, 0);
		}
	}

	if (line->term != NULL) {
		if ((pcb_draw_force_termlab) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, line))
			pcb_draw_delay_label_add((pcb_any_obj_t *)line);
	}
}

static void pcb_line_draw(pcb_draw_info_t *info, pcb_line_t *line, int allow_term_gfx)
{
	const pcb_color_t *color;
	pcb_color_t buf;
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(line->parent.layer);

	if (layer == NULL) /* if the layer is inbound, e.g. in preview, fall back using the layer recipe */
		layer = line->parent.layer;

	if (PCB_FLAG_TEST(PCB_FLAG_WARN, line))
		color = &conf_core.appearance.color.warn;
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, line)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
			if (layer->is_bound)
				PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
			else
				color = &conf_core.appearance.color.selected;
		}
		else
			color = &conf_core.appearance.color.connected;
	}
	else if (PCB_HAS_COLOROVERRIDE(line)) {
		color = line->override_color;
	}
	else if (layer->is_bound)
		PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 0);
	else
		color = &layer->meta.real.color;

	if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, line)) {
		pcb_lighten_color(color, &buf, 1.75);
		color = &buf;
	}

	pcb_gui->set_color(pcb_draw_out.fgGC, color);
	pcb_line_draw_(info, line, allow_term_gfx);
}

pcb_r_dir_t pcb_line_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *)b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(line->parent_type, &line->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_line_draw(info, line, 0);
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_line_draw_term_callback(const pcb_box_t * b, void *cl)
{
	pcb_line_t *line = (pcb_line_t *)b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(line->parent_type, &line->parent))
		return PCB_R_DIR_NOT_FOUND;

	pcb_line_draw(info, line, 1);
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

