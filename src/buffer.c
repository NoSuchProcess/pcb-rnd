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

/* functions used by paste- and move/copy buffer
 */
#include "config.h"
#include "conf_core.h"

#include "action_helper.h"
#include "buffer.h"
#include "board.h"
#include "copy.h"
#include "create.h"
#include "data.h"
#include "plug_io.h"
#include "mirror.h"
#include "misc.h"
#include "misc_util.h"
#include "polygon.h"
#include "rotate.h"
#include "remove.h"
#include "select.h"
#include "set.h"
#include "funchash_core.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_all.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *AddRatToBuffer(pcb_opctx_t *ctx, RatTypePtr);
static void *AddPolygonToBuffer(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr);
static void *AddElementToBuffer(pcb_opctx_t *ctx, ElementTypePtr);
static void *MoveRatToBuffer(pcb_opctx_t *ctx, RatTypePtr);
static void *MovePolygonToBuffer(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr);
static void *MoveElementToBuffer(pcb_opctx_t *ctx, ElementTypePtr);
static void SwapBuffer(BufferTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t AddBufferFunctions = {
	AddLineToBuffer,
	AddTextToBuffer,
	AddPolygonToBuffer,
	AddViaToBuffer,
	AddElementToBuffer,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	AddArcToBuffer,
	AddRatToBuffer
}, MoveBufferFunctions = {
MoveLineToBuffer,
		MoveTextToBuffer,
		MovePolygonToBuffer, MoveViaToBuffer, MoveElementToBuffer, NULL, NULL, NULL, NULL, NULL, MoveArcToBuffer, MoveRatToBuffer};


/* ---------------------------------------------------------------------------
 * copies a rat-line to paste buffer
 */
static void *AddRatToBuffer(pcb_opctx_t *ctx, RatTypePtr Rat)
{
	return (CreateNewRat(ctx->buffer.dst, Rat->Point1.X, Rat->Point1.Y,
											 Rat->Point2.X, Rat->Point2.Y, Rat->group1,
											 Rat->group2, Rat->Thickness, MaskFlags(Rat->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* ---------------------------------------------------------------------------
 * copies a polygon to buffer
 */
static void *AddPolygonToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	LayerTypePtr layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];
	PolygonTypePtr polygon;

	polygon = CreateNewPolygon(layer, Polygon->Flags);
	CopyPolygonLowLevel(polygon, Polygon);

	/* Update the polygon r-tree. Unlike similarly named functions for
	 * other objects, CreateNewPolygon does not do this as it creates a
	 * skeleton polygon object, which won't have correct bounds.
	 */
	if (!layer->polygon_tree)
		layer->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(layer->polygon_tree, (BoxType *) polygon, 0);

	CLEAR_FLAG(PCB_FLAG_FOUND | ctx->buffer.extraflg, polygon);
	return (polygon);
}

/* ---------------------------------------------------------------------------
 * copies a element to buffer
 */
static void *AddElementToBuffer(pcb_opctx_t *ctx, ElementTypePtr Element)
{
	ElementTypePtr element;

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

/* ---------------------------------------------------------------------------
 * moves a rat-line to paste buffer
 */
static void *MoveRatToBuffer(pcb_opctx_t *ctx, RatType * rat)
{
	r_delete_entry(ctx->buffer.src->rat_tree, (BoxType *) rat);

	ratlist_remove(rat);
	ratlist_append(&ctx->buffer.dst->Rat, rat);

	CLEAR_FLAG(PCB_FLAG_FOUND, rat);

	if (!ctx->buffer.dst->rat_tree)
		ctx->buffer.dst->rat_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(ctx->buffer.dst->rat_tree, (BoxType *) rat, 0);
	return rat;
}

/* ---------------------------------------------------------------------------
 * moves a polygon to buffer. Doesn't allocate memory for the points
 */
static void *MovePolygonToBuffer(pcb_opctx_t *ctx, LayerType * layer, PolygonType * polygon)
{
	LayerType *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	r_delete_entry(layer->polygon_tree, (BoxType *) polygon);

	polylist_remove(polygon);
	polylist_append(&lay->Polygon, polygon);

	CLEAR_FLAG(PCB_FLAG_FOUND, polygon);

	if (!lay->polygon_tree)
		lay->polygon_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(lay->polygon_tree, (BoxType *) polygon, 0);
	return (polygon);
}

/* ---------------------------------------------------------------------------
 * moves a element to buffer without allocating memory for pins/names
 */
static void *MoveElementToBuffer(pcb_opctx_t *ctx, ElementType * element)
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

/* ---------------------------------------------------------------------------
 * calculates the bounding box of the buffer
 */
void SetBufferBoundingBox(BufferTypePtr Buffer)
{
	BoxTypePtr box = GetDataBoundingBox(Buffer->Data);

	if (box)
		Buffer->BoundingBox = *box;
}

/* ---------------------------------------------------------------------------
 * clears the contents of the paste buffer
 */
void ClearBuffer(BufferTypePtr Buffer)
{
	if (Buffer && Buffer->Data) {
		FreeDataMemory(Buffer->Data);
		Buffer->Data->pcb = PCB;
	}
}

/* ----------------------------------------------------------------------
 * copies all selected and visible objects to the paste buffer
 * returns true if any objects have been removed
 */
void AddSelectedToBuffer(BufferTypePtr Buffer, Coord X, Coord Y, pcb_bool LeaveSelected)
{
	pcb_opctx_t ctx;

	ctx.buffer.pcb = PCB;

	/* switch crosshair off because adding objects to the pastebuffer
	 * may change the 'valid' area for the cursor
	 */
	if (!LeaveSelected)
		ctx.buffer.extraflg = PCB_FLAG_SELECTED;
	else
		ctx.buffer.extraflg =  0;

	notify_crosshair_change(pcb_false);
	ctx.buffer.src = PCB->Data;
	ctx.buffer.dst = Buffer->Data;
	SelectedOperation(&AddBufferFunctions, &ctx, pcb_false, PCB_TYPEMASK_ALL);

	/* set origin to passed or current position */
	if (X || Y) {
		Buffer->X = X;
		Buffer->Y = Y;
	}
	else {
		Buffer->X = Crosshair.X;
		Buffer->Y = Crosshair.Y;
	}
	notify_crosshair_change(pcb_true);
}

/* ---------------------------------------------------------------------------
 * loads element data from file/library into buffer
 * parse the file with disabled 'PCB mode' (see parser)
 * returns pcb_false on error
 * if successful, update some other stuff and reposition the pastebuffer
 */
pcb_bool LoadElementToBuffer(BufferTypePtr Buffer, const char *Name)
{
	ElementTypePtr element;

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


/*---------------------------------------------------------------------------
 * Searches for the given element by "footprint" name, and loads it
 * into the buffer.
 */

/* Returns zero on success, non-zero on error.  */
int LoadFootprintByName(BufferTypePtr Buffer, const char *Footprint)
{
	return !LoadElementToBuffer(Buffer, Footprint);
}


static const char loadfootprint_syntax[] = "LoadFootprint(filename[,refdes,value])";

static const char loadfootprint_help[] = "Loads a single footprint by name.";

/* %start-doc actions LoadFootprint

Loads a single footprint by name, rather than by reference or through
the library.  If a refdes and value are specified, those are inserted
into the footprint as well.  The footprint remains in the paste buffer.

%end-doc */

int LoadFootprint(int argc, const char **argv, Coord x, Coord y)
{
	const char *name = ACTION_ARG(0);
	const char *refdes = ACTION_ARG(1);
	const char *value = ACTION_ARG(2);
	ElementTypePtr e;

	if (!name)
		AFAIL(loadfootprint);

	if (LoadFootprintByName(PASTEBUFFER, name))
		return 1;

	if (elementlist_length(&PASTEBUFFER->Data->Element) == 0) {
		Message(PCB_MSG_DEFAULT, "Footprint %s contains no elements", name);
		return 1;
	}
	if (elementlist_length(&PASTEBUFFER->Data->Element) > 1) {
		Message(PCB_MSG_DEFAULT, "Footprint %s contains multiple elements", name);
		return 1;
	}

	e = elementlist_first(&PASTEBUFFER->Data->Element);

	if (e->Name[0].TextString)
		free(e->Name[0].TextString);
	e->Name[0].TextString = pcb_strdup(name);

	if (e->Name[1].TextString)
		free(e->Name[1].TextString);
	e->Name[1].TextString = refdes ? pcb_strdup(refdes) : 0;

	if (e->Name[2].TextString)
		free(e->Name[2].TextString);
	e->Name[2].TextString = value ? pcb_strdup(value) : 0;

	return 0;
}

/*---------------------------------------------------------------------------
 *
 * break buffer element into pieces
 */
pcb_bool SmashBufferElement(BufferTypePtr Buffer)
{
	ElementTypePtr element;
	pcb_cardinal_t group;
	LayerTypePtr clayer, slayer;

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
		FlagType f = NoFlags();
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
		LineTypePtr line;
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

/*---------------------------------------------------------------------------
 *
 * see if a polygon is a rectangle.  If so, canonicalize it.
 */

static int polygon_is_rectangle(PolygonTypePtr poly)
{
	int i, best;
	PointType temp[4];
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

/*---------------------------------------------------------------------------
 *
 * convert buffer contents into an element
 */
pcb_bool ConvertBufferToElement(BufferTypePtr Buffer)
{
	ElementTypePtr Element;
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
				Coord x1, y1, x2, y2, w, h, t;

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

/* ---------------------------------------------------------------------------
 * load PCB into buffer
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 */
pcb_bool LoadLayoutToBuffer(BufferTypePtr Buffer, const char *Filename, const char *fmt)
{
	PCBTypePtr newPCB = CreateNewPCB();

	/* new data isn't added to the undo list */
	if (!ParsePCB(newPCB, Filename, fmt, CFR_invalid)) {
		/* clear data area and replace pointer */
		ClearBuffer(Buffer);
		free(Buffer->Data);
		Buffer->Data = newPCB->Data;
		newPCB->Data = NULL;
		Buffer->X = newPCB->CursorX;
		Buffer->Y = newPCB->CursorY;
		RemovePCB(newPCB);
		Buffer->Data->pcb = PCB;
		return (pcb_true);
	}

	/* release unused memory */
	RemovePCB(newPCB);
	Buffer->Data->pcb = PCB;
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * rotates the contents of the pastebuffer
 */
void RotateBuffer(BufferTypePtr Buffer, pcb_uint8_t Number)
{
	/* rotate vias */
	VIA_LOOP(Buffer->Data);
	{
		r_delete_entry(Buffer->Data->via_tree, (BoxType *) via);
		ROTATE_VIA_LOWLEVEL(via, Buffer->X, Buffer->Y, Number);
		SetPinBoundingBox(via);
		r_insert_entry(Buffer->Data->via_tree, (BoxType *) via, 0);
	}
	END_LOOP;

	/* elements */
	ELEMENT_LOOP(Buffer->Data);
	{
		RotateElementLowLevel(Buffer->Data, element, Buffer->X, Buffer->Y, Number);
	}
	END_LOOP;

	/* all layer related objects */
	ALLLINE_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->line_tree, (BoxType *) line);
		RotateLineLowLevel(line, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->line_tree, (BoxType *) line, 0);
	}
	ENDALL_LOOP;
	ALLARC_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->arc_tree, (BoxType *) arc);
		RotateArcLowLevel(arc, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->arc_tree, (BoxType *) arc, 0);
	}
	ENDALL_LOOP;
	ALLTEXT_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->text_tree, (BoxType *) text);
		RotateTextLowLevel(text, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->text_tree, (BoxType *) text, 0);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->polygon_tree, (BoxType *) polygon);
		RotatePolygonLowLevel(polygon, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->polygon_tree, (BoxType *) polygon, 0);
	}
	ENDALL_LOOP;

	/* finally the origin and the bounding box */
	ROTATE(Buffer->X, Buffer->Y, Buffer->X, Buffer->Y, Number);
	RotateBoxLowLevel(&Buffer->BoundingBox, Buffer->X, Buffer->Y, Number);
}

static void free_rotate(Coord * x, Coord * y, Coord cx, Coord cy, double cosa, double sina)
{
	double nx, ny;
	Coord px = *x - cx;
	Coord py = *y - cy;

	nx = px * cosa + py * sina;
	ny = py * cosa - px * sina;

	*x = nx + cx;
	*y = ny + cy;
}

void
FreeRotateElementLowLevel(DataTypePtr Data, ElementTypePtr Element, Coord X, Coord Y, double cosa, double sina, Angle angle)
{
	/* solder side objects need a different orientation */

	/* the text subroutine decides by itself if the direction
	 * is to be corrected
	 */
#if 0
	ELEMENTTEXT_LOOP(Element);
	{
		if (Data && Data->name_tree[n])
			r_delete_entry(Data->name_tree[n], (BoxType *) text);
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
			r_delete_entry(Data->pin_tree, (BoxType *) pin);
		RestoreToPolygon(Data, PCB_TYPE_PIN, Element, pin);
		free_rotate(&pin->X, &pin->Y, X, Y, cosa, sina);
		SetPinBoundingBox(pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		/* pre-delete the pads before their coordinates change */
		if (Data)
			r_delete_entry(Data->pad_tree, (BoxType *) pad);
		RestoreToPolygon(Data, PCB_TYPE_PAD, Element, pad);
		free_rotate(&pad->Point1.X, &pad->Point1.Y, X, Y, cosa, sina);
		free_rotate(&pad->Point2.X, &pad->Point2.Y, X, Y, cosa, sina);
		SetLineBoundingBox((LineType *) pad);
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

void FreeRotateBuffer(BufferTypePtr Buffer, Angle angle)
{
	double cosa, sina;

	cosa = cos(angle * M_PI / 180.0);
	sina = sin(angle * M_PI / 180.0);

	/* rotate vias */
	VIA_LOOP(Buffer->Data);
	{
		r_delete_entry(Buffer->Data->via_tree, (BoxType *) via);
		free_rotate(&via->X, &via->Y, Buffer->X, Buffer->Y, cosa, sina);
		SetPinBoundingBox(via);
		r_insert_entry(Buffer->Data->via_tree, (BoxType *) via, 0);
	}
	END_LOOP;

	/* elements */
	ELEMENT_LOOP(Buffer->Data);
	{
		FreeRotateElementLowLevel(Buffer->Data, element, Buffer->X, Buffer->Y, cosa, sina, angle);
	}
	END_LOOP;

	/* all layer related objects */
	ALLLINE_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->line_tree, (BoxType *) line);
		free_rotate(&line->Point1.X, &line->Point1.Y, Buffer->X, Buffer->Y, cosa, sina);
		free_rotate(&line->Point2.X, &line->Point2.Y, Buffer->X, Buffer->Y, cosa, sina);
		SetLineBoundingBox(line);
		r_insert_entry(layer->line_tree, (BoxType *) line, 0);
	}
	ENDALL_LOOP;
	ALLARC_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->arc_tree, (BoxType *) arc);
		free_rotate(&arc->X, &arc->Y, Buffer->X, Buffer->Y, cosa, sina);
		arc->StartAngle = NormalizeAngle(arc->StartAngle + angle);
		r_insert_entry(layer->arc_tree, (BoxType *) arc, 0);
	}
	ENDALL_LOOP;
	/* FIXME: rotate text */
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->polygon_tree, (BoxType *) polygon);
		POLYGONPOINT_LOOP(polygon);
		{
			free_rotate(&point->X, &point->Y, Buffer->X, Buffer->Y, cosa, sina);
		}
		END_LOOP;
		SetPolygonBoundingBox(polygon);
		r_insert_entry(layer->polygon_tree, (BoxType *) polygon, 0);
	}
	ENDALL_LOOP;

	SetBufferBoundingBox(Buffer);
}


