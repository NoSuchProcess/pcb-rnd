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
#include "config.h"

#include "board.h"
#include "data.h"
#include "data_it.h"
#include "conf_core.h"
#include "hid_inlines.h"
#include "undo.h"
#include "rtree.h"
#include "search.h"

#include "obj_line_draw.h"

#include "obj_rat.h"
#include "obj_rat_list.h"
#include "obj_rat_op.h"

#include "obj_rat_draw.h"

/*** allocation ***/

void pcb_rat_reg(pcb_data_t *data, pcb_rat_t *rat)
{
	ratlist_append(&data->Rat, rat);
	pcb_obj_id_reg(data, rat);
	PCB_SET_PARENT(rat, data, data);
}

void pcb_rat_unreg(pcb_rat_t *rat)
{
	pcb_data_t *data = rat->parent.data;
	assert(rat->parent_type == PCB_PARENT_DATA);
	ratlist_remove(rat);
	pcb_obj_id_del(data, rat);
	PCB_CLEAR_PARENT(rat);
}

pcb_rat_t *pcb_rat_alloc_id(pcb_data_t *data, long int id)
{
	pcb_rat_t *new_obj;

	new_obj = calloc(sizeof(pcb_rat_t), 1);
	new_obj->ID = id;
	new_obj->type = PCB_OBJ_RAT;

	pcb_rat_reg(data, new_obj);

	return new_obj;
}

pcb_rat_t *pcb_rat_alloc(pcb_data_t *data)
{
	return pcb_rat_alloc_id(data, pcb_create_ID_get());
}

void pcb_rat_free(pcb_rat_t *rat)
{
	pcb_rat_unreg(rat);
	free(rat->anchor[0]);
	free(rat->anchor[1]);
	free(rat);
}

/*** utility ***/
/* creates a new rat-line */
pcb_rat_t *pcb_rat_new(pcb_data_t *Data, long int id, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_layergrp_id_t group1, pcb_layergrp_id_t group2, pcb_coord_t Thickness, pcb_flag_t Flags, pcb_any_obj_t *anchor1, pcb_any_obj_t *anchor2)
{
	pcb_rat_t *Line;

	if (id <= 0)
		id = pcb_create_ID_get();

	Line = pcb_rat_alloc_id(Data, id);
	if (!Line)
		return Line;

	Line->Flags = Flags;
	PCB_FLAG_SET(PCB_FLAG_RAT, Line);
	Line->Thickness = Thickness;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
	Line->Point1.ID = pcb_create_ID_get();
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = pcb_create_ID_get();
	Line->group1 = group1;
	Line->group2 = group2;
	pcb_line_bbox((pcb_line_t *) Line);
	if (!Data->rat_tree)
		Data->rat_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Data->rat_tree, &Line->BoundingBox);

	if (anchor1 != NULL)
		Line->anchor[0] = pcb_obj2idpath(anchor1);
	if (anchor2 != NULL)
		Line->anchor[1] = pcb_obj2idpath(anchor2);

	return Line;
}

/* DeleteRats - deletes rat lines only
 * can delete all rat lines, or only selected one */
pcb_bool pcb_rats_destroy(pcb_bool selected)
{
	pcb_opctx_t ctx;
	pcb_bool changed = pcb_false;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	PCB_RAT_LOOP(PCB->Data);
	{
		if ((!selected) || PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
			changed = pcb_true;
			pcb_ratop_remove(&ctx, line);
		}
	}
	PCB_END_LOOP;
	if (changed)
		pcb_undo_inc_serial();
	return changed;
}

/*** utility ***/

static pcb_bool rat_meets_line(pcb_line_t *line, pcb_coord_t x, pcb_coord_t y, pcb_layergrp_id_t gid)
{
	if (gid >= 0) {
		pcb_layer_t *ly = pcb_layer_get_real(line->parent.layer);
		if ((ly == NULL) || (ly->meta.real.grp != gid))
			return 0;
	}
	if ((line->Point1.X == x) && (line->Point1.Y == y)) return pcb_true;
	if ((line->Point2.X == x) && (line->Point2.Y == y)) return pcb_true;
	return pcb_is_point_on_line(x, y, 1, line);
}


static pcb_bool rat_meets_arc(pcb_arc_t *arc, pcb_coord_t x, pcb_coord_t y, pcb_layergrp_id_t gid)
{
	if (gid >= 0) {
		pcb_layer_t *ly = pcb_layer_get_real(arc->parent.layer);
		if ((ly == NULL) || (ly->meta.real.grp != gid))
			return 0;
	}
	return pcb_is_point_on_arc(x, y, 1, arc);
}


