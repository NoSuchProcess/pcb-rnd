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

/* TODO: consider removing this by moving pin/via functions here: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_pinvia_draw.h"

/*** allocation ***/

/* get next slot for a via, allocates memory if necessary */
pcb_pin_t *pcb_via_alloc(pcb_data_t * data)
{
	pcb_pin_t *new_obj;

	new_obj = calloc(sizeof(pcb_pin_t), 1);
	new_obj->type = PCB_OBJ_VIA;
	PCB_SET_PARENT(new_obj, data, data);

	pinlist_append(&data->Via, new_obj);

	return new_obj;
}

void pcb_via_free(pcb_pin_t * data)
{
	pinlist_remove(data);
	free(data);
}

/* get next slot for a pin, allocates memory if necessary */
pcb_pin_t *pcb_pin_alloc(pcb_element_t * element)
{
	pcb_pin_t *new_obj;

	new_obj = calloc(sizeof(pcb_pin_t), 1);
	new_obj->type = PCB_OBJ_PIN;
	PCB_SET_PARENT(new_obj, element, element);

	pinlist_append(&element->Pin, new_obj);

	return new_obj;
}

void pcb_pin_free(pcb_pin_t * data)
{
	pinlist_remove(data);
	free(data);
}



/*** utility ***/

/* creates a new via */
pcb_pin_t *pcb_via_new(pcb_data_t *Data, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, pcb_flag_t Flags)
{
	pcb_pin_t *Via;

	if (!pcb_create_being_lenient) {
		PCB_VIA_LOOP(Data);
		{
			if (pcb_distance(X, Y, via->X, via->Y) <= via->DrillingHole / 2 + DrillingHole / 2) {
				pcb_message(PCB_MSG_WARNING, _("%m+Dropping via at %$mD because it's hole would overlap with the via "
									"at %$mD\n"), conf_core.editor.grid_unit->allow, X, Y, via->X, via->Y);
				return (NULL);					/* don't allow via stacking */
			}
		}
		PCB_END_LOOP;
	}

	Via = pcb_via_alloc(Data);

	if (!Via)
		return (Via);
	/* copy values */
	Via->X = X;
	Via->Y = Y;
	Via->Thickness = Thickness;
	Via->Clearance = Clearance;
	Via->Mask = Mask;
	Via->DrillingHole = pcb_stub_vendor_drill_map(DrillingHole);
	if (Via->DrillingHole != DrillingHole) {
		pcb_message(PCB_MSG_INFO, _("%m+Mapped via drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, Via->DrillingHole, DrillingHole);
	}

	Via->Name = pcb_strdup_null(Name);
	Via->Flags = Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_WARN, Via);
	PCB_FLAG_SET(PCB_FLAG_VIA, Via);
	Via->ID = pcb_create_ID_get();

	/*
	 * don't complain about PCB_MIN_PINORVIACOPPER on a mounting hole (pure
	 * hole)
	 */
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && (Via->Thickness < Via->DrillingHole + PCB_MIN_PINORVIACOPPER)) {
		Via->Thickness = Via->DrillingHole + PCB_MIN_PINORVIACOPPER;
		pcb_message(PCB_MSG_WARNING, _("%m+Increased via thickness to %$mS to allow enough copper"
							" at %$mD.\n"), conf_core.editor.grid_unit->allow, Via->Thickness, Via->X, Via->Y);
	}

	pcb_add_via(Data, Via);
	return (Via);
}

static pcb_pin_t *pcb_via_copy_meta(pcb_pin_t *dst, pcb_pin_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes, 0);
	if (src->Number != NULL)
		dst->Number = pcb_strdup(src->Number);
	if (src->Name != NULL)
		dst->Name = pcb_strdup(src->Name);
	return dst;
}

pcb_pin_t *pcb_via_dup(pcb_data_t *data, pcb_pin_t *src)
{
	pcb_pin_t *p = pcb_via_new(data, src->X, src->Y, src->Thickness, src->Clearance, src->Mask, src->DrillingHole, src->Name, src->Flags);
	return pcb_via_copy_meta(p, src);
}

