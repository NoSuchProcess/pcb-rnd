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


/* select routines
 */

#include "config.h"

#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>

#include "board.h"
#include "data.h"
#include "data_it.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "polygon.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "find.h"
#include "extobj.h"
#include <librnd/core/compat_misc.h>

#include "obj_arc_draw.h"
#include "obj_line_draw.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"
#include "obj_rat_draw.h"
#include "obj_pstk_draw.h"
#include "obj_gfx_draw.h"

#include <genregex/regex_sei.h>

/* ---------------------------------------------------------------------------
 * toggles the selection of any kind of object
 * the different types are defined by search.h
 */
rnd_bool pcb_select_object(pcb_board_t *pcb)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_layer_t *layer;
	int type;

	rnd_bool changed = rnd_true;

	type = pcb_search_screen_maybe_selector(pcb_crosshair.X, pcb_crosshair.Y, PCB_SELECT_TYPES | PCB_LOOSE_SUBC(PCB) | PCB_OBJ_FLOATER, &ptr1, &ptr2, &ptr3);
	if (type == PCB_OBJ_VOID || PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_any_obj_t *) ptr2))
		return rnd_false;
	switch (type) {

	case PCB_OBJ_PSTK:
		pcb_undo_add_obj_to_flag(ptr1);
		PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, (pcb_pstk_t *) ptr2);
		pcb_pstk_invalidate_draw((pcb_pstk_t *) ptr2);
		pcb_extobj_sync_floater_flags(pcb, (const pcb_any_obj_t *)ptr2, 1, 1);
		break;

	case PCB_OBJ_LINE:
		{
			pcb_line_t *line = (pcb_line_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, line);
			pcb_line_invalidate_draw(layer, line);
			pcb_extobj_sync_floater_flags(pcb, (const pcb_any_obj_t *)line, 1, 1);
			break;
		}

	case PCB_OBJ_RAT:
		{
			pcb_rat_t *rat = (pcb_rat_t *) ptr2;

			pcb_undo_add_obj_to_flag(ptr1);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, rat);
			pcb_rat_invalidate_draw(rat);
			break;
		}

	case PCB_OBJ_ARC:
		{
			pcb_arc_t *arc = (pcb_arc_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, arc);
			pcb_arc_invalidate_draw(layer, arc);
			pcb_extobj_sync_floater_flags(pcb, (const pcb_any_obj_t *)arc, 1, 1);
			break;
		}

	case PCB_OBJ_TEXT:
		{
			pcb_text_t *text = (pcb_text_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, text);
			pcb_text_invalidate_draw(layer, text);
			pcb_extobj_sync_floater_flags(pcb, (const pcb_any_obj_t *)text, 1, 1);
			break;
		}

	case PCB_OBJ_POLY:
		{
			pcb_poly_t *poly = (pcb_poly_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, poly);
			pcb_poly_invalidate_draw(layer, poly);
			pcb_extobj_sync_floater_flags(pcb, (const pcb_any_obj_t *)poly, 1, 1);
			/* changing memory order no longer effects draw order */
			break;
		}

	case PCB_OBJ_GFX:
		{
			pcb_gfx_t *gfx = (pcb_gfx_t *)ptr2;

			layer = (pcb_layer_t *)ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, gfx);
			pcb_gfx_invalidate_draw(layer, gfx);
			pcb_extobj_sync_floater_flags(pcb, (const pcb_any_obj_t *)gfx, 1, 1);
			break;
		}


	case PCB_OBJ_SUBC:
		pcb_subc_select(pcb, (pcb_subc_t *) ptr2, PCB_CHGFLG_TOGGLE, 1);
		break;
	}
	pcb_draw();
	pcb_undo_inc_serial();
	return changed;
}

