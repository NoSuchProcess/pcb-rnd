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

/* TODO: remove this: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_elem_draw.h"

/*** allocation ***/

/* get next slot for an element, allocates memory if necessary */
pcb_element_t *GetElementMemory(pcb_data_t * data)
{
	pcb_element_t *new_obj;

	new_obj = calloc(sizeof(pcb_element_t), 1);
	elementlist_append(&data->Element, new_obj);

	return new_obj;
}

void RemoveFreeElement(pcb_element_t * data)
{
	elementlist_remove(data);
	free(data);
}

/* frees memory used by an element */
void FreeElementMemory(pcb_element_t * element)
{
	if (element == NULL)
		return;

	ELEMENTNAME_LOOP(element);
	{
		free(textstring);
	}
	END_LOOP;
	PIN_LOOP(element);
	{
		free(pin->Name);
		free(pin->Number);
	}
	END_LOOP;
	PAD_LOOP(element);
	{
		free(pad->Name);
		free(pad->Number);
	}
	END_LOOP;

	list_map0(&element->Pin, pcb_pin_t, RemoveFreePin);
	list_map0(&element->Pad, pcb_pad_t, RemoveFreePad);
	list_map0(&element->Line, pcb_line_t, RemoveFreeLine);
	list_map0(&element->Arc,  pcb_arc_t,  RemoveFreeArc);

	FreeAttributeListMemory(&element->Attributes);
	reset_obj_mem(pcb_element_t, element);
}

pcb_line_t *GetElementLineMemory(pcb_element_t *Element)
{
	pcb_line_t *line = calloc(sizeof(pcb_line_t), 1);
	linelist_append(&Element->Line, line);

	return line;
}

/*** utility ***/
/* loads element data from file/library into buffer
 * parse the file with disabled 'PCB mode' (see parser)
 * returns pcb_false on error
 * if successful, update some other stuff and reposition the pastebuffer
 */
pcb_bool LoadElementToBuffer(pcb_buffer_t *Buffer, const char *Name)
{
	pcb_element_t *element;

	ClearBuffer(Buffer);
	if (!ParseElement(Buffer->Data, Name)) {
		if (conf_core.editor.show_solder_side)
			SwapBuffer(Buffer);
		SetBufferBoundingBox(Buffer);
		if (elementlist_length(&Buffer->Data->Element)) {
			element = elementlist_first(&Buffer->Data->Element);
			Buffer->X = element->MarkX;
			Buffer->Y = element->MarkY;
		}
		else {
			Buffer->X = 0;
			Buffer->Y = 0;
		}
		return (pcb_true);
	}

	/* release memory which might have been acquired */
	ClearBuffer(Buffer);
	return (pcb_false);
}


/* Searches for the given element by "footprint" name, and loads it
   into the buffer. Returns zero on success, non-zero on error.  */
int LoadFootprintByName(pcb_buffer_t *Buffer, const char *Footprint)
{
	return !LoadElementToBuffer(Buffer, Footprint);
}


/* break buffer element into pieces */
pcb_bool SmashBufferElement(pcb_buffer_t *Buffer)
{
	pcb_element_t *element;
	pcb_cardinal_t group;
	pcb_layer_t *clayer, *slayer;

	if (elementlist_length(&Buffer->Data->Element) != 1) {
		Message(PCB_MSG_DEFAULT, _("Error!  Buffer doesn't contain a single element\n"));
		return (pcb_false);
	}
	/*
	 * At this point the buffer should contain just a single element.
	 * Now we detach the single element from the buffer and then clear the
	 * buffer, ready to receive the smashed elements.  As a result of detaching
	 * it the single element is orphaned from the buffer and thus will not be
	 * free()'d by FreeDataMemory (called via ClearBuffer).  This leaves it
	 * around for us to smash bits off it.  It then becomes our responsibility,
	 * however, to free the single element when we're finished with it.
	 */
	element = elementlist_first(&Buffer->Data->Element);
	elementlist_remove(element);
	ClearBuffer(Buffer);
	ELEMENTLINE_LOOP(element);
	{
		CreateNewLineOnLayer(&Buffer->Data->SILKLAYER,
												 line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness, 0, NoFlags());
		if (line)
			line->Number = pcb_strdup_null(NAMEONPCB_NAME(element));
	}
	END_LOOP;
	ARC_LOOP(element);
	{
		CreateNewArcOnLayer(&Buffer->Data->SILKLAYER,
												arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness, 0, NoFlags());
	}
	END_LOOP;
	PIN_LOOP(element);
	{
		pcb_flag_t f = NoFlags();
		AddFlags(f, PCB_FLAG_VIA);
		if (TEST_FLAG(PCB_FLAG_HOLE, pin))
			AddFlags(f, PCB_FLAG_HOLE);

		CreateNewVia(Buffer->Data, pin->X, pin->Y, pin->Thickness, pin->Clearance, pin->Mask, pin->DrillingHole, pin->Number, f);
	}
	END_LOOP;
	group = GetLayerGroupNumberByNumber(SWAP_IDENT ? solder_silk_layer : component_silk_layer);
	clayer = &Buffer->Data->Layer[PCB->LayerGroups.Entries[group][0]];
	group = GetLayerGroupNumberByNumber(SWAP_IDENT ? component_silk_layer : solder_silk_layer);
	slayer = &Buffer->Data->Layer[PCB->LayerGroups.Entries[group][0]];
	PAD_LOOP(element);
	{
		pcb_line_t *line;
		line = CreateNewLineOnLayer(TEST_FLAG(PCB_FLAG_ONSOLDER, pad) ? slayer : clayer,
																pad->Point1.X, pad->Point1.Y,
																pad->Point2.X, pad->Point2.Y, pad->Thickness, pad->Clearance, NoFlags());
		if (line)
			line->Number = pcb_strdup_null(pad->Number);
	}
	END_LOOP;
	FreeElementMemory(element);
	RemoveFreeElement(element);
	return (pcb_true);
}

