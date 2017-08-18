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
#include "obj_term.h"
#include "obj_text_draw.h"
#include "rtree.h"
#include "draw.h"
#include "flag.h"
#include "polygon.h"
#include "operation.h"
#include "undo.h"
#include "compat_misc.h"
#include "math_helper.h"
#include "pcb_minuid.h"

#define SUBC_AUX_NAME "subc-aux"

static void pcb_subc_attrib_post_change(pcb_attribute_list_t *list, const char *name, const char *value)
{
	pcb_subc_t *sc = (pcb_subc_t *)(((char *)list) - offsetof(pcb_subc_t, Attributes));
	if (strcmp(name, "refdes") == 0) {
		const char *inv;
		sc->refdes = value;
		inv = pcb_obj_id_invalid(sc->refdes);
		if (inv != NULL)
			pcb_message(PCB_MSG_ERROR, "Invalid character '%c' in subc refdes '%s'\n", *inv, sc->refdes);
	}
}

pcb_subc_t *pcb_subc_alloc(void)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->Attributes.post_change = pcb_subc_attrib_post_change;
	sc->data = pcb_data_new(NULL);
	sc->type = PCB_OBJ_SUBC;
	PCB_SET_PARENT(sc->data, subc, sc);
	minuid_gen(&pcb_minuid, sc->uid);
	pcb_term_init(&sc->terminals);
	return sc;
}