static void fix_box_dir(rnd_box_t *Box, int force_pos)
{
#define swap(a,b) \
do { \
	rnd_coord_t tmp; \
	tmp = a; \
	a = b; \
	b = tmp; \
} while(0)

	/* If board view is flipped, box coords need to be flipped too to reflect
	   the on-screen direction of draw */
	if (rnd_conf.editor.view.flip_x)
		swap(Box->X1, Box->X2);
	if (rnd_conf.editor.view.flip_y)
		swap(Box->Y1, Box->Y2);

	if ((force_pos) || (conf_core.editor.selection.disable_negative)) {
		if (Box->X1 > Box->X2)
			swap(Box->X1, Box->X2);
		if (Box->Y1 > Box->Y2)
			swap(Box->Y1, Box->Y2);
	}
	else {
		if (conf_core.editor.selection.symmetric_negative) {
			if (Box->Y1 > Box->Y2)
				swap(Box->Y1, Box->Y2);
		}
		/* Make sure our negative box is canonical: always from bottom-right to top-left
		   This limits all possible box coordinate orders to two, one for positive and
		   one for negative. */
		if (PCB_IS_BOX_NEGATIVE(Box)) {
			if (Box->X1 < Box->X2)
				swap(Box->X1, Box->X2);
			if (Box->Y1 < Box->Y2)
				swap(Box->Y1, Box->Y2);
		}
	}
#undef swap

}

/* ----------------------------------------------------------------------
 * selects/unselects or lists visible objects within the passed box
 * If len is NULL:
 *  Flag determines if the block is to be selected or unselected
 *  returns non-NULL if the state of any object has changed
 * if len is non-NULL:
 *  returns a list of object IDs matched the search and loads len with the
 *  length of the list. Returns NULL on no match.
 * Always limit layer search to layer types matching lyt (and never limit globals)
 */
TODO("cleanup: should be rewritten with generic ops and rtree")
static long int *ListBlock_(pcb_board_t *pcb, rnd_box_t *Box, rnd_bool Flag, pcb_layer_type_t lyt, int *len, void *(cb)(void *ctx, pcb_any_obj_t *obj), void *ctx)
{
	int changed = 0;
	int used = 0, alloced = 0;
	long int *list = NULL;

	fix_box_dir(Box, 0);
/*rnd_printf("box: %mm %mm - %mm %mm    [ %d ] %d %d\n", Box->X1, Box->Y1, Box->X2, Box->Y2, PCB_IS_BOX_NEGATIVE(Box), rnd_conf.editor.view.flip_x, rnd_conf.editor.view.flip_y);*/

/* append an object to the return list OR set the flag if there's no list */
#define append_list(undo_type, p1, obj) \
do { \
	if (len == NULL) { \
		pcb_undo_add_obj_to_flag(obj); \
		PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, obj); \
	} \
	else { \
		if (used >= alloced) { \
			alloced += 64; \
			list = realloc(list, sizeof(*list) * alloced); \
		} \
		list[used] = obj->ID; \
		used++; \
	} \
	changed = 1; \
} while(0)

