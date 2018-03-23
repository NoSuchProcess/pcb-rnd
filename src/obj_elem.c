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

/* Drawing primitive: elements */

#include "config.h"

#include "board.h"
#include "data.h"
#include "list_common.h"
#include "plug_io.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "rotate.h"
#include "remove.h"
#include "polygon.h"
#include "undo.h"
#include "obj_pinvia_op.h"
#include "obj_pad_op.h"

#include "obj_pinvia_draw.h"
#include "obj_pad_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"

#include "obj_elem.h"
#include "obj_elem_list.h"
#include "obj_elem_op.h"
#include "obj_subc_list.h"

/* TODO: remove this: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_elem_draw.h"

/*** allocation ***/

/* get next slot for an element, allocates memory if necessary */
pcb_element_t *pcb_element_alloc(pcb_data_t * data)
{
	pcb_element_t *new_obj;
	int n;

	new_obj = calloc(sizeof(pcb_element_t), 1);
	new_obj->type = PCB_OBJ_ELEMENT;
	PCB_SET_PARENT(new_obj, data, data);

	for(n = 0; n < PCB_MAX_ELEMENTNAMES; n++) {
		new_obj->Name[n].type = PCB_OBJ_ETEXT;
		PCB_SET_PARENT(&new_obj->Name[n], element, new_obj);
	}

	elementlist_append(&data->Element, new_obj);

	return new_obj;
}

void pcb_element_free(pcb_element_t * data)
{
	elementlist_remove(data);
	free(data);
}

/* frees memory used by an element */
void pcb_element_destroy(pcb_element_t * element)
{
	if (element == NULL)
		return;

	PCB_ELEMENT_NAME_LOOP(element);
	{
		free(textstring);
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(element);
	{
		free(pin->Name);
		free(pin->Number);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(element);
	{
		free(pad->Name);
		free(pad->Number);
	}
	PCB_END_LOOP;

	list_map0(&element->Pin,  pcb_pin_t,  pcb_pin_free);
	list_map0(&element->Pad,  pcb_pad_t,  pcb_pad_free);
	list_map0(&element->Line, pcb_line_t, pcb_line_free);
	list_map0(&element->Arc,  pcb_arc_t,  pcb_arc_free);

	pcb_attribute_free(&element->Attributes);
	reset_obj_mem(pcb_element_t, element);
}

pcb_line_t *pcb_element_line_alloc(pcb_element_t *Element)
{
	pcb_line_t *line = calloc(sizeof(pcb_line_t), 1);

	line->type = PCB_OBJ_ELINE;
	PCB_SET_PARENT(line, element, Element);

	linelist_append(&Element->Line, line);

	return line;
}

/*** utility ***/
/* loads element data from file/library into buffer
 * parse the file with disabled 'PCB mode' (see parser)
 * returns pcb_false on error
 * if successful, update some other stuff and reposition the pastebuffer
 */
pcb_bool pcb_element_load_to_buffer(pcb_buffer_t *Buffer, const char *Name, const char *fmt)
{
	pcb_buffer_clear(PCB, Buffer);
	if (!pcb_parse_element(Buffer->Data, Name, fmt)) {
		if (conf_core.editor.show_solder_side)
			pcb_buffer_flip_side(PCB, Buffer);
		pcb_set_buffer_bbox(Buffer);

		Buffer->X = 0;
		Buffer->Y = 0;
		Buffer->from_outside = 1;

		if (elementlist_length(&Buffer->Data->Element)) {
			pcb_element_t *element = elementlist_first(&Buffer->Data->Element);
			Buffer->X = element->MarkX;
			Buffer->Y = element->MarkY;
		}
		else if (pcb_subclist_length(&Buffer->Data->subc)) {
			pcb_subc_t *subc = pcb_subclist_first(&Buffer->Data->subc);
			pcb_subc_get_origin(subc, &Buffer->X, &Buffer->Y);
		}
		return pcb_true;
	}

	/* release memory which might have been acquired */
	pcb_buffer_clear(PCB, Buffer);
	return pcb_false;
}


/* Searches for the given element by "footprint" name, and loads it
   into the buffer. Returns zero on success, non-zero on error.  */
int pcb_element_load_footprint_by_name(pcb_buffer_t *Buffer, const char *Footprint)
{
	return !pcb_element_load_to_buffer(Buffer, Footprint, NULL);
}


/* break buffer element into pieces */
pcb_bool pcb_element_smash_buffer(pcb_buffer_t *Buffer)
{
	pcb_element_t *element;
	pcb_layergrp_id_t group, gbottom, gtop;
	pcb_layer_t *clayer, *slayer;
	char tmp[128];

	if (elementlist_length(&Buffer->Data->Element) != 1)
		return pcb_false;

	/*
	 * At this point the buffer should contain just a single element.
	 * Now we detach the single element from the buffer and then clear the
	 * buffer, ready to receive the smashed elements.  As a result of detaching
	 * it the single element is orphaned from the buffer and thus will not be
	 * free()'d by pcb_data_free(called via ClearBuffer).  This leaves it
	 * around for us to smash bits off it.  It then becomes our responsibility,
	 * however, to free the single element when we're finished with it.
	 */
	element = elementlist_first(&Buffer->Data->Element);
	elementlist_remove(element);
	pcb_buffer_clear(PCB, Buffer);
	PCB_ELEMENT_PCB_LINE_LOOP(element);
	{
		pcb_line_new(&Buffer->Data->SILKLAYER,
												 line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness, 0, pcb_no_flags());
		if (line)
			line->Number = pcb_strdup_null(PCB_ELEM_NAME_REFDES(element));
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(element);
	{
		pcb_arc_new(&Buffer->Data->SILKLAYER,
												arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness, 0, pcb_no_flags());
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(element);
	{
		pcb_pin_t *via;
		pcb_flag_t f = pcb_no_flags();
		pcb_flag_add(f, PCB_FLAG_VIA);
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin))
			f = pcb_flag_add(f, PCB_FLAG_HOLE);

		if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin))
			f = pcb_flag_add(f, PCB_FLAG_SQUARE);
		if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pin))
			f = pcb_flag_add(f, PCB_FLAG_OCTAGON);

		via = pcb_via_new(Buffer->Data, pin->X, pin->Y, pin->Thickness, pin->Clearance, pin->Mask, pin->DrillingHole, pin->Number, f);
		if (pin->Number != NULL)
			pcb_attribute_put(&via->Attributes, "term", pin->Number);
		if (pin->Flags.q != 0) {
			char tmp[16];
			sprintf(tmp, "%d", pin->Flags.q);
			pcb_attribute_put(&via->Attributes, "elem_smash_shape_id", tmp);
		}
		pcb_attribute_put(&via->Attributes, "elem_smash_pad", "1");
		pcb_sprintf(tmp, "%$mm", pin->Mask);
		pcb_attribute_put(&pin->Attributes, "elem_smash_pad_mask", tmp);
	}
	PCB_END_LOOP;

	gbottom = gtop = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gbottom, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER,    &gtop, 1);

	group = (PCB_SWAP_IDENT ? gbottom : gtop);
	clayer = &Buffer->Data->Layer[PCB->LayerGroups.grp[group].lid[0]];
	group = (PCB_SWAP_IDENT ? gtop : gbottom);
	slayer = &Buffer->Data->Layer[PCB->LayerGroups.grp[group].lid[0]];
	PCB_PAD_LOOP(element);
	{
		pcb_line_t *line;
		line = pcb_line_new(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? slayer : clayer,
																pad->Point1.X, pad->Point1.Y,
																pad->Point2.X, pad->Point2.Y, pad->Thickness, pad->Clearance, pcb_no_flags());
		if (line) {
			line->Number = pcb_strdup_null(pad->Number);
			if (pad->Number != NULL)
				pcb_attribute_put(&line->Attributes, "term", pad->Number);
			if (pad->Flags.q != 0) {
				char tmp[16];
				sprintf(tmp, "%d", pad->Flags.q);
				pcb_attribute_put(&line->Attributes, "elem_smash_shape_id", tmp);
			}
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pad))
				pcb_attribute_put(&line->Attributes, "elem_smash_shape_square", "1");
			if (PCB_FLAG_TEST(PCB_FLAG_NOPASTE, pad))
				pcb_attribute_put(&line->Attributes, "elem_smash_nopaste", "1");
			pcb_attribute_put(&line->Attributes, "elem_smash_pad", "1");
			pcb_sprintf(tmp, "%$mm", pad->Mask);
			pcb_attribute_put(&line->Attributes, "elem_smash_pad_mask", tmp);
		}
	}
	PCB_END_LOOP;
	pcb_element_destroy(element);
	pcb_element_free(element);
	return pcb_true;
}

