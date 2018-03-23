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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: pins and vias */

#include "config.h"
#include "board.h"
#include "data.h"
#include "undo.h"
#include "conf_core.h"
#include "polygon.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "stub_vendor.h"
#include "rotate.h"

#include "obj_pinvia.h"
#include "obj_pinvia_op.h"
#include "obj_subc_parent.h"
#include "obj_hash.h"

/* TODO: consider removing this by moving pin/via functions here: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_pinvia_draw.h"

#include "obj_elem.h"

/*** utility ***/

/* sets the bounding box of a pin or via */
void pcb_pin_bbox(pcb_pin_t *Pin)
{
	pcb_coord_t width;

	if ((PCB_FLAG_SQUARE_GET(Pin) > 1) && (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin)))
		abort();

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = MAX(Pin->Clearance + PIN_SIZE(Pin), Pin->Mask) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *PCB_POLY_CIRC_RADIUS_ADJ + 0.5;

	Pin->BoundingBox.X1 = Pin->X - width;
	Pin->BoundingBox.Y1 = Pin->Y - width;
	Pin->BoundingBox.X2 = Pin->X + width;
	Pin->BoundingBox.Y2 = Pin->Y + width;
	pcb_close_box(&Pin->BoundingBox);
}

void pcb_pin_copper_bbox(pcb_box_t *out, pcb_pin_t *Pin)
{
	pcb_coord_t width;

	if ((PCB_FLAG_SQUARE_GET(Pin) > 1) && (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin)))
		abort();

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = PIN_SIZE(Pin) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *PCB_POLY_CIRC_RADIUS_ADJ + 0.5;

	out->X1 = Pin->X - width;
	out->Y1 = Pin->Y - width;
	out->X2 = Pin->X + width;
	out->Y2 = Pin->Y + width;
	pcb_close_box(out);
}

void pcb_via_rotate(pcb_data_t *Data, pcb_pin_t *Via, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina)
{
	pcb_r_delete_entry(Data->via_tree, (pcb_box_t *) Via);
	pcb_rotate(&Via->X, &Via->Y, X, Y, cosa, sina);
	pcb_pin_bbox(Via);
	pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) Via);
}

void pcb_via_mirror(pcb_data_t *Data, pcb_pin_t *via, pcb_coord_t y_offs)
{
	if (Data->via_tree != NULL)
		pcb_r_delete_entry(Data->via_tree, (pcb_box_t *) via);
	via->X = PCB_SWAP_X(via->X);
	via->Y = PCB_SWAP_Y(via->Y) + y_offs;
	pcb_pin_bbox(via);
	if (Data->via_tree != NULL)
		pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) via);
}

void pcb_via_flip_side(pcb_data_t *Data, pcb_pin_t *via)
{
	pcb_r_delete_entry(Data->via_tree, (pcb_box_t *) via);
	via->X = PCB_SWAP_X(via->X);
	via->Y = PCB_SWAP_Y(via->Y);
	pcb_pin_bbox(via);
	pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) via);
}

int pcb_pin_eq(const pcb_element_t *e1, const pcb_pin_t *p1, const pcb_element_t *e2, const pcb_pin_t *p2)
{
	if (pcb_field_neq(p1, p2, Thickness) || pcb_field_neq(p1, p2, Clearance)) return 0;
	if (pcb_field_neq(p1, p2, Mask) || pcb_field_neq(p1, p2, DrillingHole)) return 0;
	if (pcb_element_neq_offsx(e1, p1, e2, p2, X) || pcb_element_neq_offsy(e1, p1, e2, p2, Y)) return 0;
	if (pcb_neqs(p1->Name, p2->Name)) return 0;
	if (pcb_neqs(p1->Number, p2->Number)) return 0;
	return 1;
}

int pcb_pin_eq_padstack(const pcb_pin_t *p1, const pcb_pin_t *p2)
{
	if (pcb_field_neq(p1, p2, Thickness) || pcb_field_neq(p1, p2, Clearance)) return 0;
	if (pcb_field_neq(p1, p2, Mask) || pcb_field_neq(p1, p2, DrillingHole)) return 0;
	return 1;
}

unsigned int pcb_pin_hash(const pcb_element_t *e, const pcb_pin_t *p)
{
	return
		pcb_hash_coord(p->Thickness) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_coord(p->Mask) ^ pcb_hash_coord(p->DrillingHole) ^
		pcb_hash_element_ox(e, p->X) ^ pcb_hash_element_oy(e, p->Y) ^
		pcb_hash_str(p->Name) ^ pcb_hash_str(p->Number);
}