#define append(undo_type, p1, obj) \
do { \
	if (cb != NULL) { \
		cb(ctx, (pcb_any_obj_t *)obj); \
		used++; \
	} \
	else \
		append_list(undo_type, p1, obj); \
} while(0)

	if (PCB_IS_BOX_NEGATIVE(Box) && ((Box->X1 == Box->X2) || (Box->Y2 == Box->Y1))) {
		if (len != NULL)
			*len = 0;
		return NULL;
	}

	if (pcb->RatOn || !Flag)
		PCB_RAT_LOOP(pcb->Data);
	{
		if (PCB_LINE_NEAR_BOX((pcb_line_t *) line, Box) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, line) != Flag) {
			append(PCB_OBJ_RAT, line, line);
			if (pcb->RatOn)
				pcb_rat_invalidate_draw(line);
		}
	}
	PCB_END_LOOP;

	/* check layers */
	LAYER_LOOP(pcb->Data, pcb_max_layer(PCB));
	{
		unsigned int lflg = pcb_layer_flags(pcb, n);

		if ((lflg &lyt) == 0)
			continue;

		if ((lflg & PCB_LYT_SILK) && (PCB_LAYERFLG_ON_VISIBLE_SIDE(lflg))) {
			if (!(pcb_silk_on(pcb) || !Flag))
				continue;
		}
		else if ((lflg & PCB_LYT_SILK) && !(PCB_LAYERFLG_ON_VISIBLE_SIDE(lflg))) {
			if (!(pcb->InvisibleObjectsOn || !Flag))
				continue;
		}
		else if (!(layer->meta.real.vis || !Flag))
			continue;

		PCB_LINE_LOOP(layer);
		{
			if (PCB_LINE_NEAR_BOX(line, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, line) != Flag) {
				append(PCB_OBJ_LINE, layer, line);
				if ((len != NULL) && layer->meta.real.vis)
					pcb_line_invalidate_draw(layer, line);
			}
		}
		PCB_END_LOOP;
		PCB_ARC_LOOP(layer);
		{
			if (PCB_ARC_NEAR_BOX(arc, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc) != Flag) {
				append(PCB_OBJ_ARC, layer, arc);
				if ((len != NULL) && layer->meta.real.vis)
					pcb_arc_invalidate_draw(layer, arc);
			}
		}
		PCB_END_LOOP;
		PCB_TEXT_LOOP(layer);
		{
			if (!Flag || pcb_text_is_visible(PCB, layer, text)) {
				if (PCB_TEXT_NEAR_BOX(text, Box)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, text)
						&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) != Flag) {
					append(PCB_OBJ_TEXT, layer, text);
					if ((len != NULL) && pcb_text_is_visible(PCB, layer, text))
						pcb_text_invalidate_draw(layer, text);
				}
			}
		}
		PCB_END_LOOP;
		PCB_POLY_LOOP(layer);
		{
			if (PCB_POLYGON_NEAR_BOX(polygon, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, polygon)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon) != Flag) {
				append(PCB_OBJ_POLY, layer, polygon);
				if ((len != NULL) && layer->meta.real.vis)
					pcb_poly_invalidate_draw(layer, polygon);
			}
		}
		PCB_END_LOOP;
		PCB_GFX_LOOP(layer);
		{
			if (PCB_GFX_NEAR_BOX(gfx, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, gfx)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, gfx) != Flag) {
				append(PCB_OBJ_GFX, layer, gfx);
				if ((len != NULL) && layer->meta.real.vis)
					pcb_gfx_invalidate_draw(layer, gfx);
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	if (PCB->SubcOn) {
		PCB_SUBC_LOOP(pcb->Data);
		{
			if (PCB_SUBC_NEAR_BOX(subc, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, subc)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc) != Flag) {

				if (len == NULL) {
					pcb_subc_select(PCB, subc, Flag, 1);
				}
				else {
					if (used >= alloced) {
						alloced += 64;
						list = realloc(list, sizeof(*list) * alloced);
					}
					list[used] = subc->ID;
					used++;
				}
				if (len != NULL) {
					changed = 1;
					DrawSubc(subc);
				}
			}
		}
		PCB_END_LOOP;
	}

	/* end with vias */
	if (pcb->pstk_on || !Flag) {
		PCB_PADSTACK_LOOP(pcb->Data);
		{
			if (pcb_pstk_near_box(padstack, Box, NULL)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, padstack) != Flag) {
				append(PCB_OBJ_PSTK, padstack, padstack);
				if ((len != NULL) && pcb->pstk_on)
					pcb_pstk_invalidate_draw(padstack);
			}
		}
		PCB_END_LOOP;
	}

	if ((len != NULL) && changed) {
		pcb_draw();
		pcb_undo_inc_serial();
	}

	if (len == NULL) {
		static long int non_zero;
		return (changed ? &non_zero : NULL);
	}
	else {
		*len = used;
		return list;
	}
}

#undef append

static int pcb_obj_near_box(pcb_any_obj_t *obj, rnd_box_t *box)
{
	switch(obj->type) {
		case PCB_OBJ_RAT:
		case PCB_OBJ_LINE: return PCB_LINE_NEAR_BOX((pcb_line_t *)obj, box);
		case PCB_OBJ_TEXT: return PCB_TEXT_NEAR_BOX((pcb_text_t *)obj, box);
		case PCB_OBJ_POLY: return PCB_POLYGON_NEAR_BOX((pcb_poly_t *)obj, box);
		case PCB_OBJ_ARC:  return PCB_ARC_NEAR_BOX((pcb_arc_t *)obj, box);
		case PCB_OBJ_GFX:  return PCB_GFX_NEAR_BOX((pcb_gfx_t *)obj, box);
		case PCB_OBJ_PSTK: return pcb_pstk_near_box((pcb_pstk_t *)obj, box, NULL);
		case PCB_OBJ_SUBC: return PCB_SUBC_NEAR_BOX((pcb_subc_t *)obj, box);
		default: return 0;
	}
}

typedef struct {
	pcb_board_t *pcb;
	rnd_box_t box;
	rnd_bool flag;
	rnd_bool invert;
} select_ctx_t;