/* -------------------------------------------------------------------------- */

static const char freerotatebuffer_syntax[] = "FreeRotateBuffer([Angle])";

static const char freerotatebuffer_help[] =
	"Rotates the current paste buffer contents by the specified angle.  The\n"
	"angle is given in degrees.  If no angle is given, the user is prompted\n" "for one.\n";

/* %start-doc actions FreeRotateBuffer

Rotates the contents of the pastebuffer by an arbitrary angle.  If no
angle is given, the user is prompted for one.

%end-doc */

int ActionFreeRotateBuffer(int argc, const char **argv, Coord x, Coord y)
{
	const char *angle_s;

	if (argc < 1)
		angle_s = gui->prompt_for("Enter Rotation (degrees, CCW):", "0");
	else
		angle_s = argv[0];

	notify_crosshair_change(pcb_false);
	FreeRotateBuffer(PASTEBUFFER, strtod(angle_s, 0));
	notify_crosshair_change(pcb_true);
	return 0;
}

/* ---------------------------------------------------------------------------
 * initializes the buffers by allocating memory
 */
void InitBuffers(void)
{
	int i;

	for (i = 0; i < MAX_BUFFER; i++)
		Buffers[i].Data = CreateNewBuffer();
}

void UninitBuffers(void)
{
	int i;

	for (i = 0; i < MAX_BUFFER; i++) {
		ClearBuffer(Buffers+i);
		free(Buffers[i].Data);
	}
}