/* see if a polygon is a rectangle.  If so, canonicalize it. */
static int polygon_is_rectangle(pcb_poly_t *poly)
{
	int i, best;
	pcb_point_t temp[4];
	if (poly->PointN != 4 || poly->HoleIndexN != 0)
		return 0;
	best = 0;
	for (i = 1; i < 4; i++)
		if (poly->Points[i].X < poly->Points[best].X || poly->Points[i].Y < poly->Points[best].Y)
			best = i;
	for (i = 0; i < 4; i++)
		temp[i] = poly->Points[(i + best) % 4];
	if (temp[0].X == temp[1].X)
		memcpy(poly->Points, temp, sizeof(temp));
	else {
		/* reverse them */
		poly->Points[0] = temp[0];
		poly->Points[1] = temp[3];
		poly->Points[2] = temp[2];
		poly->Points[3] = temp[1];
	}
	if (poly->Points[0].X == poly->Points[1].X
			&& poly->Points[1].Y == poly->Points[2].Y
			&& poly->Points[2].X == poly->Points[3].X && poly->Points[3].Y == poly->Points[0].Y)
		return 1;
	return 0;
}

/* convert buffer contents into an element */
pcb_bool pcb_element_convert_from_buffer(pcb_buffer_t *Buffer)
{
	pcb_element_t *Element;
	pcb_layergrp_id_t gbottom, gtop;
	pcb_cardinal_t pin_n = 1;
	pcb_bool hasParts = pcb_false, crooked = pcb_false;
	int onsolder;
	pcb_bool warned = pcb_false;

	gbottom = gtop = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gbottom, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER,    &gtop, 1);

	Element = pcb_element_new(PCB->Data, NULL, pcb_font(PCB, 0, 1), pcb_no_flags(),
														 NULL, NULL, NULL, PCB_PASTEBUFFER->X,
														 PCB_PASTEBUFFER->Y, 0, 100, pcb_flag_make(PCB_SWAP_IDENT ? PCB_FLAG_ONSOLDER : PCB_FLAG_NO), pcb_false);
	if (!Element)
		return pcb_false;
	PCB_VIA_LOOP(Buffer->Data);
	{
		char num[8];
		if (via->Mask < via->Thickness)
			via->Mask = via->Thickness + 2 * PCB_MASKFRAME;
		if (via->Name)
			pcb_element_pin_new(Element, via->X, via->Y, via->Thickness,
									 via->Clearance, via->Mask, via->DrillingHole,
									 NULL, via->Name, pcb_flag_mask(via->Flags, PCB_FLAG_VIA | PCB_FLAG_FOUND | PCB_FLAG_SELECTED | PCB_FLAG_WARN));
		else {
			sprintf(num, "%d", pin_n++);
			pcb_element_pin_new(Element, via->X, via->Y, via->Thickness,
									 via->Clearance, via->Mask, via->DrillingHole,
									 NULL, num, pcb_flag_mask(via->Flags, PCB_FLAG_VIA | PCB_FLAG_FOUND | PCB_FLAG_SELECTED | PCB_FLAG_WARN));
		}
		hasParts = pcb_true;
	}
	PCB_END_LOOP;

	for (onsolder = 0; onsolder < 2; onsolder++) {
		pcb_layergrp_id_t side_grp;
		int onsolderflag;

		if ((!onsolder) == (!PCB_SWAP_IDENT)) {
			side_grp = gtop;
			onsolderflag = PCB_FLAG_NO;
		}
		else {
			side_grp = gbottom;
			onsolderflag = PCB_FLAG_ONSOLDER;
		}

#define MAYBE_WARN() \
	  if (onsolder && !hasParts && !warned) \
	    { \
	      warned = pcb_true; \
	      pcb_message \
					(PCB_MSG_WARNING, _("Warning: All of the pads are on the opposite\n" \
		   "side from the component - that's probably not what\n" \
		   "you wanted\n")); \
	    } \

		/* get the side SM pads on the side we are dealing with (first component then solder) */
		PCB_COPPER_GROUP_LOOP(Buffer->Data, side_grp);
		{
			char num[8];
			PCB_LINE_LOOP(layer);
			{
				sprintf(num, "%d", pin_n++);
				pcb_element_pad_new(Element, line->Point1.X,
										 line->Point1.Y, line->Point2.X,
										 line->Point2.Y, line->Thickness,
										 line->Clearance,
										 line->Thickness + line->Clearance, NULL, line->Number ? line->Number : num, pcb_flag_make(onsolderflag));
				MAYBE_WARN();
				hasParts = pcb_true;
			}
			PCB_END_LOOP;
			PCB_POLY_LOOP(layer);
			{
				if (!polygon_is_rectangle(polygon)) {
					crooked = pcb_true;
					continue;
				}

				sprintf(num, "%d", pin_n++);

				pcb_element_pad_new_rect(Element,
					polygon->Points[2].X, polygon->Points[1].Y,
					polygon->Points[0].X, polygon->Points[0].Y,
					conf_core.design.clearance, conf_core.design.clearance,
					NULL, num, pcb_flag_make(PCB_FLAG_SQUARE | onsolderflag));

				MAYBE_WARN();
				hasParts = pcb_true;
			}
			PCB_END_LOOP;
		}
		PCB_END_LOOP;
	}

	/* now add the silkscreen. NOTE: elements must have pads or pins too */
	PCB_LINE_LOOP(&Buffer->Data->SILKLAYER);
	{
		if (line->Number && !PCB_ELEM_NAME_REFDES(Element))
			PCB_ELEM_NAME_REFDES(Element) = pcb_strdup(line->Number);
		pcb_element_line_new(Element, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness);
		hasParts = pcb_true;
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(&Buffer->Data->SILKLAYER);
	{
		pcb_element_arc_new(Element, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
		hasParts = pcb_true;
	}
	PCB_END_LOOP;
	if (!hasParts) {
		pcb_destroy_object(PCB->Data, PCB_TYPE_ELEMENT, Element, Element, Element);
		pcb_message(PCB_MSG_ERROR, _("There was nothing to convert!\n" "Elements must have some silk, pads or pins.\n"));
		return pcb_false;
	}
	if (crooked)
		pcb_message(PCB_MSG_ERROR, _("There were polygons that can't be made into pins!\n" "So they were not included in the element\n"));
	Element->MarkX = Buffer->X;
	Element->MarkY = Buffer->Y;
	if (PCB_SWAP_IDENT)
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, Element);
	pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
	pcb_buffer_clear(PCB, Buffer);
	pcb_move_obj_to_buffer(PCB, Buffer->Data, PCB->Data, PCB_TYPE_ELEMENT, Element, Element, Element);
	pcb_set_buffer_bbox(Buffer);
	return pcb_true;
}

void pcb_element_rotate(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle)
{
	/* solder side objects need a different orientation */

	/* the text subroutine decides by itself if the direction
	 * is to be corrected
	 */
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_rotate(&line->Point1.X, &line->Point1.Y, X, Y, cosa, sina);
		pcb_rotate(&line->Point2.X, &line->Point2.Y, X, Y, cosa, sina);
		pcb_line_bbox(line);
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(Element);
	{
		/* pre-delete the pins from the pin-tree before their coordinates change */
		if (Data)
			pcb_r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
		pcb_poly_restore_to_poly(Data, PCB_TYPE_PIN, Element, pin);
		pcb_rotate(&pin->X, &pin->Y, X, Y, cosa, sina);
		pcb_pin_bbox(pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		/* pre-delete the pads before their coordinates change */
		if (Data)
			pcb_r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
		pcb_poly_restore_to_poly(Data, PCB_TYPE_PAD, Element, pad);
		pcb_rotate(&pad->Point1.X, &pad->Point1.Y, X, Y, cosa, sina);
		pcb_rotate(&pad->Point2.X, &pad->Point2.Y, X, Y, cosa, sina);
		pcb_line_bbox((pcb_line_t *) pad);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_rotate(&arc->X, &arc->Y, X, Y, cosa, sina);
		arc->StartAngle = pcb_normalize_angle(arc->StartAngle + angle);
	}
	PCB_END_LOOP;

	pcb_rotate(&Element->MarkX, &Element->MarkY, X, Y, cosa, sina);
	pcb_element_bbox(Data, Element, pcb_font(PCB, 0, 1));
	pcb_poly_clear_from_poly(Data, PCB_TYPE_ELEMENT, Element, Element);
}

/* changes the side of the board an element is on; returns pcb_true if done */
pcb_bool pcb_element_change_side(pcb_element_t *Element, pcb_coord_t yoff)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return pcb_false;
	pcb_elem_invalidate_erase(Element);
abort();
/*	pcb_undo_add_obj_to_mirror(PCB_TYPE_ELEMENT, Element, Element, Element, yoff);*/
/*	pcb_element_mirror(PCB->Data, Element, yoff);*/
	pcb_elem_invalidate_draw(Element);
	return pcb_true;
}