static pcb_bool rat_meets_poly(pcb_poly_t *poly, pcb_coord_t x, pcb_coord_t y, pcb_layergrp_id_t gid)
{
	if (gid >= 0) {
		pcb_layer_t *ly = pcb_layer_get_real(poly->parent.layer);
		if ((ly == NULL) || (ly->meta.real.grp != gid))
			return 0;
	}
	return pcb_poly_is_point_in_p(x, y, 1, poly);
}

static pcb_bool rat_meets_pstk(pcb_data_t *data, pcb_pstk_t *pstk, pcb_coord_t x, pcb_coord_t y, pcb_layergrp_id_t gid)
{
	pcb_layergrp_t *g = pcb_get_layergrp(PCB, gid);
	pcb_layer_t *ly;
	if (g == NULL)
		return pcb_false;

	ly = pcb_get_layer(data, g->lid[0]);
	if (ly != NULL)
		ly = pcb_layer_get_real(ly);
	if (ly == NULL)
		return pcb_false;

	if ((pstk->x == x) && (pstk->y == y))
		return pcb_true;

	return pcb_is_point_in_pstk(x, y, 1, pstk, ly);
}


/* return the first object (the type that is most likely an endpoint of a rat)
   on a point on a layer */
static pcb_any_obj_t *find_obj_on_layer(pcb_coord_t x, pcb_coord_t y, pcb_layer_t *l)
{
	pcb_rtree_it_t it;
	pcb_box_t *n;
	pcb_rtree_box_t sb;

	sb.x1 = x; sb.x2 = x+1;
	sb.y1 = y; sb.y2 = y+1;

	if (l->line_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->line_tree, &sb); n != NULL; n = pcb_rtree_next(&it)) {
			if (rat_meets_line((pcb_line_t *)n, x, y, -1)) {
				pcb_r_end(&it);
				return (pcb_any_obj_t *)n;
			}
		}
		pcb_r_end(&it);
	}

	if (l->arc_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->arc_tree, &sb); n != NULL; n = pcb_rtree_next(&it)) {
			if (rat_meets_arc((pcb_arc_t *)n, x, y, -1)) {
				pcb_r_end(&it);
				return (pcb_any_obj_t *)n;
			}
		}
		pcb_r_end(&it);
	}

	if (l->polygon_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->polygon_tree, &sb); n != NULL; n = pcb_rtree_next(&it)) {
			if (rat_meets_poly((pcb_poly_t *)n, x, y, -1)) {
				pcb_r_end(&it);
				return (pcb_any_obj_t *)n;
			}
		}
		pcb_r_end(&it);
	}

TODO("find through text");
#if 0
	if (l->text_tree != NULL) {
		for(n = pcb_rtree_first(&it, l->text_tree, &sb); n != NULL; n = pcb_rtree_next(&it)) {
			if (rat_meets_text((pcb_text_t *)n, x, y, -1)) {
				pcb_r_end(&it);
				return (pcb_any_obj_t *)n;
			}
		}

		pcb_r_end(&it);
	}
#endif
	return NULL;
}

/* return the first object (the type that is most likely an endpoint of a rat)
   on a point on a layer group */
static pcb_any_obj_t *find_obj_on_grp(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_layergrp_id_t gid)
{
	int i;
	pcb_rtree_box_t sb;
	pcb_rtree_it_t it;
	pcb_box_t *n;
	pcb_layergrp_t *g = pcb_get_layergrp(PCB, gid);

	if (g == NULL)
		return NULL;

	sb.x1 = x; sb.x2 = x+1;
	sb.y1 = y; sb.y2 = y+1;

	if (PCB->Data->padstack_tree != NULL) {
		for(n = pcb_rtree_first(&it, data->padstack_tree, &sb); n != NULL; n = pcb_rtree_next(&it)) {
			if (rat_meets_pstk(data, (pcb_pstk_t *)n, x, y, gid)) {
				pcb_r_end(&it);
				return (pcb_any_obj_t *)n;
			}
		}
		pcb_r_end(&it);
	}

	for(i = 0; i < g->len; i++) {
		pcb_any_obj_t *o = find_obj_on_layer(x, y, &data->Layer[g->lid[i]]);
		if (o != NULL)
			return o;
	}
	return NULL;
}

