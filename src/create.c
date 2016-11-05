/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

/* functions used to create vias, pins ... */

#include "config.h"

#include <setjmp.h>

#include "conf_core.h"

#include "board.h"
#include "math_helper.h"
#include "create.h"
#include "data.h"
#include "misc.h"
#include "rtree.h"
#include "search.h"
#include "undo.h"
#include "plug_io.h"
#include "stub_vendor.h"
#include "hid_actions.h"
#include "paths.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_all.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

/* current object ID; incremented after each creation of an object */
long int ID = 1;

pcb_bool pcb_create_be_lenient = pcb_false;

/* ----------------------------------------------------------------------
 * some local prototypes
 */
static void AddTextToElement(TextTypePtr, FontTypePtr, Coord, Coord, unsigned, char *, int, FlagType);

/* ---------------------------------------------------------------------------
 *  Set the lenience mode.
 */

void CreateBeLenient(pcb_bool v)
{
	pcb_create_be_lenient = v;
}

/* ---------------------------------------------------------------------------
 * creates an new element
 * memory is allocated if needed
 */
ElementTypePtr
CreateNewElement(DataTypePtr Data, ElementTypePtr Element,
								 FontTypePtr PCBFont,
								 FlagType Flags,
								 char *Description, char *NameOnPCB, char *Value,
								 Coord TextX, Coord TextY, pcb_uint8_t Direction, int TextScale, FlagType TextFlags, pcb_bool uniqueName)
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
	Element->ID = ID++;

#ifdef DEBUG
	printf("  .... Leaving CreateNewElement.\n");
#endif

	return (Element);
}

/* ---------------------------------------------------------------------------
 * creates a new arc in an element
 */
ArcTypePtr
CreateNewArcInElement(ElementTypePtr Element,
											Coord X, Coord Y, Coord Width, Coord Height, Angle angle, Angle delta, Coord Thickness)
{
	ArcType *arc = GetElementArcMemory(Element);

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
	arc->ID = ID++;
	return arc;
}

/* ---------------------------------------------------------------------------
 * creates a new line for an element
 */
LineTypePtr CreateNewLineInElement(ElementTypePtr Element, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness)
{
	LineType *line;

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
	line->ID = ID++;
	return line;
}

/* ---------------------------------------------------------------------------
 * creates a new textobject as part of an element
 * copies the values to the appropriate text object
 */
static void
AddTextToElement(TextTypePtr Text, FontTypePtr PCBFont,
								 Coord X, Coord Y, unsigned Direction, char *TextString, int Scale, FlagType Flags)
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
	Text->ID = ID++;
}

/* ---------------------------------------------------------------------------
 * creates a new line in a symbol
 */
LineTypePtr CreateNewLineInSymbol(SymbolTypePtr Symbol, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness)
{
	LineTypePtr line = Symbol->Line;

	/* realloc new memory if necessary and clear it */
	if (Symbol->LineN >= Symbol->LineMax) {
		Symbol->LineMax += STEP_SYMBOLLINE;
		line = (LineTypePtr) realloc(line, Symbol->LineMax * sizeof(LineType));
		Symbol->Line = line;
		memset(line + Symbol->LineN, 0, STEP_SYMBOLLINE * sizeof(LineType));
	}

	/* copy values */
	line = line + Symbol->LineN++;
	line->Point1.X = X1;
	line->Point1.Y = Y1;
	line->Point2.X = X2;
	line->Point2.Y = Y2;
	line->Thickness = Thickness;
	return (line);
}

/* ---------------------------------------------------------------------------
 * parses a file with font information and installs it into the provided PCB
 * checks directories given as colon separated list by resource fontPath
 * if the fonts filename doesn't contain a directory component
 */
void CreateDefaultFont(PCBTypePtr pcb)
{
	int res = -1;
	pcb_io_err_inhibit_inc();
	conf_list_foreach_path_first(res, &conf_core.rc.default_font_file, ParseFont(&pcb->Font, __path__));
	pcb_io_err_inhibit_dec();

	if (res != 0) {
		const char *s;
		gds_t buff;
		s = conf_concat_strlist(&conf_core.rc.default_font_file, &buff, NULL, ':');
		Message(PCB_MSG_ERROR, _("Can't find font-symbol-file - there won't be font in this design. Searched: '%s'\n"), s);
		gds_uninit(&buff);
	}
}

void CreateIDBump(int min_id)
{
	if (ID < min_id)
		ID = min_id;
}

void CreateIDReset(void)
{
	ID = 1;
}

long int CreateIDGet(void)
{
	return ID++;
}
