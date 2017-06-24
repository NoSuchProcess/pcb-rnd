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
#include "undo.h"
#include "compat_misc.h"

pcb_subc_t *pcb_subc_alloc(void)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->data = pcb_data_new(NULL);
	sc->type = PCB_OBJ_SUBC;
	PCB_SET_PARENT(sc->data, subc, sc);
	return sc;
}

void pcb_subc_free(pcb_subc_t *sc)
{
	pcb_subclist_remove(sc);
	pcb_data_free(sc->data);
	free(sc->data);
	free(sc);
}


void pcb_add_subc_to_data(pcb_data_t *dt, pcb_subc_t *sc)
{
	PCB_SET_PARENT(sc->data, subc, sc);
	PCB_SET_PARENT(sc, data, dt);
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
	sc->ID = pcb_create_ID_get();
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
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, line);
		}

		while((arc = arclist_first(&src->Arc)) != NULL) {
			arclist_remove(arc);
			arclist_append(&dst->Arc, arc);
			PCB_SET_PARENT(arc, layer, dst);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
		}

		while((text = textlist_first(&src->Text)) != NULL) {
			textlist_remove(text);
			textlist_append(&dst->Text, text);
			PCB_SET_PARENT(text, layer, dst);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
		}

		while((poly = polylist_first(&src->Polygon)) != NULL) {
			polylist_remove(poly);
			polylist_append(&dst->Polygon, poly);
			PCB_SET_PARENT(poly, layer, dst);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
		}
	}

	{ /* convert globals */
		pcb_pin_t *via;

		while((via = pinlist_first(&buffer->Data->Via)) != NULL) {
			pinlist_remove(via);
			pinlist_append(&sc->data->Via, via);
			PCB_SET_PARENT(via, data, sc->data);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, via);
		}
	}

	return 0;
}

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	int n;

	/* draw per layer objects */
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

	/* draw global objects */
	{
		pcb_pin_t *via;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			pcb_coord_t ox, oy;
			ox = via->X;
			oy = via->Y;
			via->X += DX;
			via->Y += DY;
			pcb_gui->thindraw_pcb_pv(pcb_crosshair.GC, pcb_crosshair.GC, via, pcb_true, pcb_false);
			via->X = ox;
			via->Y = oy;
		}
	}

	/* mark */
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
}

pcb_subc_t *pcb_subc_dup_at(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_board_t *src_pcb;
	int n;
	pcb_subc_t *sc = pcb_subc_alloc();
	sc->ID = pcb_create_ID_get();
	PCB_SET_PARENT(sc->data, subc, sc);
	PCB_SET_PARENT(sc, data, dst);
	pcb_subclist_append(&dst->subc, sc);

	src_pcb = pcb_data_get_top(src->data);

	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;

	/* make a copy of layer data */
	for(n = 0; n < src->data->LayerN; n++) {
		pcb_layer_t *sl = src->data->Layer + n;
		pcb_layer_t *dl = sc->data->Layer + n;
		pcb_line_t *line, *nline;
		pcb_text_t *text, *ntext;
		pcb_arc_t *arc, *narc;
		gdl_iterator_t it;

		/* bind layer/resolve layers */
		if ((pcb != NULL) && (pcb == src_pcb)) {
			/* copy within the same board */
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));
			dl->meta.bound.name = pcb_strdup(sl->meta.bound.name);
			if (dl->meta.bound.real != NULL)
				pcb_layer_link_trees(dl, dl->meta.bound.real);
		}
		else if (pcb != NULL) {
			/* copying from buffer to board */
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));
			dl->meta.bound.name = pcb_strdup(sl->meta.bound.name);
			dl->meta.bound.real = pcb_layer_resolve_binding(pcb, sl);

			if (dl->meta.bound.real == NULL)
				pcb_message(PCB_MSG_WARNING, "Couldn't bind a layer of subcricuit TODO1 while placing it\n");
			else
				pcb_layer_link_trees(dl, dl->meta.bound.real);
		}
		else {
			/* copying from board to buffer */
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));

			dl->meta.bound.real = NULL;
			dl->meta.bound.name = pcb_strdup(sl->meta.bound.name);
		}


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

	}
	sc->data->LayerN = src->data->LayerN;

	/* bind the via rtree so that vias added in this subc show up on the board */
	if (pcb != NULL) {
		if (pcb->Data->via_tree == NULL)
			pcb->Data->via_tree = pcb_r_create_tree(NULL, 0, 0);
		sc->data->via_tree = pcb->Data->via_tree;
	}

	{ /* make a copy of global data */
		pcb_pin_t *via, *nvia;
		gdl_iterator_t it;

		pinlist_foreach(&src->data->Via, &it, via) {
			nvia = pcb_via_dup_at(sc->data, via, dx, dy);
			if (nvia != NULL)
				pcb_box_bump_box(&sc->BoundingBox, &nvia->BoundingBox);
		}
	}

	/* make a copy of polygons at the end so clipping caused by other objects are calculated only once */
	for(n = 0; n < src->data->LayerN; n++) {
		pcb_layer_t *sl = src->data->Layer + n;
		pcb_layer_t *dl = sc->data->Layer + n;
		pcb_polygon_t *poly, *npoly;
		gdl_iterator_t it;

		polylist_foreach(&sl->Polygon, &it, poly) {
			npoly = pcb_poly_dup_at(dl, poly, dx, dy);
			if (npoly != NULL) {
				PCB_SET_PARENT(npoly, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &npoly->BoundingBox);
			}
		}
	}

	memcpy(&sc->Flags, &src->Flags, sizeof(sc->Flags));

	pcb_close_box(&sc->BoundingBox);

	if (pcb != NULL) {
		if (!dst->subc_tree)
			dst->subc_tree = pcb_r_create_tree(NULL, 0, 0);
		pcb_r_insert_entry(dst->subc_tree, (pcb_box_t *)sc, 0);
	}

	return sc;
}

