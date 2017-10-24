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


/* select routines
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "polygon.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "find.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "obj_elem_draw.h"
#include "obj_pad_draw.h"
#include "obj_arc_draw.h"
#include "obj_pinvia_draw.h"
#include "obj_line_draw.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"
#include "obj_rat_draw.h"
#include "obj_padstack_draw.h"

#include <genregex/regex_sei.h>

void pcb_select_element(pcb_board_t *pcb, pcb_element_t *element, pcb_change_flag_t how, int redraw)
{
	/* select all pins and names of the element */
	PCB_PIN_LOOP(element);
	{
		pcb_undo_add_obj_to_flag(pin);
		PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(element);
	{
		pcb_undo_add_obj_to_flag(pad);
		PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, pad);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_PCB_TEXT_LOOP(element);
	{
		pcb_undo_add_obj_to_flag(text);
		PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, text);
	}
	PCB_END_LOOP;
	pcb_undo_add_obj_to_flag(element);
	PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, element);

	if (redraw) {
		if (pcb_silk_on(pcb) && ((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element) != 0) == PCB_SWAP_IDENT || pcb->InvisibleObjectsOn))
			if (pcb_silk_on(pcb)) {
				pcb_elem_name_invalidate_draw(element);
				pcb_elem_package_invalidate_draw(element);
			}
		if (pcb->PinOn)
			pcb_elem_pp_invalidate_draw(element);
	}
}

void pcb_select_element_name(pcb_element_t *element, pcb_change_flag_t how, int redraw)
{
	/* select all names of the element */
	PCB_ELEMENT_PCB_TEXT_LOOP(element);
	{
		pcb_undo_add_obj_to_flag(text);
		PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, text);
	}
	PCB_END_LOOP;

	if (redraw)
		pcb_elem_name_invalidate_draw(element);
}


/* ---------------------------------------------------------------------------
 * toggles the selection of any kind of object
 * the different types are defined by search.h
 */
pcb_bool pcb_select_object(pcb_board_t *pcb)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_layer_t *layer;
	int type;

	pcb_bool changed = pcb_true;

	type = pcb_search_screen(pcb_crosshair.X, pcb_crosshair.Y, PCB_SELECT_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_TYPE_NONE || PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_pin_t *) ptr2))
		return (pcb_false);
	switch (type) {
	case PCB_TYPE_VIA:
		pcb_undo_add_obj_to_flag(ptr1);
		PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, (pcb_pin_t *) ptr1);
		pcb_via_invalidate_draw((pcb_pin_t *) ptr1);
		break;

	case PCB_TYPE_PADSTACK:
		pcb_undo_add_obj_to_flag(ptr1);
		PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, (pcb_padstack_t *) ptr1);
		pcb_padstack_invalidate_draw((pcb_padstack_t *) ptr1);
		break;

	case PCB_TYPE_LINE:
		{
			pcb_line_t *line = (pcb_line_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, line);
			pcb_line_invalidate_draw(layer, line);
			break;
		}

	case PCB_TYPE_RATLINE:
		{
			pcb_rat_t *rat = (pcb_rat_t *) ptr2;

			pcb_undo_add_obj_to_flag(ptr1);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, rat);
			pcb_rat_invalidate_draw(rat);
			break;
		}

	case PCB_TYPE_ARC:
		{
			pcb_arc_t *arc = (pcb_arc_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, arc);
			pcb_arc_invalidate_draw(layer, arc);
			break;
		}

	case PCB_TYPE_TEXT:
		{
			pcb_text_t *text = (pcb_text_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, text);
			pcb_text_invalidate_draw(layer, text);
			break;
		}

	case PCB_TYPE_POLYGON:
		{
			pcb_poly_t *poly = (pcb_poly_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, poly);
			pcb_poly_invalidate_draw(layer, poly);
			/* changing memory order no longer effects draw order */
			break;
		}

	case PCB_TYPE_PIN:
		pcb_undo_add_obj_to_flag(ptr2);
		PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, (pcb_pin_t *) ptr2);
		pcb_pin_invalidate_draw((pcb_pin_t *) ptr2);
		break;

	case PCB_TYPE_PAD:
		pcb_undo_add_obj_to_flag(ptr2);
		PCB_FLAG_TOGGLE(PCB_FLAG_SELECTED, (pcb_pad_t *) ptr2);
		pcb_pad_invalidate_draw((pcb_pad_t *) ptr2);
		break;

	case PCB_TYPE_ELEMENT_NAME:
		pcb_select_element_name((pcb_element_t *) ptr1, PCB_CHGFLG_TOGGLE, 1);
		break;

	case PCB_TYPE_ELEMENT:
		pcb_select_element(pcb, (pcb_element_t *) ptr1, PCB_CHGFLG_TOGGLE, 1);
		break;

	case PCB_TYPE_SUBC:
		pcb_subc_select(pcb, (pcb_subc_t *) ptr1, PCB_CHGFLG_TOGGLE, 1);
		break;
	}
	pcb_draw();
	pcb_undo_inc_serial();
	return (changed);
}

