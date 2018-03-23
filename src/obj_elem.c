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

/*** ops ***/

/* removes an element */
void *pcb_elemop_remove(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	/* erase from screen */
	if ((pcb_silk_on(PCB) || PCB->PinOn) && (PCB_FRONT(Element) || PCB->InvisibleObjectsOn))
		pcb_elem_invalidate_erase(Element);
	pcb_undo_move_obj_to_remove(PCB_TYPE_ELEMENT, Element, Element, Element);
	return NULL;
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