pcb_any_obj_t *pcb_rat_anchor_guess(pcb_rat_t *rat, int end, pcb_bool update)
{
	pcb_data_t *data = rat->parent.data;
	pcb_idpath_t **path = &rat->anchor[!!end];
	pcb_coord_t x = (end == 0) ? rat->Point1.X : rat->Point2.X;
	pcb_coord_t y = (end == 0) ? rat->Point1.Y : rat->Point2.Y;
	pcb_layergrp_id_t gid = (end == 0) ? rat->group1 : rat->group2;
	pcb_any_obj_t *ao;

	/* (relatively) cheap thest if existing anchor is valid */
	if (*path != NULL) {
		ao = pcb_idpath2obj_in(data, *path);
		if (ao != NULL) {
			switch(ao->type) {
				case PCB_OBJ_LINE: if (rat_meets_line((pcb_line_t *)ao, x, y, gid)) return ao; break;
				case PCB_OBJ_ARC:  if (rat_meets_arc((pcb_arc_t *)ao, x, y, gid)) return ao; break;
				case PCB_OBJ_POLY: if (rat_meets_poly((pcb_poly_t *)ao, x, y, gid)) return ao; break;
				case PCB_OBJ_PSTK: if (rat_meets_pstk(data, (pcb_pstk_t *)ao, x, y, gid)) return ao; break;
				TODO("find through text")
				default: break;
			}
		}
	}

	/* if we got here, there was no anchor object set or it was outdated - find
	   the currently valid object by endpoint */
	ao = find_obj_on_grp(data, x, y, gid);
	if (update) {
		if (*path != NULL)
			pcb_idpath_destroy(*path);
		if (ao == NULL)
			*path = NULL;
		else
			*path = pcb_obj2idpath(ao);
	}

	return ao;
}

void pcb_rat_all_anchor_guess(pcb_data_t *data)
{
	pcb_rat_t *rat;
	gdl_iterator_t it;
	ratlist_foreach(&data->Rat, &it, rat) {
		pcb_rat_anchor_guess(rat, 0, pcb_true);
		pcb_rat_anchor_guess(rat, 1, pcb_true);
	}
}


/*** ops ***/
/* copies a rat-line to paste buffer */
void *pcb_ratop_add_to_buffer(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	pcb_rat_t *res = pcb_rat_new(ctx->buffer.dst, -1, Rat->Point1.X, Rat->Point1.Y,
		Rat->Point2.X, Rat->Point2.Y, Rat->group1, Rat->group2, Rat->Thickness,
		pcb_flag_mask(Rat->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg),
		NULL, NULL);

	res->anchor[0] = Rat->anchor[0] == NULL ? NULL : pcb_idpath_dup(Rat->anchor[0]);
	res->anchor[1] = Rat->anchor[1] == NULL ? NULL : pcb_idpath_dup(Rat->anchor[1]);
	return res;
}

/* moves a rat-line between board and buffer */
void *pcb_ratop_move_buffer(pcb_opctx_t *ctx, pcb_rat_t * rat)
{
	pcb_r_delete_entry(ctx->buffer.src->rat_tree, (pcb_box_t *) rat);

	pcb_rat_unreg(rat);
	pcb_rat_reg(ctx->buffer.dst, rat);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, rat);

	if (!ctx->buffer.dst->rat_tree)
		ctx->buffer.dst->rat_tree = pcb_r_create_tree();
	pcb_r_insert_entry(ctx->buffer.dst->rat_tree, (pcb_box_t *) rat);

	return rat;
}

/* inserts a point into a rat-line */
void *pcb_ratop_insert_point(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	pcb_line_t *newone;

	newone = pcb_line_new_merge(CURRENT, Rat->Point1.X, Rat->Point1.Y,
																	ctx->insert.x, ctx->insert.y, conf_core.design.line_thickness, 2 * conf_core.design.clearance, Rat->Flags);
	if (!newone)
		return newone;
	pcb_undo_add_obj_to_create(PCB_OBJ_LINE, CURRENT, newone, newone);
	pcb_rat_invalidate_erase(Rat);
	pcb_line_invalidate_draw(CURRENT, newone);
	newone = pcb_line_new_merge(CURRENT, Rat->Point2.X, Rat->Point2.Y,
																	ctx->insert.x, ctx->insert.y, conf_core.design.line_thickness, 2 * conf_core.design.clearance, Rat->Flags);
	if (newone) {
		pcb_undo_add_obj_to_create(PCB_OBJ_LINE, CURRENT, newone, newone);
		pcb_line_invalidate_draw(CURRENT, newone);
	}
	pcb_undo_move_obj_to_remove(PCB_OBJ_RAT, Rat, Rat, Rat);
	return newone;
}

/* move a rat to a layer: convert it into a layer line */
void *pcb_ratop_move_to_layer(pcb_opctx_t *ctx, pcb_rat_t * Rat)
{
	pcb_line_t *newone;
	newone = pcb_line_new(ctx->move.dst_layer, Rat->Point1.X, Rat->Point1.Y,
																Rat->Point2.X, Rat->Point2.Y, conf_core.design.line_thickness, 2 * conf_core.design.clearance, Rat->Flags);
	if (conf_core.editor.clear_line)
		conf_set_editor(clear_line, 1);
	if (!newone)
		return NULL;
	pcb_undo_add_obj_to_create(PCB_OBJ_LINE, ctx->move.dst_layer, newone, newone);
	if (PCB->RatOn)
		pcb_rat_invalidate_erase(Rat);
	pcb_undo_move_obj_to_remove(PCB_OBJ_RAT, Rat, Rat, Rat);
	pcb_line_invalidate_draw(ctx->move.dst_layer, newone);
	return newone;
}