static void fix_box_dir(pcb_box_t *Box, int force_pos)
{
#define swap(a,b) \
do { \
	pcb_coord_t tmp; \
	tmp = a; \
	a = b; \
	b = tmp; \
} while(0)

	/* If board view is flipped, box coords need to be flipped too to reflect
	   the on-screen direction of draw */
	if (conf_core.editor.view.flip_x)
		swap(Box->X1, Box->X2);
	if (conf_core.editor.view.flip_y)
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
 */
#warning cleanup TODO: should be rewritten with generic ops and rtree
static long int *ListBlock_(pcb_board_t *pcb, pcb_box_t *Box, pcb_bool Flag, int *len)
{
	int changed = 0;
	int used = 0, alloced = 0;
	long int *list = NULL;

	fix_box_dir(Box, 0);
/*pcb_printf("box: %mm %mm - %mm %mm    [ %d ] %d %d\n", Box->X1, Box->Y1, Box->X2, Box->Y2, PCB_IS_BOX_NEGATIVE(Box), conf_core.editor.view.flip_x, conf_core.editor.view.flip_y);*/

/* append an object to the return list OR set the flag if there's no list */
#define append(undo_type, p1, obj) \
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

	if (PCB_IS_BOX_NEGATIVE(Box) && ((Box->X1 == Box->X2) || (Box->Y2 == Box->Y1))) {
		if (len != NULL)
			*len = 0;
		return NULL;
	}

	if (pcb->RatOn || !Flag)
		PCB_RAT_LOOP(pcb->Data);
	{
		if (PCB_LINE_NEAR_BOX((pcb_line_t *) line, Box) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, line) != Flag) {
			append(PCB_TYPE_RATLINE, line, line);
			if (pcb->RatOn)
				pcb_rat_invalidate_draw(line);
		}
	}
	PCB_END_LOOP;

	/* check layers */
	LAYER_LOOP(pcb->Data, pcb_max_layer);
	{
		unsigned int lflg = pcb_layer_flags(pcb, n);

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
				append(PCB_TYPE_LINE, layer, line);
				if (layer->meta.real.vis)
					pcb_line_invalidate_draw(layer, line);
			}
		}
		PCB_END_LOOP;
		PCB_ARC_LOOP(layer);
		{
			if (PCB_ARC_NEAR_BOX(arc, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc) != Flag) {
				append(PCB_TYPE_ARC, layer, arc);
				if (layer->meta.real.vis)
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
					append(PCB_TYPE_TEXT, layer, text);
					if (pcb_text_is_visible(PCB, layer, text))
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
				append(PCB_TYPE_POLYGON, layer, polygon);
				if (layer->meta.real.vis)
					pcb_poly_invalidate_draw(layer, polygon);
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
				changed = 1;
				DrawSubc(subc);
			}
		}
		PCB_END_LOOP;
	}

	/* elements */
	PCB_ELEMENT_LOOP(pcb->Data);
	{
		{
			pcb_bool gotElement = pcb_false;
			if ((pcb_silk_on(pcb) || !Flag)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, element)
					&& ((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element) != 0) == PCB_SWAP_IDENT || pcb->InvisibleObjectsOn)) {
				if (PCB_BOX_NEAR_BOX(&PCB_ELEM_TEXT_VISIBLE(PCB, element).BoundingBox, Box)
						&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, &PCB_ELEM_TEXT_VISIBLE(PCB, element))
						&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, &PCB_ELEM_TEXT_VISIBLE(PCB, element)) != Flag) {
					/* select all names of element */
					PCB_ELEMENT_PCB_TEXT_LOOP(element);
					{
						append(PCB_TYPE_ELEMENT_NAME, element, text);
					}
					PCB_END_LOOP;
					if (pcb_silk_on(pcb))
						pcb_elem_name_invalidate_draw(element);
				}
				if ((pcb->PinOn || !Flag) && PCB_ELEMENT_NEAR_BOX(element, Box))
					if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, element) != Flag) {
						append(PCB_TYPE_ELEMENT, element, element);
						PCB_PIN_LOOP(element);
						{
							if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pin) != Flag) {
								append(PCB_TYPE_PIN, element, pin);
								if (pcb->PinOn)
									pcb_pin_invalidate_draw(pin);
							}
						}
						PCB_END_LOOP;
						PCB_PAD_LOOP(element);
						{
							if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pad) != Flag) {
								append(PCB_TYPE_PAD, element, pad);
								if (pcb->PinOn)
									pcb_pad_invalidate_draw(pad);
							}
						}
						PCB_END_LOOP;
						if (pcb->PinOn)
							pcb_elem_invalidate_draw(element);
						gotElement = pcb_true;
					}
			}
			if ((pcb->PinOn || !Flag) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, element) && !gotElement) {
				PCB_PIN_LOOP(element);
				{
					if ((PCB_VIA_OR_PIN_NEAR_BOX(pin, Box)
							 && PCB_FLAG_TEST(PCB_FLAG_SELECTED, pin) != Flag)) {
						append(PCB_TYPE_PIN, element, pin);
						if (pcb->PinOn)
							pcb_pin_invalidate_draw(pin);
					}
				}
				PCB_END_LOOP;
				PCB_PAD_LOOP(element);
				{
					if (PCB_PAD_NEAR_BOX(pad, Box)
							&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, pad) != Flag
							&& (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) == PCB_SWAP_IDENT || pcb->InvisibleObjectsOn || !Flag)) {
						append(PCB_TYPE_PAD, element, pad);
						if (pcb->PinOn)
							pcb_pad_invalidate_draw(pad);
					}
				}
				PCB_END_LOOP;
			}
		}
	}
	PCB_END_LOOP;

	/* end with vias */
	if (pcb->ViaOn || !Flag) {
		PCB_VIA_LOOP(pcb->Data);
		{
			if (PCB_VIA_OR_PIN_NEAR_BOX(via, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, via)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, via) != Flag) {
				append(PCB_TYPE_VIA, via, via);
				if (pcb->ViaOn)
					pcb_via_invalidate_draw(via);
			}
		}
		PCB_END_LOOP;

		PCB_PADSTACK_LOOP(pcb->Data);
		{
			if (pcb_padstack_near_box(padstack, Box)
					&& !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack)
					&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, padstack) != Flag) {
				append(PCB_TYPE_PADSTACK, padstack, padstack);
				if (pcb->ViaOn)
					pcb_padstack_invalidate_draw(padstack);
			}
		}
		PCB_END_LOOP;

	}

	if (changed) {
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

static int pcb_obj_near_box(pcb_any_obj_t *obj, pcb_box_t *box)
{
	switch(obj->type) {
		case PCB_OBJ_RAT:
		case PCB_OBJ_LINE: return PCB_LINE_NEAR_BOX((pcb_line_t *)obj, box);
		case PCB_OBJ_TEXT: return PCB_TEXT_NEAR_BOX((pcb_text_t *)obj, box);
		case PCB_OBJ_POLYGON: return PCB_POLYGON_NEAR_BOX((pcb_poly_t *)obj, box);
		case PCB_OBJ_ARC:  return PCB_ARC_NEAR_BOX((pcb_arc_t *)obj, box);
		case PCB_OBJ_PAD:  return PCB_PAD_NEAR_BOX((pcb_pad_t *)obj, box);
		case PCB_OBJ_PADSTACK: return pcb_padstack_near_box((pcb_padstack_t *)obj, box);
		case PCB_OBJ_PIN:
		case PCB_OBJ_VIA:  return PCB_VIA_OR_PIN_NEAR_BOX((pcb_pin_t *)obj, box);
		case PCB_OBJ_ELEMENT: return PCB_ELEMENT_NEAR_BOX((pcb_element_t *)obj, box);
		case PCB_OBJ_SUBC: return PCB_SUBC_NEAR_BOX((pcb_subc_t *)obj, box);
		default: return 0;
	}
}

typedef struct {
	pcb_box_t box;
	pcb_bool flag;
} select_ctx_t;

static pcb_r_dir_t pcb_select_block_cb(const pcb_box_t *box, void *cl)
{
	select_ctx_t *ctx = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box;

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj) == ctx->flag) /* cheap check on the flag: don't do anything if the flag is already right */
		return PCB_R_DIR_NOT_FOUND;

	if (!pcb_obj_near_box(obj, &ctx->box)) /* detailed box matching */
		return PCB_R_DIR_NOT_FOUND;

	pcb_undo_add_obj_to_flag((void *)obj);
	PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, ctx->flag, obj);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* ----------------------------------------------------------------------
 * selects/unselects all visible objects within the passed box
 * Flag determines if the block is to be selected or unselected
 * returns pcb_true if the state of any object has changed
 */