pcb_pin_t *pcb_via_dup_at(pcb_data_t *data, pcb_pin_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_pin_t *p = pcb_via_new(data, src->X+dx, src->Y+dy, src->Thickness, src->Clearance, src->Mask, src->DrillingHole, src->Name, src->Flags);
	return pcb_via_copy_meta(p, src);
}

/* creates a new pin in an element */
pcb_pin_t *pcb_element_pin_new(pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, const char *Number, pcb_flag_t Flags)
{
	pcb_pin_t *pin = pcb_pin_alloc(Element);

	/* copy values */
	pin->X = X;
	pin->Y = Y;
	pin->Thickness = Thickness;
	pin->Clearance = Clearance;
	pin->Mask = Mask;
	pin->Name = pcb_strdup_null(Name);
	pin->Number = pcb_strdup_null(Number);
	pin->Flags = Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_WARN, pin);
	PCB_FLAG_SET(PCB_FLAG_PIN, pin);
	pin->ID = pcb_create_ID_get();
	pin->Element = Element;

	/*
	 * If there is no vendor drill map installed, this will simply
	 * return DrillingHole.
	 */
	pin->DrillingHole = pcb_stub_vendor_drill_map(DrillingHole);

	/* Unless we should not map drills on this element, map them! */
	if (pcb_stub_vendor_is_element_mappable(Element)) {
		if (pin->DrillingHole < PCB_MIN_PINORVIASIZE) {
			pcb_message(PCB_MSG_WARNING, _("%m+Did not map pin #%s (%s) drill hole because %$mS is below the minimum allowed size\n"),
							conf_core.editor.grid_unit->allow, PCB_UNKNOWN(Number), PCB_UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
		else if (pin->DrillingHole > PCB_MAX_PINORVIASIZE) {
			pcb_message(PCB_MSG_WARNING, _("%m+Did not map pin #%s (%s) drill hole because %$mS is above the maximum allowed size\n"),
							conf_core.editor.grid_unit->allow, PCB_UNKNOWN(Number), PCB_UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
		else if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)
						 && (pin->DrillingHole > pin->Thickness - PCB_MIN_PINORVIACOPPER)) {
			pcb_message(PCB_MSG_WARNING, _("%m+Did not map pin #%s (%s) drill hole because %$mS does not leave enough copper\n"),
							conf_core.editor.grid_unit->allow, PCB_UNKNOWN(Number), PCB_UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
	}
	else {
		pin->DrillingHole = DrillingHole;
	}

	if (pin->DrillingHole != DrillingHole) {
		pcb_message(PCB_MSG_INFO, _("%m+Mapped pin drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, pin->DrillingHole, DrillingHole);
	}

	return (pin);
}



void pcb_add_via(pcb_data_t *Data, pcb_pin_t *Via)
{
	pcb_pin_bbox(Via);
	if (!Data->via_tree)
		Data->via_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) Via, 0);
	PCB_SET_PARENT(Via, data, Data);
}

/* sets the bounding box of a pin or via */
void pcb_pin_bbox(pcb_pin_t *Pin)
{
	pcb_coord_t width;

	if ((PCB_FLAG_SQUARE_GET(Pin) > 1) && (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))) {
		pcb_polyarea_t *p = pcb_poly_from_pin(Pin, PIN_SIZE(Pin), Pin->Clearance);
		pcb_polyarea_bbox(p, &Pin->BoundingBox);
		pcb_polyarea_free(&p);
	}

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

	if ((PCB_FLAG_SQUARE_GET(Pin) > 1) && (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))) {
		pcb_polyarea_t *p = pcb_poly_from_pin(Pin, PIN_SIZE(Pin), 0);
		pcb_polyarea_bbox(p, out);
		pcb_polyarea_free(&p);
		return;
	}

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
	pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) Via, 0);
}

void pcb_via_mirror(pcb_data_t *Data, pcb_pin_t *via, pcb_coord_t y_offs)
{
	if (Data->via_tree != NULL)
		pcb_r_delete_entry(Data->via_tree, (pcb_box_t *) via);
	via->X = PCB_SWAP_X(via->X);
	via->Y = PCB_SWAP_Y(via->Y) + y_offs;
	pcb_pin_bbox(via);
	if (Data->via_tree != NULL)
		pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) via, 0);
}

