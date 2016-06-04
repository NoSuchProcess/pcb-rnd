/* $Id$ */

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

/* functions used to create vias, pins ...
 */

#include "config.h"

#include <assert.h>
#include <memory.h>
#include <setjmp.h>
#include <stdlib.h>

#include "global.h"
#include "conf_core.h"

#include "create.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "mymem.h"
#include "misc.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rtree.h"
#include "search.h"
#include "set.h"
#include "undo.h"
#include "plug_io.h"
#include "stub_vendor.h"
#include "hid_actions.h"
#include "paths.h"

RCSID("$Id$");

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static int ID = 1;							/* current object ID; incremented after */
				/* each creation of an object */

static bool be_lenient = false;

/* ----------------------------------------------------------------------
 * some local prototypes
 */
static void AddTextToElement(TextTypePtr, FontTypePtr, Coord, Coord, unsigned, char *, int, FlagType);

/* ---------------------------------------------------------------------------
 *  Set the lenience mode.
 */

void CreateBeLenient(bool v)
{
	be_lenient = v;
}

/* ---------------------------------------------------------------------------
 * creates a new paste buffer
 */
DataTypePtr CreateNewBuffer(void)
{
	DataTypePtr data;
	data = (DataTypePtr) calloc(1, sizeof(DataType));
	data->pcb = (PCBTypePtr) PCB;
	return data;
}

/* ---------------------------------------------------------------------------
 * Perhaps PCB should internally just use the Settings colors?  For now,
 * use this to set PCB colors so the config can reassign PCB colors.
 */
void pcb_colors_from_settings(PCBTypePtr ptr)
{
	int i;

	/* copy default settings */
	ptr->ConnectedColor = conf_core.appearance.color.connected;
	ptr->ElementColor = conf_core.appearance.color.element;
	ptr->ElementColor_nonetlist = conf_core.appearance.color.element_nonetlist;
	ptr->RatColor = conf_core.appearance.color.rat;
	ptr->InvisibleObjectsColor = conf_core.appearance.color.invisible_objects;
	ptr->InvisibleMarkColor = conf_core.appearance.color.invisible_mark;
	ptr->ElementSelectedColor = conf_core.appearance.color.element_selected;
	ptr->RatSelectedColor = conf_core.appearance.color.rat_selected;
	ptr->PinColor = conf_core.appearance.color.pin;
	ptr->PinSelectedColor = conf_core.appearance.color.pin_selected;
	ptr->PinNameColor = conf_core.appearance.color.pin_name;
	ptr->ViaColor = conf_core.appearance.color.via;
	ptr->ViaSelectedColor = conf_core.appearance.color.via_selected;
	ptr->WarnColor = conf_core.appearance.color.warn;
	ptr->MaskColor = conf_core.appearance.color.mask;
	for (i = 0; i < MAX_LAYER; i++) {
		ptr->Data->Layer[i].Color = conf_core.appearance.color.layer[i];
		ptr->Data->Layer[i].SelectedColor = conf_core.appearance.color.layer_selected[i];
	}
	ptr->Data->Layer[component_silk_layer].Color =
		conf_core.editor.show_solder_side ? conf_core.appearance.color.invisible_objects : conf_core.appearance.color.element;
	ptr->Data->Layer[component_silk_layer].SelectedColor = conf_core.appearance.color.element_selected;
	ptr->Data->Layer[solder_silk_layer].Color = conf_core.editor.show_solder_side ? conf_core.appearance.color.element : conf_core.appearance.color.invisible_objects;
	ptr->Data->Layer[solder_silk_layer].SelectedColor = conf_core.appearance.color.element_selected;
}

/* ---------------------------------------------------------------------------
 * creates a new PCB
 */