pcb_bool pcb_select_block(pcb_board_t *pcb, pcb_box_t *Box, pcb_bool flag)
{
	select_ctx_t ctx;

	ctx.box = *Box;
	ctx.flag = flag;

	fix_box_dir(Box, 1);

	return pcb_data_r_search(pcb->Data, PCB_OBJ_ANY, Box, NULL, pcb_select_block_cb, &ctx, NULL) == PCB_R_DIR_FOUND_CONTINUE;
}

/* ----------------------------------------------------------------------
 * List all visible objects within the passed box
 */
long int *pcb_list_block(pcb_board_t *pcb, pcb_box_t *Box, int *len)
{
	return ListBlock_(pcb, Box, 1, len);
}

/* ----------------------------------------------------------------------
 * selects/unselects all objects which were found during a connection scan
 * Flag determines if they are to be selected or unselected
 * returns pcb_true if the state of any object has changed
 *
 * text objects and elements cannot be selected by this routine
 */
pcb_bool pcb_select_connection(pcb_board_t *pcb, pcb_bool Flag)
{
	pcb_bool changed = pcb_false;

	if (pcb->RatOn)
		PCB_RAT_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, line)) {
			pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, line);
			pcb_rat_invalidate_draw(line);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;

	PCB_LINE_VISIBLE_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, line) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line)) {
			pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, line);
			pcb_line_invalidate_draw(layer, line);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_VISIBLE_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc)) {
			pcb_undo_add_obj_to_flag(arc);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, arc);
			pcb_arc_invalidate_draw(layer, arc);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_VISIBLE_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, polygon) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, polygon)) {
			pcb_undo_add_obj_to_flag(polygon);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, polygon);
			pcb_poly_invalidate_draw(layer, polygon);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;

	if (pcb->PinOn && pcb_silk_on(pcb)) {
		PCB_PIN_ALL_LOOP(pcb->Data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, element) && PCB_FLAG_TEST(PCB_FLAG_FOUND, pin)) {
				pcb_undo_add_obj_to_flag(pin);
				PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, pin);
				pcb_pin_invalidate_draw(pin);
				changed = pcb_true;
			}
		}
		PCB_ENDALL_LOOP;
		PCB_PAD_ALL_LOOP(pcb->Data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, element) && PCB_FLAG_TEST(PCB_FLAG_FOUND, pad)) {
				pcb_undo_add_obj_to_flag(pad);
				PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, pad);
				pcb_pad_invalidate_draw(pad);
				changed = pcb_true;
			}
		}
		PCB_ENDALL_LOOP;
	}

	if (pcb->ViaOn)
		PCB_VIA_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, via) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, via)) {
			pcb_undo_add_obj_to_flag(via);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, via);
			pcb_via_invalidate_draw(via);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	return (changed);
}