void pcb_swap_buffers(void)
{
	int i;

	for (i = 0; i < MAX_BUFFER; i++)
		SwapBuffer(&Buffers[i]);
	SetCrosshairRangeToBuffer();
}

void MirrorBuffer(BufferTypePtr Buffer)
{
	int i;

	if (elementlist_length(&Buffer->Data->Element)) {
		Message(PCB_MSG_DEFAULT, _("You can't mirror a buffer that has elements!\n"));
		return;
	}
	for (i = 0; i < max_copper_layer + 2; i++) {
		LayerTypePtr layer = Buffer->Data->Layer + i;
		if (textlist_length(&layer->Text)) {
			Message(PCB_MSG_DEFAULT, _("You can't mirror a buffer that has text!\n"));
			return;
		}
	}
	/* set buffer offset to 'mark' position */
	Buffer->X = SWAP_X(Buffer->X);
	Buffer->Y = SWAP_Y(Buffer->Y);
	VIA_LOOP(Buffer->Data);
	{
		via->X = SWAP_X(via->X);
		via->Y = SWAP_Y(via->Y);
	}
	END_LOOP;
	ALLLINE_LOOP(Buffer->Data);
	{
		line->Point1.X = SWAP_X(line->Point1.X);
		line->Point1.Y = SWAP_Y(line->Point1.Y);
		line->Point2.X = SWAP_X(line->Point2.X);
		line->Point2.Y = SWAP_Y(line->Point2.Y);
	}
	ENDALL_LOOP;
	ALLARC_LOOP(Buffer->Data);
	{
		arc->X = SWAP_X(arc->X);
		arc->Y = SWAP_Y(arc->Y);
		arc->StartAngle = SWAP_ANGLE(arc->StartAngle);
		arc->Delta = SWAP_DELTA(arc->Delta);
		SetArcBoundingBox(arc);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		POLYGONPOINT_LOOP(polygon);
		{
			point->X = SWAP_X(point->X);
			point->Y = SWAP_Y(point->Y);
		}
		END_LOOP;
		SetPolygonBoundingBox(polygon);
	}
	ENDALL_LOOP;
	SetBufferBoundingBox(Buffer);
}