unsigned int pcb_pin_hash_padstack(const pcb_pin_t *p)
{
	return
		pcb_hash_coord(p->Thickness) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_coord(p->Mask) ^ pcb_hash_coord(p->DrillingHole);
}

/*** ops ***/

/* moves a via beteen board and buffer without allocating memory for the name */
void *pcb_viaop_move_buffer(pcb_opctx_t *ctx, pcb_pin_t * via)
{
	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_VIA, via, via);

	pcb_r_delete_entry(ctx->buffer.src->via_tree, (pcb_box_t *) via);
	pinlist_remove(via);
	pinlist_append(&ctx->buffer.dst->Via, via);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, via);

	if (!ctx->buffer.dst->via_tree)
		ctx->buffer.dst->via_tree = pcb_r_create_tree();
	pcb_r_insert_entry(ctx->buffer.dst->via_tree, (pcb_box_t *) via);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_VIA, via, via);
	PCB_SET_PARENT(via, data, ctx->buffer.dst);
	return via;
}

/* changes the thermal on a via */
void *pcb_viaop_change_thermal(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	pcb_undo_add_obj_to_flag(Via);
	if (!ctx->chgtherm.style)										/* remove the thermals */
		PCB_FLAG_THERM_CLEAR(INDEXOFCURRENT, Via);
	else
		PCB_FLAG_THERM_ASSIGN(INDEXOFCURRENT, ctx->chgtherm.style, Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	pcb_via_invalidate_draw(Via);
	return Via;
}

/* changes the thermal on a pin */
void *pcb_pinop_change_thermal(pcb_opctx_t *ctx, pcb_element_t *element, pcb_pin_t *Pin)
{
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, element, Pin, Pin, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	pcb_undo_add_obj_to_flag(Pin);
	if (!ctx->chgtherm.style)										/* remove the thermals */
		PCB_FLAG_THERM_CLEAR(INDEXOFCURRENT, Pin);
	else
		PCB_FLAG_THERM_ASSIGN(INDEXOFCURRENT, ctx->chgtherm.style, Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, element, Pin, Pin, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	pcb_pin_invalidate_draw(Pin);
	return Pin;
}

/* changes the size of a via */
void *pcb_viaop_change_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = ctx->chgsize.is_absolute ? ctx->chgsize.value : Via->Thickness + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return NULL;
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIASIZE && value >= Via->DrillingHole + PCB_MIN_PINORVIACOPPER && value != Via->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_VIA, Via, Via, Via);
		pcb_via_invalidate_erase(Via);
		pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Via, Via);
		if (Via->Mask) {
abort();
/*			pcb_undo_add_obj_to_mask_size(PCB_TYPE_VIA, Via, Via, Via);*/
			Via->Mask += value - Via->Thickness;
		}
		Via->Thickness = value;
		pcb_pin_bbox(Via);
		pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
		pcb_via_invalidate_draw(Via);
		return Via;
	}
	return NULL;
}

/* changes the drilling hole of a via */
void *pcb_viaop_change_2nd_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Via->DrillingHole + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return NULL;
	if (value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) || value <= Via->Thickness - PCB_MIN_PINORVIACOPPER)
			&& value != Via->DrillingHole) {
abort();
/*		pcb_undo_add_obj_to_2nd_size(PCB_TYPE_VIA, Via, Via, Via);*/
		pcb_via_invalidate_erase(Via);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
		Via->DrillingHole = value;
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via)) {
			pcb_undo_add_obj_to_size(PCB_TYPE_VIA, Via, Via, Via);
			Via->Thickness = value;
		}
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
		pcb_via_invalidate_draw(Via);
		return Via;
	}
	return NULL;
}

/* changes the drilling hole of a pin */
void *pcb_pinop_change_2nd_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Pin->DrillingHole + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return NULL;
	if (value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) || value <= Pin->Thickness - PCB_MIN_PINORVIACOPPER)
			&& value != Pin->DrillingHole) {
abort();
/*		pcb_undo_add_obj_to_2nd_size(PCB_TYPE_PIN, Element, Pin, Pin);*/
		pcb_pin_invalidate_erase(Pin);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->DrillingHole = value;
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin)) {
			pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, Pin, Pin);
			Pin->Thickness = value;
		}
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		pcb_pin_invalidate_draw(Pin);
		return Pin;
	}
	return NULL;
}


