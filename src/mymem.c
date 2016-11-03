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


/* memory management functions
 */

#include "config.h"

#include <memory.h>

#include "data.h"
#include "mymem.h"
#include "rtree.h"
#include "rats_patch.h"

/* ---------------------------------------------------------------------------
 * local prototypes
 */
/* memset object to 0, but keep the link field */
#define reset_obj_mem(type, obj) \
do { \
	gdl_elem_t __lnk__ = obj->link; \
	memset(obj, 0, sizeof(type)); \
	obj->link = __lnk__; \
} while(0) \

/* ---------------------------------------------------------------------------
 * get next slot for a rubberband connection, allocates memory if necessary
 */
RubberbandTypePtr GetRubberbandMemory(void)
{
	RubberbandTypePtr ptr = Crosshair.AttachedObject.Rubberband;

	/* realloc new memory if necessary and clear it */
	if (Crosshair.AttachedObject.RubberbandN >= Crosshair.AttachedObject.RubberbandMax) {
		Crosshair.AttachedObject.RubberbandMax += STEP_RUBBERBAND;
		ptr = (RubberbandTypePtr) realloc(ptr, Crosshair.AttachedObject.RubberbandMax * sizeof(RubberbandType));
		Crosshair.AttachedObject.Rubberband = ptr;
		memset(ptr + Crosshair.AttachedObject.RubberbandN, 0, STEP_RUBBERBAND * sizeof(RubberbandType));
	}
	return (ptr + Crosshair.AttachedObject.RubberbandN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a pin, allocates memory if necessary
 */
PinType *GetPinMemory(ElementType * element)
{
	PinType *new_obj;

	new_obj = calloc(sizeof(PinType), 1);
	pinlist_append(&element->Pin, new_obj);

	return new_obj;
}

void RemoveFreePin(PinType * data)
{
	pinlist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a pad, allocates memory if necessary
 */
PadType *GetPadMemory(ElementType * element)
{
	PadType *new_obj;

	new_obj = calloc(sizeof(PadType), 1);
	padlist_append(&element->Pad, new_obj);

	return new_obj;
}

void RemoveFreePad(PadType * data)
{
	padlist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a via, allocates memory if necessary
 */
PinType *GetViaMemory(DataType * data)
{
	PinType *new_obj;

	new_obj = calloc(sizeof(PinType), 1);
	pinlist_append(&data->Via, new_obj);

	return new_obj;
}

void RemoveFreeVia(PinType * data)
{
	pinlist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a Rat, allocates memory if necessary
 */
RatType *GetRatMemory(DataType * data)
{
	RatType *new_obj;

	new_obj = calloc(sizeof(RatType), 1);
	ratlist_append(&data->Rat, new_obj);

	return new_obj;
}

void RemoveFreeRat(RatType * data)
{
	ratlist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a line, allocates memory if necessary
 */
LineType *GetLineMemory(LayerType * layer)
{
	LineType *new_obj;

	new_obj = calloc(sizeof(LineType), 1);
	linelist_append(&layer->Line, new_obj);

	return new_obj;
}

void RemoveFreeLine(LineType * data)
{
	linelist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for an arc, allocates memory if necessary
 */
ArcTypePtr GetArcMemory(LayerType * layer)
{
	ArcType *new_obj;

	new_obj = calloc(sizeof(ArcType), 1);
	arclist_append(&layer->Arc, new_obj);

	return new_obj;
}

void RemoveFreeArc(ArcType * data)
{
	arclist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a text object, allocates memory if necessary
 */
TextTypePtr GetTextMemory(LayerType * layer)
{
	TextType *new_obj;

	new_obj = calloc(sizeof(TextType), 1);
	textlist_append(&layer->Text, new_obj);

	return new_obj;
}

void RemoveFreeText(TextType * data)
{
	textlist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * get next slot for a polygon object, allocates memory if necessary
 */
PolygonType *GetPolygonMemory(LayerType * layer)
{
	PolygonType *new_obj;

	new_obj = calloc(sizeof(PolygonType), 1);
	polylist_append(&layer->Polygon, new_obj);

	return new_obj;
}

void RemoveFreePolygon(PolygonType * data)
{
	polylist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * gets the next slot for a point in a polygon struct, allocates memory
 * if necessary
 */
PointTypePtr GetPointMemoryInPolygon(PolygonTypePtr Polygon)
{
	PointTypePtr points = Polygon->Points;

	/* realloc new memory if necessary and clear it */
	if (Polygon->PointN >= Polygon->PointMax) {
		Polygon->PointMax += STEP_POLYGONPOINT;
		points = (PointTypePtr) realloc(points, Polygon->PointMax * sizeof(PointType));
		Polygon->Points = points;
		memset(points + Polygon->PointN, 0, STEP_POLYGONPOINT * sizeof(PointType));
	}
	return (points + Polygon->PointN++);
}

/* ---------------------------------------------------------------------------
 * gets the next slot for a point in a polygon struct, allocates memory
 * if necessary
 */
pcb_cardinal_t *GetHoleIndexMemoryInPolygon(PolygonTypePtr Polygon)
{
	pcb_cardinal_t *holeindex = Polygon->HoleIndex;

	/* realloc new memory if necessary and clear it */
	if (Polygon->HoleIndexN >= Polygon->HoleIndexMax) {
		Polygon->HoleIndexMax += STEP_POLYGONHOLEINDEX;
		holeindex = (pcb_cardinal_t *) realloc(holeindex, Polygon->HoleIndexMax * sizeof(int));
		Polygon->HoleIndex = holeindex;
		memset(holeindex + Polygon->HoleIndexN, 0, STEP_POLYGONHOLEINDEX * sizeof(int));
	}
	return (holeindex + Polygon->HoleIndexN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for an element, allocates memory if necessary
 */
ElementType *GetElementMemory(DataType * data)
{
	ElementType *new_obj;

	new_obj = calloc(sizeof(ElementType), 1);
	elementlist_append(&data->Element, new_obj);

	return new_obj;
}

void RemoveFreeElement(ElementType * data)
{
	elementlist_remove(data);
	free(data);
}

/* ---------------------------------------------------------------------------
 * frees memory used by a polygon
 */
void FreePolygonMemory(PolygonType * polygon)
{
	if (polygon == NULL)
		return;

	free(polygon->Points);
	free(polygon->HoleIndex);

	if (polygon->Clipped)
		poly_Free(&polygon->Clipped);
	poly_FreeContours(&polygon->NoHoles);

	reset_obj_mem(PolygonType, polygon);
}

/* ---------------------------------------------------------------------------
 * frees memory used by an element
 */
void FreeElementMemory(ElementType * element)
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

	list_map0(&element->Pin, PinType, RemoveFreePin);
	list_map0(&element->Pad, PadType, RemoveFreePad);
	list_map0(&element->Line, LineType, RemoveFreeLine);
	list_map0(&element->Arc,  ArcType,  RemoveFreeArc);

	FreeAttributeListMemory(&element->Attributes);
	reset_obj_mem(ElementType, element);
}

/* ---------------------------------------------------------------------------
 * strips leading and trailing blanks from the passed string and
 * returns a pointer to the new 'duped' one or NULL if the old one
 * holds only white space characters
 */
char *StripWhiteSpaceAndDup(const char *S)
{
	const char *p1, *p2;
	char *copy;
	size_t length;

	if (!S || !*S)
		return (NULL);

	/* strip leading blanks */
	for (p1 = S; *p1 && isspace((int) *p1); p1++);

	/* strip trailing blanks and get string length */
	length = strlen(p1);
	for (p2 = p1 + length - 1; length && isspace((int) *p2); p2--, length--);

	/* string is not empty -> allocate memory */
	if (length) {
		copy = (char *) realloc(NULL, length + 1);
		strncpy(copy, p1, length);
		copy[length] = '\0';
		return (copy);
	}
	else
		return (NULL);
}

LineType *GetElementLineMemory(ElementType *Element)
{
	LineType *line = calloc(sizeof(LineType), 1);
	linelist_append(&Element->Line, line);

	return line;
}

ArcType *GetElementArcMemory(ElementType *Element)
{
	ArcType *arc = calloc(sizeof(ArcType), 1);

	arclist_append(&Element->Arc, arc);
	return arc;
}
