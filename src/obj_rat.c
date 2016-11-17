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

/* Drawing primitive: pins and vias */

#include "config.h"

#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "undo.h"
#include "rtree.h"

#include "obj_line_draw.h"

#include "obj_rat.h"
#include "obj_rat_list.h"
#include "obj_rat_op.h"



/* TODO: consider moving the code from draw.c here and remove this: */
#include "draw.h"
#include "obj_rat_draw.h"

/* TODO: merge rats.[ch] too */
#include "rats.h"

/*** allocation ***/
/* get next slot for a Rat, allocates memory if necessary */
pcb_rat_t *pcb_rat_alloc(pcb_data_t *data)
{
	pcb_rat_t *new_obj;

	new_obj = calloc(sizeof(pcb_rat_t), 1);
	ratlist_append(&data->Rat, new_obj);

	return new_obj;
}

void pcb_rat_free(pcb_rat_t *data)
{
	ratlist_remove(data);
	free(data);
}

/*** utility ***/
/* creates a new rat-line */
pcb_rat_t *pcb_rat_new(pcb_data_t *Data, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_cardinal_t group1, pcb_cardinal_t group2, pcb_coord_t Thickness, pcb_flag_t Flags)
{
	pcb_rat_t *Line = pcb_rat_alloc(Data);

	if (!Line)
		return (Line);

	Line->ID = CreateIDGet();
	Line->Flags = Flags;
	PCB_FLAG_SET(PCB_FLAG_RAT, Line);
	Line->Thickness = Thickness;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
	Line->Point1.ID = CreateIDGet();
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = CreateIDGet();
	Line->group1 = group1;
	Line->group2 = group2;
	pcb_line_bbox((pcb_line_t *) Line);
	if (!Data->rat_tree)
		Data->rat_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Data->rat_tree, &Line->BoundingBox, 0);
	return (Line);
}

/* DeleteRats - deletes rat lines only
 * can delete all rat lines, or only selected one */
pcb_bool pcb_rats_destroy(pcb_bool selected)
{
	pcb_opctx_t ctx;
	pcb_bool changed = pcb_false;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_true;
	ctx.remove.destroy_target = NULL;

	PCB_RAT_LOOP(PCB->Data);
	{
		if ((!selected) || PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
			changed = pcb_true;
			RemoveRat(&ctx, line);
		}
	}
	PCB_END_LOOP;
	if (changed) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (changed);
}


