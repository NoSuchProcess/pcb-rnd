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
static void GetGridLockCoordinates(int, void *, void *, void *, Coord *, Coord *);

/* Local variables */

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