/* changes the clearance size of a via */
void *pcb_viaop_change_clear_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Via->Clearance + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return NULL;
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, value);
	if (!ctx->chgsize.is_absolute && ctx->chgsize.value < 0 && value < PCB->Bloat * 2)
		value = 0;
	if (ctx->chgsize.value > 0 && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (Via->Clearance == value)
		return NULL;
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_undo_add_obj_to_clear_size(PCB_TYPE_VIA, Via, Via, Via);
	pcb_via_invalidate_erase(Via);
	pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	Via->Clearance = value;
	pcb_pin_bbox(Via);
	pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_invalidate_draw(Via);
	Via->Element = NULL;
	return Via;
}


/* changes the size of a pin */
void *pcb_pinop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Pin->Thickness + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return NULL;
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIASIZE && value >= Pin->DrillingHole + PCB_MIN_PINORVIACOPPER && value != Pin->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, Pin, Pin);
abort();
/*		pcb_undo_add_obj_to_mask_size(PCB_TYPE_PIN, Element, Pin, Pin);*/
		pcb_pin_invalidate_erase(Pin);
		pcb_r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->Mask += value - Pin->Thickness;
		Pin->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		pcb_pin_invalidate_draw(Pin);
		return Pin;
	}
	return NULL;
}

/* changes the clearance size of a pin */
void *pcb_pinop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Pin->Clearance + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return NULL;
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, value);
	if (!ctx->chgsize.is_absolute && ctx->chgsize.value < 0 && value < PCB->Bloat * 2)
		value = 0;
	if (ctx->chgsize.value > 0 && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (Pin->Clearance == value)
		return NULL;
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_undo_add_obj_to_clear_size(PCB_TYPE_PIN, Element, Pin, Pin);
	pcb_pin_invalidate_erase(Pin);
	pcb_r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
	Pin->Clearance = value;
	/* SetElementBB updates all associated rtrees */
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_pin_invalidate_draw(Pin);
	return Pin;
}

/* changes the name of a via */
void *pcb_viaop_change_name(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	char *old = Via->Name;

	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Via)) {
		pcb_pin_name_invalidate_erase(Via);
		Via->Name = ctx->chgname.new_name;
		pcb_pin_name_invalidate_draw(Via);
	}
	else
		Via->Name = ctx->chgname.new_name;
	return old;
}

/* changes the name of a pin */
void *pcb_pinop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	char *old = Pin->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Pin)) {
		pcb_pin_name_invalidate_erase(Pin);
		Pin->Name = ctx->chgname.new_name;
		pcb_pin_name_invalidate_draw(Pin);
	}
	else
		Pin->Name = ctx->chgname.new_name;
	return old;
}

/* changes the square flag of a via */
void *pcb_viaop_change_square(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return NULL;
	pcb_via_invalidate_erase(Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, NULL, Via, Via, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	pcb_undo_add_obj_to_flag(Via);
	PCB_FLAG_SQUARE_ASSIGN(ctx->chgsize.value, Via);
	if (ctx->chgsize.value == 0)
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, Via);
	else
		PCB_FLAG_SET(PCB_FLAG_SQUARE, Via);
	pcb_pin_bbox(Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, NULL, Via, Via, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	pcb_via_invalidate_draw(Via);
	return Via;
}

/* changes the square flag of a pin */
void *pcb_pinop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return NULL;
	pcb_pin_invalidate_erase(Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_undo_add_obj_to_flag(Pin);
	PCB_FLAG_SQUARE_ASSIGN(ctx->chgsize.value, Pin);
	if (ctx->chgsize.value == 0)
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, Pin);
	else
		PCB_FLAG_SET(PCB_FLAG_SQUARE, Pin);
	pcb_pin_bbox(Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_pin_invalidate_draw(Pin);
	return Pin;
}

/* sets the square flag of a pin */
void *pcb_pinop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		return NULL;

	return (pcb_pinop_change_square(ctx, Element, Pin));
}

/* clears the square flag of a pin */
void *pcb_pinop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || !PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		return NULL;

	return (pcb_pinop_change_square(ctx, Element, Pin));
}