/* see if a polygon is a rectangle.  If so, canonicalize it. */
static int polygon_is_rectangle(pcb_polygon_t *poly)
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
pcb_bool ConvertBufferToElement(pcb_buffer_t *Buffer)
{
	pcb_element_t *Element;
	pcb_cardinal_t group;
	pcb_cardinal_t pin_n = 1;
	pcb_bool hasParts = pcb_false, crooked = pcb_false;
	int onsolder;
	pcb_bool warned = pcb_false;

	if (Buffer->Data->pcb == 0)
		Buffer->Data->pcb = PCB;

	Element = CreateNewElement(PCB->Data, NULL, &PCB->Font, NoFlags(),
														 NULL, NULL, NULL, PASTEBUFFER->X,
														 PASTEBUFFER->Y, 0, 100, MakeFlags(SWAP_IDENT ? PCB_FLAG_ONSOLDER : PCB_FLAG_NO), pcb_false);
	if (!Element)
		return (pcb_false);
	VIA_LOOP(Buffer->Data);
	{
		char num[8];
		if (via->Mask < via->Thickness)
			via->Mask = via->Thickness + 2 * MASKFRAME;
		if (via->Name)
			CreateNewPin(Element, via->X, via->Y, via->Thickness,
									 via->Clearance, via->Mask, via->DrillingHole,
									 NULL, via->Name, MaskFlags(via->Flags, PCB_FLAG_VIA | PCB_FLAG_FOUND | PCB_FLAG_SELECTED | PCB_FLAG_WARN));
		else {
			sprintf(num, "%d", pin_n++);
			CreateNewPin(Element, via->X, via->Y, via->Thickness,
									 via->Clearance, via->Mask, via->DrillingHole,
									 NULL, num, MaskFlags(via->Flags, PCB_FLAG_VIA | PCB_FLAG_FOUND | PCB_FLAG_SELECTED | PCB_FLAG_WARN));
		}
		hasParts = pcb_true;
	}
	END_LOOP;

	for (onsolder = 0; onsolder < 2; onsolder++) {
		int silk_layer;
		int onsolderflag;

		if ((!onsolder) == (!SWAP_IDENT)) {
			silk_layer = component_silk_layer;
			onsolderflag = PCB_FLAG_NO;
		}
		else {
			silk_layer = solder_silk_layer;
			onsolderflag = PCB_FLAG_ONSOLDER;
		}

#define MAYBE_WARN() \
	  if (onsolder && !hasParts && !warned) \
	    { \
	      warned = pcb_true; \
	      Message \
					(PCB_MSG_WARNING, _("Warning: All of the pads are on the opposite\n" \
		   "side from the component - that's probably not what\n" \
		   "you wanted\n")); \
	    } \

		/* get the component-side SM pads */
		group = GetLayerGroupNumberByNumber(silk_layer);
		GROUP_LOOP(Buffer->Data, group);
		{
			char num[8];
			LINE_LOOP(layer);
			{
				sprintf(num, "%d", pin_n++);
				CreateNewPad(Element, line->Point1.X,
										 line->Point1.Y, line->Point2.X,
										 line->Point2.Y, line->Thickness,
										 line->Clearance,
										 line->Thickness + line->Clearance, NULL, line->Number ? line->Number : num, MakeFlags(onsolderflag));
				MAYBE_WARN();
				hasParts = pcb_true;
			}
			END_LOOP;
			POLYGON_LOOP(layer);
			{
				pcb_coord_t x1, y1, x2, y2, w, h, t;

				if (!polygon_is_rectangle(polygon)) {
					crooked = pcb_true;
					continue;
				}

				w = polygon->Points[2].X - polygon->Points[0].X;
				h = polygon->Points[1].Y - polygon->Points[0].Y;
				t = (w < h) ? w : h;
				x1 = polygon->Points[0].X + t / 2;
				y1 = polygon->Points[0].Y + t / 2;
				x2 = x1 + (w - t);
				y2 = y1 + (h - t);

				sprintf(num, "%d", pin_n++);
				CreateNewPad(Element,
										 x1, y1, x2, y2, t,
										 2 * conf_core.design.clearance, t + conf_core.design.clearance, NULL, num, MakeFlags(PCB_FLAG_SQUARE | onsolderflag));
				MAYBE_WARN();
				hasParts = pcb_true;
			}
			END_LOOP;
		}
		END_LOOP;
	}

	/* now add the silkscreen. NOTE: elements must have pads or pins too */
	LINE_LOOP(&Buffer->Data->SILKLAYER);
	{
		if (line->Number && !NAMEONPCB_NAME(Element))
			NAMEONPCB_NAME(Element) = pcb_strdup(line->Number);
		CreateNewLineInElement(Element, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness);
		hasParts = pcb_true;
	}
	END_LOOP;
	ARC_LOOP(&Buffer->Data->SILKLAYER);
	{
		CreateNewArcInElement(Element, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
		hasParts = pcb_true;
	}
	END_LOOP;
	if (!hasParts) {
		DestroyObject(PCB->Data, PCB_TYPE_ELEMENT, Element, Element, Element);
		Message(PCB_MSG_DEFAULT, _("There was nothing to convert!\n" "Elements must have some silk, pads or pins.\n"));
		return (pcb_false);
	}
	if (crooked)
		Message(PCB_MSG_DEFAULT, _("There were polygons that can't be made into pins!\n" "So they were not included in the element\n"));
	Element->MarkX = Buffer->X;
	Element->MarkY = Buffer->Y;
	if (SWAP_IDENT)
		SET_FLAG(PCB_FLAG_ONSOLDER, Element);
	SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
	ClearBuffer(Buffer);
	MoveObjectToBuffer(Buffer->Data, PCB->Data, PCB_TYPE_ELEMENT, Element, Element, Element);
	SetBufferBoundingBox(Buffer);
	return (pcb_true);
}

void FreeRotateElementLowLevel(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle)
{
	/* solder side objects need a different orientation */

	/* the text subroutine decides by itself if the direction
	 * is to be corrected
	 */
#if 0
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(Data->name_tree[n], (pcb_box_t *) text);
		RotateTextLowLevel(text, X, Y, Number);
	}
	END_LOOP;
#endif
	ELEMENTLINE_LOOP(Element);
	{
		free_rotate(&line->Point1.X, &line->Point1.Y, X, Y, cosa, sina);
		free_rotate(&line->Point2.X, &line->Point2.Y, X, Y, cosa, sina);
		SetLineBoundingBox(line);
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		/* pre-delete the pins from the pin-tree before their coordinates change */
		if (Data)
			r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
		RestoreToPolygon(Data, PCB_TYPE_PIN, Element, pin);
		free_rotate(&pin->X, &pin->Y, X, Y, cosa, sina);
		SetPinBoundingBox(pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		/* pre-delete the pads before their coordinates change */
		if (Data)
			r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
		RestoreToPolygon(Data, PCB_TYPE_PAD, Element, pad);
		free_rotate(&pad->Point1.X, &pad->Point1.Y, X, Y, cosa, sina);
		free_rotate(&pad->Point2.X, &pad->Point2.Y, X, Y, cosa, sina);
		SetLineBoundingBox((pcb_line_t *) pad);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		free_rotate(&arc->X, &arc->Y, X, Y, cosa, sina);
		arc->StartAngle = NormalizeAngle(arc->StartAngle + angle);
	}
	END_LOOP;

	free_rotate(&Element->MarkX, &Element->MarkY, X, Y, cosa, sina);
	SetElementBoundingBox(Data, Element, &PCB->Font);
	ClearFromPolygon(Data, PCB_TYPE_ELEMENT, Element, Element);
}

/* changes the side of the board an element is on; returns pcb_true if done */
pcb_bool ChangeElementSide(pcb_element_t *Element, pcb_coord_t yoff)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (pcb_false);
	EraseElement(Element);
	AddObjectToMirrorUndoList(PCB_TYPE_ELEMENT, Element, Element, Element, yoff);
	MirrorElementCoordinates(PCB->Data, Element, yoff);
	DrawElement(Element);
	return (pcb_true);
}

/* changes the side of all selected and visible elements;
   returns pcb_true if anything has changed */