void pcb_subc_free(pcb_subc_t *sc)
{
	pcb_term_uninit(&sc->terminals);
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

static pcb_line_t *add_aux_line(pcb_layer_t *aux, const char *key, const char *val, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_line_t *l = pcb_line_new(aux, x1, y1, x2, y2, PCB_MM_TO_COORD(0.1), 0, pcb_no_flags());
	pcb_attribute_put(&l->Attributes, key, val);
	return l;
}

static pcb_line_t *find_aux_line(pcb_layer_t *aux, const char *key)
{
	pcb_line_t *line;
	gdl_iterator_t it;
	linelist_foreach(&aux->Line, &it, line) {
		const char *val = pcb_attribute_get(&line->Attributes, "subc-role");
		if ((val != NULL) && (strcmp(val, key) == 0))
			return line;
	}
	return NULL;
}

static int pcb_subc_cache_update(pcb_subc_t *sc)
{
	if (sc->aux_layer == NULL) {
		int n;
		for(n = 0; n < sc->data->LayerN; n++) {
			if (strcmp(sc->data->Layer[n].meta.bound.name, SUBC_AUX_NAME) == 0) {
				sc->aux_layer = sc->data->Layer + n;
				break;
			}
		}
	}

	if (sc->aux_cache[0] != NULL)
		return 0;

	if (sc->aux_layer == NULL) {
		pcb_message(PCB_MSG_WARNING, "Can't find subc aux layer\n");
		return -1;
	}

	sc->aux_cache[PCB_SUBCH_ORIGIN] = find_aux_line(sc->aux_layer, "origin");
	sc->aux_cache[PCB_SUBCH_X] = find_aux_line(sc->aux_layer, "x");
	sc->aux_cache[PCB_SUBCH_Y] = find_aux_line(sc->aux_layer, "y");

	return 0;
}

int pcb_subc_get_origin(pcb_subc_t *sc, pcb_coord_t *x, pcb_coord_t *y)
{
	if ((pcb_subc_cache_update(sc) != 0) || (sc->aux_cache[PCB_SUBCH_ORIGIN] == NULL))
		return -1;

	*x = sc->aux_cache[PCB_SUBCH_ORIGIN]->Point1.X;
	*y = sc->aux_cache[PCB_SUBCH_ORIGIN]->Point1.Y;
	return 0;
}

int pcb_subc_get_rotation(pcb_subc_t *sc, double *rot)
{
	if ((pcb_subc_cache_update(sc) != 0) || (sc->aux_cache[PCB_SUBCH_X] == NULL))
		return -1;

	*rot = PCB_RAD_TO_DEG * atan2(sc->aux_cache[PCB_SUBCH_X]->Point2.Y - sc->aux_cache[PCB_SUBCH_X]->Point1.Y, sc->aux_cache[PCB_SUBCH_X]->Point2.X - sc->aux_cache[PCB_SUBCH_X]->Point1.X);
	return 0;
}


static void pcb_subc_cache_invalidate(pcb_subc_t *sc)
{
	sc->aux_cache[0] = NULL;
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

	/* create aux layer */
	{
		pcb_layer_t *aux = &sc->data->Layer[sc->data->LayerN++];
		pcb_coord_t unit = PCB_MM_TO_COORD(1);

		memset(aux, 0, sizeof(pcb_layer_t));
		aux->meta.bound.name = pcb_strdup(SUBC_AUX_NAME);
		aux->meta.bound.type = PCB_LYT_VIRTUAL | PCB_LYT_NOEXPORT | PCB_LYT_MISC;
		aux->grp = -1;
		aux->parent = sc->data;

		add_aux_line(aux, "subc-role", "origin", buffer->X, buffer->Y, buffer->X, buffer->Y);
		add_aux_line(aux, "subc-role", "x", buffer->X, buffer->Y, buffer->X+unit, buffer->Y);
		add_aux_line(aux, "subc-role", "y", buffer->X, buffer->Y, buffer->X, buffer->Y+unit);
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

static void pcb_subc_draw_origin(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	pcb_line_t *origin;
	pcb_subc_cache_update(sc);

	origin = sc->aux_cache[PCB_SUBCH_ORIGIN];

	if (origin == NULL)
		return;

	DX += (origin->Point1.X + origin->Point2.X) / 2;
	DY += (origin->Point1.Y + origin->Point2.Y) / 2;

	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
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
			pcb_text_draw_xor(text, DX, DY);
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

	pcb_subc_draw_origin(sc, DX, DY);
}

#define MAYBE_KEEP_ID(dst, src) \
do { \
	if ((keep_ids) && (dst != NULL)) \
		dst->ID = src->ID; \
} while(0)

static pcb_subc_t *pcb_subc_copy_meta(pcb_subc_t *dst, pcb_subc_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}


pcb_subc_t *pcb_subc_dup_at(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src, pcb_coord_t dx, pcb_coord_t dy, pcb_bool keep_ids)
{
	pcb_board_t *src_pcb;
	int n;
	pcb_subc_t *sc = pcb_subc_alloc();

	if (keep_ids)
		sc->ID = src->ID;
	else
		sc->ID = pcb_create_ID_get();

	minuid_cpy(sc->uid, src->uid);
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
			MAYBE_KEEP_ID(nline, line);
			if (nline != NULL) {
				PCB_SET_PARENT(nline, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &nline->BoundingBox);
			}
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			narc = pcb_arc_dup_at(dl, arc, dx, dy);
			MAYBE_KEEP_ID(narc, arc);
			if (narc != NULL) {
				PCB_SET_PARENT(narc, layer, dl);
				pcb_box_bump_box(&sc->BoundingBox, &narc->BoundingBox);
			}
		}

		textlist_foreach(&sl->Text, &it, text) {
			ntext = pcb_text_dup_at(dl, text, dx, dy);
			MAYBE_KEEP_ID(ntext, text);
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
			MAYBE_KEEP_ID(nvia, via);
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
			MAYBE_KEEP_ID(npoly, poly);
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

	return pcb_subc_copy_meta(sc, src);
}

pcb_subc_t *pcb_subc_dup(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src)
{
	return pcb_subc_dup_at(pcb, dst, src, 0, 0, pcb_false);
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

	sc = pcb_subc_dup_at(PCB, PCB->Data, src, ctx->copy.DeltaX, ctx->copy.DeltaY, pcb_false);

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
	pcb_board_t *dst_top = pcb_data_get_top(ctx->buffer.dst);
	int dst_is_pcb = ((dst_top != NULL) && (dst_top->Data == ctx->buffer.dst));

	EraseSubc(sc);

	/* move the subc */
	if (ctx->buffer.pcb->Data->subc_tree != NULL)
		pcb_r_delete_entry(ctx->buffer.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subclist_remove(sc);
	pcb_subclist_append(&ctx->buffer.dst->subc, sc);

	if (dst_is_pcb) {
		if (ctx->buffer.dst->subc_tree == NULL)
			ctx->buffer.dst->subc_tree = pcb_r_create_tree(NULL, 0, 0);
		pcb_r_insert_entry(ctx->buffer.dst->subc_tree, (pcb_box_t *)sc, 0);
	}

	/* move layer local */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_layer_t *dl;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;
		int src_has_real_layer = (sl->meta.bound.real != NULL);

		if (dst_is_pcb) {
			dl = pcb_layer_resolve_binding(dst_top, sl);
			if (dl != NULL) {
				pcb_layer_link_trees(sl, dl);
			}
			else {
				/* need to create the trees so that move and other ops work */
				if (sl->line_tree == NULL) sl->line_tree = pcb_r_create_tree(NULL, 0, 0);
				if (sl->arc_tree == NULL) sl->arc_tree = pcb_r_create_tree(NULL, 0, 0);
				if (sl->text_tree == NULL) sl->text_tree = pcb_r_create_tree(NULL, 0, 0);
				if (sl->polygon_tree == NULL) sl->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
				pcb_message(PCB_MSG_ERROR, "Couldn't bind subc layer TODO on buffer move\n");
			}
		}
		else
			dl = ctx->buffer.dst->Layer + n;

		linelist_foreach(&sl->Line, &it, line) {
			if (src_has_real_layer) {
				pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_LINE, sl, line);
				pcb_r_delete_entry(sl->line_tree, (pcb_box_t *)line);
			}
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, line);
			if ((dl != NULL) && (dl->line_tree != NULL))
				pcb_r_insert_entry(dl->line_tree, (pcb_box_t *)line, 0);
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			if (src_has_real_layer) {
				pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_ARC, sl, arc);
				pcb_r_delete_entry(sl->arc_tree, (pcb_box_t *)arc);
			}
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
			if ((dl != NULL) && (dl->arc_tree != NULL))
				pcb_r_insert_entry(dl->arc_tree, (pcb_box_t *)arc, 0);
		}

		textlist_foreach(&sl->Text, &it, text) {
			if (src_has_real_layer) {
				pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_LINE, sl, text);
				pcb_r_delete_entry(sl->text_tree, (pcb_box_t *)text);
			}
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
			if ((dl != NULL) && (dl->text_tree != NULL))
				pcb_r_insert_entry(dl->text_tree, (pcb_box_t *)text, 0);
		}

		polylist_foreach(&sl->Polygon, &it, poly) {
			if (src_has_real_layer)
				pcb_r_delete_entry(sl->polygon_tree, (pcb_box_t *)poly);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
			if ((dl != NULL) && (dl->polygon_tree != NULL))
				pcb_r_insert_entry(dl->polygon_tree, (pcb_box_t *)poly, 0);
		}

		if (!dst_is_pcb) {
			/* keep only the layer binding match, unbound other aspects */
			sl->meta.bound.real = NULL;
			sl->arc_tree = sl->line_tree = sl->text_tree = sl->polygon_tree = NULL;
		}
		else
			sl->meta.bound.real = dl;
	}


	/* move globals */
	{
		pcb_pin_t *via;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			if (sc->data->via_tree != NULL)
				pcb_r_delete_entry(sc->data->via_tree, (pcb_box_t *)via);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, via);
			if (ctx->buffer.dst->via_tree != NULL)
				pcb_r_insert_entry(ctx->buffer.dst->via_tree, (pcb_box_t *)via, 0);
		}
	}

	/* bind globals */
	if (dst_is_pcb) {
		if (ctx->buffer.dst->via_tree == NULL)
			ctx->buffer.dst->via_tree = pcb_r_create_tree(NULL, 0, 0);
		sc->data->via_tree = ctx->buffer.dst->via_tree;
	}
	else
		sc->data->via_tree = NULL;

	/* init poly clips */
	if (dst_is_pcb) {
		for(n = 0; n < sc->data->LayerN; n++) {
			pcb_polygon_t *poly;
			gdl_iterator_t it;
			pcb_layer_t *sl = sc->data->Layer + n;

			polylist_foreach(&sl->Polygon, &it, poly) {
				pcb_poly_init_clip(ctx->buffer.dst, sl->meta.bound.real, poly);
			}
		}
	}

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, sc);
	PCB_SET_PARENT(sc, data, ctx->buffer.dst);
	return sc;
}