/* changes the side of all selected and visible elements;
   returns pcb_true if anything has changed */
pcb_bool pcb_selected_element_change_side(void)
{
	pcb_bool change = pcb_false;

	if (PCB->PinOn && pcb_silk_on(PCB))
		PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, element)) {
			change |= pcb_element_change_side(element, 0);
		}
	}
	PCB_END_LOOP;
	return change;
}

/* changes the layout-name of an element */
char *pcb_element_text_change(pcb_board_t * pcb, pcb_data_t * data, pcb_element_t *Element, int which, char *new_name)
{
	char *old = Element->Name[which].TextString;

	Element->Name[which].type = PCB_OBJ_ETEXT;
	PCB_SET_PARENT(&Element->Name[which], element, Element);
#ifdef DEBUG
	printf("In ChangeElementText, updating old TextString %s to %s\n", old, new_name);
#endif

	if (pcb && which == PCB_ELEMNAME_IDX_VISIBLE())
		pcb_elem_name_invalidate_erase(Element);

	pcb_r_delete_entry(data->name_tree[which], &Element->Name[which].BoundingBox);

	Element->Name[which].TextString = new_name;
	pcb_text_bbox(pcb_font(PCB, 0, 1), &Element->Name[which]);

	pcb_r_insert_entry(data->name_tree[which], &Element->Name[which].BoundingBox);

	if (pcb && which == PCB_ELEMNAME_IDX_VISIBLE())
		pcb_elem_name_invalidate_draw(Element);

	return old;
}

void pcb_element_text_update(pcb_board_t *pcb, pcb_data_t *data, pcb_element_t *Element, int which)
{
	if (pcb && which == PCB_ELEMNAME_IDX_VISIBLE())
		pcb_elem_name_invalidate_erase(Element);

	pcb_r_delete_entry(data->name_tree[which], &Element->Name[which].BoundingBox);
	pcb_text_bbox(pcb_font(PCB, 0, 1), &Element->Name[which]);
	pcb_r_insert_entry(data->name_tree[which], &Element->Name[which].BoundingBox);

	if (pcb && which == PCB_ELEMNAME_IDX_VISIBLE())
		pcb_elem_name_invalidate_draw(Element);
}

void pcb_element_text_set_font(pcb_board_t *pcb, pcb_data_t *data, pcb_element_t *Element, int which, pcb_font_id_t fid)
{
	if (pcb && which == PCB_ELEMNAME_IDX_VISIBLE())
		pcb_elem_name_invalidate_erase(Element);

	pcb_r_delete_entry(data->name_tree[which], &Element->Name[which].BoundingBox);
	Element->Name[which].fid = fid;
	pcb_text_bbox(pcb_font(PCB, 0, 1), &Element->Name[which]);
	pcb_r_insert_entry(data->name_tree[which], &Element->Name[which].BoundingBox);

	if (pcb && which == PCB_ELEMNAME_IDX_VISIBLE())
		pcb_elem_name_invalidate_draw(Element);
}