void pcb_via_flip_side(pcb_data_t *Data, pcb_pin_t *via)
{
	pcb_r_delete_entry(Data->via_tree, (pcb_box_t *) via);
	via->X = PCB_SWAP_X(via->X);
	via->Y = PCB_SWAP_Y(via->Y);
	pcb_pin_bbox(via);
	pcb_r_insert_entry(Data->via_tree, (pcb_box_t *) via, 0);
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
/* copies a via to paste buffer */
void *pcb_viaop_add_to_buffer(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_pin_t *v = pcb_via_new(ctx->buffer.dst, Via->X, Via->Y, Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, pcb_flag_mask(Via->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg));
	return pcb_via_copy_meta(v, Via);
}

/* moves a via to paste buffer without allocating memory for the name */
void *pcb_viaop_move_to_buffer(pcb_opctx_t *ctx, pcb_pin_t * via)
{
	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_VIA, via, via);

	pcb_r_delete_entry(ctx->buffer.src->via_tree, (pcb_box_t *) via);
	pinlist_remove(via);
	pinlist_append(&ctx->buffer.dst->Via, via);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, via);

	if (!ctx->buffer.dst->via_tree)
		ctx->buffer.dst->via_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(ctx->buffer.dst->via_tree, (pcb_box_t *) via, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_VIA, via, via);
	PCB_SET_PARENT(via, data, ctx->buffer.dst);
	return via;
}

/* changes the thermal on a via */
void *pcb_viaop_change_thermal(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, Via, Via, Via);
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
	pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, element, Pin, Pin);
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
	pcb_coord_t value = ctx->chgsize.absolute ? ctx->chgsize.absolute : Via->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIASIZE && value >= Via->DrillingHole + PCB_MIN_PINORVIACOPPER && value != Via->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_VIA, Via, Via, Via);
		pcb_via_invalidate_erase(Via);
		pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Via, Via);
		if (Via->Mask) {
			pcb_undo_add_obj_to_mask_size(PCB_TYPE_VIA, Via, Via, Via);
			Via->Mask += value - Via->Thickness;
		}
		Via->Thickness = value;
		pcb_pin_bbox(Via);
		pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
		pcb_via_invalidate_draw(Via);
		return (Via);
	}
	return (NULL);
}

/* changes the drilling hole of a via */
void *pcb_viaop_change_2nd_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->DrillingHole + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) || value <= Via->Thickness - PCB_MIN_PINORVIACOPPER)
			&& value != Via->DrillingHole) {
		pcb_undo_add_obj_to_2nd_size(PCB_TYPE_VIA, Via, Via, Via);
		pcb_via_invalidate_erase(Via);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
		Via->DrillingHole = value;
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via)) {
			pcb_undo_add_obj_to_size(PCB_TYPE_VIA, Via, Via, Via);
			Via->Thickness = value;
		}
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
		pcb_via_invalidate_draw(Via);
		return (Via);
	}
	return (NULL);
}

/* changes the drilling hole of a pin */
void *pcb_pinop_change_2nd_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->DrillingHole + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) || value <= Pin->Thickness - PCB_MIN_PINORVIACOPPER)
			&& value != Pin->DrillingHole) {
		pcb_undo_add_obj_to_2nd_size(PCB_TYPE_PIN, Element, Pin, Pin);
		pcb_pin_invalidate_erase(Pin);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->DrillingHole = value;
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin)) {
			pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, Pin, Pin);
			Pin->Thickness = value;
		}
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		pcb_pin_invalidate_draw(Pin);
		return (Pin);
	}
	return (NULL);
}


/* changes the clearance size of a via */
void *pcb_viaop_change_clear_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, value);
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (Via->Clearance == value)
		return NULL;
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_undo_add_obj_to_clear_size(PCB_TYPE_VIA, Via, Via, Via);
	pcb_via_invalidate_erase(Via);
	pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	Via->Clearance = value;
	pcb_pin_bbox(Via);
	pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_invalidate_draw(Via);
	Via->Element = NULL;
	return (Via);
}


