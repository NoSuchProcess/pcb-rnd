/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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
 */

#include "config.h"

#include "buffer.h"
#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "obj_subc.h"
#include "obj_subc_op.h"
#include "obj_text_draw.h"
#include "rtree.h"
#include "draw.h"
#include "flag.h"
#include "polygon.h"
#include "operation.h"

pcb_subc_t *pcb_subc_alloc(void)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->data = pcb_data_new(NULL);
	sc->type = PCB_OBJ_SUBC;
	PCB_SET_PARENT(sc->data, subc, sc);
	return sc;
}

void pcb_add_subc_to_data(pcb_data_t *dt, pcb_subc_t *sc)
{
	PCB_SET_PARENT(sc->data, subc, sc);
	pcb_subclist_append(&dt->subc, sc);
}

static pcb_layer_t *pcb_subc_layer_create_buff(pcb_subc_t *sc, pcb_layer_t *src)
{
	pcb_layer_t *dst = &sc->data->Layer[sc->data->LayerN++];

	memcpy(&dst->meta, &src->meta, sizeof(src->meta));
	dst->comb = src->comb;
	dst->grp = -1;
	dst->parent = sc->data;
	return dst;
}

int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer)
{
	pcb_subc_t *sc;
	int n;

	sc = pcb_subc_alloc();
	pcb_add_subc_to_data(buffer->Data, sc);

	/* create layer matches and copy objects */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_t *dst, *src;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;

		if (pcb_layer_is_pure_empty(&buffer->Data->Layer[n]))
			continue;

		src = &buffer->Data->Layer[n];
		dst = pcb_subc_layer_create_buff(sc, src);

		while((line = linelist_first(&src->Line)) != NULL) {
			linelist_remove(line);
			linelist_append(&dst->Line, line);
			PCB_SET_PARENT(line, layer, dst);
		}

		while((arc = arclist_first(&src->Arc)) != NULL) {
			arclist_remove(arc);
			arclist_append(&dst->Arc, arc);
			PCB_SET_PARENT(arc, layer, dst);
		}

		while((text = textlist_first(&src->Text)) != NULL) {
			textlist_remove(text);
			textlist_append(&dst->Text, text);
			PCB_SET_PARENT(text, layer, dst);
		}

		while((poly = polylist_first(&src->Polygon)) != NULL) {
			polylist_remove(poly);
			polylist_append(&dst->Polygon, poly);
			PCB_SET_PARENT(poly, layer, dst);
		}
	}

	pcb_message(PCB_MSG_ERROR, "pcb_subc_convert_from_buffer(): not yet implemented\n");
	return -1;
}

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	int n;

	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *ly = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;

		linelist_foreach(&ly->Line, &it, line)
			pcb_gui->draw_line(pcb_crosshair.GC, DX + line->Point1.X, DY + line->Point1.Y, DX + line->Point2.X, DY + line->Point2.Y);

		arclist_foreach(&ly->Arc, &it, arc)
			pcb_gui->draw_arc(pcb_crosshair.GC, DX + arc->X, DY + arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);

		polylist_foreach(&ly->Polygon, &it, poly)
			XORPolygon(poly, DX, DY, 0);

		textlist_foreach(&ly->Text, &it, text)
			XORDrawText(text, DX, DY);
	}

	/* mark */
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
}