void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_t *nsc;
	nsc = pcb_subc_dup_at(NULL, ctx->buffer.dst, sc, 0, 0, pcb_true);
	if (ctx->buffer.extraflg & PCB_FLAG_SELECTED)
		pcb_subc_select(NULL, nsc, PCB_CHGFLG_CLEAR, 0);
	return nsc;
}

/* break buffer subc into pieces */
pcb_bool pcb_subc_smash_buffer(pcb_buffer_t *buff)
{
	pcb_subc_t *subc;

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


void pcb_subc_select(pcb_board_t *pcb, pcb_subc_t *sc, pcb_change_flag_t how, int redraw)
{
	pcb_opctx_t ctx;

	pcb_undo_add_obj_to_flag(PCB_TYPE_SUBC, sc, sc, sc);

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
	pcb_subc_t *newsc, *newsc2;
	int n;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, subc))
		return (pcb_false);


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
	newsc2 = pcb_subc_dup_at(PCB, PCB->Data, newsc, 0, 0, pcb_true);
	newsc2->ID = newsc->ID;
	pcb_undo_add_subc_to_otherside(PCB_TYPE_SUBC, newsc2, newsc2, newsc2, yoff);

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

	pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.element);
	pcb_gui->set_line_cap(Output.fgGC, Trace_Cap);
	pcb_gui->set_line_width(Output.fgGC, 0);
	pcb_subc_draw_origin(subc, 0, 0);

	pcb_gui->set_color(Output.fgGC, selected ? conf_core.appearance.color.subc_selected : conf_core.appearance.color.subc);
	pcb_gui->set_line_width(Output.fgGC, 0);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_draw_dashed_line(Output.fgGC, bb->X1, bb->Y1, bb->X2, bb->Y1);
	pcb_draw_dashed_line(Output.fgGC, bb->X1, bb->Y1, bb->X1, bb->Y2);
	pcb_draw_dashed_line(Output.fgGC, bb->X2, bb->Y2, bb->X2, bb->Y1);
	pcb_draw_dashed_line(Output.fgGC, bb->X2, bb->Y2, bb->X1, bb->Y2);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

	if (subc->refdes != NULL)
		pcb_term_label_draw(bb->X1, bb->Y1, 50.0, 0, 0, subc->refdes);

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_subc_t *pcb_subc_by_name(pcb_data_t *base, const char *name)
{
#warning subc TODO: hierarchy
	PCB_SUBC_LOOP(base);
	{
		if ((subc->refdes != NULL) && (PCB_NSTRCMP(subc->refdes, name) == 0))
			return subc;
	}
	PCB_END_LOOP;
	return NULL;
}
