/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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

/* misc functions used by several modules */

#include "config.h"
#include "conf_core.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>

#include "board.h"
#include "box.h"
#include "crosshair.h"
#include "data.h"
#include "plug_io.h"
#include "error.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "rtree.h"
#include "rotate.h"
#include "rubberband.h"
#include "set.h"
#include "undo.h"
#include "compat_misc.h"
#include "obj_all.h"

/* forward declarations */
static char *BumpName(char *);
static void GetGridLockCoordinates(int, void *, void *, void *, Coord *, Coord *);

/* Local variables */

/* ---------------------------------------------------------------------------
 * sets the bounding box of a pad
 */
void SetPadBoundingBox(PadTypePtr Pad)
{
	Coord width;
	Coord deltax;
	Coord deltay;

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = (Pad->Thickness + Pad->Clearance + 1) / 2;
	width = MAX(width, (Pad->Mask + 1) / 2);
	deltax = Pad->Point2.X - Pad->Point1.X;
	deltay = Pad->Point2.Y - Pad->Point1.Y;

	if (TEST_FLAG(PCB_FLAG_SQUARE, Pad) && deltax != 0 && deltay != 0) {
		/* slanted square pad */
		double theta;
		Coord btx, bty;

		/* T is a vector half a thickness long, in the direction of
		   one of the corners.  */
		theta = atan2(deltay, deltax);
		btx = width * cos(theta + M_PI / 4) * sqrt(2.0);
		bty = width * sin(theta + M_PI / 4) * sqrt(2.0);


		Pad->BoundingBox.X1 = MIN(MIN(Pad->Point1.X - btx, Pad->Point1.X - bty), MIN(Pad->Point2.X + btx, Pad->Point2.X + bty));
		Pad->BoundingBox.X2 = MAX(MAX(Pad->Point1.X - btx, Pad->Point1.X - bty), MAX(Pad->Point2.X + btx, Pad->Point2.X + bty));
		Pad->BoundingBox.Y1 = MIN(MIN(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MIN(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
		Pad->BoundingBox.Y2 = MAX(MAX(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MAX(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
	}
	else {
		/* Adjust for our discrete polygon approximation */
		width = (double) width *POLY_CIRC_RADIUS_ADJ + 0.5;

		Pad->BoundingBox.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - width;
		Pad->BoundingBox.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + width;
		Pad->BoundingBox.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - width;
		Pad->BoundingBox.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + width;
	}
	close_box(&Pad->BoundingBox);
}

/* ---------------------------------------------------------------------------
 * sets the bounding box of a polygons
 */
void SetPolygonBoundingBox(PolygonTypePtr Polygon)
{
	Polygon->BoundingBox.X1 = Polygon->BoundingBox.Y1 = MAX_COORD;
	Polygon->BoundingBox.X2 = Polygon->BoundingBox.Y2 = 0;
	POLYGONPOINT_LOOP(Polygon);
	{
		MAKEMIN(Polygon->BoundingBox.X1, point->X);
		MAKEMIN(Polygon->BoundingBox.Y1, point->Y);
		MAKEMAX(Polygon->BoundingBox.X2, point->X);
		MAKEMAX(Polygon->BoundingBox.Y2, point->Y);
	}
	/* boxes don't include the lower right corner */
	close_box(&Polygon->BoundingBox);
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * sets the bounding box of an elements
 */
void SetElementBoundingBox(DataTypePtr Data, ElementTypePtr Element, FontTypePtr Font)
{
	BoxTypePtr box, vbox;

	if (Data && Data->element_tree)
		r_delete_entry(Data->element_tree, (BoxType *) Element);
	/* first update the text objects */
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(Data->name_tree[n], (BoxType *) text);
		SetTextBoundingBox(Font, text);
		if (Data && !Data->name_tree[n])
			Data->name_tree[n] = r_create_tree(NULL, 0, 0);
		if (Data)
			r_insert_entry(Data->name_tree[n], (BoxType *) text, 0);
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
			r_delete_entry(Data->pin_tree, (BoxType *) pin);
		SetPinBoundingBox(pin);
		if (Data) {
			if (!Data->pin_tree)
				Data->pin_tree = r_create_tree(NULL, 0, 0);
			r_insert_entry(Data->pin_tree, (BoxType *) pin, 0);
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
			r_delete_entry(Data->pad_tree, (BoxType *) pad);
		SetPadBoundingBox(pad);
		if (Data) {
			if (!Data->pad_tree)
				Data->pad_tree = r_create_tree(NULL, 0, 0);
			r_insert_entry(Data->pad_tree, (BoxType *) pad, 0);
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

typedef struct {
	int nplated;
	int nunplated;
} HoleCountStruct;

static r_dir_t hole_counting_callback(const BoxType * b, void *cl)
{
	PinTypePtr pin = (PinTypePtr) b;
	HoleCountStruct *hcs = (HoleCountStruct *) cl;
	if (TEST_FLAG(PCB_FLAG_HOLE, pin))
		hcs->nunplated++;
	else
		hcs->nplated++;
	return R_DIR_FOUND_CONTINUE;
}

/* ---------------------------------------------------------------------------
 * counts the number of plated and unplated holes in the design within
 * a given area of the board. To count for the whole board, pass NULL
 * within_area.
 */
void CountHoles(int *plated, int *unplated, const BoxType * within_area)
{
	HoleCountStruct hcs = { 0, 0 };

	r_search(PCB->Data->pin_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);
	r_search(PCB->Data->via_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}


/* ---------------------------------------------------------------------------
 * gets minimum and maximum coordinates
 * returns NULL if layout is empty
 */
BoxTypePtr GetDataBoundingBox(DataTypePtr Data)
{
	static BoxType box;
	/* FIX ME: use r_search to do this much faster */

	/* preset identifiers with highest and lowest possible values */
	box.X1 = box.Y1 = MAX_COORD;
	box.X2 = box.Y2 = -MAX_COORD;

	/* now scan for the lowest/highest X and Y coordinate */
	VIA_LOOP(Data);
	{
		box.X1 = MIN(box.X1, via->X - via->Thickness / 2);
		box.Y1 = MIN(box.Y1, via->Y - via->Thickness / 2);
		box.X2 = MAX(box.X2, via->X + via->Thickness / 2);
		box.Y2 = MAX(box.Y2, via->Y + via->Thickness / 2);
	}
	END_LOOP;
	ELEMENT_LOOP(Data);
	{
		box.X1 = MIN(box.X1, element->BoundingBox.X1);
		box.Y1 = MIN(box.Y1, element->BoundingBox.Y1);
		box.X2 = MAX(box.X2, element->BoundingBox.X2);
		box.Y2 = MAX(box.Y2, element->BoundingBox.Y2);
		{
			TextTypePtr text = &NAMEONPCB_TEXT(element);
			box.X1 = MIN(box.X1, text->BoundingBox.X1);
			box.Y1 = MIN(box.Y1, text->BoundingBox.Y1);
			box.X2 = MAX(box.X2, text->BoundingBox.X2);
			box.Y2 = MAX(box.Y2, text->BoundingBox.Y2);
		};
	}
	END_LOOP;
	ALLLINE_LOOP(Data);
	{
		box.X1 = MIN(box.X1, line->Point1.X - line->Thickness / 2);
		box.Y1 = MIN(box.Y1, line->Point1.Y - line->Thickness / 2);
		box.X1 = MIN(box.X1, line->Point2.X - line->Thickness / 2);
		box.Y1 = MIN(box.Y1, line->Point2.Y - line->Thickness / 2);
		box.X2 = MAX(box.X2, line->Point1.X + line->Thickness / 2);
		box.Y2 = MAX(box.Y2, line->Point1.Y + line->Thickness / 2);
		box.X2 = MAX(box.X2, line->Point2.X + line->Thickness / 2);
		box.Y2 = MAX(box.Y2, line->Point2.Y + line->Thickness / 2);
	}
	ENDALL_LOOP;
	ALLARC_LOOP(Data);
	{
		box.X1 = MIN(box.X1, arc->BoundingBox.X1);
		box.Y1 = MIN(box.Y1, arc->BoundingBox.Y1);
		box.X2 = MAX(box.X2, arc->BoundingBox.X2);
		box.Y2 = MAX(box.Y2, arc->BoundingBox.Y2);
	}
	ENDALL_LOOP;
	ALLTEXT_LOOP(Data);
	{
		box.X1 = MIN(box.X1, text->BoundingBox.X1);
		box.Y1 = MIN(box.Y1, text->BoundingBox.Y1);
		box.X2 = MAX(box.X2, text->BoundingBox.X2);
		box.Y2 = MAX(box.Y2, text->BoundingBox.Y2);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Data);
	{
		box.X1 = MIN(box.X1, polygon->BoundingBox.X1);
		box.Y1 = MIN(box.Y1, polygon->BoundingBox.Y1);
		box.X2 = MAX(box.X2, polygon->BoundingBox.X2);
		box.Y2 = MAX(box.Y2, polygon->BoundingBox.Y2);
	}
	ENDALL_LOOP;
	return (IsDataEmpty(Data) ? NULL : &box);
}

/* ---------------------------------------------------------------------------
 * transforms symbol coordinates so that the left edge of each symbol
 * is at the zero position. The y coordinates are moved so that min(y) = 0
 *
 */
void SetFontInfo(FontTypePtr Ptr)
{
	pcb_cardinal_t i, j;
	SymbolTypePtr symbol;
	LineTypePtr line;
	Coord totalminy = MAX_COORD;

	/* calculate cell with and height (is at least DEFAULT_CELLSIZE)
	 * maximum cell width and height
	 * minimum x and y position of all lines
	 */
	Ptr->MaxWidth = DEFAULT_CELLSIZE;
	Ptr->MaxHeight = DEFAULT_CELLSIZE;
	for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++) {
		Coord minx, miny, maxx, maxy;

		/* next one if the index isn't used or symbol is empty (SPACE) */
		if (!symbol->Valid || !symbol->LineN)
			continue;

		minx = miny = MAX_COORD;
		maxx = maxy = 0;
		for (line = symbol->Line, j = symbol->LineN; j; j--, line++) {
			minx = MIN(minx, line->Point1.X);
			miny = MIN(miny, line->Point1.Y);
			minx = MIN(minx, line->Point2.X);
			miny = MIN(miny, line->Point2.Y);
			maxx = MAX(maxx, line->Point1.X);
			maxy = MAX(maxy, line->Point1.Y);
			maxx = MAX(maxx, line->Point2.X);
			maxy = MAX(maxy, line->Point2.Y);
		}

		/* move symbol to left edge */
		for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
			MOVE_LINE_LOWLEVEL(line, -minx, 0);

		/* set symbol bounding box with a minimum cell size of (1,1) */
		symbol->Width = maxx - minx + 1;
		symbol->Height = maxy + 1;

		/* check total min/max  */
		Ptr->MaxWidth = MAX(Ptr->MaxWidth, symbol->Width);
		Ptr->MaxHeight = MAX(Ptr->MaxHeight, symbol->Height);
		totalminy = MIN(totalminy, miny);
	}

	/* move coordinate system to the upper edge (lowest y on screen) */
	for (i = 0, symbol = Ptr->Symbol; i <= MAX_FONTPOSITION; i++, symbol++)
		if (symbol->Valid) {
			symbol->Height -= totalminy;
			for (line = symbol->Line, j = symbol->LineN; j; j--, line++)
				MOVE_LINE_LOWLEVEL(line, 0, -totalminy);
		}

	/* setup the box for the default symbol */
	Ptr->DefaultSymbol.X1 = Ptr->DefaultSymbol.Y1 = 0;
	Ptr->DefaultSymbol.X2 = Ptr->DefaultSymbol.X1 + Ptr->MaxWidth;
	Ptr->DefaultSymbol.Y2 = Ptr->DefaultSymbol.Y1 + Ptr->MaxHeight;
}

/* ---------------------------------------------------------------------------
 * creates a filename from a template
 * %f is replaced by the filename
 * %p by the searchpath
 */
#warning TODO: kill this in favor of pcb_strdup_subst
char *EvaluateFilename(const char *Template, const char *Path, const char *Filename, const char *Parameter)
{
	gds_t command;
	const char *p;

	if (conf_core.rc.verbose) {
		printf("EvaluateFilename:\n");
		printf("\tTemplate: \033[33m%s\033[0m\n", Template);
		printf("\tPath: \033[33m%s\033[0m\n", Path);
		printf("\tFilename: \033[33m%s\033[0m\n", Filename);
		printf("\tParameter: \033[33m%s\033[0m\n", Parameter);
	}

	gds_init(&command);

	for (p = Template; p && *p; p++) {
		/* copy character or add string to command */
		if (*p == '%' && (*(p + 1) == 'f' || *(p + 1) == 'p' || *(p + 1) == 'a'))
			switch (*(++p)) {
			case 'a':
				gds_append_str(&command, Parameter);
				break;
			case 'f':
				gds_append_str(&command, Filename);
				break;
			case 'p':
				gds_append_str(&command, Path);
				break;
			}
		else
			gds_append(&command, *p);
	}

	if (conf_core.rc.verbose)
		printf("EvaluateFilename: \033[32m%s\033[0m\n", command.array);

	return command.array;
}

/* ---------------------------------------------------------------------------
 * returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
BoxTypePtr GetObjectBoundingBox(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_TEXT:
	case PCB_TYPE_POLYGON:
	case PCB_TYPE_PAD:
	case PCB_TYPE_PIN:
	case PCB_TYPE_ELEMENT_NAME:
		return (BoxType *) Ptr2;
	case PCB_TYPE_VIA:
	case PCB_TYPE_ELEMENT:
		return (BoxType *) Ptr1;
	case PCB_TYPE_POLYGON_POINT:
	case PCB_TYPE_LINE_POINT:
		return (BoxType *) Ptr3;
	default:
		Message(PCB_MSG_DEFAULT, "Request for bounding box of unsupported type %d\n", Type);
		return (BoxType *) Ptr2;
	}
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

/*
 * make a unique name for the name on board
 * this can alter the contents of the input string
 */
char *UniqueElementName(DataTypePtr Data, char *Name)
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

static void GetGridLockCoordinates(int type, void *ptr1, void *ptr2, void *ptr3, Coord * x, Coord * y)
{
	switch (type) {
	case PCB_TYPE_VIA:
		*x = ((PinTypePtr) ptr2)->X;
		*y = ((PinTypePtr) ptr2)->Y;
		break;
	case PCB_TYPE_LINE:
		*x = ((LineTypePtr) ptr2)->Point1.X;
		*y = ((LineTypePtr) ptr2)->Point1.Y;
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		*x = ((TextTypePtr) ptr2)->X;
		*y = ((TextTypePtr) ptr2)->Y;
		break;
	case PCB_TYPE_ELEMENT:
		*x = ((ElementTypePtr) ptr2)->MarkX;
		*y = ((ElementTypePtr) ptr2)->MarkY;
		break;
	case PCB_TYPE_POLYGON:
		*x = ((PolygonTypePtr) ptr2)->Points[0].X;
		*y = ((PolygonTypePtr) ptr2)->Points[0].Y;
		break;

	case PCB_TYPE_LINE_POINT:
	case PCB_TYPE_POLYGON_POINT:
		*x = ((PointTypePtr) ptr3)->X;
		*y = ((PointTypePtr) ptr3)->Y;
		break;
	case PCB_TYPE_ARC:
		{
			BoxTypePtr box;

			box = GetArcEnds((ArcTypePtr) ptr2);
			*x = box->X1;
			*y = box->Y1;
			break;
		}
	}
}

void AttachForCopy(Coord PlaceX, Coord PlaceY)
{
	BoxTypePtr box;
	Coord mx = 0, my = 0;

	Crosshair.AttachedObject.RubberbandN = 0;
	if (!conf_core.editor.snap_pin) {
		/* dither the grab point so that the mark, center, etc
		 * will end up on a grid coordinate
		 */
		GetGridLockCoordinates(Crosshair.AttachedObject.Type,
													 Crosshair.AttachedObject.Ptr1,
													 Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3, &mx, &my);
		mx = GridFit(mx, PCB->Grid, PCB->GridOffsetX) - mx;
		my = GridFit(my, PCB->Grid, PCB->GridOffsetY) - my;
	}
	Crosshair.AttachedObject.X = PlaceX - mx;
	Crosshair.AttachedObject.Y = PlaceY - my;
	if (!Marked.status || conf_core.editor.local_ref)
		SetLocalRef(PlaceX - mx, PlaceY - my, pcb_true);
	Crosshair.AttachedObject.State = STATE_SECOND;

	/* get boundingbox of object and set cursor range */
	box = GetObjectBoundingBox(Crosshair.AttachedObject.Type,
														 Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
	SetCrosshairRange(Crosshair.AttachedObject.X - box->X1,
										Crosshair.AttachedObject.Y - box->Y1,
										PCB->MaxWidth - (box->X2 - Crosshair.AttachedObject.X),
										PCB->MaxHeight - (box->Y2 - Crosshair.AttachedObject.Y));

	/* get all attached objects if necessary */
	if ((conf_core.editor.mode != PCB_MODE_COPY) && conf_core.editor.rubber_band_mode)
		LookupRubberbandLines(Crosshair.AttachedObject.Type,
													Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
	if (conf_core.editor.mode != PCB_MODE_COPY &&
			(Crosshair.AttachedObject.Type == PCB_TYPE_ELEMENT ||
			 Crosshair.AttachedObject.Type == PCB_TYPE_VIA ||
			 Crosshair.AttachedObject.Type == PCB_TYPE_LINE || Crosshair.AttachedObject.Type == PCB_TYPE_LINE_POINT))
		LookupRatLines(Crosshair.AttachedObject.Type,
									 Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
}

void r_delete_element(DataType * data, ElementType * element)
{
	r_delete_entry(data->element_tree, (BoxType *) element);
	PIN_LOOP(element);
	{
		r_delete_entry(data->pin_tree, (BoxType *) pin);
	}
	END_LOOP;
	PAD_LOOP(element);
	{
		r_delete_entry(data->pad_tree, (BoxType *) pad);
	}
	END_LOOP;
	ELEMENTTEXT_LOOP(element);
	{
		r_delete_entry(data->name_tree[n], (BoxType *) text);
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * Returns a best guess about the orientation of an element.  The
 * value corresponds to the rotation; a difference is the right value
 * to pass to RotateElementLowLevel.  However, the actual value is no
 * indication of absolute rotation; only relative rotation is
 * meaningful.
 */

int ElementOrientation(ElementType * e)
{
	Coord pin1x, pin1y, pin2x, pin2y, dx, dy;
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

int ActionListRotations(int argc, const char **argv, Coord x, Coord y)
{
	ELEMENT_LOOP(PCB->Data);
	{
		printf("%d %s\n", ElementOrientation(element), NAMEONPCB_NAME(element));
	}
	END_LOOP;

	return 0;
}

HID_Action misc_action_list[] = {
	{"ListRotations", 0, ActionListRotations,
	 0, 0}
	,
};

REGISTER_ACTIONS(misc_action_list, NULL)