/* ---------------------------------------------------------------------------
 * selects objects as defined by Type by name;
 * it's a case insensitive match
 * returns pcb_true if any object has been selected
 */
#define REGEXEC(arg) \
	(method == PCB_SM_REGEX ? regexec_match_all(regex, (arg)) : strlst_match(pat, (arg)))

static int regexec_match_all(re_sei_t *preg, const char *string)
{
	return !!re_sei_exec(preg, string);
}

/* case insensitive match of each element in the array pat against name
   returns 1 if any of them matched */
static int strlst_match(const char **pat, const char *name)
{
	for (; *pat != NULL; pat++)
		if (pcb_strcasecmp(*pat, name) == 0)
			return 1;
	return 0;
}

pcb_bool pcb_select_object_by_name(pcb_board_t *pcb, int Type, const char *name_pattern, pcb_bool Flag, pcb_search_method_t method)
{
	pcb_bool changed = pcb_false;
	const char **pat = NULL;
	char *pattern_copy = NULL;
	re_sei_t *regex;

	if (method == PCB_SM_REGEX) {
		/* compile the regular expression */
		regex = re_sei_comp(name_pattern);
		if (re_sei_errno(regex) != 0) {
			pcb_message(PCB_MSG_ERROR, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
			re_sei_free(regex);
			return (pcb_false);
		}
	}
	else {
		char *s, *next;
		int n, w;

		/* We're going to mess with the delimiters. Create a copy. */
		pattern_copy = pcb_strdup(name_pattern);

		/* count the number of patterns */
		for (s = pattern_copy, w = 0; *s != '\0'; s++) {
			if (*s == '|')
				w++;
		}

		pat = malloc((w + 2) * sizeof(char *));	/* make room for the NULL too */
		for (s = pattern_copy, n = 0; s != NULL; s = next) {
			char *end;
/*fprintf(stderr, "S: '%s'\n", s, next);*/
			while (isspace(*s))
				s++;
			next = strchr(s, '|');
			if (next != NULL) {
				*next = '\0';
				next++;
			}
			end = s + strlen(s) - 1;
			while ((end >= s) && (isspace(*end))) {
				*end = '\0';
				end--;
			}
			if (*s != '\0') {
				pat[n] = s;
				n++;
			}
		}
		pat[n] = NULL;
	}

	/* loop over all visible objects with names */
	if (Type & PCB_TYPE_TEXT)
		PCB_TEXT_ALL_LOOP(pcb->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, text)
				&& pcb_text_is_visible(PCB, layer, text)
				&& text->TextString && REGEXEC(text->TextString)
				&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) != Flag) {
			pcb_undo_add_obj_to_flag(text);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, text);
			pcb_text_invalidate_draw(layer, text);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;

	if (pcb_silk_on(pcb) && (Type & PCB_TYPE_ELEMENT))
		PCB_ELEMENT_LOOP(pcb->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, element)
				&& ((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element) != 0) == PCB_SWAP_IDENT || pcb->InvisibleObjectsOn)
				&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, element) != Flag) {
			const char* name = PCB_ELEM_NAME_VISIBLE(PCB, element);
			if (name && REGEXEC(name)) {
				pcb_undo_add_obj_to_flag(element);
				PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, element);
				PCB_PIN_LOOP(element);
				{
					pcb_undo_add_obj_to_flag(pin);
					PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, pin);
				}
				PCB_END_LOOP;
				PCB_PAD_LOOP(element);
				{
					pcb_undo_add_obj_to_flag(pad);
					PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, pad);
				}
				PCB_END_LOOP;
				PCB_ELEMENT_PCB_TEXT_LOOP(element);
				{
					pcb_undo_add_obj_to_flag(text);
					PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, text);
				}
				PCB_END_LOOP;
				pcb_elem_name_invalidate_draw(element);
				pcb_elem_invalidate_draw(element);
				changed = pcb_true;
			}
		}
	}
	PCB_END_LOOP;
	if (pcb->PinOn && (Type & PCB_TYPE_PIN))
		PCB_PIN_ALL_LOOP(pcb->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, element)
				&& pin->Name && REGEXEC(pin->Name)
				&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, pin) != Flag) {
			pcb_undo_add_obj_to_flag(pin);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, pin);
			pcb_pin_invalidate_draw(pin);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	if (pcb->PinOn && (Type & PCB_TYPE_PAD))
		PCB_PAD_ALL_LOOP(pcb->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, element)
				&& ((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) != 0) == PCB_SWAP_IDENT || pcb->InvisibleObjectsOn)
				&& PCB_FLAG_TEST(PCB_FLAG_SELECTED, pad) != Flag)
			if (pad->Name && REGEXEC(pad->Name)) {
				pcb_undo_add_obj_to_flag(pad);
				PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, pad);
				pcb_pad_invalidate_draw(pad);
				changed = pcb_true;
			}
	}
	PCB_ENDALL_LOOP;
	if (pcb->ViaOn && (Type & PCB_TYPE_VIA))
		PCB_VIA_LOOP(pcb->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_LOCK, via)
				&& via->Name && REGEXEC(via->Name) && PCB_FLAG_TEST(PCB_FLAG_SELECTED, via) != Flag) {
			pcb_undo_add_obj_to_flag(via);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, Flag, via);
			pcb_via_invalidate_draw(via);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	if (Type & PCB_TYPE_NET) {
		pcb_conn_lookup_init();
		changed = pcb_reset_conns(pcb_true) || changed;

		PCB_MENU_LOOP(&(pcb->NetlistLib[PCB_NETLIST_EDITED]));
		{
			pcb_cardinal_t i;
			pcb_lib_entry_t *entry;
			pcb_connection_t conn;

			/* Name[0] and Name[1] are special purpose, not the actual name */
			if (menu->Name && menu->Name[0] != '\0' && menu->Name[1] != '\0' && REGEXEC(menu->Name + 2)) {
				for (i = menu->EntryN, entry = menu->Entry; i; i--, entry++)
					if (pcb_rat_seek_pad(entry, &conn, pcb_false))
						pcb_rat_find_hook(conn.obj, pcb_true, pcb_true);
			}
		}
		PCB_END_LOOP;

		changed = pcb_select_connection(pcb, Flag) || changed;
		changed = pcb_reset_conns(pcb_false) || changed;
		pcb_conn_lookup_uninit();
	}

	if (method == PCB_SM_REGEX)
		re_sei_free(regex);

	if (changed) {
		pcb_undo_inc_serial();
		pcb_draw();
	}
	if (pat != NULL)
		free(pat);
	if (pattern_copy != NULL)
		free(pattern_copy);
	return (changed);
}