pcb_subc_t *pcb_subc_dup_at(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	int n;
	pcb_subc_t *sc = pcb_subc_alloc();
	PCB_SET_PARENT(sc->data, subc, sc);
	pcb_subclist_append(&dst->subc, sc);

	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;

	/* make a copy */
	for(n = 0; n < src->data->LayerN; n++) {
		pcb_layer_t *sl = src->data->Layer + n;
		pcb_layer_t *dl = sc->data->Layer + n;
		pcb_line_t *line, *nline;
		pcb_text_t *text, *ntext;
		pcb_polygon_t *poly, *npoly;
		pcb_arc_t *arc, *narc;
		gdl_iterator_t it;

		memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));


		dl->meta.bound.real = NULL;
		/* bind layer to the stack provided by pcb/dst */
		if (pcb != NULL)
			dl->meta.bound.real = pcb_layer_resolve_binding(pcb, sl);

		if (dl->meta.bound.real == NULL)
			pcb_message(PCB_MSG_WARNING, "Couldn't bind a layer of subcricuit TODO while placing it\n");
		else
			pcb_layer_link_trees(dl, dl->meta.bound.real);

		linelist_foreach(&sl->Line, &it, line) {
			nline = pcb_line_dup_at(dl, line, dx, dy);
			if (nline != NULL) {
				PCB_SET_PARENT(nline, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &nline->BoundingBox);
			}
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			narc = pcb_arc_dup_at(dl, arc, dx, dy);
			if (narc != NULL) {
				PCB_SET_PARENT(narc, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &narc->BoundingBox);
			}
		}

		textlist_foreach(&sl->Text, &it, text) {
			ntext = pcb_text_dup_at(dl, text, dx, dy);
			if (ntext != NULL) {
				PCB_SET_PARENT(ntext, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &ntext->BoundingBox);
			}
		}

		polylist_foreach(&sl->Polygon, &it, poly) {
			npoly = pcb_poly_dup_at(dl, poly, dx, dy);
			if (npoly != NULL) {
				PCB_SET_PARENT(npoly, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &npoly->BoundingBox);
			}
		}
	}

	sc->data->LayerN = src->data->LayerN;

	pcb_close_box(&sc->BoundingBox);

	if (!dst->subc_tree)
		dst->subc_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(dst->subc_tree, (pcb_box_t *)sc, 0);

	return sc;
}

pcb_subc_t *pcb_subc_dup(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src)
{
	return pcb_subc_dup_at(pcb, dst, src, 0, 0);
}

/* erases a subc on a layer */
static void EraseSubc(pcb_subc_t *sc)
{
	pcb_draw_invalidate(sc);
/*	pcb_flag_erase(&sc->Flags); ??? */
}

void DrawSubc(pcb_subc_t *sc)
{
	pcb_draw_invalidate(sc);
}


/* Execute an operation on all children on an sc and update the bbox of the sc */
void *pcb_subc_op(pcb_data_t *Data, pcb_subc_t *sc, pcb_opfunc_t *opfunc, pcb_opctx_t *ctx)
{
	int n;

	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;

	EraseSubc(sc);

	if (Data->subc_tree != NULL)
		pcb_r_delete_entry(Data->subc_tree, (pcb_box_t *)sc);
	else
		Data->subc_tree = pcb_r_create_tree(NULL, 0, 0);


	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;


		linelist_foreach(&sl->Line, &it, line) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_LINE, sl, line, line);
			pcb_box_bump_box(&sc->BoundingBox, &line->BoundingBox);
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_ARC, sl, arc, arc);
			pcb_box_bump_box(&sc->BoundingBox, &arc->BoundingBox);
		}

		textlist_foreach(&sl->Text, &it, text) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_TEXT, sl, text, text);
			pcb_box_bump_box(&sc->BoundingBox, &text->BoundingBox);
		}

		polylist_foreach(&sl->Polygon, &it, poly) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_POLYGON, sl, poly, poly);
			pcb_box_bump_box(&sc->BoundingBox, &poly->BoundingBox);
		}

	}

	pcb_close_box(&sc->BoundingBox);
	pcb_r_insert_entry(Data->subc_tree, (pcb_box_t *)sc, 0);
	DrawSubc(sc);
	pcb_draw();
	return sc;
}


/* copies a subcircuit onto the PCB.  Then does a draw. */
void *pcb_subcop_copy(pcb_opctx_t *ctx, pcb_subc_t *src)
{
	pcb_subc_t *sc;

	sc = pcb_subc_dup_at(PCB, PCB->Data, src, ctx->copy.DeltaX, ctx->copy.DeltaY);

#warning TODO:
/*	pcb_undo_add_obj_to_create(PCB_TYPE_ELEMENT, element, element, element);*/

	return (sc);
}