/* changes the size of a pin */
void *pcb_pinop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && value <= PCB_MAX_PINORVIASIZE &&
			value >= PCB_MIN_PINORVIASIZE && value >= Pin->DrillingHole + PCB_MIN_PINORVIACOPPER && value != Pin->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, Pin, Pin);
		pcb_undo_add_obj_to_mask_size(PCB_TYPE_PIN, Element, Pin, Pin);
		pcb_pin_invalidate_erase(Pin);
		pcb_r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->Mask += value - Pin->Thickness;
		Pin->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		pcb_pin_invalidate_draw(Pin);
		return (Pin);
	}
	return (NULL);
}

/* changes the clearance size of a pin */
void *pcb_pinop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (value < 0)
		value = 0;
	value = MIN(PCB_MAX_LINESIZE, value);
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (Pin->Clearance == value)
		return NULL;
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_undo_add_obj_to_clear_size(PCB_TYPE_PIN, Element, Pin, Pin);
	pcb_pin_invalidate_erase(Pin);
	pcb_r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
	Pin->Clearance = value;
	/* SetElementBB updates all associated rtrees */
	pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_pin_invalidate_draw(Pin);
	return (Pin);
}

/* changes the name of a via */
void *pcb_viaop_change_name(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	char *old = Via->Name;

	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Via)) {
		pcb_pin_name_invalidate_erase(Via);
		Via->Name = ctx->chgname.new_name;
		pcb_pin_name_invalidate_draw(Via);
	}
	else
		Via->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the name of a pin */
void *pcb_pinop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	char *old = Pin->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin)) {
		pcb_pin_name_invalidate_erase(Pin);
		Pin->Name = ctx->chgname.new_name;
		pcb_pin_name_invalidate_draw(Pin);
	}
	else
		Pin->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the number of a pin */
void *pcb_pinop_change_num(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	char *old = Pin->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin)) {
		pcb_pin_name_invalidate_erase(Pin);
		Pin->Number = ctx->chgname.new_name;
		pcb_pin_name_invalidate_draw(Pin);
	}
	else
		Pin->Number = ctx->chgname.new_name;
	return (old);
}


/* changes the square flag of a via */
void *pcb_viaop_change_square(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	pcb_via_invalidate_erase(Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, NULL, Via, Via, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, NULL, Via, Via);
	PCB_FLAG_SQUARE_ASSIGN(ctx->chgsize.absolute, Via);
	if (ctx->chgsize.absolute == 0)
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, Via);
	else
		PCB_FLAG_SET(PCB_FLAG_SQUARE, Via);
	pcb_pin_bbox(Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, NULL, Via, Via, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	pcb_via_invalidate_draw(Via);
	return (Via);
}

/* changes the square flag of a pin */
void *pcb_pinop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	pcb_pin_invalidate_erase(Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, Element, Pin, Pin);
	PCB_FLAG_SQUARE_ASSIGN(ctx->chgsize.absolute, Pin);
	if (ctx->chgsize.absolute == 0)
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, Pin);
	else
		PCB_FLAG_SET(PCB_FLAG_SQUARE, Pin);
	pcb_pin_bbox(Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_pin_invalidate_draw(Pin);
	return (Pin);
}

/* sets the square flag of a pin */
void *pcb_pinop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (pcb_pinop_change_square(ctx, Element, Pin));
}

/* clears the square flag of a pin */
void *pcb_pinop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || !PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (pcb_pinop_change_square(ctx, Element, Pin));
}

/* changes the octagon flag of a via */
void *pcb_viaop_change_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	pcb_via_invalidate_erase(Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, Via, Via, Via);
	PCB_FLAG_TOGGLE(PCB_FLAG_OCTAGON, Via);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_invalidate_draw(Via);
	return (Via);
}

/* sets the octagon flag of a via */
void *pcb_viaop_set_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via) || PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (pcb_viaop_change_octagon(ctx, Via));
}