pcb_bool ChangeSelectedElementSide(void)
{
	pcb_bool change = pcb_false;

	if (PCB->PinOn && PCB->ElementOn)
		ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, element)) {
			change |= ChangeElementSide(element, 0);
		}
	}
	END_LOOP;
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* changes the layout-name of an element */
char *ChangeElementText(pcb_board_t * pcb, pcb_data_t * data, pcb_element_t *Element, int which, char *new_name)
{
	char *old = Element->Name[which].TextString;

#ifdef DEBUG
	printf("In ChangeElementText, updating old TextString %s to %s\n", old, new_name);
#endif

	if (pcb && which == NAME_INDEX())
		EraseElementName(Element);

	r_delete_entry(data->name_tree[which], &Element->Name[which].BoundingBox);

	Element->Name[which].TextString = new_name;
	SetTextBoundingBox(&PCB->Font, &Element->Name[which]);

	r_insert_entry(data->name_tree[which], &Element->Name[which].BoundingBox, 0);

	if (pcb && which == NAME_INDEX())
		DrawElementName(Element);

	return old;
}

/* copies data from one element to another and creates the destination; if necessary */
pcb_element_t *CopyElementLowLevel(pcb_data_t *Data, pcb_element_t *Dest, pcb_element_t *Src, pcb_bool uniqueName, pcb_coord_t dx, pcb_coord_t dy)
{
	int i;
	/* release old memory if necessary */
	if (Dest)
		FreeElementMemory(Dest);

	/* both coordinates and flags are the same */
	Dest = CreateNewElement(Data, Dest, &PCB->Font,
													MaskFlags(Src->Flags, PCB_FLAG_FOUND),
													DESCRIPTION_NAME(Src), NAMEONPCB_NAME(Src),
													VALUE_NAME(Src), DESCRIPTION_TEXT(Src).X + dx,
													DESCRIPTION_TEXT(Src).Y + dy,
													DESCRIPTION_TEXT(Src).Direction,
													DESCRIPTION_TEXT(Src).Scale, MaskFlags(DESCRIPTION_TEXT(Src).Flags, PCB_FLAG_FOUND), uniqueName);

	/* abort on error */
	if (!Dest)
		return (Dest);

	ELEMENTLINE_LOOP(Src);
	{
		CreateNewLineInElement(Dest, line->Point1.X + dx,
													 line->Point1.Y + dy, line->Point2.X + dx, line->Point2.Y + dy, line->Thickness);
	}
	END_LOOP;
	PIN_LOOP(Src);
	{
		CreateNewPin(Dest, pin->X + dx, pin->Y + dy, pin->Thickness,
								 pin->Clearance, pin->Mask, pin->DrillingHole, pin->Name, pin->Number, MaskFlags(pin->Flags, PCB_FLAG_FOUND));
	}
	END_LOOP;
	PAD_LOOP(Src);
	{
		CreateNewPad(Dest, pad->Point1.X + dx, pad->Point1.Y + dy,
								 pad->Point2.X + dx, pad->Point2.Y + dy, pad->Thickness,
								 pad->Clearance, pad->Mask, pad->Name, pad->Number, MaskFlags(pad->Flags, PCB_FLAG_FOUND));
	}
	END_LOOP;
	ARC_LOOP(Src);
	{
		CreateNewArcInElement(Dest, arc->X + dx, arc->Y + dy, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
	}
	END_LOOP;

	for (i = 0; i < Src->Attributes.Number; i++)
		AttributePutToList(&Dest->Attributes, Src->Attributes.List[i].name, Src->Attributes.List[i].value, 0);

	Dest->MarkX = Src->MarkX + dx;
	Dest->MarkY = Src->MarkY + dy;

	SetElementBoundingBox(Data, Dest, &PCB->Font);
	return (Dest);
}

/* creates an new element; memory is allocated if needed */
pcb_element_t *CreateNewElement(pcb_data_t *Data, pcb_element_t *Element,
	pcb_font_t *PCBFont, pcb_flag_t Flags, char *Description, char *NameOnPCB,
	char *Value, pcb_coord_t TextX, pcb_coord_t TextY, pcb_uint8_t Direction,
	int TextScale, pcb_flag_t TextFlags, pcb_bool uniqueName)
{
#ifdef DEBUG
	printf("Entered CreateNewElement.....\n");
#endif

	if (!Element)
		Element = GetElementMemory(Data);

	/* copy values and set additional information */
	TextScale = MAX(MIN_TEXTSCALE, TextScale);
	AddTextToElement(&DESCRIPTION_TEXT(Element), PCBFont, TextX, TextY, Direction, Description, TextScale, TextFlags);
	if (uniqueName)
		NameOnPCB = UniqueElementName(Data, NameOnPCB);
	AddTextToElement(&NAMEONPCB_TEXT(Element), PCBFont, TextX, TextY, Direction, NameOnPCB, TextScale, TextFlags);
	AddTextToElement(&VALUE_TEXT(Element), PCBFont, TextX, TextY, Direction, Value, TextScale, TextFlags);
	DESCRIPTION_TEXT(Element).Element = Element;
	NAMEONPCB_TEXT(Element).Element = Element;
	VALUE_TEXT(Element).Element = Element;
	Element->Flags = Flags;
	Element->ID = CreateIDGet();

#ifdef DEBUG
	printf("  .... Leaving CreateNewElement.\n");
#endif

	return (Element);
}

/* creates a new arc in an element */
pcb_arc_t *CreateNewArcInElement(pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y,
	pcb_coord_t Width, pcb_coord_t Height, pcb_angle_t angle, pcb_angle_t delta, pcb_coord_t Thickness)
{
	pcb_arc_t *arc = GetElementArcMemory(Element);

	/* set Delta (0,360], StartAngle in [0,360) */
	if (delta < 0) {
		delta = -delta;
		angle -= delta;
	}
	angle = NormalizeAngle(angle);
	delta = NormalizeAngle(delta);
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
	arc->ID = CreateIDGet();
	return arc;
}

/* creates a new line for an element */
pcb_line_t *CreateNewLineInElement(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness)
{
	pcb_line_t *line;

	if (Thickness == 0)
		return NULL;

	line = GetElementLineMemory(Element);

	/* copy values */
	line->Point1.X = X1;
	line->Point1.Y = Y1;
	line->Point2.X = X2;
	line->Point2.Y = Y2;
	line->Thickness = Thickness;
	line->Flags = NoFlags();
	line->ID = CreateIDGet();
	return line;
}

/* creates a new textobject as part of an element
   copies the values to the appropriate text object */
void AddTextToElement(pcb_text_t *Text, pcb_font_t *PCBFont, pcb_coord_t X, pcb_coord_t Y,
	unsigned Direction, char *TextString, int Scale, pcb_flag_t Flags)
{
	free(Text->TextString);
	Text->TextString = (TextString && *TextString) ? pcb_strdup(TextString) : NULL;
	Text->X = X;
	Text->Y = Y;
	Text->Direction = Direction;
	Text->Flags = Flags;
	Text->Scale = Scale;

	/* calculate size of the bounding box */
	SetTextBoundingBox(PCBFont, Text);
	Text->ID = CreateIDGet();
}

/* mirrors the coordinates of an element; an additional offset is passed */
void MirrorElementCoordinates(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t yoff)
{
	r_delete_element(Data, Element);
	ELEMENTLINE_LOOP(Element);
	{
		line->Point1.X = SWAP_X(line->Point1.X);
		line->Point1.Y = SWAP_Y(line->Point1.Y) + yoff;
		line->Point2.X = SWAP_X(line->Point2.X);
		line->Point2.Y = SWAP_Y(line->Point2.Y) + yoff;
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		RestoreToPolygon(Data, PCB_TYPE_PIN, Element, pin);
		pin->X = SWAP_X(pin->X);
		pin->Y = SWAP_Y(pin->Y) + yoff;
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		RestoreToPolygon(Data, PCB_TYPE_PAD, Element, pad);
		pad->Point1.X = SWAP_X(pad->Point1.X);
		pad->Point1.Y = SWAP_Y(pad->Point1.Y) + yoff;
		pad->Point2.X = SWAP_X(pad->Point2.X);
		pad->Point2.Y = SWAP_Y(pad->Point2.Y) + yoff;
		TOGGLE_FLAG(PCB_FLAG_ONSOLDER, pad);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		arc->X = SWAP_X(arc->X);
		arc->Y = SWAP_Y(arc->Y) + yoff;
		arc->StartAngle = SWAP_ANGLE(arc->StartAngle);
		arc->Delta = SWAP_DELTA(arc->Delta);
	}
	END_LOOP;
	ELEMENTTEXT_LOOP(Element);
	{
		text->X = SWAP_X(text->X);
		text->Y = SWAP_Y(text->Y) + yoff;
		TOGGLE_FLAG(PCB_FLAG_ONSOLDER, text);
	}
	END_LOOP;
	Element->MarkX = SWAP_X(Element->MarkX);
	Element->MarkY = SWAP_Y(Element->MarkY) + yoff;

	/* now toggle the solder-side flag */
	TOGGLE_FLAG(PCB_FLAG_ONSOLDER, Element);
	/* this inserts all of the rtree data too */
	SetElementBoundingBox(Data, Element, &PCB->Font);
	ClearFromPolygon(Data, PCB_TYPE_ELEMENT, Element, Element);
}

/* sets the bounding box of an elements */
void SetElementBoundingBox(pcb_data_t *Data, pcb_element_t *Element, pcb_font_t *Font)
{
	pcb_box_t *box, *vbox;

	if (Data && Data->element_tree)
		r_delete_entry(Data->element_tree, (pcb_box_t *) Element);
	/* first update the text objects */
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(Data->name_tree[n], (pcb_box_t *) text);
		SetTextBoundingBox(Font, text);
		if (Data && !Data->name_tree[n])
			Data->name_tree[n] = r_create_tree(NULL, 0, 0);
		if (Data)
			r_insert_entry(Data->name_tree[n], (pcb_box_t *) text, 0);
	}
	END_LOOP;

	/* do not include the elementnames bounding box which
	 * is handled separately
	 */
	box = &Element->BoundingBox;
	vbox = &Element->VBox;
	box->X1 = box->Y1 = MAX_COORD;
	box->X2 = box->Y2 = 0;
	ELEMENTLINE_LOOP(Element);
	{
		SetLineBoundingBox(line);
		MAKEMIN(box->X1, line->Point1.X - (line->Thickness + 1) / 2);
		MAKEMIN(box->Y1, line->Point1.Y - (line->Thickness + 1) / 2);
		MAKEMIN(box->X1, line->Point2.X - (line->Thickness + 1) / 2);
		MAKEMIN(box->Y1, line->Point2.Y - (line->Thickness + 1) / 2);
		MAKEMAX(box->X2, line->Point1.X + (line->Thickness + 1) / 2);
		MAKEMAX(box->Y2, line->Point1.Y + (line->Thickness + 1) / 2);
		MAKEMAX(box->X2, line->Point2.X + (line->Thickness + 1) / 2);
		MAKEMAX(box->Y2, line->Point2.Y + (line->Thickness + 1) / 2);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		SetArcBoundingBox(arc);
		MAKEMIN(box->X1, arc->BoundingBox.X1);
		MAKEMIN(box->Y1, arc->BoundingBox.Y1);
		MAKEMAX(box->X2, arc->BoundingBox.X2);
		MAKEMAX(box->Y2, arc->BoundingBox.Y2);
	}
	END_LOOP;
	*vbox = *box;
	PIN_LOOP(Element);
	{
		if (Data && Data->pin_tree)
			r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
		SetPinBoundingBox(pin);
		if (Data) {
			if (!Data->pin_tree)
				Data->pin_tree = r_create_tree(NULL, 0, 0);
			r_insert_entry(Data->pin_tree, (pcb_box_t *) pin, 0);
		}
		MAKEMIN(box->X1, pin->BoundingBox.X1);
		MAKEMIN(box->Y1, pin->BoundingBox.Y1);
		MAKEMAX(box->X2, pin->BoundingBox.X2);
		MAKEMAX(box->Y2, pin->BoundingBox.Y2);
		MAKEMIN(vbox->X1, pin->X - pin->Thickness / 2);
		MAKEMIN(vbox->Y1, pin->Y - pin->Thickness / 2);
		MAKEMAX(vbox->X2, pin->X + pin->Thickness / 2);
		MAKEMAX(vbox->Y2, pin->Y + pin->Thickness / 2);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		if (Data && Data->pad_tree)
			r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
		SetPadBoundingBox(pad);
		if (Data) {
			if (!Data->pad_tree)
				Data->pad_tree = r_create_tree(NULL, 0, 0);
			r_insert_entry(Data->pad_tree, (pcb_box_t *) pad, 0);
		}
		MAKEMIN(box->X1, pad->BoundingBox.X1);
		MAKEMIN(box->Y1, pad->BoundingBox.Y1);
		MAKEMAX(box->X2, pad->BoundingBox.X2);
		MAKEMAX(box->Y2, pad->BoundingBox.Y2);
		MAKEMIN(vbox->X1, MIN(pad->Point1.X, pad->Point2.X) - pad->Thickness / 2);
		MAKEMIN(vbox->Y1, MIN(pad->Point1.Y, pad->Point2.Y) - pad->Thickness / 2);
		MAKEMAX(vbox->X2, MAX(pad->Point1.X, pad->Point2.X) + pad->Thickness / 2);
		MAKEMAX(vbox->Y2, MAX(pad->Point1.Y, pad->Point2.Y) + pad->Thickness / 2);
	}
	END_LOOP;
	/* now we set the PCB_FLAG_EDGE2 of the pad if Point2
	 * is closer to the outside edge than Point1
	 */
	PAD_LOOP(Element);
	{
		if (pad->Point1.Y == pad->Point2.Y) {
			/* horizontal pad */
			if (box->X2 - pad->Point2.X < pad->Point1.X - box->X1)
				SET_FLAG(PCB_FLAG_EDGE2, pad);
			else
				CLEAR_FLAG(PCB_FLAG_EDGE2, pad);
		}
		else {
			/* vertical pad */
			if (box->Y2 - pad->Point2.Y < pad->Point1.Y - box->Y1)
				SET_FLAG(PCB_FLAG_EDGE2, pad);
			else
				CLEAR_FLAG(PCB_FLAG_EDGE2, pad);
		}
	}
	END_LOOP;

	/* mark pins with component orientation */
	if ((box->X2 - box->X1) > (box->Y2 - box->Y1)) {
		PIN_LOOP(Element);
		{
			SET_FLAG(PCB_FLAG_EDGE2, pin);
		}
		END_LOOP;
	}
	else {
		PIN_LOOP(Element);
		{
			CLEAR_FLAG(PCB_FLAG_EDGE2, pin);
		}
		END_LOOP;
	}
	close_box(box);
	close_box(vbox);
	if (Data && !Data->element_tree)
		Data->element_tree = r_create_tree(NULL, 0, 0);
	if (Data)
		r_insert_entry(Data->element_tree, box, 0);
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
	return (temp);
}