extern pcb_opfunc_t MoveFunctions, Rotate90Functions, ChgFlagFunctions;

void *pcb_subcop_move(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	return pcb_subc_op(PCB->Data, sc, &MoveFunctions, ctx);
}

void *pcb_subcop_rotate90(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	return pcb_subc_op(PCB->Data, sc, &Rotate90Functions, ctx);
}

void *pcb_subcop_move_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	int n;

	if (ctx->buffer.pcb->Data->subc_tree != NULL)
		pcb_r_delete_entry(ctx->buffer.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subclist_remove(sc);
	pcb_subclist_append(&ctx->buffer.dst->subc, sc);

	EraseSubc(sc);

	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;

		linelist_foreach(&sl->Line, &it, line) {
			pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_LINE, sl, line);
			pcb_r_delete_entry(sl->line_tree, (pcb_box_t *)line);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, line);
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_ARC, sl, arc);
			pcb_r_delete_entry(sl->arc_tree, (pcb_box_t *)arc);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
		}

		textlist_foreach(&sl->Text, &it, text) {
			pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_LINE, sl, text);
			pcb_r_delete_entry(sl->text_tree, (pcb_box_t *)text);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
		}

		polylist_foreach(&sl->Polygon, &it, poly) {
			pcb_r_delete_entry(sl->polygon_tree, (pcb_box_t *)poly);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
		}

	}

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, sc);
	PCB_SET_PARENT(sc, data, ctx->buffer.dst);
	return sc;
}

void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	return pcb_subc_dup_at(PCB, ctx->buffer.dst, sc, 0, 0);
}


void *pcb_subcop_change_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_clear_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_1st_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_2nd_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_nonetlist(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_name(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_destroy(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_remove(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_clear_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_set_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_clear_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_set_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}

void *pcb_subcop_change_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO
	abort();
}


#define PCB_SUBC_FLAGS (PCB_FLAG_FOUND | PCB_FLAG_CLEARLINE | PCB_FLAG_SELECTED | PCB_FLAG_AUTO | PCB_FLAG_LOCK | PCB_FLAG_VISIT)
void *pcb_subcop_change_flag(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	if ((ctx->chgflag.flag & PCB_SUBC_FLAGS) != ctx->chgflag.flag)
		return NULL;
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, sc);
#warning subc TODO (revise flags)
	abort();
}


void pcb_select_subc(pcb_board_t *pcb, pcb_subc_t *sc, pcb_change_flag_t how, int redraw)
{
	pcb_opctx_chgflag_t ctx;

	ctx.pcb = pcb;
	ctx.how = how;
	ctx.flag = PCB_FLAG_SELECTED;

	pcb_subc_op(PCB->Data, sc, &ChgFlagFunctions, &ctx);
	PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, sc);
	if (redraw)
		DrawSubc(sc);
}


#include "conf_core.h"
#include "draw.h"
pcb_r_dir_t draw_subc_mark_callback(const pcb_box_t *b, void *cl)
{
	pcb_subc_t *subc = (pcb_subc_t *) b;
	pcb_box_t *bb = &subc->BoundingBox;
	int selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc);

	pcb_gui->set_color(Output.fgGC, selected ? conf_core.appearance.color.subc_selected : conf_core.appearance.color.subc);
	pcb_gui->set_line_cap(Output.fgGC, Trace_Cap);
	pcb_gui->set_line_width(Output.fgGC, 0);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_draw_dashed_line(Output.fgGC, bb->X1, bb->Y1, bb->X2, bb->Y1);
	pcb_draw_dashed_line(Output.fgGC, bb->X1, bb->Y1, bb->X1, bb->Y2);
	pcb_draw_dashed_line(Output.fgGC, bb->X2, bb->Y2, bb->X2, bb->Y1);
	pcb_draw_dashed_line(Output.fgGC, bb->X2, bb->Y2, bb->X1, bb->Y2);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

	return PCB_R_DIR_FOUND_CONTINUE;
}