pcb_subc_t *pcb_subc_dup(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src)
{
	return pcb_subc_dup_at(pcb, dst, src, 0, 0);
}

void pcb_subc_bbox(pcb_subc_t *sc)
{
	pcb_data_bbox(&sc->BoundingBox, sc->data);
}


/* erases a subc on a layer */
void EraseSubc(pcb_subc_t *sc)
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

	if (pcb_data_get_top(Data) != NULL) {
		if (Data->subc_tree != NULL)
			pcb_r_delete_entry(Data->subc_tree, (pcb_box_t *)sc);
		else
			Data->subc_tree = pcb_r_create_tree(NULL, 0, 0);
	}

	/* execute on layer locals */
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


	/* execute on globals */
	{
		pcb_pin_t *via;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_VIA, via, via, via);
			pcb_box_bump_box(&sc->BoundingBox, &via->BoundingBox);
		}
	}

	pcb_close_box(&sc->BoundingBox);
	if (pcb_data_get_top(Data) != NULL)
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

	pcb_undo_add_obj_to_create(PCB_TYPE_SUBC, sc, sc, sc);

	return (sc);
}

extern pcb_opfunc_t MoveFunctions, Rotate90Functions, ChgFlagFunctions, ChangeSizeFunctions, ChangeClearSizeFunctions, Change1stSizeFunctions, Change2ndSizeFunctions;
extern pcb_opfunc_t ChangeOctagonFunctions, SetOctagonFunctions, ClrOctagonFunctions;
extern pcb_opfunc_t ChangeSquareFunctions, SetSquareFunctions, ClrSquareFunctions;

void *pcb_subcop_move(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_board_t *pcb = pcb_data_get_top(sc->data);
	return pcb_subc_op((pcb != NULL ? pcb->Data : NULL), sc, &MoveFunctions, ctx);
}

void *pcb_subcop_rotate90(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_board_t *pcb = pcb_data_get_top(sc->data);
	return pcb_subc_op((pcb != NULL ? pcb->Data : NULL), sc, &Rotate90Functions, ctx);
}

void *pcb_subcop_move_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	int n;

	if (ctx->buffer.pcb->Data->subc_tree != NULL)
		pcb_r_delete_entry(ctx->buffer.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subclist_remove(sc);
	pcb_subclist_append(&ctx->buffer.dst->subc, sc);

	EraseSubc(sc);

	/* move layer local */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_layer_t *dl = ctx->buffer.dst->Layer + n;
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

		/* keep only the layer binding match, unbound other aspects */
		sl->meta.bound.real = NULL;
		sl->arc_tree = sl->line_tree = sl->text_tree = sl->polygon_tree = NULL;
	}


	/* move globals */
	{
		pcb_pin_t *via;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			pcb_r_delete_entry(sc->data->via_tree, (pcb_box_t *)via);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, via);
		}
	}

	sc->data->via_tree = NULL;


	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, sc);
	PCB_SET_PARENT(sc, data, ctx->buffer.dst);
	return sc;
}

void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_t *nsc;
	nsc = pcb_subc_dup_at(NULL, ctx->buffer.dst, sc, 0, 0);
	if (ctx->buffer.extraflg & PCB_FLAG_SELECTED)
		pcb_select_subc(NULL, nsc, PCB_CHGFLG_CLEAR, 0);
	return nsc;
}