/* clears the octagon flag of a via */
void *pcb_viaop_clear_octagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via) || !PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (pcb_viaop_change_octagon(ctx, Via));
}

/* changes the octagon flag of a pin */
void *pcb_pinop_change_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	pcb_pin_invalidate_erase(Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, Element, Pin, Pin);
	PCB_FLAG_TOGGLE(PCB_FLAG_OCTAGON, Pin);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	pcb_pin_invalidate_draw(Pin);
	return (Pin);
}

/* sets the octagon flag of a pin */
void *pcb_pinop_set_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (pcb_pinop_change_octagon(ctx, Element, Pin));
}

/* clears the octagon flag of a pin */
void *pcb_pinop_clear_octagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || !PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (pcb_pinop_change_octagon(ctx, Element, Pin));
}

/* changes the hole flag of a via */
pcb_bool pcb_pin_change_hole(pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (pcb_false);
	pcb_via_invalidate_erase(Via);
	pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, Via, Via, Via);
	pcb_undo_add_obj_to_mask_size(PCB_TYPE_VIA, Via, Via, Via);
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
	pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_invalidate_draw(Via);
	pcb_draw();
	return (pcb_true);
}

/* changes the mask size of a pin */
void *pcb_pinop_change_mask_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Mask + ctx->chgsize.delta;

	value = MAX(value, 0);
	if (value == Pin->Mask && ctx->chgsize.absolute == 0)
		value = Pin->Thickness;
	if (value != Pin->Mask) {
		pcb_undo_add_obj_to_mask_size(PCB_TYPE_PIN, Element, Pin, Pin);
		pcb_pin_invalidate_erase(Pin);
		pcb_r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		Pin->Mask = value;
		pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
		pcb_pin_invalidate_draw(Pin);
		return (Pin);
	}
	return (NULL);
}

/* changes the mask size of a via */
void *pcb_viaop_change_mask_size(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value;

	value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->Mask + ctx->chgsize.delta;
	value = MAX(value, 0);
	if (value != Via->Mask) {
		pcb_undo_add_obj_to_mask_size(PCB_TYPE_VIA, Via, Via, Via);
		pcb_via_invalidate_erase(Via);
		pcb_r_delete_entry(PCB->Data->via_tree, &Via->BoundingBox);
		Via->Mask = value;
		pcb_pin_bbox(Via);
		pcb_r_insert_entry(PCB->Data->via_tree, &Via->BoundingBox, 0);
		pcb_via_invalidate_draw(Via);
		return (Via);
	}
	return (NULL);
}

/* copies a via */
void *pcb_viaop_copy(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_pin_t *via;

	via = pcb_via_new(PCB->Data, Via->X + ctx->copy.DeltaX, Via->Y + ctx->copy.DeltaY,
										 Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, pcb_flag_mask(Via->Flags, PCB_FLAG_FOUND));
	if (!via)
		return (via);
	pcb_via_invalidate_draw(via);
	pcb_undo_add_obj_to_create(PCB_TYPE_VIA, via, via, via);
	return (via);
}

/* moves a via */
void *pcb_viaop_move(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	pcb_via_move(Via, ctx->move.dx, ctx->move.dy);
	if (PCB->ViaOn)
		pcb_via_invalidate_erase(Via);
	pcb_r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_VIA, Via, Via);
	if (PCB->ViaOn) {
		pcb_via_invalidate_draw(Via);
		pcb_draw();
	}
	return (Via);
}

/* destroys a via */
void *pcb_viaop_destroy(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_r_delete_entry(ctx->remove.destroy_target->via_tree, (pcb_box_t *) Via);
	free(Via->Name);

	pcb_via_free(Via);
	return NULL;
}

/* removes a via */
void *pcb_viaop_remove(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	/* erase from screen and memory */
	if (PCB->ViaOn) {
		pcb_via_invalidate_erase(Via);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	pcb_undo_move_obj_to_remove(PCB_TYPE_VIA, Via, Via, Via);
	PCB_CLEAR_PARENT(Via);
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
		pcb_r_insert_entry(data->via_tree, (pcb_box_t *)via, 0);
		pcb_poly_clear_from_poly(ctx->rotate.pcb->Data, PCB_TYPE_VIA, via, via);
	}
	pcb_via_invalidate_draw(via);
	return via;
}