/* make a unique name for the name on board
 * this can alter the contents of the input string */
char *UniqueElementName(pcb_data_t *Data, char *Name)
{
	pcb_bool unique = pcb_true;
	/* null strings are ok */
	if (!Name || !*Name)
		return (Name);

	for (;;) {
		ELEMENT_LOOP(Data);
		{
			if (NAMEONPCB_NAME(element) && NSTRCMP(NAMEONPCB_NAME(element), Name) == 0) {
				Name = BumpName(Name);
				unique = pcb_false;
				break;
			}
		}
		END_LOOP;
		if (unique)
			return (Name);
		unique = pcb_true;
	}
}

void r_delete_element(pcb_data_t * data, pcb_element_t * element)
{
	r_delete_entry(data->element_tree, (pcb_box_t *) element);
	PIN_LOOP(element);
	{
		r_delete_entry(data->pin_tree, (pcb_box_t *) pin);
	}
	END_LOOP;
	PAD_LOOP(element);
	{
		r_delete_entry(data->pad_tree, (pcb_box_t *) pad);
	}
	END_LOOP;
	ELEMENTTEXT_LOOP(element);
	{
		r_delete_entry(data->name_tree[n], (pcb_box_t *) text);
	}
	END_LOOP;
}

/* Returns a best guess about the orientation of an element.  The
 * value corresponds to the rotation; a difference is the right value
 * to pass to RotateElementLowLevel.  However, the actual value is no
 * indication of absolute rotation; only relative rotation is
 * meaningful.
 */