/* copies data from one element to another and creates the destination; if necessary */
pcb_element_t *pcb_element_copy(pcb_data_t *Data, pcb_element_t *Dest, pcb_element_t *Src, pcb_bool uniqueName, pcb_coord_t dx, pcb_coord_t dy)
{
	int i;
	/* release old memory if necessary */
	if (Dest)
		pcb_element_destroy(Dest);

	/* both coordinates and flags are the same */
	Dest = pcb_element_new(Data, Dest, pcb_font(PCB, 0, 1),
													pcb_flag_mask(Src->Flags, PCB_FLAG_FOUND),
													PCB_ELEM_NAME_DESCRIPTION(Src), PCB_ELEM_NAME_REFDES(Src),
													PCB_ELEM_NAME_VALUE(Src), PCB_ELEM_TEXT_DESCRIPTION(Src).X + dx,
													PCB_ELEM_TEXT_DESCRIPTION(Src).Y + dy,
													PCB_ELEM_TEXT_DESCRIPTION(Src).Direction,
													PCB_ELEM_TEXT_DESCRIPTION(Src).Scale, pcb_flag_mask(PCB_ELEM_TEXT_DESCRIPTION(Src).Flags, PCB_FLAG_FOUND), uniqueName);

	/* abort on error */
	if (!Dest)
		return Dest;

	PCB_ELEMENT_PCB_LINE_LOOP(Src);
	{
		pcb_element_line_new(Dest, line->Point1.X + dx,
													 line->Point1.Y + dy, line->Point2.X + dx, line->Point2.Y + dy, line->Thickness);
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(Src);
	{
		pcb_element_pin_new(Dest, pin->X + dx, pin->Y + dy, pin->Thickness,
								 pin->Clearance, pin->Mask, pin->DrillingHole, pin->Name, pin->Number, pcb_flag_mask(pin->Flags, PCB_FLAG_FOUND));
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Src);
	{
		pcb_element_pad_new(Dest, pad->Point1.X + dx, pad->Point1.Y + dy,
								 pad->Point2.X + dx, pad->Point2.Y + dy, pad->Thickness,
								 pad->Clearance, pad->Mask, pad->Name, pad->Number, pcb_flag_mask(pad->Flags, PCB_FLAG_FOUND));
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Src);
	{
		pcb_element_arc_new(Dest, arc->X + dx, arc->Y + dy, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
	}
	PCB_END_LOOP;

	for (i = 0; i < Src->Attributes.Number; i++)
		pcb_attribute_put(&Dest->Attributes, Src->Attributes.List[i].name, Src->Attributes.List[i].value);

	Dest->MarkX = Src->MarkX + dx;
	Dest->MarkY = Src->MarkY + dy;

	pcb_element_bbox(Data, Dest, pcb_font(PCB, 0, 1));
	return Dest;
}

/* creates an new element; memory is allocated if needed */
pcb_element_t *pcb_element_new(pcb_data_t *Data, pcb_element_t *Element,
	pcb_font_t *PCBFont, pcb_flag_t Flags, char *Description, char *NameOnPCB,
	char *Value, pcb_coord_t TextX, pcb_coord_t TextY, pcb_uint8_t Direction,
	int TextScale, pcb_flag_t TextFlags, pcb_bool uniqueName)
{
#ifdef DEBUG
	printf("Entered CreateNewElement.....\n");
#endif

	if (!Element)
		Element = pcb_element_alloc(Data);

	/* copy values and set additional information */
	TextScale = MAX(PCB_MIN_TEXTSCALE, TextScale);
	pcb_element_text_set(&PCB_ELEM_TEXT_DESCRIPTION(Element), PCBFont, TextX, TextY, Direction, Description, TextScale, TextFlags);
	if (uniqueName)
		NameOnPCB = pcb_element_uniq_name(Data, NameOnPCB);
	pcb_element_text_set(&PCB_ELEM_TEXT_REFDES(Element), PCBFont, TextX, TextY, Direction, NameOnPCB, TextScale, TextFlags);
	pcb_element_text_set(&PCB_ELEM_TEXT_VALUE(Element), PCBFont, TextX, TextY, Direction, Value, TextScale, TextFlags);
	PCB_ELEM_TEXT_DESCRIPTION(Element).Element = Element;
	PCB_ELEM_TEXT_REFDES(Element).Element = Element;
	PCB_ELEM_TEXT_VALUE(Element).Element = Element;
	Element->Flags = Flags;
	Element->ID = pcb_create_ID_get();

#ifdef DEBUG
	printf("  .... Leaving CreateNewElement.\n");
#endif

	return Element;
}

/* creates a new arc in an element */
pcb_arc_t *pcb_element_arc_new(pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y,
	pcb_coord_t Width, pcb_coord_t Height, pcb_angle_t angle, pcb_angle_t delta, pcb_coord_t Thickness)
{
	pcb_arc_t *arc = pcb_element_arc_alloc(Element);

	/* set Delta (0,360], StartAngle in [0,360) */
	if (delta < 0) {
		delta = -delta;
		angle -= delta;
	}
	angle = pcb_normalize_angle(angle);
	delta = pcb_normalize_angle(delta);
	if (delta == 0)
		delta = 360;

	/* copy values */
	arc->X = X;
	arc->Y = Y;
	arc->Width = Width;
	arc->Height = Height;
	arc->StartAngle = angle;
	arc->Delta = delta;
	arc->Thickness = Thickness;
	arc->ID = pcb_create_ID_get();
	return arc;
}

/* creates a new line for an element */
pcb_line_t *pcb_element_line_new(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness)
{
	pcb_line_t *line;

	if (Thickness == 0)
		return NULL;

	line = pcb_element_line_alloc(Element);

	/* copy values */
	line->Point1.X = X1;
	line->Point1.Y = Y1;
	line->Point2.X = X2;
	line->Point2.Y = Y2;
	line->Thickness = Thickness;
	line->Flags = pcb_no_flags();
	line->ID = pcb_create_ID_get();
	return line;
}

/* creates a new textobject as part of an element
   copies the values to the appropriate text object */
void pcb_element_text_set(pcb_text_t *Text, pcb_font_t *PCBFont, pcb_coord_t X, pcb_coord_t Y,
	unsigned Direction, const char *TextString, int Scale, pcb_flag_t Flags)
{
	free(Text->TextString);
	Text->TextString = (TextString && *TextString) ? pcb_strdup(TextString) : NULL;
	Text->X = X;
	Text->Y = Y;
	Text->Direction = Direction;
	Text->Flags = Flags;
	Text->Scale = Scale;

	/* calculate size of the bounding box */
	pcb_text_bbox(PCBFont, Text);
	Text->ID = pcb_create_ID_get();
}

/* mirrors the coordinates of an element; an additional offset is passed */
void pcb_element_mirror(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t yoff)
{
	r_delete_element(Data, Element);
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		line->Point1.X = PCB_SWAP_X(line->Point1.X);
		line->Point1.Y = PCB_SWAP_Y(line->Point1.Y) + yoff;
		line->Point2.X = PCB_SWAP_X(line->Point2.X);
		line->Point2.Y = PCB_SWAP_Y(line->Point2.Y) + yoff;
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(Element);
	{
		pcb_poly_restore_to_poly(Data, PCB_TYPE_PIN, Element, pin);
		pin->X = PCB_SWAP_X(pin->X);
		pin->Y = PCB_SWAP_Y(pin->Y) + yoff;
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		pcb_poly_restore_to_poly(Data, PCB_TYPE_PAD, Element, pad);
		pad->Point1.X = PCB_SWAP_X(pad->Point1.X);
		pad->Point1.Y = PCB_SWAP_Y(pad->Point1.Y) + yoff;
		pad->Point2.X = PCB_SWAP_X(pad->Point2.X);
		pad->Point2.Y = PCB_SWAP_Y(pad->Point2.Y) + yoff;
		PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, pad);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		arc->X = PCB_SWAP_X(arc->X);
		arc->Y = PCB_SWAP_Y(arc->Y) + yoff;
		arc->StartAngle = PCB_SWAP_ANGLE(arc->StartAngle);
		arc->Delta = PCB_SWAP_DELTA(arc->Delta);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_PCB_TEXT_LOOP(Element);
	{
		text->X = PCB_SWAP_X(text->X);
		text->Y = PCB_SWAP_Y(text->Y) + yoff;
		PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
	}
	PCB_END_LOOP;
	Element->MarkX = PCB_SWAP_X(Element->MarkX);
	Element->MarkY = PCB_SWAP_Y(Element->MarkY) + yoff;

	/* now toggle the solder-side flag */
	PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, Element);
	/* this inserts all of the rtree data too */
	pcb_element_bbox(Data, Element, pcb_font(PCB, 0, 1));
	pcb_poly_clear_from_poly(Data, PCB_TYPE_ELEMENT, Element, Element);
}

int pcb_element_is_empty(pcb_element_t *Element)
{
	return
		(pinlist_length(&Element->Pin) == 0)
		&& (padlist_length(&Element->Pad) == 0)
		&& (linelist_length(&Element->Line) == 0)
		&& (arclist_length(&Element->Arc) == 0);
}

/* sets the bounding box of an elements */
void pcb_element_bbox(pcb_data_t *Data, pcb_element_t *Element, pcb_font_t *Font)
{
	pcb_box_t *box, *vbox;

	if (Data && Data->element_tree)
		pcb_r_delete_entry(Data->element_tree, (pcb_box_t *) Element);

	if (pcb_element_is_empty(Element)) {
		pcb_message(PCB_MSG_ERROR, "Internal error: can not calculate bounding box of empty element. Please report this bug.\n");
		assert(!"empty elem");
		return;
	}

	/* first update the text objects */
	PCB_ELEMENT_PCB_TEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			pcb_r_delete_entry(Data->name_tree[n], (pcb_box_t *) text);
		pcb_text_bbox(Font, text);
		if (Data && !Data->name_tree[n])
			Data->name_tree[n] = pcb_r_create_tree();
		if (Data)
			pcb_r_insert_entry(Data->name_tree[n], (pcb_box_t *) text);
	}
	PCB_END_LOOP;

	/* do not include the elementnames bounding box which
	 * is handled separately
	 */
	box = &Element->BoundingBox;
	vbox = &Element->VBox;
	box->X1 = box->Y1 = PCB_MAX_COORD;
	box->X2 = box->Y2 = 0;
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_line_bbox(line);
		PCB_MAKE_MIN(box->X1, line->Point1.X - (line->Thickness + 1) / 2);
		PCB_MAKE_MIN(box->Y1, line->Point1.Y - (line->Thickness + 1) / 2);
		PCB_MAKE_MIN(box->X1, line->Point2.X - (line->Thickness + 1) / 2);
		PCB_MAKE_MIN(box->Y1, line->Point2.Y - (line->Thickness + 1) / 2);
		PCB_MAKE_MAX(box->X2, line->Point1.X + (line->Thickness + 1) / 2);
		PCB_MAKE_MAX(box->Y2, line->Point1.Y + (line->Thickness + 1) / 2);
		PCB_MAKE_MAX(box->X2, line->Point2.X + (line->Thickness + 1) / 2);
		PCB_MAKE_MAX(box->Y2, line->Point2.Y + (line->Thickness + 1) / 2);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_arc_bbox(arc);
		PCB_MAKE_MIN(box->X1, arc->BoundingBox.X1);
		PCB_MAKE_MIN(box->Y1, arc->BoundingBox.Y1);
		PCB_MAKE_MAX(box->X2, arc->BoundingBox.X2);
		PCB_MAKE_MAX(box->Y2, arc->BoundingBox.Y2);
	}
	PCB_END_LOOP;
	*vbox = *box;
	PCB_PIN_LOOP(Element);
	{
		if (Data && Data->pin_tree)
			pcb_r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
		pcb_pin_bbox(pin);
		if (Data) {
			if (!Data->pin_tree)
				Data->pin_tree = pcb_r_create_tree();
			pcb_r_insert_entry(Data->pin_tree, (pcb_box_t *) pin);
		}
		PCB_MAKE_MIN(box->X1, pin->BoundingBox.X1);
		PCB_MAKE_MIN(box->Y1, pin->BoundingBox.Y1);
		PCB_MAKE_MAX(box->X2, pin->BoundingBox.X2);
		PCB_MAKE_MAX(box->Y2, pin->BoundingBox.Y2);
		PCB_MAKE_MIN(vbox->X1, pin->X - pin->Thickness / 2);
		PCB_MAKE_MIN(vbox->Y1, pin->Y - pin->Thickness / 2);
		PCB_MAKE_MAX(vbox->X2, pin->X + pin->Thickness / 2);
		PCB_MAKE_MAX(vbox->Y2, pin->Y + pin->Thickness / 2);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		if (Data && Data->pad_tree)
			pcb_r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
		pcb_pad_bbox(pad);
		if (Data) {
			if (!Data->pad_tree)
				Data->pad_tree = pcb_r_create_tree();
			pcb_r_insert_entry(Data->pad_tree, (pcb_box_t *) pad);
		}
		PCB_MAKE_MIN(box->X1, pad->BoundingBox.X1);
		PCB_MAKE_MIN(box->Y1, pad->BoundingBox.Y1);
		PCB_MAKE_MAX(box->X2, pad->BoundingBox.X2);
		PCB_MAKE_MAX(box->Y2, pad->BoundingBox.Y2);
		PCB_MAKE_MIN(vbox->X1, MIN(pad->Point1.X, pad->Point2.X) - pad->Thickness / 2);
		PCB_MAKE_MIN(vbox->Y1, MIN(pad->Point1.Y, pad->Point2.Y) - pad->Thickness / 2);
		PCB_MAKE_MAX(vbox->X2, MAX(pad->Point1.X, pad->Point2.X) + pad->Thickness / 2);
		PCB_MAKE_MAX(vbox->Y2, MAX(pad->Point1.Y, pad->Point2.Y) + pad->Thickness / 2);
	}
	PCB_END_LOOP;
	/* now we set the PCB_FLAG_EDGE2 of the pad if Point2
	 * is closer to the outside edge than Point1
	 */
	PCB_PAD_LOOP(Element);
	{
		if (pad->Point1.Y == pad->Point2.Y) {
			/* horizontal pad */
			if (box->X2 - pad->Point2.X < pad->Point1.X - box->X1)
				PCB_FLAG_SET(PCB_FLAG_EDGE2, pad);
			else
				PCB_FLAG_CLEAR(PCB_FLAG_EDGE2, pad);
		}
		else {
			/* vertical pad */
			if (box->Y2 - pad->Point2.Y < pad->Point1.Y - box->Y1)
				PCB_FLAG_SET(PCB_FLAG_EDGE2, pad);
			else
				PCB_FLAG_CLEAR(PCB_FLAG_EDGE2, pad);
		}
	}
	PCB_END_LOOP;

	/* mark pins with component orientation */
	if ((box->X2 - box->X1) > (box->Y2 - box->Y1)) {
		PCB_PIN_LOOP(Element);
		{
			PCB_FLAG_SET(PCB_FLAG_EDGE2, pin);
		}
		PCB_END_LOOP;
	}
	else {
		PCB_PIN_LOOP(Element);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_EDGE2, pin);
		}
		PCB_END_LOOP;
	}
	pcb_close_box(box);
	pcb_close_box(vbox);
	if (Data && !Data->element_tree)
		Data->element_tree = pcb_r_create_tree();
	if (Data)
		pcb_r_insert_entry(Data->element_tree, box);
}