PCBTypePtr CreateNewPCB_(bool SetDefaultNames)
{
	PCBTypePtr ptr;
	int i;

	/* allocate memory, switch all layers on and copy resources */
	ptr = (PCBTypePtr) calloc(1, sizeof(PCBType));
	ptr->Data = CreateNewBuffer();
	ptr->Data->pcb = (PCBTypePtr) ptr;

	ptr->ThermStyle = 4;
	ptr->IsleArea = 2.e8;
	ptr->SilkActive = false;
	ptr->RatDraw = false;

	/* NOTE: we used to set all the pcb flags on ptr here, but we don't need to do that anymore due to the new conf system */
	ptr->Clipping = 1;						/* this is the most useful starting point for now */

	ptr->Grid = conf_core.editor.grid;
	ParseGroupString(conf_core.design.groups, &ptr->LayerGroups, MAX_LAYER);
	hid_action("RouteStylesChanged");
	ptr->Zoom = conf_core.editor.zoom;
	ptr->MaxWidth = conf_core.design.max_width;
	ptr->MaxHeight = conf_core.design.max_height;
	ptr->ID = ID++;
	ptr->ThermScale = 0.5;

	ptr->Bloat = conf_core.design.bloat;
	ptr->Shrink = conf_core.design.shrink;
	ptr->minWid = conf_core.design.min_wid;
	ptr->minSlk = conf_core.design.min_slk;
	ptr->minDrill = conf_core.design.min_drill;
	ptr->minRing = conf_core.design.min_ring;

	for (i = 0; i < MAX_LAYER; i++)
		ptr->Data->Layer[i].Name = strdup(conf_core.design.default_layer_name[i]);

	CreateDefaultFont(ptr);

	return (ptr);
}

PCBTypePtr CreateNewPCB()
{
	PCBTypePtr old, nw;
	int dpcb;

	old = PCB;

	PCB = NULL;

	dpcb = -1;
	conf_list_foreach_path_first(dpcb, &conf_core.rc.default_pcb_file, LoadPCB(__path__, false, true));

#warning TODO: load from a lihata list instead? same for font file?
	if (dpcb == 0) {
		nw = PCB;
		if (nw->Filename != NULL) {
			/* make sure the new PCB doesn't inherit the name of the default pcb */
			free(nw->Filename);
			nw->Filename = NULL;
		}
	}
	else
		nw = NULL;

	PCB = old;
	return nw;
}


/* This post-processing step adds the top and bottom silk layers to a
 * pre-existing PCB.
 */
int CreateNewPCBPost(PCBTypePtr pcb, int use_defaults)
{
	/* copy default settings */
	pcb_colors_from_settings(pcb);

	return 0;
}

/* ---------------------------------------------------------------------------
 * creates a new via
 */