static rnd_r_dir_t pcb_select_block_cb(const rnd_box_t *box, void *cl)
{
	select_ctx_t *ctx = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box;

	if (!ctx->invert && (PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj) == ctx->flag)) /* cheap check on the flag: don't do anything if the flag is already right */
		return RND_R_DIR_NOT_FOUND;

	/* do not let locked object selected, but allow deselection */
	if ((PCB_FLAG_TEST(PCB_FLAG_LOCK, obj) == rnd_true) && (ctx->flag))
		return RND_R_DIR_NOT_FOUND;

	if (!pcb_obj_near_box(obj, &ctx->box)) /* detailed box matching */
		return RND_R_DIR_NOT_FOUND;

	pcb_undo_add_obj_to_flag((void *)obj);
	pcb_draw_obj(obj);
	if (ctx->invert)
		PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, obj);
	else
		PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, ctx->flag, obj);
	pcb_extobj_sync_floater_flags(ctx->pcb, obj, 1, 1);
	return RND_R_DIR_FOUND_CONTINUE;
}

/* ----------------------------------------------------------------------
 * selects/unselects all visible objects within the passed box
 * Flag determines if the block is to be selected or unselected
 * returns rnd_true if the state of any object has changed
 */
rnd_bool pcb_select_block(pcb_board_t *pcb, rnd_box_t *Box, rnd_bool flag, rnd_bool vis_only, rnd_bool invert)
{
	select_ctx_t ctx;

	fix_box_dir(Box, 0);

	ctx.pcb = pcb;
	ctx.box = *Box;
	ctx.flag = flag;
	ctx.invert = invert;

	fix_box_dir(Box, 1);

	return pcb_data_r_search(pcb->Data, PCB_OBJ_ANY, Box, NULL, pcb_select_block_cb, &ctx, NULL, vis_only) == RND_R_DIR_FOUND_CONTINUE;
}

/* ----------------------------------------------------------------------
 * List all visible objects within the passed box
 */
long int *pcb_list_block(pcb_board_t *pcb, rnd_box_t *Box, int *len)
{
	return ListBlock_(pcb, Box, 1, PCB_LYT_ANYWHERE|PCB_LYT_ANYTHING, len, NULL, NULL);
}

int pcb_list_block_cb(pcb_board_t *pcb, rnd_box_t *Box, void *(cb)(void *ctx, pcb_any_obj_t *obj), void *ctx)
{
	int len = 0;
	ListBlock_(pcb, Box, -1, PCB_LYT_ANYWHERE|PCB_LYT_ANYTHING, &len, cb, ctx);
	return len;
}

int pcb_list_lyt_block_cb(pcb_board_t *pcb, pcb_layer_type_t lyt, rnd_box_t *Box, void *(cb)(void *ctx, pcb_any_obj_t *obj), void *ctx)
{
	int len = 0;
	ListBlock_(pcb, Box, -1, lyt, ((cb == NULL) ? &len : NULL), cb, ctx);
	return len;
}

/* ----------------------------------------------------------------------
 * selects/unselects all objects which were found during a connection scan
 * Flag determines if they are to be selected or unselected
 * returns rnd_true if the state of any object has changed
 *
 * text objects and subcircuits cannot be selected by this routine
 */
static rnd_bool pcb_select_connection_(pcb_data_t *data, rnd_bool Flag)
{
	rnd_bool changed = rnd_false;
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	for(o = pcb_data_first(&it, data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, o)) {
			pcb_undo_add_obj_to_flag(o);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, o);
			pcb_draw_obj(o);
			changed = rnd_true;
		}
		if ((o->type == PCB_OBJ_SUBC) && (pcb_select_connection_(((pcb_subc_t *)o)->data, Flag)))
			changed = rnd_true;
	}

	return changed;
}

rnd_bool pcb_select_connection(pcb_board_t *pcb, rnd_bool Flag)
{
	return pcb_select_connection_(pcb->Data, Flag);
}

/* ---------------------------------------------------------------------------
 * selects objects as defined by Type by name;
 * it's a case insensitive match
 * returns rnd_true if any object has been selected
 */
#define REGEXEC(arg) \
	(method == PCB_SM_REGEX ? regexec_match_all(regex, (arg)) : strlst_match(pat, (arg)))

static int regexec_match_all(re_sei_t *preg, const char *string)
{
	return !!re_sei_exec(preg, string);
}

/* case insensitive match of each item in the array pat against name
   returns 1 if any of them matched */
static int strlst_match(const char **pat, const char *name)
{
	for (; *pat != NULL; pat++)
		if (rnd_strcasecmp(*pat, name) == 0)
			return 1;
	return 0;
}