static char *BumpName(char *Name)
{
	int num;
	char c, *start;
	static char temp[256];

	start = Name;
	/* seek end of string */
	while (*Name != 0)
		Name++;
	/* back up to potential number */
	for (Name--; isdigit((int) *Name); Name--);
	Name++;
	if (*Name)
		num = atoi(Name) + 1;
	else
		num = 1;
	c = *Name;
	*Name = 0;
	sprintf(temp, "%s%d", start, num);
	/* if this is not our string, put back the blown character */
	if (start != temp)
		*Name = c;
	return temp;
}


/* make a unique name for the name on board
 * this can alter the contents of the input string */
char *pcb_element_uniq_name(pcb_data_t *Data, char *Name)
{
	pcb_bool unique = pcb_true;
	/* null strings are ok */
	if (!Name || !*Name)
		return Name;

	for (;;) {
		PCB_ELEMENT_LOOP(Data);
		{
			if (PCB_ELEM_NAME_REFDES(element) && PCB_NSTRCMP(PCB_ELEM_NAME_REFDES(element), Name) == 0) {
				Name = BumpName(Name);
				unique = pcb_false;
				break;
			}
		}
		PCB_END_LOOP;
		if (unique)
			return Name;
		unique = pcb_true;
	}
}

void r_delete_element(pcb_data_t * data, pcb_element_t * element)
{
	pcb_r_delete_entry(data->element_tree, (pcb_box_t *) element);
	PCB_PIN_LOOP(element);
	{
		pcb_r_delete_entry(data->pin_tree, (pcb_box_t *) pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(element);
	{
		pcb_r_delete_entry(data->pad_tree, (pcb_box_t *) pad);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_PCB_TEXT_LOOP(element);
	{
		pcb_r_delete_entry(data->name_tree[n], (pcb_box_t *) text);
	}
	PCB_END_LOOP;
}

/* Returns a best guess about the orientation of an element.  The
 * value corresponds to the rotation; a difference is the right value
 * to pass to RotateElementLowLevel.  However, the actual value is no
 * indication of absolute rotation; only relative rotation is
 * meaningful.
 */
int pcb_element_get_orientation(pcb_element_t * e)
{
	pcb_coord_t pin1x, pin1y, pin2x, pin2y, dx, dy;
	pcb_bool found_pin1 = 0;
	pcb_bool found_pin2 = 0;

	/* in case we don't find pin 1 or 2, make sure we have initialized these variables */
	pin1x = 0;
	pin1y = 0;
	pin2x = 0;
	pin2y = 0;

	PCB_PIN_LOOP(e);
	{
		if (PCB_NSTRCMP(pin->Number, "1") == 0) {
			pin1x = pin->X;
			pin1y = pin->Y;
			found_pin1 = 1;
		}
		else if (PCB_NSTRCMP(pin->Number, "2") == 0) {
			pin2x = pin->X;
			pin2y = pin->Y;
			found_pin2 = 1;
		}
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(e);
	{
		if (PCB_NSTRCMP(pad->Number, "1") == 0) {
			pin1x = (pad->Point1.X + pad->Point2.X) / 2;
			pin1y = (pad->Point1.Y + pad->Point2.Y) / 2;
			found_pin1 = 1;
		}
		else if (PCB_NSTRCMP(pad->Number, "2") == 0) {
			pin2x = (pad->Point1.X + pad->Point2.X) / 2;
			pin2y = (pad->Point1.Y + pad->Point2.Y) / 2;
			found_pin2 = 1;
		}
	}
	PCB_END_LOOP;

	if (found_pin1 && found_pin2) {
		dx = pin2x - pin1x;
		dy = pin2y - pin1y;
	}
	else if (found_pin1 && (pin1x || pin1y)) {
		dx = pin1x;
		dy = pin1y;
	}
	else if (found_pin2 && (pin2x || pin2y)) {
		dx = pin2x;
		dy = pin2y;
	}
	else
		return 0;

	if (coord_abs(dx) > coord_abs(dy))
		return dx > 0 ? 0 : 2;
	return dy > 0 ? 3 : 1;
}

/* moves a element by +-X and +-Y */
void pcb_element_move(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t DX, pcb_coord_t DY)
{
	if (Data)
		pcb_r_delete_entry(Data->element_tree, (pcb_box_t *) Element);
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_line_move(line, DX, DY);
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(Element);
	{
		if (Data) {
			pcb_r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
			pcb_poly_restore_to_poly(Data, PCB_TYPE_PIN, Element, pin);
		}
		pcb_pin_move(pin, DX, DY);
		if (Data) {
			pcb_r_insert_entry(Data->pin_tree, (pcb_box_t *) pin);
			pcb_poly_clear_from_poly(Data, PCB_TYPE_PIN, Element, pin);
		}
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		if (Data) {
			pcb_r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
			pcb_poly_restore_to_poly(Data, PCB_TYPE_PAD, Element, pad);
		}
		pcb_pad_move(pad, DX, DY);
		if (Data) {
			pcb_r_insert_entry(Data->pad_tree, (pcb_box_t *) pad);
			pcb_poly_clear_from_poly(Data, PCB_TYPE_PAD, Element, pad);
		}
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_arc_move(arc, DX, DY);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_PCB_TEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			pcb_r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		pcb_text_move(text, DX, DY);
		if (Data && Data->name_tree[n])
			pcb_r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
	}
	PCB_END_LOOP;
	PCB_BOX_MOVE_LOWLEVEL(&Element->BoundingBox, DX, DY);
	PCB_BOX_MOVE_LOWLEVEL(&Element->VBox, DX, DY);
	PCB_MOVE(Element->MarkX, Element->MarkY, DX, DY);
	if (Data)
		pcb_r_insert_entry(Data->element_tree, (pcb_box_t *) Element);
}

void *pcb_element_remove(pcb_element_t *Element)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;
	PCB_CLEAR_PARENT(Element);

	res = pcb_elemop_remove(&ctx, Element);
	pcb_draw();
	return res;
}

/* rotate an element in 90 degree steps */
void pcb_element_rotate90(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	/* solder side objects need a different orientation */

	/* the text subroutine decides by itself if the direction
	 * is to be corrected
	 */
	PCB_ELEMENT_PCB_TEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			pcb_r_delete_entry(Data->name_tree[n], (pcb_box_t *) text);
		pcb_text_rotate90(text, X, Y, Number);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_line_rotate90(line, X, Y, Number);
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(Element);
	{
		/* pre-delete the pins from the pin-tree before their coordinates change */
		if (Data)
			pcb_r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
		pcb_poly_restore_to_poly(Data, PCB_TYPE_PIN, Element, pin);
		PCB_PIN_ROTATE90(pin, X, Y, Number);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		/* pre-delete the pads before their coordinates change */
		if (Data)
			pcb_r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
		pcb_poly_restore_to_poly(Data, PCB_TYPE_PAD, Element, pad);
		PCB_PAD_ROTATE90(pad, X, Y, Number);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_arc_rotate90(arc, X, Y, Number);
	}
	PCB_END_LOOP;
	PCB_COORD_ROTATE90(Element->MarkX, Element->MarkY, X, Y, Number);
	/* SetElementBoundingBox reenters the rtree data */
	pcb_element_bbox(Data, Element, pcb_font(PCB, 0, 1));
	pcb_poly_clear_from_poly(Data, PCB_TYPE_ELEMENT, Element, Element);
}

#if 0
unsigned int pcb_element_hash(const pcb_element_t *e)
{
	unsigned int val = 0;
	gdl_iterator_t it;

	{
		pcb_pin_t *p;
		pinlist_foreach(&e->Pin, &it, p) {
			val ^= pcb_pin_hash(e, p);
		}
	}

	{
		pcb_pad_t *p;
		padlist_foreach(&e->Pad, &it, p) {
			val ^= pcb_pad_hash(e, p);
		}
	}

	{
		pcb_line_t *l;
		linelist_foreach(&e->Line, &it, l) {
			val ^= pcb_line_hash(e, l);
		}
	}

	{
		pcb_arc_t *a;
		linelist_foreach(&e->Arc, &it, a) {
			val ^= pcb_arc_hash(e, a);
		}
	}

	return val;
}

int pcb_element_eq(const pcb_element_t *e1, const pcb_element_t *e2)
{
	/* Require the same objects in the same order - bail out at the first mismatch */

	{
		pcb_pin_t *p1, *p2;
		p1 = pinlist_first((pinlist_t *)&e1->Pin);
		p2 = pinlist_first((pinlist_t *)&e2->Pin);
		for(;;) {
			if ((p1 == NULL) && (p2 == NULL))
				break;
			if (!pcb_pin_eq(e1, p1, e2, p2))
				return 0;
			p1 = pinlist_next(p1);
			p2 = pinlist_next(p2);
		}
	}

	{
		pcb_pad_t *p1, *p2;
		p1 = padlist_first((padlist_t *)&e1->Pad);
		p2 = padlist_first((padlist_t *)&e2->Pad);
		for(;;) {
			if ((p1 == NULL) && (p2 == NULL))
				break;
			if (!pcb_pad_eq(e1, p1, e2, p2))
				return 0;
			p1 = padlist_next(p1);
			p2 = padlist_next(p2);
		}
	}

	{
		pcb_line_t *l1, *l2;
		l1 = linelist_first((linelist_t *)&e1->Line);
		l2 = linelist_first((linelist_t *)&e2->Line);
		for(;;) {
			if ((l1 == NULL) && (l2 == NULL))
				break;
			if (!pcb_line_eq(e1, l1, e2, l2))
				return 0;
			l1 = linelist_next(l1);
			l2 = linelist_next(l2);
		}
	}

	{
		pcb_arc_t *a1, *a2;
		a1 = arclist_first((arclist_t *)&e1->Arc);
		a2 = arclist_first((arclist_t *)&e2->Arc);
		for(;;) {
			if ((a1 == NULL) && (a2 == NULL))
				break;
			if (!pcb_arc_eq(e1, a1, e2, a2))
				return 0;
			a1 = arclist_next(a1);
			a2 = arclist_next(a2);
		}
	}

	return 1;
}
#endif


/*** ops ***/
/* copies a element to buffer */
void *pcb_elemop_add_to_buffer(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_element_t *element;

	element = pcb_element_alloc(ctx->buffer.dst);
	pcb_element_copy(ctx->buffer.dst, element, Element, pcb_false, 0, 0);
	PCB_FLAG_CLEAR(ctx->buffer.extraflg, element);
	if (ctx->buffer.extraflg) {
		PCB_ELEMENT_PCB_TEXT_LOOP(element);
		{
			PCB_FLAG_CLEAR(ctx->buffer.extraflg, text);
		}
		PCB_END_LOOP;
		PCB_PIN_LOOP(element);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_FOUND | ctx->buffer.extraflg, pin);
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			PCB_FLAG_CLEAR(PCB_FLAG_FOUND | ctx->buffer.extraflg, pad);
		}
		PCB_END_LOOP;
	}
	return element;
}

/* moves a element to buffer without allocating memory for pins/names */
void *pcb_elemop_move_buffer(pcb_opctx_t *ctx, pcb_element_t * element)
{
	/*
	 * Delete the element from the source (remove it from trees,
	 * restore to polygons)
	 */
	r_delete_element(ctx->buffer.src, element);

	elementlist_remove(element);
	elementlist_append(&ctx->buffer.dst->Element, element);

	PCB_PIN_LOOP(element);
	{
		pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_PIN, element, pin);
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(element);
	{
		pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_PAD, element, pad);
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, pad);
	}
	PCB_END_LOOP;
	pcb_element_bbox(ctx->buffer.dst, element, pcb_font(PCB, 0, 1));
	/*
	 * Now clear the from the polygons in the destination
	 */
	PCB_PIN_LOOP(element);
	{
		pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_PIN, element, pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(element);
	{
		pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_PAD, element, pad);
	}
	PCB_END_LOOP;

	PCB_SET_PARENT(element, data, ctx->buffer.dst);

	return element;
}