int ElementOrientation(pcb_element_t * e)
{
	pcb_coord_t pin1x, pin1y, pin2x, pin2y, dx, dy;
	pcb_bool found_pin1 = 0;
	pcb_bool found_pin2 = 0;

	/* in case we don't find pin 1 or 2, make sure we have initialized these variables */
	pin1x = 0;
	pin1y = 0;
	pin2x = 0;
	pin2y = 0;

	PIN_LOOP(e);
	{
		if (NSTRCMP(pin->Number, "1") == 0) {
			pin1x = pin->X;
			pin1y = pin->Y;
			found_pin1 = 1;
		}
		else if (NSTRCMP(pin->Number, "2") == 0) {
			pin2x = pin->X;
			pin2y = pin->Y;
			found_pin2 = 1;
		}
	}
	END_LOOP;

	PAD_LOOP(e);
	{
		if (NSTRCMP(pad->Number, "1") == 0) {
			pin1x = (pad->Point1.X + pad->Point2.X) / 2;
			pin1y = (pad->Point1.Y + pad->Point2.Y) / 2;
			found_pin1 = 1;
		}
		else if (NSTRCMP(pad->Number, "2") == 0) {
			pin2x = (pad->Point1.X + pad->Point2.X) / 2;
			pin2y = (pad->Point1.Y + pad->Point2.Y) / 2;
			found_pin2 = 1;
		}
	}
	END_LOOP;

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
void MoveElementLowLevel(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t DX, pcb_coord_t DY)
{
	if (Data)
		r_delete_entry(Data->element_tree, (pcb_box_t *) Element);
	ELEMENTLINE_LOOP(Element);
	{
		MOVE_LINE_LOWLEVEL(line, DX, DY);
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		if (Data) {
			r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
			RestoreToPolygon(Data, PCB_TYPE_PIN, Element, pin);
		}
		MOVE_PIN_LOWLEVEL(pin, DX, DY);
		if (Data) {
			r_insert_entry(Data->pin_tree, (pcb_box_t *) pin, 0);
			ClearFromPolygon(Data, PCB_TYPE_PIN, Element, pin);
		}
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		if (Data) {
			r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
			RestoreToPolygon(Data, PCB_TYPE_PAD, Element, pad);
		}
		MOVE_PAD_LOWLEVEL(pad, DX, DY);
		if (Data) {
			r_insert_entry(Data->pad_tree, (pcb_box_t *) pad, 0);
			ClearFromPolygon(Data, PCB_TYPE_PAD, Element, pad);
		}
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		MOVE_ARC_LOWLEVEL(arc, DX, DY);
	}
	END_LOOP;
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		MOVE_TEXT_LOWLEVEL(text, DX, DY);
		if (Data && Data->name_tree[n])
			r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text, 0);
	}
	END_LOOP;
	MOVE_BOX_LOWLEVEL(&Element->BoundingBox, DX, DY);
	MOVE_BOX_LOWLEVEL(&Element->VBox, DX, DY);
	MOVE(Element->MarkX, Element->MarkY, DX, DY);
	if (Data)
		r_insert_entry(Data->element_tree, (pcb_box_t *) Element, 0);
}

void *RemoveElement(pcb_element_t *Element)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveElement_op(&ctx, Element);
}

/* rotate an element in 90 degree steps */
void RotateElementLowLevel(pcb_data_t *Data, pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	/* solder side objects need a different orientation */

	/* the text subroutine decides by itself if the direction
	 * is to be corrected
	 */
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(Data->name_tree[n], (pcb_box_t *) text);
		RotateTextLowLevel(text, X, Y, Number);
	}
	END_LOOP;
	ELEMENTLINE_LOOP(Element);
	{
		RotateLineLowLevel(line, X, Y, Number);
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		/* pre-delete the pins from the pin-tree before their coordinates change */
		if (Data)
			r_delete_entry(Data->pin_tree, (pcb_box_t *) pin);
		RestoreToPolygon(Data, PCB_TYPE_PIN, Element, pin);
		ROTATE_PIN_LOWLEVEL(pin, X, Y, Number);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		/* pre-delete the pads before their coordinates change */
		if (Data)
			r_delete_entry(Data->pad_tree, (pcb_box_t *) pad);
		RestoreToPolygon(Data, PCB_TYPE_PAD, Element, pad);
		ROTATE_PAD_LOWLEVEL(pad, X, Y, Number);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		RotateArcLowLevel(arc, X, Y, Number);
	}
	END_LOOP;
	ROTATE(Element->MarkX, Element->MarkY, X, Y, Number);
	/* SetElementBoundingBox reenters the rtree data */
	SetElementBoundingBox(Data, Element, &PCB->Font);
	ClearFromPolygon(Data, PCB_TYPE_ELEMENT, Element, Element);
}