/* break buffer subc into pieces */
pcb_bool pcb_subc_smash_buffer(pcb_buffer_t *buff)
{
	pcb_subc_t *subc;
	int n;

	if (pcb_subclist_length(&buff->Data->subc) != 1)
		return (pcb_false);

	subc = pcb_subclist_first(&buff->Data->subc);
	pcb_subclist_remove(subc);

	pcb_data_free(buff->Data);
	buff->Data = subc->data;
	buff->Data->parent_type = PCB_PARENT_INVALID;
	buff->Data->parent.data = NULL;

	return (pcb_true);
}


void *pcb_subcop_change_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_clear_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeClearSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_1st_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &Change1stSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_2nd_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &Change2ndSizeFunctions, ctx);
	return sc;
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
	if (ctx->remove.pcb->Data->subc_tree != NULL)
		pcb_r_delete_entry(ctx->remove.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subclist_remove(sc);
	EraseSubc(sc);
	pcb_subc_free(sc);
	return NULL;
}

void *pcb_subcop_remove(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	EraseSubc(sc);
	if (!ctx->remove.bulk)
		pcb_draw();
	pcb_undo_move_obj_to_remove(PCB_TYPE_SUBC, sc, sc, sc);
	PCB_CLEAR_PARENT(sc);
	return NULL;
}

void *pcb_subcop_clear_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ClrOctagonFunctions, ctx);
	return sc;
}

void *pcb_subcop_set_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &SetOctagonFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeOctagonFunctions, ctx);
	return sc;
}

void *pcb_subcop_clear_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ClrSquareFunctions, ctx);
	return sc;
}

void *pcb_subcop_set_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &SetSquareFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeSquareFunctions, ctx);
	return sc;
}


#define PCB_SUBC_FLAGS (PCB_FLAG_FOUND | PCB_FLAG_SELECTED | PCB_FLAG_AUTO | PCB_FLAG_LOCK | PCB_FLAG_VISIT | PCB_FLAG_NONETLIST)
void *pcb_subcop_change_flag(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgflag.pcb->Data, sc, &ChgFlagFunctions, ctx);
	if ((ctx->chgflag.flag & PCB_SUBC_FLAGS) == ctx->chgflag.flag)
		PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, sc);
	return sc;
}


void pcb_select_subc(pcb_board_t *pcb, pcb_subc_t *sc, pcb_change_flag_t how, int redraw)
{
	pcb_opctx_t ctx;

	ctx.chgflag.pcb = pcb;
	ctx.chgflag.how = how;
	ctx.chgflag.flag = PCB_FLAG_SELECTED;

	pcb_subc_op((pcb == NULL ? NULL : pcb->Data), sc, &ChgFlagFunctions, &ctx);
	PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, sc);
	if (redraw)
		DrawSubc(sc);
}

/* mirrors the coordinates of a subcircuit; an additional offset is passed */
void pcb_subc_mirror(pcb_data_t *data, pcb_subc_t *subc, pcb_coord_t y_offs)
{
	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);

	pcb_data_mirror(subc->data, y_offs);
	pcb_subc_bbox(subc);

	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc, 0);
}

pcb_bool pcb_subc_change_side(pcb_subc_t *subc, pcb_coord_t yoff)
{
	pcb_opctx_t ctx;
	pcb_subc_t *newsc;
	int n;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, subc))
		return (pcb_false);

	pcb_undo_add_obj_to_mirror(PCB_TYPE_SUBC, subc, subc, subc, yoff);

	/* move subc into a local "buffer" */
	memset(&ctx, 0, sizeof(ctx));
	ctx.buffer.pcb = pcb_data_get_top(subc->data);
	ctx.buffer.dst = pcb_data_new(NULL);
	ctx.buffer.src = PCB->Data;
	newsc = pcb_subcop_move_to_buffer(&ctx, subc);

	/* mirror object geometry and stackup */
	pcb_subc_mirror(NULL, newsc, yoff);
	for(n = 0; n < newsc->data->LayerN; n++) {
		pcb_layer_t *ly = newsc->data->Layer + n;
		if (ly->meta.bound.type & PCB_LYT_TOP)
			ly->meta.bound.type = (ly->meta.bound.type & ~PCB_LYT_TOP) | PCB_LYT_BOTTOM;
		else if (ly->meta.bound.type & PCB_LYT_BOTTOM)
			ly->meta.bound.type = (ly->meta.bound.type & ~PCB_LYT_BOTTOM) | PCB_LYT_TOP;
	}

	/* place the new subc */
	pcb_subc_dup_at(PCB, PCB->Data, newsc, 0, 0);
	pcb_data_free(ctx.buffer.dst);
	return pcb_true;
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