/* changes the drilling hole of all pins of an element; returns pcb_true if changed */
void *pcb_elemop_change_2nd_size(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool changed = pcb_false;
	pcb_coord_t value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : pin->DrillingHole + ctx->chgsize.value;
		if (value <= PCB_MAX_PINORVIASIZE &&
				value >= PCB_MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin) || value <= pin->Thickness - PCB_MIN_PINORVIACOPPER)
				&& value != pin->DrillingHole) {
			changed = pcb_true;
abort();
/*			pcb_undo_add_obj_to_2nd_size(PCB_TYPE_PIN, Element, pin, pin);*/
			pcb_pin_invalidate_erase(pin);
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->DrillingHole = value;
			if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)) {
				pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pcb_pin_invalidate_draw(pin);
		}
	}
	PCB_END_LOOP;
	if (changed)
		return Element;
	else
		return NULL;
}

/* changes ring dia of all pins of an element; returns pcb_true if changed */
void *pcb_elemop_change_1st_size(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool changed = pcb_false;
	pcb_coord_t value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : pin->DrillingHole + ctx->chgsize.value;
		if (value <= PCB_MAX_PINORVIASIZE && value >= pin->DrillingHole + PCB_MIN_PINORVIACOPPER && value != pin->Thickness) {
			changed = pcb_true;
			pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, pin, pin);
			pcb_pin_invalidate_erase(pin);
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->Thickness = value;
			if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)) {
				pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pcb_pin_invalidate_draw(pin);
		}
	}
	PCB_END_LOOP;
	if (changed)
		return Element;
	else
		return NULL;
}

/* changes the clearance of all pins of an element; returns pcb_true if changed */
void *pcb_elemop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool changed = pcb_false;
	pcb_coord_t value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : pin->Clearance + ctx->chgsize.value;
		if (value <= PCB_MAX_PINORVIASIZE &&
				value >= PCB_MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin) || value <= pin->Thickness - PCB_MIN_PINORVIACOPPER)
				&& value != pin->Clearance) {
			changed = pcb_true;
			pcb_undo_add_obj_to_clear_size(PCB_TYPE_PIN, Element, pin, pin);
			pcb_pin_invalidate_erase(pin);
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->Clearance = value;
			if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)) {
				pcb_undo_add_obj_to_size(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pcb_pin_invalidate_draw(pin);
		}
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(Element);
	{
		value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : pad->Clearance + ctx->chgsize.value;
		if (value <= PCB_MAX_PINORVIASIZE && value >= PCB_MIN_PINORVIAHOLE && value != pad->Clearance) {
			changed = pcb_true;
			pcb_undo_add_obj_to_clear_size(PCB_TYPE_PAD, Element, pad, pad);
			pcb_pad_invalidate_erase(pad);
			pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PAD, Element, pad);
			pcb_r_delete_entry(PCB->Data->pad_tree, &pad->BoundingBox);
			pad->Clearance = value;
			if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pad)) {
				pcb_undo_add_obj_to_size(PCB_TYPE_PAD, Element, pad, pad);
				pad->Thickness = value;
			}
			/* SetElementBB updates all associated rtrees */
			pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));

			pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PAD, Element, pad);
			pcb_pad_invalidate_draw(pad);
		}
	}
	PCB_END_LOOP;

	if (changed)
		return Element;
	else
		return NULL;
}