#define PCB_PIN_FLAGS (PCB_FLAG_FOUND | PCB_FLAG_HOLE | PCB_FLAG_PININPOLY | PCB_FLAG_SELECTED | PCB_FLAG_AUTO | PCB_FLAG_LOCK | PCB_FLAG_VISIT)
void *pcb_pinop_change_flag(pcb_opctx_t *ctx, pcb_pin_t *pin)
{
	if ((ctx->chgflag.flag & PCB_PIN_FLAGS) != ctx->chgflag.flag)
		return NULL;
	pcb_undo_add_obj_to_flag(PCB_FLAG_TEST(PCB_FLAG_PIN, pin) ? PCB_TYPE_PIN : PCB_TYPE_VIA, pin, pin, pin);
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, pin);
	return pin;
}


/*** draw ***/

/* setup color for pin or via */
static void SetPVColor(pcb_pin_t *Pin, int Type)
{
	char *color;
	char buf[sizeof("#XXXXXX")];

	if (Type == PCB_TYPE_VIA) {
		if (!pcb_draw_doing_pinout && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, Pin)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, Pin))
				color = conf_core.appearance.color.warn;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, Pin))
				color = conf_core.appearance.color.via_selected;
			else
				color = conf_core.appearance.color.connected;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, Pin)) {
				assert(color != NULL);
				pcb_lighten_color(color, buf, 1.75);
				color = buf;
			}
		}
		else
			color = conf_core.appearance.color.via;
	}
	else {
		if (!pcb_draw_doing_pinout && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, Pin)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, Pin))
				color = conf_core.appearance.color.warn;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, Pin))
				color = conf_core.appearance.color.pin_selected;
			else
				color = conf_core.appearance.color.connected;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, Pin)) {
				assert(color != NULL);
				pcb_lighten_color(color, buf, 1.75);
				color = buf;
			}
		}
		else
			color = conf_core.appearance.color.pin;
	}

	pcb_gui->set_color(Output.fgGC, color);
}

static void _draw_pv_name(pcb_pin_t * pv)
{
	pcb_box_t box;
	pcb_bool vert, flip_x, flip_y;
	pcb_coord_t x_off, y_off;
	pcb_text_t text;
	char buff[128];
	const char *pn;

	PCB_DRAW_BBOX(pv);

#warning subc TODO: remove the name/number logics when elements are removed
	if (pv->term != NULL)
		pn = pv->term;
	else if (!pv->Name || !pv->Name[0]) { /* old: elements */
		pn = PCB_EMPTY(pv->Number);
	}
	else { /* old: elements */
		pn = PCB_EMPTY(conf_core.editor.show_number ? pv->Number : pv->Name);
		if (pn == NULL)
			pn = "n/a";
	}

	if (PCB_FLAG_INTCONN_GET(pv) > 0)
		pcb_snprintf(buff, sizeof(buff), "%s[%d]", pn, PCB_FLAG_INTCONN_GET(pv));
	else
		strcpy(buff, pn);
	text.TextString = buff;

	vert = PCB_FLAG_TEST(PCB_FLAG_EDGE2, pv);
	flip_x = conf_core.editor.view.flip_x;
	flip_y = conf_core.editor.view.flip_y;

	if (vert) {
		x_off = -pv->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		y_off = -pv->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
		box.X1 = pv->X + (flip_x ? -x_off : x_off);
		box.Y1 = pv->Y + (flip_y ? -y_off : y_off);
	}
	else {
		x_off = pv->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		y_off = -pv->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.X1 = pv->X + (flip_x ? -x_off : x_off);
		box.Y1 = pv->Y + (flip_y ? -y_off : y_off);
	}

	pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.pin_name);

	text.Flags = (flip_x ^ flip_y) ? pcb_flag_make (PCB_FLAG_ONSOLDER) : pcb_no_flags();
	/* Set font height to approx 56% of pin thickness */
	text.Scale = 56 * pv->Thickness / PCB_FONT_CAPHEIGHT;
	text.X = box.X1;
	text.Y = box.Y1;
	text.fid = 0;
	text.Direction = (vert ? 1 : 0) + (flip_x ? 2 : 0);

	if (pcb_gui->gui)
		pcb_draw_doing_pinout++;
	pcb_text_draw(&text, 0);
	if (pcb_gui->gui)
		pcb_draw_doing_pinout--;
}