/* changes the octagon flag of a via */
void *pcb_viaop_change_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return NULL;
	pcb_via_invalidate_erase(Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_undo_add_obj_to_flag(Via);
	PCB_FLAG_TOGGLE(PCB_FLAG_OCTAGON, Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_invalidate_draw(Via);
	return Via;
}

/* sets the octagon flag of a via */
void *pcb_viaop_set_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via) || PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Via))
		return NULL;

	return (pcb_viaop_change_octagon(ctx, Via));
}

/* clears the octagon flag of a via */
void *pcb_viaop_clear_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via) || !PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Via))
		return NULL;

	return (pcb_viaop_change_octagon(ctx, Via));
}

/* changes the octagon flag of a pin */
void *pcb_pinop_change_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return NULL;
	pcb_pin_invalidate_erase(Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_undo_add_obj_to_flag(Pin);
	PCB_FLAG_TOGGLE(PCB_FLAG_OCTAGON, Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_pin_invalidate_draw(Pin);
	return Pin;
}

/* sets the octagon flag of a pin */
void *pcb_pinop_set_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Pin))
		return NULL;

	return (pcb_pinop_change_octagon(ctx, Element, Pin));
}

/* clears the octagon flag of a pin */
void *pcb_pinop_clear_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || !PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Pin))
		return NULL;

	return (pcb_pinop_change_octagon(ctx, Element, Pin));
}

/* changes the hole flag of a via */
pcb_bool pcb_pin_change_hole(pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return pcb_false;
	pcb_via_invalidate_erase(Via);
	pcb_undo_add_obj_to_flag(Via);
abort();
/*	pcb_undo_add_obj_to_mask_size(PCB_TYPE_VIA, Via, Via, Via);*/
	pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	PCB_FLAG_TOGGLE(PCB_FLAG_HOLE, Via);

	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via)) {
		/* A tented via becomes an minimally untented hole.  An untented
		   via retains its mask clearance.  */
		if (Via->Mask > Via->Thickness) {
			Via->Mask = (Via->DrillingHole + (Via->Mask - Via->Thickness));
		}
		else if (Via->Mask < Via->DrillingHole) {
			Via->Mask = Via->DrillingHole + 2 * PCB_MASKFRAME;
		}
	}
	else {
		Via->Mask = (Via->Thickness + (Via->Mask - Via->DrillingHole));
	}

	pcb_pin_bbox(Via);
	pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_invalidate_draw(Via);
	pcb_draw();
	return pcb_true;
}

/* changes the mask size of a pin */
void *pcb_pinop_change_mask_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Pin->Mask + ctx->chgsize.value;

	value = MAX(value, 0);
	if (value == Pin->Mask && ctx->chgsize.value == 0)
		value = Pin->Thickness;
	if (value != Pin->Mask) {
abort();
/*		pcb_undo_add_obj_to_mask_size(PCB_TYPE_PIN, Element, Pin, Pin);*/
		pcb_pin_invalidate_erase(Pin);
		pcb_r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		Pin->Mask = value;
		pcb_pin_invalidate_draw(Pin);
		return Pin;
	}
	return NULL;
}

/* changes the mask size of a via */
void *pcb_viaop_change_mask_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value;

	value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : Via->Mask + ctx->chgsize.value;
	value = MAX(value, 0);
	if (value != Via->Mask) {
abort();
/*		pcb_undo_add_obj_to_mask_size(PCB_TYPE_VIA, Via, Via, Via);*/
		pcb_via_invalidate_erase(Via);
		pcb_r_delete_entry(PCB->Data->via_tree, &Via->BoundingBox);
		Via->Mask = value;
		pcb_pin_bbox(Via);
		pcb_r_insert_entry(PCB->Data->via_tree, &Via->BoundingBox);
		pcb_via_invalidate_draw(Via);
		return Via;
	}
	return NULL;
}

/* moves a via */
void *pcb_viaop_move_noclip(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB->ViaOn)
		pcb_via_invalidate_erase(Via);
	pcb_via_move(Via, ctx->move.dx, ctx->move.dy);
	if (PCB->ViaOn)
		pcb_via_invalidate_draw(Via);
	return Via;
}

void *pcb_viaop_move(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_viaop_move_noclip(ctx, Via);
	pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	return Via;
}

void *pcb_viaop_clip(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (ctx->clip.restore) {
		pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	}
	return Via;
}


/* destroys a via */

/* removes a via */
void *pcb_viaop_remove(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	/* erase from screen and memory */
	if (PCB->ViaOn)
		pcb_via_invalidate_erase(Via);
	pcb_undo_move_obj_to_remove(PCB_TYPE_VIA, Via, Via, Via);
	return NULL;
}