/* ---------------------------------------------------------------------------
 * flip components/tracks from one side to the other
 */
static void SwapBuffer(BufferTypePtr Buffer)
{
	int j, k;
	pcb_cardinal_t sgroup, cgroup;
	LayerType swap;

	ELEMENT_LOOP(Buffer->Data);
	{
		r_delete_element(Buffer->Data, element);
		MirrorElementCoordinates(Buffer->Data, element, 0);
	}
	END_LOOP;
	/* set buffer offset to 'mark' position */
	Buffer->X = SWAP_X(Buffer->X);
	Buffer->Y = SWAP_Y(Buffer->Y);
	VIA_LOOP(Buffer->Data);
	{
		r_delete_entry(Buffer->Data->via_tree, (BoxType *) via);
		via->X = SWAP_X(via->X);
		via->Y = SWAP_Y(via->Y);
		SetPinBoundingBox(via);
		r_insert_entry(Buffer->Data->via_tree, (BoxType *) via, 0);
	}
	END_LOOP;
	ALLLINE_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->line_tree, (BoxType *) line);
		line->Point1.X = SWAP_X(line->Point1.X);
		line->Point1.Y = SWAP_Y(line->Point1.Y);
		line->Point2.X = SWAP_X(line->Point2.X);
		line->Point2.Y = SWAP_Y(line->Point2.Y);
		SetLineBoundingBox(line);
		r_insert_entry(layer->line_tree, (BoxType *) line, 0);
	}
	ENDALL_LOOP;
	ALLARC_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->arc_tree, (BoxType *) arc);
		arc->X = SWAP_X(arc->X);
		arc->Y = SWAP_Y(arc->Y);
		arc->StartAngle = SWAP_ANGLE(arc->StartAngle);
		arc->Delta = SWAP_DELTA(arc->Delta);
		SetArcBoundingBox(arc);
		r_insert_entry(layer->arc_tree, (BoxType *) arc, 0);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->polygon_tree, (BoxType *) polygon);
		POLYGONPOINT_LOOP(polygon);
		{
			point->X = SWAP_X(point->X);
			point->Y = SWAP_Y(point->Y);
		}
		END_LOOP;
		SetPolygonBoundingBox(polygon);
		r_insert_entry(layer->polygon_tree, (BoxType *) polygon, 0);
		/* hmmm, how to handle clip */
	}
	ENDALL_LOOP;
	ALLTEXT_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->text_tree, (BoxType *) text);
		text->X = SWAP_X(text->X);
		text->Y = SWAP_Y(text->Y);
		TOGGLE_FLAG(PCB_FLAG_ONSOLDER, text);
		SetTextBoundingBox(&PCB->Font, text);
		r_insert_entry(layer->text_tree, (BoxType *) text, 0);
	}
	ENDALL_LOOP;
	/* swap silkscreen layers */
	swap = Buffer->Data->Layer[solder_silk_layer];
	Buffer->Data->Layer[solder_silk_layer] = Buffer->Data->Layer[component_silk_layer];
	Buffer->Data->Layer[component_silk_layer] = swap;

	/* swap layer groups when balanced */
	sgroup = GetLayerGroupNumberByNumber(solder_silk_layer);
	cgroup = GetLayerGroupNumberByNumber(component_silk_layer);
	if (PCB->LayerGroups.Number[cgroup] == PCB->LayerGroups.Number[sgroup]) {
		for (j = k = 0; j < PCB->LayerGroups.Number[sgroup]; j++) {
			int t1, t2;
			pcb_cardinal_t cnumber = PCB->LayerGroups.Entries[cgroup][k];
			pcb_cardinal_t snumber = PCB->LayerGroups.Entries[sgroup][j];

			if (snumber >= max_copper_layer)
				continue;
			swap = Buffer->Data->Layer[snumber];

			while (cnumber >= max_copper_layer) {
				k++;
				cnumber = PCB->LayerGroups.Entries[cgroup][k];
			}
			Buffer->Data->Layer[snumber] = Buffer->Data->Layer[cnumber];
			Buffer->Data->Layer[cnumber] = swap;
			k++;
			/* move the thermal flags with the layers */
			ALLPIN_LOOP(Buffer->Data);
			{
				t1 = TEST_THERM(snumber, pin);
				t2 = TEST_THERM(cnumber, pin);
				ASSIGN_THERM(snumber, t2, pin);
				ASSIGN_THERM(cnumber, t1, pin);
			}
			ENDALL_LOOP;
			VIA_LOOP(Buffer->Data);
			{
				t1 = TEST_THERM(snumber, via);
				t2 = TEST_THERM(cnumber, via);
				ASSIGN_THERM(snumber, t2, via);
				ASSIGN_THERM(cnumber, t1, via);
			}
			END_LOOP;
		}
	}
	SetBufferBoundingBox(Buffer);
}