static void _draw_pv(pcb_pin_t *pv, pcb_bool draw_hole)
{
	if (conf_core.editor.thin_draw)
		pcb_gui->thindraw_pcb_pv(Output.fgGC, Output.fgGC, pv, draw_hole, pcb_false);
	else
		pcb_gui->fill_pcb_pv(Output.fgGC, Output.bgGC, pv, draw_hole, pcb_false);
	
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pv) && pcb_draw_doing_pinout)
		_draw_pv_name(pv);
}

void pcb_pin_draw(pcb_pin_t *pin, pcb_bool draw_hole)
{
	SetPVColor(pin, PCB_TYPE_PIN);
	_draw_pv(pin, draw_hole);
}

pcb_r_dir_t pcb_pin_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_draw((pcb_pin_t *) b, pcb_false);
	return PCB_R_DIR_FOUND_CONTINUE;
}

void draw_pin_name(pcb_pin_t *pin)
{
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pin) && PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, pin))
		_draw_pv_name(pin);
}

pcb_r_dir_t pcb_pin_name_draw_callback(const pcb_box_t * b, void *cl)
{
	draw_pin_name((pcb_pin_t *) b);
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pin_clear_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pin = (pcb_pin_t *) b;
	if (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly)
		pcb_gui->thindraw_pcb_pv(Output.pmGC, Output.pmGC, pin, pcb_false, pcb_true);
	else
		pcb_gui->fill_pcb_pv(Output.pmGC, Output.pmGC, pin, pcb_false, pcb_true);
	return PCB_R_DIR_FOUND_CONTINUE;
}


static void draw_via(pcb_pin_t *via, pcb_bool draw_hole)
{
	SetPVColor(via, PCB_TYPE_VIA);
	_draw_pv(via, draw_hole);
}

pcb_r_dir_t pcb_via_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *via = (pcb_pin_t *)b;

	if (PCB->SubcPartsOn || !pcb_gobj_parent_subc(via->parent_type, &via->parent))
		draw_via(via, pcb_false);
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_hole_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	int plated = cl ? *(int *) cl : -1;
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if ((plated == 0 && !PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) || (plated == 1 && PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_gobj_parent_subc(pv->parent_type, &pv->parent))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (conf_core.editor.thin_draw) {
		if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
			pcb_gui->set_line_cap(Output.fgGC, Round_Cap);
			pcb_gui->set_line_width(Output.fgGC, 0);
			pcb_gui->draw_arc(Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
		}
	}
	else
		pcb_gui->fill_circle(Output.bgGC, pv->X, pv->Y, pv->DrillingHole / 2);

	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, pv))
			color = conf_core.appearance.color.warn;
		else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pv))
			color = conf_core.appearance.color.via_selected;
		else
			color = conf_core.appearance.color.black;

		if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, pv)) {
			assert(color != NULL);
			pcb_lighten_color(color, buf, 1.75);
			color = buf;
		}
		pcb_gui->set_color(Output.fgGC, color);

		pcb_gui->set_line_cap(Output.fgGC, Round_Cap);
		pcb_gui->set_line_width(Output.fgGC, 0);
		pcb_gui->draw_arc(Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

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
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Via))
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
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin))
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
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Via))
		pcb_via_name_invalidate_draw(Via);
}

void pcb_via_name_invalidate_draw(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void pcb_pin_invalidate_draw(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if ((!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin))
			|| pcb_draw_doing_pinout)
		pcb_pin_name_invalidate_draw(Pin);
}

void pcb_pin_name_invalidate_draw(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}