PinTypePtr
CreateNewVia(DataTypePtr Data,
						 Coord X, Coord Y,
						 Coord Thickness, Coord Clearance, Coord Mask, Coord DrillingHole, const char *Name, FlagType Flags)
{
	PinTypePtr Via;

	if (!be_lenient) {
		VIA_LOOP(Data);
		{
			if (Distance(X, Y, via->X, via->Y) <= via->DrillingHole / 2 + DrillingHole / 2) {
				Message(_("%m+Dropping via at %$mD because it's hole would overlap with the via "
									"at %$mD\n"), conf_core.editor.grid_unit->allow, X, Y, via->X, via->Y);
				return (NULL);					/* don't allow via stacking */
			}
		}
		END_LOOP;
	}

	Via = GetViaMemory(Data);

	if (!Via)
		return (Via);
	/* copy values */
	Via->X = X;
	Via->Y = Y;
	Via->Thickness = Thickness;
	Via->Clearance = Clearance;
	Via->Mask = Mask;
	Via->DrillingHole = stub_vendorDrillMap(DrillingHole);
	if (Via->DrillingHole != DrillingHole) {
		Message(_("%m+Mapped via drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, Via->DrillingHole, DrillingHole);
	}

	Via->Name = STRDUP(Name);
	Via->Flags = Flags;
	CLEAR_FLAG(WARNFLAG, Via);
	SET_FLAG(VIAFLAG, Via);
	Via->ID = ID++;

	/* 
	 * don't complain about MIN_PINORVIACOPPER on a mounting hole (pure
	 * hole)
	 */
	if (!TEST_FLAG(HOLEFLAG, Via) && (Via->Thickness < Via->DrillingHole + MIN_PINORVIACOPPER)) {
		Via->Thickness = Via->DrillingHole + MIN_PINORVIACOPPER;
		Message(_("%m+Increased via thickness to %$mS to allow enough copper"
							" at %$mD.\n"), conf_core.editor.grid_unit->allow, Via->Thickness, Via->X, Via->Y);
	}

	SetPinBoundingBox(Via);
	if (!Data->via_tree)
		Data->via_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Data->via_tree, (BoxTypePtr) Via, 0);
	return (Via);
}

struct line_info {
	Coord X1, X2, Y1, Y2;
	Coord Thickness;
	FlagType Flags;
	LineType test, *ans;
	jmp_buf env;
};

static int line_callback(const BoxType * b, void *cl)
{
	LineTypePtr line = (LineTypePtr) b;
	struct line_info *i = (struct line_info *) cl;

	if (line->Point1.X == i->X1 && line->Point2.X == i->X2 && line->Point1.Y == i->Y1 && line->Point2.Y == i->Y2) {
		i->ans = (LineTypePtr) (-1);
		longjmp(i->env, 1);
	}
	/* check the other point order */
	if (line->Point1.X == i->X1 && line->Point2.X == i->X2 && line->Point1.Y == i->Y1 && line->Point2.Y == i->Y2) {
		i->ans = (LineTypePtr) (-1);
		longjmp(i->env, 1);
	}
	if (line->Point2.X == i->X1 && line->Point1.X == i->X2 && line->Point2.Y == i->Y1 && line->Point1.Y == i->Y2) {
		i->ans = (LineTypePtr) - 1;
		longjmp(i->env, 1);
	}
	/* remove unnecessary line points */
	if (line->Thickness == i->Thickness
			/* don't merge lines if the clear flags differ  */
			&& TEST_FLAG(CLEARLINEFLAG, line) == TEST_FLAG(CLEARLINEFLAG, i)) {
		if (line->Point1.X == i->X1 && line->Point1.Y == i->Y1) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (IsPointOnLine(i->X1, i->Y1, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X1 && line->Point2.Y == i->Y1) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X2;
			i->test.Point2.Y = i->Y2;
			if (IsPointOnLine(i->X1, i->Y1, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point1.X == i->X2 && line->Point1.Y == i->Y2) {
			i->test.Point1.X = line->Point2.X;
			i->test.Point1.Y = line->Point2.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (IsPointOnLine(i->X2, i->Y2, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
		else if (line->Point2.X == i->X2 && line->Point2.Y == i->Y2) {
			i->test.Point1.X = line->Point1.X;
			i->test.Point1.Y = line->Point1.Y;
			i->test.Point2.X = i->X1;
			i->test.Point2.Y = i->Y1;
			if (IsPointOnLine(i->X2, i->Y2, 0.0, &i->test)) {
				i->ans = line;
				longjmp(i->env, 1);
			}
		}
	}
	return 0;
}


/* ---------------------------------------------------------------------------
 * creates a new line on a layer and checks for overlap and extension
 */
LineTypePtr
CreateDrawnLineOnLayer(LayerTypePtr Layer,
											 Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, FlagType Flags)
{
	struct line_info info;
	BoxType search;

	search.X1 = MIN(X1, X2);
	search.X2 = MAX(X1, X2);
	search.Y1 = MIN(Y1, Y2);
	search.Y2 = MAX(Y1, Y2);
	if (search.Y2 == search.Y1)
		search.Y2++;
	if (search.X2 == search.X1)
		search.X2++;
	info.X1 = X1;
	info.X2 = X2;
	info.Y1 = Y1;
	info.Y2 = Y2;
	info.Thickness = Thickness;
	info.Flags = Flags;
	info.test.Thickness = 0;
	info.test.Flags = NoFlags();
	info.ans = NULL;
	/* prevent stacking of duplicate lines
	 * and remove needless intermediate points
	 * verify that the layer is on the board first!
	 */
	if (setjmp(info.env) == 0) {
		r_search(Layer->line_tree, &search, NULL, line_callback, &info);
		return CreateNewLineOnLayer(Layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags);
	}

	if ((void *) info.ans == (void *) (-1))
		return NULL;								/* stacked line */
	/* remove unnecessary points */
	if (info.ans) {
		/* must do this BEFORE getting new line memory */
		MoveObjectToRemoveUndoList(LINE_TYPE, Layer, info.ans, info.ans);
		X1 = info.test.Point1.X;
		X2 = info.test.Point2.X;
		Y1 = info.test.Point1.Y;
		Y2 = info.test.Point2.Y;
	}
	return CreateNewLineOnLayer(Layer, X1, Y1, X2, Y2, Thickness, Clearance, Flags);
}

LineTypePtr
CreateNewLineOnLayer(LayerTypePtr Layer,
										 Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, FlagType Flags)
{
	LineTypePtr Line;

	Line = GetLineMemory(Layer);
	if (!Line)
		return (Line);
	Line->ID = ID++;
	Line->Flags = Flags;
	CLEAR_FLAG(RATFLAG, Line);
	Line->Thickness = Thickness;
	Line->Clearance = Clearance;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
	Line->Point1.ID = ID++;
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = ID++;
	SetLineBoundingBox(Line);
	if (!Layer->line_tree)
		Layer->line_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
	return (Line);
}

/* ---------------------------------------------------------------------------
 * creates a new rat-line
 */
RatTypePtr
CreateNewRat(DataTypePtr Data, Coord X1, Coord Y1,
						 Coord X2, Coord Y2, Cardinal group1, Cardinal group2, Coord Thickness, FlagType Flags)
{
	RatTypePtr Line = GetRatMemory(Data);

	if (!Line)
		return (Line);

	Line->ID = ID++;
	Line->Flags = Flags;
	SET_FLAG(RATFLAG, Line);
	Line->Thickness = Thickness;
	Line->Point1.X = X1;
	Line->Point1.Y = Y1;
	Line->Point1.ID = ID++;
	Line->Point2.X = X2;
	Line->Point2.Y = Y2;
	Line->Point2.ID = ID++;
	Line->group1 = group1;
	Line->group2 = group2;
	SetLineBoundingBox((LineTypePtr) Line);
	if (!Data->rat_tree)
		Data->rat_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Data->rat_tree, &Line->BoundingBox, 0);
	return (Line);
}

/* ---------------------------------------------------------------------------
 * creates a new arc on a layer
 */
ArcTypePtr
CreateNewArcOnLayer(LayerTypePtr Layer,
										Coord X1, Coord Y1,
										Coord width, Coord height, Angle sa, Angle dir, Coord Thickness, Coord Clearance, FlagType Flags)
{
	ArcTypePtr Arc;

	ARC_LOOP(Layer);
	{
		if (arc->X == X1 && arc->Y == Y1 && arc->Width == width &&
				NormalizeAngle(arc->StartAngle) == NormalizeAngle(sa) && arc->Delta == dir)
			return (NULL);						/* prevent stacked arcs */
	}
	END_LOOP;
	Arc = GetArcMemory(Layer);
	if (!Arc)
		return (Arc);

	Arc->ID = ID++;
	Arc->Flags = Flags;
	Arc->Thickness = Thickness;
	Arc->Clearance = Clearance;
	Arc->X = X1;
	Arc->Y = Y1;
	Arc->Width = width;
	Arc->Height = height;
	Arc->StartAngle = sa;
	Arc->Delta = dir;
	SetArcBoundingBox(Arc);
	if (!Layer->arc_tree)
		Layer->arc_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
	return (Arc);
}


/* ---------------------------------------------------------------------------
 * creates a new polygon from the old formats rectangle data
 */
PolygonTypePtr CreateNewPolygonFromRectangle(LayerTypePtr Layer, Coord X1, Coord Y1, Coord X2, Coord Y2, FlagType Flags)
{
	PolygonTypePtr polygon = CreateNewPolygon(Layer, Flags);
	if (!polygon)
		return (polygon);

	CreateNewPointInPolygon(polygon, X1, Y1);
	CreateNewPointInPolygon(polygon, X2, Y1);
	CreateNewPointInPolygon(polygon, X2, Y2);
	CreateNewPointInPolygon(polygon, X1, Y2);
	SetPolygonBoundingBox(polygon);
	if (!Layer->polygon_tree)
		Layer->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->polygon_tree, (BoxTypePtr) polygon, 0);
	return (polygon);
}

/* ---------------------------------------------------------------------------
 * creates a new text on a layer
 */
TextTypePtr
CreateNewText(LayerTypePtr Layer, FontTypePtr PCBFont,
							Coord X, Coord Y, unsigned Direction, int Scale, char *TextString, FlagType Flags)
{
	TextType *text;

	if (TextString == NULL)
		return NULL;

	text = GetTextMemory(Layer);
	if (text == NULL)
		return NULL;

	/* copy values, width and height are set by drawing routine
	 * because at this point we don't know which symbols are available
	 */
	text->X = X;
	text->Y = Y;
	text->Direction = Direction;
	text->Flags = Flags;
	text->Scale = Scale;
	text->TextString = strdup(TextString);

	/* calculate size of the bounding box */
	SetTextBoundingBox(PCBFont, text);
	text->ID = ID++;
	if (!Layer->text_tree)
		Layer->text_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->text_tree, (BoxTypePtr) text, 0);
	return (text);
}

/* ---------------------------------------------------------------------------
 * creates a new polygon on a layer
 */
PolygonTypePtr CreateNewPolygon(LayerTypePtr Layer, FlagType Flags)
{
	PolygonTypePtr polygon = GetPolygonMemory(Layer);

	/* copy values */
	polygon->Flags = Flags;
	polygon->ID = ID++;
	polygon->Clipped = NULL;
	polygon->NoHoles = NULL;
	polygon->NoHolesValid = 0;
	return (polygon);
}

/* ---------------------------------------------------------------------------
 * creates a new point in a polygon
 */
PointTypePtr CreateNewPointInPolygon(PolygonTypePtr Polygon, Coord X, Coord Y)
{
	PointTypePtr point = GetPointMemoryInPolygon(Polygon);

	/* copy values */
	point->X = X;
	point->Y = Y;
	point->ID = ID++;
	return (point);
}

/* ---------------------------------------------------------------------------
 * creates a new hole in a polygon
 */
PolygonType *CreateNewHoleInPolygon(PolygonType * Polygon)
{
	Cardinal *holeindex = GetHoleIndexMemoryInPolygon(Polygon);
	*holeindex = Polygon->PointN;
	return Polygon;
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
								 Coord TextX, Coord TextY, BYTE Direction, int TextScale, FlagType TextFlags, bool uniqueName)
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
	ArcType *arc;

	arc = calloc(sizeof(ArcType), 1);
	arclist_append(&Element->Arc, arc);

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

	line = calloc(sizeof(LineType), 1);
	linelist_append(&Element->Line, line);

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
 * creates a new pin in an element
 */
PinTypePtr
CreateNewPin(ElementTypePtr Element,
						 Coord X, Coord Y,
						 Coord Thickness, Coord Clearance, Coord Mask, Coord DrillingHole, char *Name, char *Number, FlagType Flags)
{
	PinTypePtr pin = GetPinMemory(Element);

	/* copy values */
	pin->X = X;
	pin->Y = Y;
	pin->Thickness = Thickness;
	pin->Clearance = Clearance;
	pin->Mask = Mask;
	pin->Name = STRDUP(Name);
	pin->Number = STRDUP(Number);
	pin->Flags = Flags;
	CLEAR_FLAG(WARNFLAG, pin);
	SET_FLAG(PINFLAG, pin);
	pin->ID = ID++;
	pin->Element = Element;

	/* 
	 * If there is no vendor drill map installed, this will simply
	 * return DrillingHole.
	 */
	pin->DrillingHole = stub_vendorDrillMap(DrillingHole);

	/* Unless we should not map drills on this element, map them! */
	if (stub_vendorIsElementMappable(Element)) {
		if (pin->DrillingHole < MIN_PINORVIASIZE) {
			Message(_("%m+Did not map pin #%s (%s) drill hole because %$mS is below the minimum allowed size\n"),
							conf_core.editor.grid_unit->allow, UNKNOWN(Number), UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
		else if (pin->DrillingHole > MAX_PINORVIASIZE) {
			Message(_("%m+Did not map pin #%s (%s) drill hole because %$mS is above the maximum allowed size\n"),
							conf_core.editor.grid_unit->allow, UNKNOWN(Number), UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
		else if (!TEST_FLAG(HOLEFLAG, pin)
						 && (pin->DrillingHole > pin->Thickness - MIN_PINORVIACOPPER)) {
			Message(_("%m+Did not map pin #%s (%s) drill hole because %$mS does not leave enough copper\n"),
							conf_core.editor.grid_unit->allow, UNKNOWN(Number), UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
	}
	else {
		pin->DrillingHole = DrillingHole;
	}

	if (pin->DrillingHole != DrillingHole) {
		Message(_("%m+Mapped pin drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, pin->DrillingHole, DrillingHole);
	}

	return (pin);
}

/* ---------------------------------------------------------------------------
 * creates a new pad in an element
 */
PadTypePtr
CreateNewPad(ElementTypePtr Element,
						 Coord X1, Coord Y1, Coord X2,
						 Coord Y2, Coord Thickness, Coord Clearance, Coord Mask, char *Name, char *Number, FlagType Flags)
{
	PadTypePtr pad = GetPadMemory(Element);

	/* copy values */
	if (X1 > X2 || (X1 == X2 && Y1 > Y2)) {
		pad->Point1.X = X2;
		pad->Point1.Y = Y2;
		pad->Point2.X = X1;
		pad->Point2.Y = Y1;
	}
	else {
		pad->Point1.X = X1;
		pad->Point1.Y = Y1;
		pad->Point2.X = X2;
		pad->Point2.Y = Y2;
	}
	pad->Thickness = Thickness;
	pad->Clearance = Clearance;
	pad->Mask = Mask;
	pad->Name = STRDUP(Name);
	pad->Number = STRDUP(Number);
	pad->Flags = Flags;
	CLEAR_FLAG(WARNFLAG, pad);
	pad->ID = ID++;
	pad->Element = Element;
	return (pad);
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
	Text->TextString = (TextString && *TextString) ? strdup(TextString) : NULL;
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
	conf_list_foreach_path_first(res, &conf_core.rc.default_font_file, ParseFont(&pcb->Font, __path__));
#warning TODO: print content of list
	if (res != 0)
		Message(_("Can't find font-symbol-file '%s'\n"), "<print paths here>");
}

/* ---------------------------------------------------------------------------
 * adds a new line to the rubberband list of 'Crosshair.AttachedObject'
 * if Layer == 0  it is a rat line
 */
RubberbandTypePtr CreateNewRubberbandEntry(LayerTypePtr Layer, LineTypePtr Line, PointTypePtr MovedPoint)
{
	RubberbandTypePtr ptr = GetRubberbandMemory();

	/* we toggle the RUBBERENDFLAG of the line to determine if */
	/* both points are being moved. */
	TOGGLE_FLAG(RUBBERENDFLAG, Line);
	ptr->Layer = Layer;
	ptr->Line = Line;
	ptr->MovedPoint = MovedPoint;
	return (ptr);
}

/* ---------------------------------------------------------------------------
 * Add a new net to the netlist menu
 */
LibraryMenuTypePtr CreateNewNet(LibraryTypePtr lib, char *name, char *style)
{
	LibraryMenuTypePtr menu;
	char temp[64];

	sprintf(temp, "  %s", name);
	menu = GetLibraryMenuMemory(lib, NULL);
	menu->Name = strdup(temp);
	menu->flag = 1;								/* net is enabled by default */
	if (style == NULL || NSTRCMP("(unknown)", style) == 0)
		menu->Style = NULL;
	else
		menu->Style = strdup(style);
	return (menu);
}

/* ---------------------------------------------------------------------------
 * Add a connection to the net
 */
LibraryEntryTypePtr CreateNewConnection(LibraryMenuTypePtr net, char *conn)
{
	LibraryEntryTypePtr entry = GetLibraryEntryMemory(net);

	entry->ListEntry = STRDUP(conn);
	entry->ListEntry_dontfree = 0;

	return (entry);
}

/* ---------------------------------------------------------------------------
 * Add an attribute to a list.
 */
AttributeTypePtr CreateNewAttribute(AttributeListTypePtr list, char *name, char *value)
{
	if (list->Number >= list->Max) {
		list->Max += 10;
		list->List = (AttributeType *) realloc(list->List, list->Max * sizeof(AttributeType));
	}
	list->List[list->Number].name = STRDUP(name);
	list->List[list->Number].value = STRDUP(value);
	list->Number++;
	return &list->List[list->Number - 1];
}