/* ----------------------------------------------------------------------
 * moves the passed object to the passed buffer and removes it
 * from its original place
 */
void *MoveObjectToBuffer(DataTypePtr Destination, DataTypePtr Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	/* setup local identifiers used by move operations */
	ctx.buffer.pcb = PCB;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	return (ObjectOperation(&MoveBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

/* ----------------------------------------------------------------------
 * Adds the passed object to the passed buffer
 */
void *CopyObjectToBuffer(DataTypePtr Destination, DataTypePtr Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.buffer.pcb = PCB;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	return (ObjectOperation(&AddBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

/* ---------------------------------------------------------------------- */

static const char pastebuffer_syntax[] =
	"PasteBuffer(AddSelected|Clear|1..MAX_BUFFER)\n"
	"PasteBuffer(Rotate, 1..3)\n" "PasteBuffer(Convert|Restore|Mirror)\n" "PasteBuffer(ToLayout, X, Y, units)\n" "PasteBuffer(Save, Filename, [format], [force])";

static const char pastebuffer_help[] = "Various operations on the paste buffer.";

/* %start-doc actions PasteBuffer

There are a number of paste buffers; the actual limit is a
compile-time constant @code{MAX_BUFFER} in @file{globalconst.h}.  It
is currently @code{5}.  One of these is the ``current'' paste buffer,
often referred to as ``the'' paste buffer.

@table @code

@item AddSelected
Copies the selected objects to the current paste buffer.

@item Clear
Remove all objects from the current paste buffer.

@item Convert
Convert the current paste buffer to an element.  Vias are converted to
pins, lines are converted to pads.

@item Restore
Convert any elements in the paste buffer back to vias and lines.

@item Mirror
Flip all objects in the paste buffer vertically (up/down flip).  To mirror
horizontally, combine this with rotations.

@item Rotate
Rotates the current buffer.  The number to pass is 1..3, where 1 means
90 degrees counter clockwise, 2 means 180 degrees, and 3 means 90
degrees clockwise (270 CCW).

@item Save
Saves any elements in the current buffer to the indicated file. If
format is specified, try to use that file format, else use the default.
If force is specified, overwrite target, don't ask.

@item ToLayout
Pastes any elements in the current buffer to the indicated X, Y
coordinates in the layout.  The @code{X} and @code{Y} are treated like
@code{delta} is for many other objects.  For each, if it's prefixed by
@code{+} or @code{-}, then that amount is relative to the last
location.  Otherwise, it's absolute.  Units can be
@code{mil} or @code{mm}; if unspecified, units are PCB's internal
units, currently 1/100 mil.


@item 1..MAX_BUFFER
Selects the given buffer to be the current paste buffer.

@end table

%end-doc */

static int ActionPasteBuffer(int argc, const char **argv, Coord x, Coord y)
{
	const char *function = argc ? argv[0] : "";
	const char *sbufnum = argc > 1 ? argv[1] : "";
	const char *fmt = argc > 2 ? argv[2] : NULL;
	const char *forces = argc > 3 ? argv[3] : NULL;
	const char *name;
	static char *default_file = NULL;
	pcb_bool free_name = pcb_false;
	int force = (forces != NULL) && ((*forces == '1') || (*forces == 'y') || (*forces == 'Y'));

	notify_crosshair_change(pcb_false);
	if (function) {
		switch (funchash_get(function, NULL)) {
			/* clear contents of paste buffer */
		case F_Clear:
			ClearBuffer(PASTEBUFFER);
			break;

			/* copies objects to paste buffer */
		case F_AddSelected:
			AddSelectedToBuffer(PASTEBUFFER, 0, 0, pcb_false);
			break;

			/* converts buffer contents into an element */
		case F_Convert:
			ConvertBufferToElement(PASTEBUFFER);
			break;

			/* break up element for editing */
		case F_Restore:
			SmashBufferElement(PASTEBUFFER);
			break;

			/* Mirror buffer */
		case F_Mirror:
			MirrorBuffer(PASTEBUFFER);
			break;

		case F_Rotate:
			if (sbufnum) {
				RotateBuffer(PASTEBUFFER, (pcb_uint8_t) atoi(sbufnum));
				SetCrosshairRangeToBuffer();
			}
			break;

		case F_Save:
			if (elementlist_length(&PASTEBUFFER->Data->Element) == 0) {
				Message(PCB_MSG_DEFAULT, _("Buffer has no elements!\n"));
				break;
			}
			free_name = pcb_false;
			if (argc <= 1) {
				name = gui->fileselect(_("Save Paste Buffer As ..."),
															 _("Choose a file to save the contents of the\n"
																 "paste buffer to.\n"), default_file, ".fp", "footprint", 0);

				if (default_file) {
					free(default_file);
					default_file = NULL;
				}
				if (name && *name) {
					default_file = pcb_strdup(name);
				}
				free_name = pcb_true;
			}

			else
				name = argv[1];

			{
				FILE *exist;

				if ((!force) && ((exist = fopen(name, "r")))) {
					fclose(exist);
					if (gui->confirm_dialog(_("File exists!  Ok to overwrite?"), 0))
						SaveBufferElements(name, fmt);
				}
				else
					SaveBufferElements(name, fmt);

				if (free_name && name)
					free((char*)name);
			}
			break;

		case F_ToLayout:
			{
				static Coord oldx = 0, oldy = 0;
				Coord x, y;
				pcb_bool absolute;

				if (argc == 1) {
					x = y = 0;
				}
				else if (argc == 3 || argc == 4) {
					x = GetValue(ACTION_ARG(1), ACTION_ARG(3), &absolute, NULL);
					if (!absolute)
						x += oldx;
					y = GetValue(ACTION_ARG(2), ACTION_ARG(3), &absolute, NULL);
					if (!absolute)
						y += oldy;
				}
				else {
					notify_crosshair_change(pcb_true);
					AFAIL(pastebuffer);
				}

				oldx = x;
				oldy = y;
				if (CopyPastebufferToLayout(x, y))
					SetChangedFlag(pcb_true);
			}
			break;

			/* set number */
		default:
			{
				int number = atoi(function);

				/* correct number */
				if (number)
					SetBufferNumber(number - 1);
			}
		}
	}

	notify_crosshair_change(pcb_true);
	return 0;
}

/* --------------------------------------------------------------------------- */

HID_Action buffer_action_list[] = {
	{"FreeRotateBuffer", 0, ActionFreeRotateBuffer,
	 freerotatebuffer_help, freerotatebuffer_syntax}
	,
	{"LoadFootprint", 0, LoadFootprint,
	 loadfootprint_help, loadfootprint_syntax}
	,
	{"PasteBuffer", 0, ActionPasteBuffer,
	 pastebuffer_help, pastebuffer_syntax}
};

REGISTER_ACTIONS(buffer_action_list, NULL)