/*** ops ***/
/* copies a rat-line to paste buffer */
void *AddRatToBuffer(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	return (pcb_rat_new(ctx->buffer.dst, Rat->Point1.X, Rat->Point1.Y,
		Rat->Point2.X, Rat->Point2.Y, Rat->group1, Rat->group2, Rat->Thickness,
		pcb_flag_mask(Rat->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* moves a rat-line to paste buffer */
void *MoveRatToBuffer(pcb_opctx_t *ctx, pcb_rat_t * rat)
{
	pcb_r_delete_entry(ctx->buffer.src->rat_tree, (pcb_box_t *) rat);

	ratlist_remove(rat);
	ratlist_append(&ctx->buffer.dst->Rat, rat);

	PCB_FLAG_CLEAR(PCB_FLAG_FOUND, rat);

	if (!ctx->buffer.dst->rat_tree)
		ctx->buffer.dst->rat_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(ctx->buffer.dst->rat_tree, (pcb_box_t *) rat, 0);
	return rat;
}

/* inserts a point into a rat-line */
void *InsertPointIntoRat(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	pcb_line_t *newone;

	newone = pcb_line_new_merge(CURRENT, Rat->Point1.X, Rat->Point1.Y,
																	ctx->insert.x, ctx->insert.y, conf_core.design.line_thickness, 2 * conf_core.design.clearance, Rat->Flags);
	if (!newone)
		return newone;
	pcb_undo_add_obj_to_create(PCB_TYPE_LINE, CURRENT, newone, newone);
	EraseRat(Rat);
	DrawLine(CURRENT, newone);
	newone = pcb_line_new_merge(CURRENT, Rat->Point2.X, Rat->Point2.Y,
																	ctx->insert.x, ctx->insert.y, conf_core.design.line_thickness, 2 * conf_core.design.clearance, Rat->Flags);
	if (newone) {
		pcb_undo_add_obj_to_create(PCB_TYPE_LINE, CURRENT, newone, newone);
		DrawLine(CURRENT, newone);
	}
	pcb_undo_move_obj_to_remove(PCB_TYPE_RATLINE, Rat, Rat, Rat);
	pcb_draw();
	return (newone);
}

/* moves a line between layers */
void *MoveRatToLayer(pcb_opctx_t *ctx, pcb_rat_t * Rat)
{
	pcb_line_t *newone;
	/*pcb_coord_t X1 = Rat->Point1.X, Y1 = Rat->Point1.Y;
	   pcb_coord_t X1 = Rat->Point1.X, Y1 = Rat->Point1.Y;
	   if PCB_FLAG_VIA
	   if we're on a pin, add a thermal
	   else make a via and a wire, but 0-length wire not good
	   else as before */

	newone = pcb_line_new(ctx->move.dst_layer, Rat->Point1.X, Rat->Point1.Y,
																Rat->Point2.X, Rat->Point2.Y, conf_core.design.line_thickness, 2 * conf_core.design.clearance, Rat->Flags);
	if (conf_core.editor.clear_line)
		conf_set_editor(clear_line, 1);
	if (!newone)
		return (NULL);
	pcb_undo_add_obj_to_create(PCB_TYPE_LINE, ctx->move.dst_layer, newone, newone);
	if (PCB->RatOn)
		EraseRat(Rat);
	pcb_undo_move_obj_to_remove(PCB_TYPE_RATLINE, Rat, Rat, Rat);
	DrawLine(ctx->move.dst_layer, newone);
	pcb_draw();
	return (newone);
}

/* destroys a rat */
void *DestroyRat(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	if (ctx->remove.destroy_target->rat_tree)
		pcb_r_delete_entry(ctx->remove.destroy_target->rat_tree, &Rat->BoundingBox);

	pcb_rat_free(Rat);
	return NULL;
}

/* removes a rat */
void *RemoveRat(pcb_opctx_t *ctx, pcb_rat_t *Rat)
{
	/* erase from screen and memory */
	if (PCB->RatOn) {
		EraseRat(Rat);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	pcb_undo_move_obj_to_remove(PCB_TYPE_RATLINE, Rat, Rat, Rat);
	return NULL;
}

/*** draw ***/
pcb_r_dir_t draw_rat_callback(const pcb_box_t * b, void *cl)
{
	pcb_rat_t *rat = (pcb_rat_t *) b;

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, rat)) {
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, rat))
			pcb_gui->set_color(Output.fgGC, PCB->RatSelectedColor);
		else
			pcb_gui->set_color(Output.fgGC, PCB->ConnectedColor);
	}
	else
		pcb_gui->set_color(Output.fgGC, PCB->RatColor);

	if (conf_core.appearance.rat_thickness < 20)
		rat->Thickness = pcb_pixel_slop * conf_core.appearance.rat_thickness;
	/* rats.c set PCB_FLAG_VIA if this rat goes to a containing poly: draw a donut */
	if (PCB_FLAG_TEST(PCB_FLAG_VIA, rat)) {
		int w = rat->Thickness;

		if (conf_core.editor.thin_draw)
			pcb_gui->set_line_width(Output.fgGC, 0);
		else
			pcb_gui->set_line_width(Output.fgGC, w);
		pcb_gui->draw_arc(Output.fgGC, rat->Point1.X, rat->Point1.Y, w * 2, w * 2, 0, 360);
	}
	else
		_draw_line((pcb_line_t *) rat);
	return R_DIR_FOUND_CONTINUE;
}

void EraseRat(pcb_rat_t *Rat)
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
		EraseLine((pcb_line_t *) Rat);
	pcb_flag_erase(&Rat->Flags);
}

void DrawRat(pcb_rat_t *Rat)
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
		DrawLine(NULL, (pcb_line_t *) Rat);
}