/* changes the scaling factor of an element's outline; returns pcb_true if changed */
void *pcb_elemop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_coord_t value;
	pcb_bool changed = pcb_false;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	if (pcb_silk_on(PCB))
		pcb_elem_invalidate_erase(Element);
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : line->Thickness + ctx->chgsize.value;
		if (value <= PCB_MAX_LINESIZE && value >= PCB_MIN_LINESIZE && value != line->Thickness) {
			pcb_undo_add_obj_to_size(PCB_TYPE_ELEMENT_LINE, Element, line, line);
			line->Thickness = value;
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		value = (ctx->chgsize.is_absolute) ? ctx->chgsize.value : arc->Thickness + ctx->chgsize.value;
		if (value <= PCB_MAX_LINESIZE && value >= PCB_MIN_LINESIZE && value != arc->Thickness) {
			pcb_undo_add_obj_to_size(PCB_TYPE_ELEMENT_ARC, Element, arc, arc);
			arc->Thickness = value;
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	if (pcb_silk_on(PCB)) {
		pcb_elem_invalidate_draw(Element);
	}
	if (changed)
		return Element;
	return NULL;
}

/* changes the scaling factor of a elementname object; returns pcb_true if changed */
void *pcb_elemop_change_name_size(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	int value = ctx->chgsize.is_absolute ? PCB_COORD_TO_MIL(ctx->chgsize.value)
		: PCB_ELEM_TEXT_DESCRIPTION(Element).Scale + PCB_COORD_TO_MIL(ctx->chgsize.value);

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, &Element->Name[0]))
		return NULL;
	if (value <= PCB_MAX_TEXTSCALE && value >= PCB_MIN_TEXTSCALE) {
		pcb_elem_name_invalidate_erase(Element);
		PCB_ELEMENT_PCB_TEXT_LOOP(Element);
		{
			pcb_undo_add_obj_to_size(PCB_TYPE_ELEMENT_NAME, Element, text, text);
			pcb_r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
			text->Scale = value;
			pcb_text_bbox(pcb_font(PCB, 0, 1), text);
			pcb_r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		}
		PCB_END_LOOP;
		pcb_elem_name_invalidate_draw(Element);
		return Element;
	}
	return NULL;
}


void *pcb_elemop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, &Element->Name[0]))
		return NULL;
	if (PCB_ELEMNAME_IDX_VISIBLE() == PCB_ELEMNAME_IDX_REFDES) {
		if (conf_core.editor.unique_names && pcb_element_uniq_name(PCB->Data, ctx->chgname.new_name) != ctx->chgname.new_name) {
			pcb_message(PCB_MSG_ERROR, _("Error: The name \"%s\" is not unique!\n"), ctx->chgname.new_name);
			return ((char *) -1);
		}
	}

	return pcb_element_text_change(PCB, PCB->Data, Element, PCB_ELEMNAME_IDX_VISIBLE(), ctx->chgname.new_name);
}

void *pcb_elemop_change_nonetlist(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_FLAG_TOGGLE(PCB_FLAG_NONETLIST, Element);
	return Element;
}


/* changes the square flag of all pins on an element */
void *pcb_elemop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *ans = NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		ans = pcb_pinop_change_square(ctx, Element, pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		ans = pcb_padop_change_square(ctx, Element, pad);
	}
	PCB_END_LOOP;
	return ans;
}

/* sets the square flag of all pins on an element */
void *pcb_elemop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *ans = NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		ans = pcb_pinop_set_square(ctx, Element, pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		ans = pcb_padop_set_square(ctx, Element, pad);
	}
	PCB_END_LOOP;
	return ans;
}

/* clears the square flag of all pins on an element */
void *pcb_elemop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *ans = NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		ans = pcb_pinop_clear_square(ctx, Element, pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		ans = pcb_padop_clear_square(ctx, Element, pad);
	}
	PCB_END_LOOP;
	return ans;
}

/* changes the octagon flags of all pins of an element */
void *pcb_elemop_change_octagon(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *result = NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		pcb_pinop_change_octagon(ctx, Element, pin);
		result = Element;
	}
	PCB_END_LOOP;
	return result;
}

/* sets the octagon flags of all pins of an element */
void *pcb_elemop_set_octagon(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *result = NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		pcb_pinop_set_octagon(ctx, Element, pin);
		result = Element;
	}
	PCB_END_LOOP;
	return result;
}

/* clears the octagon flags of all pins of an element */
void *pcb_elemop_clear_octagon(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *result = NULL;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Element))
		return NULL;
	PCB_PIN_LOOP(Element);
	{
		pcb_pinop_clear_octagon(ctx, Element, pin);
		result = Element;
	}
	PCB_END_LOOP;
	return result;
}

/* copies an element onto the PCB.  Then does a draw. */
void *pcb_elemop_copy(pcb_opctx_t *ctx, pcb_element_t *Element)
{

#ifdef DEBUG
	printf("Entered pcb_elemop_copy, trying to copy element %s\n", Element->Name[1].TextString);
#endif

	pcb_element_t *element = pcb_element_copy(PCB->Data,
																							 NULL, Element,
																							 conf_core.editor.unique_names, ctx->copy.DeltaX,
																							 ctx->copy.DeltaY);

	/* this call clears the polygons */
	pcb_undo_add_obj_to_create(PCB_TYPE_ELEMENT, element, element, element);
	if (pcb_silk_on(PCB) && (PCB_FRONT(element) || PCB->InvisibleObjectsOn)) {
		pcb_elem_name_invalidate_draw(element);
		pcb_elem_package_invalidate_draw(element);
	}
	if (PCB->PinOn) {
		pcb_elem_pp_invalidate_draw(element);
	}
#ifdef DEBUG
	printf(" ... Leaving pcb_elemop_copy.\n");
#endif
	return element;
}

/* moves all names of an element to a new position */
void *pcb_elemop_move_name(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (pcb_silk_on(PCB) && (PCB_FRONT(Element) || PCB->InvisibleObjectsOn)) {
		pcb_elem_name_invalidate_erase(Element);
		PCB_ELEMENT_PCB_TEXT_LOOP(Element);
		{
			if (PCB->Data->name_tree[n])
				pcb_r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
			pcb_text_move(text, ctx->move.dx, ctx->move.dy);
			if (PCB->Data->name_tree[n])
				pcb_r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		}
		PCB_END_LOOP;
		pcb_elem_name_invalidate_draw(Element);
	}
	else {
		PCB_ELEMENT_PCB_TEXT_LOOP(Element);
		{
			if (PCB->Data->name_tree[n])
				pcb_r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
			pcb_text_move(text, ctx->move.dx, ctx->move.dy);
			if (PCB->Data->name_tree[n])
				pcb_r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		}
		PCB_END_LOOP;
	}
	return Element;
}

/* moves an element */
void *pcb_elemop_move(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (pcb_silk_on(PCB) && (PCB_FRONT(Element) || PCB->InvisibleObjectsOn)) {
		pcb_elem_invalidate_erase(Element);
		pcb_element_move(PCB->Data, Element, ctx->move.dx, ctx->move.dy);
		pcb_elem_name_invalidate_draw(Element);
		pcb_elem_package_invalidate_draw(Element);
	}
	else {
		if (PCB->PinOn)
			pcb_elem_pp_invalidate_erase(Element);
		pcb_element_move(PCB->Data, Element, ctx->move.dx, ctx->move.dy);
	}
	if (PCB->PinOn) {
		pcb_elem_pp_invalidate_draw(Element);
	}
	return Element;
}

/* destroys a element */
void *pcb_elemop_destroy(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (ctx->remove.destroy_target->element_tree)
		pcb_r_delete_entry(ctx->remove.destroy_target->element_tree, (pcb_box_t *) Element);
	if (ctx->remove.destroy_target->pin_tree) {
		PCB_PIN_LOOP(Element);
		{
			pcb_r_delete_entry(ctx->remove.destroy_target->pin_tree, (pcb_box_t *) pin);
		}
		PCB_END_LOOP;
	}
	if (ctx->remove.destroy_target->pad_tree) {
		PCB_PAD_LOOP(Element);
		{
			pcb_r_delete_entry(ctx->remove.destroy_target->pad_tree, (pcb_box_t *) pad);
		}
		PCB_END_LOOP;
	}
	PCB_ELEMENT_PCB_TEXT_LOOP(Element);
	{
		if (ctx->remove.destroy_target->name_tree[n])
			pcb_r_delete_entry(ctx->remove.destroy_target->name_tree[n], (pcb_box_t *) text);
	}
	PCB_END_LOOP;
	pcb_element_destroy(Element);

	pcb_element_free(Element);

	return NULL;
}