/*** ops ***/
/* copies a element to buffer */
void *AddElementToBuffer(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_element_t *element;

	element = GetElementMemory(ctx->buffer.dst);
	CopyElementLowLevel(ctx->buffer.dst, element, Element, pcb_false, 0, 0);
	CLEAR_FLAG(ctx->buffer.extraflg, element);
	if (ctx->buffer.extraflg) {
		ELEMENTTEXT_LOOP(element);
		{
			CLEAR_FLAG(ctx->buffer.extraflg, text);
		}
		END_LOOP;
		PIN_LOOP(element);
		{
			CLEAR_FLAG(PCB_FLAG_FOUND | ctx->buffer.extraflg, pin);
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			CLEAR_FLAG(PCB_FLAG_FOUND | ctx->buffer.extraflg, pad);
		}
		END_LOOP;
	}
	return (element);
}

/* moves a element to buffer without allocating memory for pins/names */
void *MoveElementToBuffer(pcb_opctx_t *ctx, pcb_element_t * element)
{
	/*
	 * Delete the element from the source (remove it from trees,
	 * restore to polygons)
	 */
	r_delete_element(ctx->buffer.src, element);

	elementlist_remove(element);
	elementlist_append(&ctx->buffer.dst->Element, element);

	PIN_LOOP(element);
	{
		RestoreToPolygon(ctx->buffer.src, PCB_TYPE_PIN, element, pin);
		CLEAR_FLAG(PCB_FLAG_WARN | PCB_FLAG_FOUND, pin);
	}
	END_LOOP;
	PAD_LOOP(element);
	{
		RestoreToPolygon(ctx->buffer.src, PCB_TYPE_PAD, element, pad);
		CLEAR_FLAG(PCB_FLAG_WARN | PCB_FLAG_FOUND, pad);
	}
	END_LOOP;
	SetElementBoundingBox(ctx->buffer.dst, element, &PCB->Font);
	/*
	 * Now clear the from the polygons in the destination
	 */
	PIN_LOOP(element);
	{
		ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_PIN, element, pin);
	}
	END_LOOP;
	PAD_LOOP(element);
	{
		ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_PAD, element, pad);
	}
	END_LOOP;

	return element;
}


/* changes the drilling hole of all pins of an element; returns pcb_true if changed */
void *ChangeElement2ndSize(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool changed = pcb_false;
	pcb_coord_t value;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : pin->DrillingHole + ctx->chgsize.delta;
		if (value <= MAX_PINORVIASIZE &&
				value >= MIN_PINORVIAHOLE && (TEST_FLAG(PCB_FLAG_HOLE, pin) || value <= pin->Thickness - MIN_PINORVIACOPPER)
				&& value != pin->DrillingHole) {
			changed = pcb_true;
			AddObjectTo2ndSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
			ErasePin(pin);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->DrillingHole = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pin)) {
				AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			DrawPin(pin);
		}
	}
	END_LOOP;
	if (changed)
		return (Element);
	else
		return (NULL);
}

/* changes ring dia of all pins of an element; returns pcb_true if changed */
void *ChangeElement1stSize(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool changed = pcb_false;
	pcb_coord_t value;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : pin->DrillingHole + ctx->chgsize.delta;
		if (value <= MAX_PINORVIASIZE && value >= pin->DrillingHole + MIN_PINORVIACOPPER && value != pin->Thickness) {
			changed = pcb_true;
			AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
			ErasePin(pin);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->Thickness = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pin)) {
				AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			DrawPin(pin);
		}
	}
	END_LOOP;
	if (changed)
		return (Element);
	else
		return (NULL);
}

/* changes the clearance of all pins of an element; returns pcb_true if changed */
void *ChangeElementClearSize(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool changed = pcb_false;
	pcb_coord_t value;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : pin->Clearance + ctx->chgsize.delta;
		if (value <= MAX_PINORVIASIZE &&
				value >= MIN_PINORVIAHOLE && (TEST_FLAG(PCB_FLAG_HOLE, pin) || value <= pin->Thickness - MIN_PINORVIACOPPER)
				&& value != pin->Clearance) {
			changed = pcb_true;
			AddObjectToClearSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
			ErasePin(pin);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->Clearance = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pin)) {
				AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			DrawPin(pin);
		}
	}
	END_LOOP;

	PAD_LOOP(Element);
	{
		value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : pad->Clearance + ctx->chgsize.delta;
		if (value <= MAX_PINORVIASIZE && value >= MIN_PINORVIAHOLE && value != pad->Clearance) {
			changed = pcb_true;
			AddObjectToClearSizeUndoList(PCB_TYPE_PAD, Element, pad, pad);
			ErasePad(pad);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, pad);
			r_delete_entry(PCB->Data->pad_tree, &pad->BoundingBox);
			pad->Clearance = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pad)) {
				AddObjectToSizeUndoList(PCB_TYPE_PAD, Element, pad, pad);
				pad->Thickness = value;
			}
			/* SetElementBB updates all associated rtrees */
			SetElementBoundingBox(PCB->Data, Element, &PCB->Font);

			ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, pad);
			DrawPad(pad);
		}
	}
	END_LOOP;

	if (changed)
		return (Element);
	else
		return (NULL);
}

/* changes the scaling factor of an element's outline; returns pcb_true if changed */
void *ChangeElementSize(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_coord_t value;
	pcb_bool changed = pcb_false;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	if (PCB->ElementOn)
		EraseElement(Element);
	ELEMENTLINE_LOOP(Element);
	{
		value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : line->Thickness + ctx->chgsize.delta;
		if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != line->Thickness) {
			AddObjectToSizeUndoList(PCB_TYPE_ELEMENT_LINE, Element, line, line);
			line->Thickness = value;
			changed = pcb_true;
		}
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : arc->Thickness + ctx->chgsize.delta;
		if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != arc->Thickness) {
			AddObjectToSizeUndoList(PCB_TYPE_ELEMENT_ARC, Element, arc, arc);
			arc->Thickness = value;
			changed = pcb_true;
		}
	}
	END_LOOP;
	if (PCB->ElementOn) {
		DrawElement(Element);
	}
	if (changed)
		return (Element);
	return (NULL);
}

/* changes the scaling factor of a elementname object; returns pcb_true if changed */
void *ChangeElementNameSize(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	int value = ctx->chgsize.absolute ? PCB_COORD_TO_MIL(ctx->chgsize.absolute)
		: DESCRIPTION_TEXT(Element).Scale + PCB_COORD_TO_MIL(ctx->chgsize.delta);

	if (TEST_FLAG(PCB_FLAG_LOCK, &Element->Name[0]))
		return (NULL);
	if (value <= MAX_TEXTSCALE && value >= MIN_TEXTSCALE) {
		EraseElementName(Element);
		ELEMENTTEXT_LOOP(Element);
		{
			AddObjectToSizeUndoList(PCB_TYPE_ELEMENT_NAME, Element, text, text);
			r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
			text->Scale = value;
			SetTextBoundingBox(&PCB->Font, text);
			r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text, 0);
		}
		END_LOOP;
		DrawElementName(Element);
		return (Element);
	}
	return (NULL);
}