void *pcb_viaop_rotate90(pcb_opctx_t *ctx, pcb_pin_t *via)
{
	pcb_data_t *data = via->parent.data;
	assert(via->parent_type == PCB_PARENT_DATA);

	pcb_via_invalidate_erase(via);
	if (data->via_tree != NULL) {
		pcb_poly_restore_to_poly(ctx->rotate.pcb->Data, PCB_TYPE_VIA, via, via);
		pcb_r_delete_entry(data->via_tree, (pcb_box_t *)via);
	}
	PCB_VIA_ROTATE90(via, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	pcb_pin_bbox(via);
	if (data->via_tree != NULL) {
		pcb_r_insert_entry(data->via_tree, (pcb_box_t *)via);
		pcb_poly_clear_from_poly(ctx->rotate.pcb->Data, PCB_TYPE_VIA, via, via);
	}
	pcb_via_invalidate_draw(via);
	return via;
}

void *pcb_viaop_rotate(pcb_opctx_t *ctx, pcb_pin_t *via)
{
	pcb_data_t *data = via->parent.data;
	assert(via->parent_type == PCB_PARENT_DATA);

	pcb_via_invalidate_erase(via);
	if (data->via_tree != NULL) {
		pcb_poly_restore_to_poly(data, PCB_TYPE_VIA, via, via);
		pcb_r_delete_entry(data->via_tree, (pcb_box_t *)via);
	}

	pcb_rotate(&via->X, &via->Y, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina);

	pcb_pin_bbox(via);
	if (data->via_tree != NULL) {
		pcb_r_insert_entry(data->via_tree, (pcb_box_t *)via);
		pcb_poly_clear_from_poly(data, PCB_TYPE_VIA, via, via);
	}
	pcb_via_invalidate_draw(via);
	return via;
}

void *pcb_viaop_change_flag(pcb_opctx_t *ctx, pcb_pin_t *pin)
{
	static pcb_flag_values_t pcb_pin_flags = 0;
	if (pcb_pin_flags == 0)
		pcb_pin_flags = pcb_obj_valid_flags(PCB_TYPE_PIN);

	if ((ctx->chgflag.flag & pcb_pin_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (pin->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(pin);
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, pin);
	return pin;
}

void *pcb_pinop_change_flag(pcb_opctx_t *ctx, pcb_element_t *elem, pcb_pin_t *pin)
{
	return pcb_viaop_change_flag(ctx, pin);
}

void *pcb_pinop_invalidate_label(pcb_opctx_t *ctx, pcb_pin_t *pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, pin))
		pcb_pin_name_invalidate_erase(pin);
	else
		pcb_pin_name_invalidate_draw(pin);
	return pin;
}


/*** draw ***/
static void GatherPVName(pcb_pin_t *Ptr)
{
	pcb_box_t box;
	pcb_bool vert = PCB_FLAG_TEST(PCB_FLAG_EDGE2, Ptr);

	if (vert) {
		box.X1 = Ptr->X - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.Y1 = Ptr->Y - Ptr->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = Ptr->X + Ptr->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		box.Y1 = Ptr->Y - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
	}

	if (vert) {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	else {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	pcb_draw_invalidate(&box);
}

void pcb_via_invalidate_erase(pcb_pin_t *Via)
{
	pcb_draw_invalidate(Via);
	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Via))
		pcb_via_name_invalidate_erase(Via);
	pcb_flag_erase(&Via->Flags);
}

void pcb_via_name_invalidate_erase(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void pcb_pin_invalidate_erase(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if (PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Pin))
		pcb_pin_name_invalidate_erase(Pin);
	pcb_flag_erase(&Pin->Flags);
}

void pcb_pin_name_invalidate_erase(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}

void pcb_via_invalidate_draw(pcb_pin_t *Via)
{
	pcb_draw_invalidate(Via);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Via))
		pcb_via_name_invalidate_draw(Via);
}

void pcb_via_name_invalidate_draw(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void pcb_pin_invalidate_draw(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if ((!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && PCB_FLAG_TEST(PCB_FLAG_TERMNAME, Pin))
			|| pcb_draw_doing_pinout)
		pcb_pin_name_invalidate_draw(Pin);
}

void pcb_pin_name_invalidate_draw(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}
