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
#include "list_common.h"
#include "obj_all.h"

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

LineType *GetElementLineMemory(ElementType *Element)
{
	LineType *line = calloc(sizeof(LineType), 1);
	linelist_append(&Element->Line, line);

	return line;
}