void *ChangeElementName(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, &Element->Name[0]))
		return (NULL);
	if (NAME_INDEX() == NAMEONPCB_INDEX) {
		if (conf_core.editor.unique_names && UniqueElementName(PCB->Data, ctx->chgname.new_name) != ctx->chgname.new_name) {
			Message(PCB_MSG_DEFAULT, _("Error: The name \"%s\" is not unique!\n"), ctx->chgname.new_name);
			return ((char *) -1);
		}
	}

	return ChangeElementText(PCB, PCB->Data, Element, NAME_INDEX(), ctx->chgname.new_name);
}

void *ChangeElementNonetlist(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	TOGGLE_FLAG(PCB_FLAG_NONETLIST, Element);
	return Element;
}


/* changes the square flag of all pins on an element */
void *ChangeElementSquare(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *ans = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ans = ChangePinSquare(ctx, Element, pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ans = ChangePadSquare(ctx, Element, pad);
	}
	END_LOOP;
	return (ans);
}

/* sets the square flag of all pins on an element */
void *SetElementSquare(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *ans = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ans = SetPinSquare(ctx, Element, pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ans = SetPadSquare(ctx, Element, pad);
	}
	END_LOOP;
	return (ans);
}

/* clears the square flag of all pins on an element */
void *ClrElementSquare(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *ans = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ans = ClrPinSquare(ctx, Element, pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ans = ClrPadSquare(ctx, Element, pad);
	}
	END_LOOP;
	return (ans);
}

/* changes the octagon flags of all pins of an element */
void *ChangeElementOctagon(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *result = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ChangePinOctagon(ctx, Element, pin);
		result = Element;
	}
	END_LOOP;
	return (result);
}

/* sets the octagon flags of all pins of an element */
void *SetElementOctagon(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *result = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		SetPinOctagon(ctx, Element, pin);
		result = Element;
	}
	END_LOOP;
	return (result);
}

/* clears the octagon flags of all pins of an element */
void *ClrElementOctagon(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	void *result = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ClrPinOctagon(ctx, Element, pin);
		result = Element;
	}
	END_LOOP;
	return (result);
}

/* copies an element onto the PCB.  Then does a draw. */
void *CopyElement(pcb_opctx_t *ctx, pcb_element_t *Element)
{

#ifdef DEBUG
	printf("Entered CopyElement, trying to copy element %s\n", Element->Name[1].TextString);
#endif

	pcb_element_t *element = CopyElementLowLevel(PCB->Data,
																							 NULL, Element,
																							 conf_core.editor.unique_names, ctx->copy.DeltaX,
																							 ctx->copy.DeltaY);

	/* this call clears the polygons */
	AddObjectToCreateUndoList(PCB_TYPE_ELEMENT, element, element, element);
	if (PCB->ElementOn && (FRONT(element) || PCB->InvisibleObjectsOn)) {
		DrawElementName(element);
		DrawElementPackage(element);
	}
	if (PCB->PinOn) {
		DrawElementPinsAndPads(element);
	}
#ifdef DEBUG
	printf(" ... Leaving CopyElement.\n");
#endif
	return (element);
}

/* moves all names of an element to a new position */
void *MoveElementName(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (PCB->ElementOn && (FRONT(Element) || PCB->InvisibleObjectsOn)) {
		EraseElementName(Element);
		ELEMENTTEXT_LOOP(Element);
		{
			if (PCB->Data->name_tree[n])
				r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
			MOVE_TEXT_LOWLEVEL(text, ctx->move.dx, ctx->move.dy);
			if (PCB->Data->name_tree[n])
				r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text, 0);
		}
		END_LOOP;
		DrawElementName(Element);
		Draw();
	}
	else {
		ELEMENTTEXT_LOOP(Element);
		{
			if (PCB->Data->name_tree[n])
				r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
			MOVE_TEXT_LOWLEVEL(text, ctx->move.dx, ctx->move.dy);
			if (PCB->Data->name_tree[n])
				r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text, 0);
		}
		END_LOOP;
	}
	return (Element);
}

/* moves an element */
void *MoveElement(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_bool didDraw = pcb_false;

	if (PCB->ElementOn && (FRONT(Element) || PCB->InvisibleObjectsOn)) {
		EraseElement(Element);
		MoveElementLowLevel(PCB->Data, Element, ctx->move.dx, ctx->move.dy);
		DrawElementName(Element);
		DrawElementPackage(Element);
		didDraw = pcb_true;
	}
	else {
		if (PCB->PinOn)
			EraseElementPinsAndPads(Element);
		MoveElementLowLevel(PCB->Data, Element, ctx->move.dx, ctx->move.dy);
	}
	if (PCB->PinOn) {
		DrawElementPinsAndPads(Element);
		didDraw = pcb_true;
	}
	if (didDraw)
		Draw();
	return (Element);
}

/* destroys a element */
void *DestroyElement(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	if (ctx->remove.destroy_target->element_tree)
		r_delete_entry(ctx->remove.destroy_target->element_tree, (pcb_box_t *) Element);
	if (ctx->remove.destroy_target->pin_tree) {
		PIN_LOOP(Element);
		{
			r_delete_entry(ctx->remove.destroy_target->pin_tree, (pcb_box_t *) pin);
		}
		END_LOOP;
	}
	if (ctx->remove.destroy_target->pad_tree) {
		PAD_LOOP(Element);
		{
			r_delete_entry(ctx->remove.destroy_target->pad_tree, (pcb_box_t *) pad);
		}
		END_LOOP;
	}
	ELEMENTTEXT_LOOP(Element);
	{
		if (ctx->remove.destroy_target->name_tree[n])
			r_delete_entry(ctx->remove.destroy_target->name_tree[n], (pcb_box_t *) text);
	}
	END_LOOP;
	FreeElementMemory(Element);

	RemoveFreeElement(Element);

	return NULL;
}

/* removes an element */
void *RemoveElement_op(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	/* erase from screen */
	if ((PCB->ElementOn || PCB->PinOn) && (FRONT(Element) || PCB->InvisibleObjectsOn)) {
		EraseElement(Element);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_ELEMENT, Element, Element, Element);
	return NULL;
}