/* destroys a rat */
void *pcb_ratop_destroy(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	if (ctx->remove.destroy_target->rat_tree)
		pcb_r_delete_entry(ctx->remove.destroy_target->rat_tree, &Rat->BoundingBox);

	pcb_rat_free(Rat);
	return NULL;
}

/* removes a rat */
void *pcb_ratop_remove(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	/* erase from screen and memory */
	if (PCB->RatOn)
		pcb_rat_invalidate_erase(Rat);
	pcb_undo_move_obj_to_remove(PCB_OBJ_RAT, Rat, Rat, Rat);
	return NULL;
}

/*** draw ***/
pcb_r_dir_t pcb_rat_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, rat)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, rat))
			pcb_gui->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.selected);
		else
			pcb_gui->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.connected);
	}
	else if (PCB_HAS_COLOROVERRIDE(rat)) {
		pcb_gui->set_color(pcb_draw_out.fgGC, rat->override_color);
	}
	else
		pcb_gui->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.rat);

	if (conf_core.appearance.rat_thickness < 20)
		rat->Thickness = pcb_pixel_slop * conf_core.appearance.rat_thickness;
	/* rats.c set PCB_FLAG_VIA if this rat goes to a containing poly: draw a donut */
	if (PCB_FLAG_TEST(PCB_FLAG_VIA, rat)) {
		int w = rat->Thickness;

		if (conf_core.editor.thin_draw || conf_core.editor.wireframe_draw)
			pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
		else
			pcb_hid_set_line_width(pcb_draw_out.fgGC, w);
		pcb_gui->draw_arc(pcb_draw_out.fgGC, rat->Point1.X, rat->Point1.Y, w * 2, w * 2, 0, 360);
	}
	else
		pcb_line_draw_(NULL, (pcb_line_t *) rat, 0);
	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_rat_invalidate_erase(pcb_rat_t *Rat)
{
	if (PCB_FLAG_TEST(PCB_FLAG_VIA, Rat)) {
		pcb_coord_t w = Rat->Thickness;

		pcb_box_t b;

		b.X1 = Rat->Point1.X - w * 2 - w / 2;
		b.X2 = Rat->Point1.X + w * 2 + w / 2;
		b.Y1 = Rat->Point1.Y - w * 2 - w / 2;
		b.Y2 = Rat->Point1.Y + w * 2 + w / 2;
		pcb_draw_invalidate(&b);
	}
	else
		pcb_line_invalidate_erase((pcb_line_t *) Rat);
	pcb_flag_erase(&Rat->Flags);
}

void pcb_rat_invalidate_draw(pcb_rat_t *Rat)
{
	if (conf_core.appearance.rat_thickness < 20)
		Rat->Thickness = pcb_pixel_slop * conf_core.appearance.rat_thickness;
	/* rats.c set PCB_FLAG_VIA if this rat goes to a containing poly: draw a donut */
	if (PCB_FLAG_TEST(PCB_FLAG_VIA, Rat)) {
		pcb_coord_t w = Rat->Thickness;

		pcb_box_t b;

		b.X1 = Rat->Point1.X - w * 2 - w / 2;
		b.X2 = Rat->Point1.X + w * 2 + w / 2;
		b.Y1 = Rat->Point1.Y - w * 2 - w / 2;
		b.Y2 = Rat->Point1.Y + w * 2 + w / 2;
		pcb_draw_invalidate(&b);
	}
	else
		pcb_line_invalidate_draw(NULL, (pcb_line_t *) Rat);
}

void pcb_rat_update_obj_removed(pcb_board_t *pcb, pcb_any_obj_t *obj)
{
	pcb_rat_t *rat;
	gdl_iterator_t it;

	if (obj->type == PCB_OBJ_SUBC) { 
		pcb_subc_t *subc = (pcb_subc_t *)obj;
		pcb_any_obj_t *o;
		pcb_data_it_t it2;

		/* subcircuit means checking against each part object;
		   this is O(R*P), where R is the number of rat lines and P is the
		   number of subc parts - maybe an rtree based approach would be better */
		for(o = pcb_data_first(&it2, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it2))
			pcb_rat_update_obj_removed(pcb, o);
		return;
	}

	ratlist_foreach(&pcb->Data->Rat, &it, rat) {
		if ((pcb_rat_anchor_guess(rat, 0, 1) == obj) || (pcb_rat_anchor_guess(rat, 1, 1) == obj)) {
			if (PCB->RatOn)
				pcb_rat_invalidate_erase(rat);
			pcb_undo_move_obj_to_remove(PCB_OBJ_RAT, rat, rat, rat);
		}
	}
}