/* removes an element */
void *pcb_elemop_remove(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	/* erase from screen */
	if ((pcb_silk_on(PCB) || PCB->PinOn) && (PCB_FRONT(Element) || PCB->InvisibleObjectsOn))
		pcb_elem_invalidate_erase(Element);
	pcb_undo_move_obj_to_remove(PCB_TYPE_ELEMENT, Element, Element, Element);
	return NULL;
}

/* rotates an element */
void *pcb_elemop_rotate90(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_elem_invalidate_erase(Element);
	pcb_element_rotate90(PCB->Data, Element, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	pcb_elem_invalidate_draw(Element);
	return Element;
}

/* ----------------------------------------------------------------------
 * rotates the name of an element
 */
void *pcb_elemop_rotate90_name(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_elem_name_invalidate_erase(Element);
	PCB_ELEMENT_PCB_TEXT_LOOP(Element);
	{
		pcb_r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		pcb_text_rotate90(text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
		pcb_r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
	}
	PCB_END_LOOP;
	pcb_elem_name_invalidate_draw(Element);
	return Element;
}

/*** draw ***/
void pcb_elem_name_draw(pcb_element_t * element)
{
	if ((conf_core.editor.hide_names && pcb_gui->gui) || PCB_FLAG_TEST(PCB_FLAG_HIDENAME, element))
		return;
	if (pcb_draw_doing_pinout || pcb_draw_doing_assy)
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element);
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, &PCB_ELEM_TEXT_VISIBLE(PCB, element)))
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element_selected);
	else if (PCB_FRONT(element)) {
#warning TODO: why do we test for Names flag here instead of elements flag?
		if (PCB_FLAG_TEST(PCB_FLAG_NONETLIST, element))
			pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element_nonetlist);
		else
			pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element);
	}
	else
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.invisible_objects);

	pcb_text_draw_(&PCB_ELEM_TEXT_VISIBLE(PCB, element), PCB->minSlk, 0);

}

pcb_r_dir_t pcb_elem_name_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	pcb_element_t *element = (pcb_element_t *) text->Element;
	int *side = cl;

	if (PCB_FLAG_TEST(PCB_FLAG_HIDENAME, element))
		return PCB_R_DIR_NOT_FOUND;

	if (PCB_ON_SIDE(element, *side))
		pcb_elem_name_draw(element);
	return PCB_R_DIR_NOT_FOUND;
}

void pcb_elem_pp_draw(pcb_element_t * element)
{
}


void pcb_elem_package_draw(pcb_element_t * element)
{
	/* set color and draw lines, arcs, text and pins */
	if (pcb_draw_doing_pinout || pcb_draw_doing_assy)
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element);
	else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element_selected);
	else if (PCB_FRONT(element))
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.element);
	else
		pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.invisible_objects);

	/* draw lines, arcs, text and pins */
	PCB_ELEMENT_PCB_LINE_LOOP(element);
	{
		pcb_line_draw_(line, 0);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(element);
	{
		pcb_arc_draw_(arc, 0);
	}
	PCB_END_LOOP;
}

void pcb_elem_draw(pcb_element_t *element)
{
	pcb_elem_package_draw(element);
	pcb_elem_name_draw(element);
	pcb_elem_pp_draw(element);
}

pcb_r_dir_t pcb_elem_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_element_t *element = (pcb_element_t *) b;
	int *side = cl;

	if (PCB_ON_SIDE(element, *side))
		pcb_elem_package_draw(element);
	return PCB_R_DIR_FOUND_CONTINUE;
}

static void DrawEMark(pcb_element_t *e, pcb_coord_t X, pcb_coord_t Y, pcb_bool invisible)
{
	pcb_coord_t mark_size = PCB_EMARK_SIZE;
	if (!PCB->InvisibleObjectsOn && invisible)
		return;

	if (pinlist_length(&e->Pin) != 0) {
		pcb_pin_t *pin0 = pinlist_first(&e->Pin);
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pin0))
			mark_size = MIN(mark_size, pin0->DrillingHole / 2);
		else
			mark_size = MIN(mark_size, pin0->Thickness / 2);
	}

	if (padlist_length(&e->Pad) != 0) {
		pcb_pad_t *pad0 = padlist_first(&e->Pad);
		mark_size = MIN(mark_size, pad0->Thickness / 2);
	}

	pcb_gui->set_color(pcb_draw_out.fgGC, invisible ? conf_core.appearance.color.invisible_mark : conf_core.appearance.color.element);
	pcb_gui->set_line_cap(pcb_draw_out.fgGC, Trace_Cap);
	pcb_gui->set_line_width(pcb_draw_out.fgGC, 0);
	pcb_gui->draw_line(pcb_draw_out.fgGC, X - mark_size, Y, X, Y - mark_size);
	pcb_gui->draw_line(pcb_draw_out.fgGC, X + mark_size, Y, X, Y - mark_size);
	pcb_gui->draw_line(pcb_draw_out.fgGC, X - mark_size, Y, X, Y + mark_size);
	pcb_gui->draw_line(pcb_draw_out.fgGC, X + mark_size, Y, X, Y + mark_size);

	/*
	 * If an element is locked, place a "L" on top of the "diamond".
	 * This provides a nice visual indication that it is locked that
	 * works even for color blind users.
	 */
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, e)) {
		pcb_gui->draw_line(pcb_draw_out.fgGC, X, Y, X + 2 * mark_size, Y);
		pcb_gui->draw_line(pcb_draw_out.fgGC, X, Y, X, Y - 4 * mark_size);
	}
}

pcb_r_dir_t pcb_elem_mark_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_element_t *element = (pcb_element_t *) b;

	DrawEMark(element, element->MarkX, element->MarkY, !PCB_FRONT(element));
	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_elem_invalidate_erase(pcb_element_t *Element)
{
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_line_invalidate_erase(line);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_arc_invalidate_erase(arc);
	}
	PCB_END_LOOP;
	pcb_elem_name_invalidate_erase(Element);
	pcb_elem_pp_invalidate_erase(Element);
	pcb_flag_erase(&Element->Flags);
}

void pcb_elem_pp_invalidate_erase(pcb_element_t *Element)
{
	PCB_PIN_LOOP(Element);
	{
		pcb_pin_invalidate_erase(pin);
	}
	PCB_END_LOOP;
	PCB_PAD_LOOP(Element);
	{
		pcb_pad_invalidate_erase(pad);
	}
	PCB_END_LOOP;
}

void pcb_elem_name_invalidate_erase(pcb_element_t *Element)
{
	if (PCB_FLAG_TEST(PCB_FLAG_HIDENAME, Element)) {
		return;
	}
	pcb_text_invalidate_draw(NULL, &PCB_ELEM_TEXT_VISIBLE(PCB, Element));
}

void pcb_elem_invalidate_draw(pcb_element_t *Element)
{
	pcb_elem_package_invalidate_draw(Element);
	pcb_elem_name_invalidate_draw(Element);
	pcb_elem_pp_invalidate_draw(Element);
}

void pcb_elem_name_invalidate_draw(pcb_element_t *Element)
{
	if (PCB_FLAG_TEST(PCB_FLAG_HIDENAME, Element))
		return;
	pcb_text_invalidate_draw(NULL, &PCB_ELEM_TEXT_VISIBLE(PCB, Element));
}

void pcb_elem_package_invalidate_draw(pcb_element_t *Element)
{
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_line_invalidate_draw(NULL, line);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_arc_invalidate_draw(NULL, arc);
	}
	PCB_END_LOOP;
}

void pcb_elem_pp_invalidate_draw(pcb_element_t *Element)
{
	PCB_PAD_LOOP(Element);
	{
		if (pcb_draw_doing_pinout || pcb_draw_doing_assy || PCB_FRONT(pad) || PCB->InvisibleObjectsOn)
			pcb_pad_invalidate_draw(pad);
	}
	PCB_END_LOOP;
	PCB_PIN_LOOP(Element);
	{
		pcb_pin_invalidate_draw(pin);
	}
	PCB_END_LOOP;
}

pcb_bool pcb_layer_is_paste_auto_empty(pcb_board_t *pcb, pcb_side_t side)
{
	pcb_bool paste_empty = pcb_true;
	PCB_PAD_ALL_LOOP(pcb->Data);
	{
		if (PCB_ON_SIDE(pad, side) && !PCB_FLAG_TEST(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			paste_empty = pcb_false;
			break;
		}
	}
	PCB_ENDALL_LOOP;
	return paste_empty;
}