/* rotates an element */
void *RotateElement(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	EraseElement(Element);
	RotateElementLowLevel(PCB->Data, Element, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	DrawElement(Element);
	Draw();
	return (Element);
}

/* ----------------------------------------------------------------------
 * rotates the name of an element
 */
void *RotateElementName(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	EraseElementName(Element);
	ELEMENTTEXT_LOOP(Element);
	{
		r_delete_entry(PCB->Data->name_tree[n], (pcb_box_t *) text);
		RotateTextLowLevel(text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
		r_insert_entry(PCB->Data->name_tree[n], (pcb_box_t *) text, 0);
	}
	END_LOOP;
	DrawElementName(Element);
	Draw();
	return (Element);
}

/*** draw ***/
void draw_element_name(pcb_element_t * element)
{
	if ((conf_core.editor.hide_names && gui->gui) || TEST_FLAG(PCB_FLAG_HIDENAME, element))
		return;
	if (pcb_draw_doing_pinout || pcb_draw_doing_assy)
		gui->set_color(Output.fgGC, PCB->ElementColor);
	else if (TEST_FLAG(PCB_FLAG_SELECTED, &ELEMENT_TEXT(PCB, element)))
		gui->set_color(Output.fgGC, PCB->ElementSelectedColor);
	else if (FRONT(element)) {
#warning TODO: why do we test for Names flag here instead of elements flag?
		if (TEST_FLAG(PCB_FLAG_NONETLIST, element))
			gui->set_color(Output.fgGC, PCB->ElementColor_nonetlist);
		else
			gui->set_color(Output.fgGC, PCB->ElementColor);
	}
	else
		gui->set_color(Output.fgGC, PCB->InvisibleObjectsColor);

	DrawTextLowLevel(&ELEMENT_TEXT(PCB, element), PCB->minSlk);

}

pcb_r_dir_t draw_element_name_callback(const pcb_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	pcb_element_t *element = (pcb_element_t *) text->Element;
	int *side = cl;

	if (TEST_FLAG(PCB_FLAG_HIDENAME, element))
		return R_DIR_NOT_FOUND;

	if (ON_SIDE(element, *side))
		draw_element_name(element);
	return R_DIR_NOT_FOUND;
}

void draw_element_pins_and_pads(pcb_element_t * element)
{
	PAD_LOOP(element);
	{
		if (pcb_draw_doing_pinout || pcb_draw_doing_assy || FRONT(pad) || PCB->InvisibleObjectsOn)
			draw_pad(pad);
	}
	END_LOOP;
	PIN_LOOP(element);
	{
		draw_pin(pin, pcb_true);
	}
	END_LOOP;
}


void draw_element_package(pcb_element_t * element)
{
	/* set color and draw lines, arcs, text and pins */
	if (pcb_draw_doing_pinout || pcb_draw_doing_assy)
		gui->set_color(Output.fgGC, PCB->ElementColor);
	else if (TEST_FLAG(PCB_FLAG_SELECTED, element))
		gui->set_color(Output.fgGC, PCB->ElementSelectedColor);
	else if (FRONT(element))
		gui->set_color(Output.fgGC, PCB->ElementColor);
	else
		gui->set_color(Output.fgGC, PCB->InvisibleObjectsColor);

	/* draw lines, arcs, text and pins */
	ELEMENTLINE_LOOP(element);
	{
		_draw_line(line);
	}
	END_LOOP;
	ARC_LOOP(element);
	{
		_draw_arc(arc);
	}
	END_LOOP;
}

void draw_element(pcb_element_t *element)
{
	draw_element_package(element);
	draw_element_name(element);
	draw_element_pins_and_pads(element);
}

pcb_r_dir_t draw_element_callback(const pcb_box_t * b, void *cl)
{
	pcb_element_t *element = (pcb_element_t *) b;
	int *side = cl;

	if (ON_SIDE(element, *side))
		draw_element_package(element);
	return R_DIR_FOUND_CONTINUE;
}

static void DrawEMark(pcb_element_t *e, pcb_coord_t X, pcb_coord_t Y, pcb_bool invisible)
{
	pcb_coord_t mark_size = EMARK_SIZE;
	if (!PCB->InvisibleObjectsOn && invisible)
		return;

	if (pinlist_length(&e->Pin) != 0) {
		pcb_pin_t *pin0 = pinlist_first(&e->Pin);
		if (TEST_FLAG(PCB_FLAG_HOLE, pin0))
			mark_size = MIN(mark_size, pin0->DrillingHole / 2);
		else
			mark_size = MIN(mark_size, pin0->Thickness / 2);
	}

	if (padlist_length(&e->Pad) != 0) {
		pcb_pad_t *pad0 = padlist_first(&e->Pad);
		mark_size = MIN(mark_size, pad0->Thickness / 2);
	}

	gui->set_color(Output.fgGC, invisible ? PCB->InvisibleMarkColor : PCB->ElementColor);
	gui->set_line_cap(Output.fgGC, Trace_Cap);
	gui->set_line_width(Output.fgGC, 0);
	gui->draw_line(Output.fgGC, X - mark_size, Y, X, Y - mark_size);
	gui->draw_line(Output.fgGC, X + mark_size, Y, X, Y - mark_size);
	gui->draw_line(Output.fgGC, X - mark_size, Y, X, Y + mark_size);
	gui->draw_line(Output.fgGC, X + mark_size, Y, X, Y + mark_size);

	/*
	 * If an element is locked, place a "L" on top of the "diamond".
	 * This provides a nice visual indication that it is locked that
	 * works even for color blind users.
	 */
	if (TEST_FLAG(PCB_FLAG_LOCK, e)) {
		gui->draw_line(Output.fgGC, X, Y, X + 2 * mark_size, Y);
		gui->draw_line(Output.fgGC, X, Y, X, Y - 4 * mark_size);
	}
}

pcb_r_dir_t draw_element_mark_callback(const pcb_box_t * b, void *cl)
{
	pcb_element_t *element = (pcb_element_t *) b;

	DrawEMark(element, element->MarkX, element->MarkY, !FRONT(element));
	return R_DIR_FOUND_CONTINUE;
}

void EraseElement(pcb_element_t *Element)
{
	ELEMENTLINE_LOOP(Element);
	{
		EraseLine(line);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		EraseArc(arc);
	}
	END_LOOP;
	EraseElementName(Element);
	EraseElementPinsAndPads(Element);
	EraseFlags(&Element->Flags);
}

void EraseElementPinsAndPads(pcb_element_t *Element)
{
	PIN_LOOP(Element);
	{
		ErasePin(pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ErasePad(pad);
	}
	END_LOOP;
}

void EraseElementName(pcb_element_t *Element)
{
	if (TEST_FLAG(PCB_FLAG_HIDENAME, Element)) {
		return;
	}
	DrawText(NULL, &ELEMENT_TEXT(PCB, Element));
}

void DrawElement(pcb_element_t *Element)
{
	DrawElementPackage(Element);
	DrawElementName(Element);
	DrawElementPinsAndPads(Element);
}

void DrawElementName(pcb_element_t *Element)
{
	if (TEST_FLAG(PCB_FLAG_HIDENAME, Element))
		return;
	DrawText(NULL, &ELEMENT_TEXT(PCB, Element));
}

void DrawElementPackage(pcb_element_t *Element)
{
	ELEMENTLINE_LOOP(Element);
	{
		DrawLine(NULL, line);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		DrawArc(NULL, arc);
	}
	END_LOOP;
}

void DrawElementPinsAndPads(pcb_element_t *Element)
{
	PAD_LOOP(Element);
	{
		if (pcb_draw_doing_pinout || pcb_draw_doing_assy || FRONT(pad) || PCB->InvisibleObjectsOn)
			DrawPad(pad);
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		DrawPin(pin);
	}
	END_LOOP;
}

