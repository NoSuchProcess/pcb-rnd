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
#include "data.h"
#include "plug_io.h"
#include "polygon.h"
#include "rotate.h"
#include "remove.h"
#include "select.h"
#include "set.h"
#include "draw.h"
#include "undo.h"
#include "funchash_core.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "obj_all_op.h"

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
};

static pcb_opfunc_t MoveBufferFunctions = {
	MoveLineToBuffer,
	MoveTextToBuffer,
	MovePolygonToBuffer,
	MoveViaToBuffer,
	MoveElementToBuffer,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	MoveArcToBuffer,
	MoveRatToBuffer
};

/* ---------------------------------------------------------------------------
 * calculates the bounding box of the buffer
 */
void pcb_set_buffer_bbox(pcb_buffer_t *Buffer)
{
	pcb_box_t *box = pcb_data_bbox(Buffer->Data);

	if (box)
		Buffer->BoundingBox = *box;
}

/* ---------------------------------------------------------------------------
 * clears the contents of the paste buffer
 */
void pcb_buffer_clear(pcb_buffer_t *Buffer)
{
	if (Buffer && Buffer->Data) {
		pcb_data_free(Buffer->Data);
		Buffer->Data->pcb = PCB;
	}
}

/* ----------------------------------------------------------------------
 * copies all selected and visible objects to the paste buffer
 * returns true if any objects have been removed
 */
void pcb_buffer_add_selected(pcb_buffer_t *Buffer, pcb_coord_t X, pcb_coord_t Y, pcb_bool LeaveSelected)
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

	pcb_notify_crosshair_change(pcb_false);
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
	pcb_notify_crosshair_change(pcb_true);
}

/*---------------------------------------------------------------------------*/

static const char loadfootprint_syntax[] = "pcb_load_footprint(filename[,refdes,value])";

static const char loadfootprint_help[] = "Loads a single footprint by name.";

/* %start-doc actions LoadFootprint

Loads a single footprint by name, rather than by reference or through
the library.  If a refdes and value are specified, those are inserted
into the footprint as well.  The footprint remains in the paste buffer.

%end-doc */

int pcb_load_footprint(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *name = PCB_ACTION_ARG(0);
	const char *refdes = PCB_ACTION_ARG(1);
	const char *value = PCB_ACTION_ARG(2);
	pcb_element_t *e;

	if (!name)
		PCB_AFAIL(loadfootprint);

	if (LoadFootprintByName(PCB_PASTEBUFFER, name))
		return 1;

	if (elementlist_length(&PCB_PASTEBUFFER->Data->Element) == 0) {
		pcb_message(PCB_MSG_DEFAULT, "Footprint %s contains no elements", name);
		return 1;
	}
	if (elementlist_length(&PCB_PASTEBUFFER->Data->Element) > 1) {
		pcb_message(PCB_MSG_DEFAULT, "Footprint %s contains multiple elements", name);
		return 1;
	}

	e = elementlist_first(&PCB_PASTEBUFFER->Data->Element);

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

/* ---------------------------------------------------------------------------
 * load PCB into buffer
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 */
pcb_bool pcb_buffer_load_layout(pcb_buffer_t *Buffer, const char *Filename, const char *fmt)
{
	pcb_board_t *newPCB = pcb_board_new();

	/* new data isn't added to the undo list */
	if (!ParsePCB(newPCB, Filename, fmt, CFR_invalid)) {
		/* clear data area and replace pointer */
		pcb_buffer_clear(Buffer);
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
void pcb_buffer_rotate(pcb_buffer_t *Buffer, pcb_uint8_t Number)
{
	/* rotate vias */
	VIA_LOOP(Buffer->Data);
	{
		r_delete_entry(Buffer->Data->via_tree, (pcb_box_t *) via);
		ROTATE_VIA_LOWLEVEL(via, Buffer->X, Buffer->Y, Number);
		pcb_pin_bbox(via);
		r_insert_entry(Buffer->Data->via_tree, (pcb_box_t *) via, 0);
	}
	END_LOOP;

	/* elements */
	PCB_ELEMENT_LOOP(Buffer->Data);
	{
		pcb_element_rotate90(Buffer->Data, element, Buffer->X, Buffer->Y, Number);
	}
	END_LOOP;

	/* all layer related objects */
	ALLLINE_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->line_tree, (pcb_box_t *) line);
		pcb_line_rotate90(line, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->line_tree, (pcb_box_t *) line, 0);
	}
	ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
		pcb_arc_rotate90(arc, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->arc_tree, (pcb_box_t *) arc, 0);
	}
	ENDALL_LOOP;
	ALLTEXT_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->text_tree, (pcb_box_t *) text);
		pcb_text_rotate90(text, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->text_tree, (pcb_box_t *) text, 0);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->polygon_tree, (pcb_box_t *) polygon);
		pcb_poly_rotate90(polygon, Buffer->X, Buffer->Y, Number);
		r_insert_entry(layer->polygon_tree, (pcb_box_t *) polygon, 0);
	}
	ENDALL_LOOP;

	/* finally the origin and the bounding box */
	ROTATE(Buffer->X, Buffer->Y, Buffer->X, Buffer->Y, Number);
	RotateBoxLowLevel(&Buffer->BoundingBox, Buffer->X, Buffer->Y, Number);
}

void pcb_buffer_free_rotate(pcb_buffer_t *Buffer, pcb_angle_t angle)
{
	double cosa, sina;

	cosa = cos(angle * M_PI / 180.0);
	sina = sin(angle * M_PI / 180.0);

#warning unravel: move these to the corresponding obj_*.[ch]
	/* rotate vias */
	VIA_LOOP(Buffer->Data);
	{
		r_delete_entry(Buffer->Data->via_tree, (pcb_box_t *) via);
		free_rotate(&via->X, &via->Y, Buffer->X, Buffer->Y, cosa, sina);
		pcb_pin_bbox(via);
		r_insert_entry(Buffer->Data->via_tree, (pcb_box_t *) via, 0);
	}
	END_LOOP;

	/* elements */
	PCB_ELEMENT_LOOP(Buffer->Data);
	{
		pcb_element_rotate(Buffer->Data, element, Buffer->X, Buffer->Y, cosa, sina, angle);
	}
	END_LOOP;

	/* all layer related objects */
	ALLLINE_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->line_tree, (pcb_box_t *) line);
		free_rotate(&line->Point1.X, &line->Point1.Y, Buffer->X, Buffer->Y, cosa, sina);
		free_rotate(&line->Point2.X, &line->Point2.Y, Buffer->X, Buffer->Y, cosa, sina);
		pcb_line_bbox(line);
		r_insert_entry(layer->line_tree, (pcb_box_t *) line, 0);
	}
	ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
		free_rotate(&arc->X, &arc->Y, Buffer->X, Buffer->Y, cosa, sina);
		arc->StartAngle = NormalizeAngle(arc->StartAngle + angle);
		r_insert_entry(layer->arc_tree, (pcb_box_t *) arc, 0);
	}
	ENDALL_LOOP;
	/* FIXME: rotate text */
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->polygon_tree, (pcb_box_t *) polygon);
		POLYGONPOINT_LOOP(polygon);
		{
			free_rotate(&point->X, &point->Y, Buffer->X, Buffer->Y, cosa, sina);
		}
		END_LOOP;
		pcb_poly_bbox(polygon);
		r_insert_entry(layer->polygon_tree, (pcb_box_t *) polygon, 0);
	}
	ENDALL_LOOP;

	pcb_set_buffer_bbox(Buffer);
}

/* ---------------------------------------------------------------------------
 * creates a new paste buffer
 */
pcb_data_t *pcb_buffer_new(void)
{
	pcb_data_t *data;
	data = (pcb_data_t *) calloc(1, sizeof(pcb_data_t));
	data->pcb = (pcb_board_t *) PCB;
	return data;
}


/* -------------------------------------------------------------------------- */

static const char freerotatebuffer_syntax[] = "pcb_buffer_free_rotate([Angle])";

static const char freerotatebuffer_help[] =
	"Rotates the current paste buffer contents by the specified angle.  The\n"
	"angle is given in degrees.  If no angle is given, the user is prompted\n" "for one.\n";

/* %start-doc actions FreeRotateBuffer

Rotates the contents of the pastebuffer by an arbitrary angle.  If no
angle is given, the user is prompted for one.

%end-doc */

int ActionFreeRotateBuffer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *angle_s;

	if (argc < 1)
		angle_s = gui->prompt_for("Enter Rotation (degrees, CCW):", "0");
	else
		angle_s = argv[0];

	pcb_notify_crosshair_change(pcb_false);
	pcb_buffer_free_rotate(PCB_PASTEBUFFER, strtod(angle_s, 0));
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}

/* ---------------------------------------------------------------------------
 * initializes the buffers by allocating memory
 */
void pcb_init_buffers(void)
{
	int i;

	for (i = 0; i < MAX_BUFFER; i++)
		Buffers[i].Data = pcb_buffer_new();
}

void pcb_uninit_buffers(void)
{
	int i;

	for (i = 0; i < MAX_BUFFER; i++) {
		pcb_buffer_clear(Buffers+i);
		free(Buffers[i].Data);
	}
}


void pcb_buffer_mirror(pcb_buffer_t *Buffer)
{
	int i;

	if (elementlist_length(&Buffer->Data->Element)) {
		pcb_message(PCB_MSG_DEFAULT, _("You can't mirror a buffer that has elements!\n"));
		return;
	}
	for (i = 0; i < max_copper_layer + 2; i++) {
		pcb_layer_t *layer = Buffer->Data->Layer + i;
		if (textlist_length(&layer->Text)) {
			pcb_message(PCB_MSG_DEFAULT, _("You can't mirror a buffer that has text!\n"));
			return;
		}
	}
	/* set buffer offset to 'mark' position */
	Buffer->X = PCB_SWAP_X(Buffer->X);
	Buffer->Y = PCB_SWAP_Y(Buffer->Y);
	VIA_LOOP(Buffer->Data);
	{
		via->X = PCB_SWAP_X(via->X);
		via->Y = PCB_SWAP_Y(via->Y);
	}
	END_LOOP;
	ALLLINE_LOOP(Buffer->Data);
	{
		line->Point1.X = PCB_SWAP_X(line->Point1.X);
		line->Point1.Y = PCB_SWAP_Y(line->Point1.Y);
		line->Point2.X = PCB_SWAP_X(line->Point2.X);
		line->Point2.Y = PCB_SWAP_Y(line->Point2.Y);
	}
	ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		arc->X = PCB_SWAP_X(arc->X);
		arc->Y = PCB_SWAP_Y(arc->Y);
		arc->StartAngle = SWAP_ANGLE(arc->StartAngle);
		arc->Delta = SWAP_DELTA(arc->Delta);
		pcb_arc_bbox(arc);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		POLYGONPOINT_LOOP(polygon);
		{
			point->X = PCB_SWAP_X(point->X);
			point->Y = PCB_SWAP_Y(point->Y);
		}
		END_LOOP;
		pcb_poly_bbox(polygon);
	}
	ENDALL_LOOP;
	pcb_set_buffer_bbox(Buffer);
}


/* ---------------------------------------------------------------------------
 * flip components/tracks from one side to the other
 */
void pcb_buffer_swap(pcb_buffer_t *Buffer)
{
	int j, k;
	pcb_cardinal_t sgroup, cgroup;
	pcb_layer_t swap;

	PCB_ELEMENT_LOOP(Buffer->Data);
	{
		r_delete_element(Buffer->Data, element);
		pcb_element_mirror(Buffer->Data, element, 0);
	}
	END_LOOP;
	/* set buffer offset to 'mark' position */
	Buffer->X = PCB_SWAP_X(Buffer->X);
	Buffer->Y = PCB_SWAP_Y(Buffer->Y);
	VIA_LOOP(Buffer->Data);
	{
		r_delete_entry(Buffer->Data->via_tree, (pcb_box_t *) via);
		via->X = PCB_SWAP_X(via->X);
		via->Y = PCB_SWAP_Y(via->Y);
		pcb_pin_bbox(via);
		r_insert_entry(Buffer->Data->via_tree, (pcb_box_t *) via, 0);
	}
	END_LOOP;
	ALLLINE_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->line_tree, (pcb_box_t *) line);
		line->Point1.X = PCB_SWAP_X(line->Point1.X);
		line->Point1.Y = PCB_SWAP_Y(line->Point1.Y);
		line->Point2.X = PCB_SWAP_X(line->Point2.X);
		line->Point2.Y = PCB_SWAP_Y(line->Point2.Y);
		pcb_line_bbox(line);
		r_insert_entry(layer->line_tree, (pcb_box_t *) line, 0);
	}
	ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->arc_tree, (pcb_box_t *) arc);
		arc->X = PCB_SWAP_X(arc->X);
		arc->Y = PCB_SWAP_Y(arc->Y);
		arc->StartAngle = SWAP_ANGLE(arc->StartAngle);
		arc->Delta = SWAP_DELTA(arc->Delta);
		pcb_arc_bbox(arc);
		r_insert_entry(layer->arc_tree, (pcb_box_t *) arc, 0);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->polygon_tree, (pcb_box_t *) polygon);
		POLYGONPOINT_LOOP(polygon);
		{
			point->X = PCB_SWAP_X(point->X);
			point->Y = PCB_SWAP_Y(point->Y);
		}
		END_LOOP;
		pcb_poly_bbox(polygon);
		r_insert_entry(layer->polygon_tree, (pcb_box_t *) polygon, 0);
		/* hmmm, how to handle clip */
	}
	ENDALL_LOOP;
	ALLTEXT_LOOP(Buffer->Data);
	{
		r_delete_entry(layer->text_tree, (pcb_box_t *) text);
		text->X = PCB_SWAP_X(text->X);
		text->Y = PCB_SWAP_Y(text->Y);
		PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
		pcb_text_bbox(&PCB->Font, text);
		r_insert_entry(layer->text_tree, (pcb_box_t *) text, 0);
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
				t1 = PCB_FLAG_THERM_TEST(snumber, pin);
				t2 = PCB_FLAG_THERM_TEST(cnumber, pin);
				PCB_FLAG_THERM_ASSIGN(snumber, t2, pin);
				PCB_FLAG_THERM_ASSIGN(cnumber, t1, pin);
			}
			ENDALL_LOOP;
			VIA_LOOP(Buffer->Data);
			{
				t1 = PCB_FLAG_THERM_TEST(snumber, via);
				t2 = PCB_FLAG_THERM_TEST(cnumber, via);
				PCB_FLAG_THERM_ASSIGN(snumber, t2, via);
				PCB_FLAG_THERM_ASSIGN(cnumber, t1, via);
			}
			END_LOOP;
		}
	}
	pcb_set_buffer_bbox(Buffer);
}

void pcb_swap_buffers(void)
{
	int i;

	for (i = 0; i < MAX_BUFFER; i++)
		pcb_buffer_swap(&Buffers[i]);
	SetCrosshairRangeToBuffer();
}

/* ----------------------------------------------------------------------
 * moves the passed object to the passed buffer and removes it
 * from its original place
 */
void *pcb_move_obj_to_buffer(pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
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
void *pcb_copy_obj_to_buffer(pcb_data_t *Destination, pcb_data_t *Src, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.buffer.pcb = PCB;
	ctx.buffer.dst = Destination;
	ctx.buffer.src = Src;
	return (ObjectOperation(&AddBufferFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}

/* ---------------------------------------------------------------------------
 * pastes the contents of the buffer to the layout. Only visible objects
 * are handled by the routine.
 */
pcb_bool pcb_buffer_copy_to_layout(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_cardinal_t i;
	pcb_bool changed = pcb_false;
	pcb_opctx_t ctx;

#ifdef DEBUG
	printf("Entering CopyPastebufferToLayout.....\n");
#endif

	/* set movement vector */
	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = X - PCB_PASTEBUFFER->X;
	ctx.copy.DeltaY = Y - PCB_PASTEBUFFER->Y;

	/* paste all layers */
	for (i = 0; i < max_copper_layer + 2; i++) {
		pcb_layer_t *sourcelayer = &PCB_PASTEBUFFER->Data->Layer[i], *destlayer = LAYER_PTR(i);

		if (destlayer->On) {
			changed = changed || (!LAYER_IS_PCB_EMPTY(sourcelayer));
			LINE_LOOP(sourcelayer);
			{
				CopyLine(&ctx, destlayer, line);
			}
			END_LOOP;
			PCB_ARC_LOOP(sourcelayer);
			{
				CopyArc(&ctx, destlayer, arc);
			}
			END_LOOP;
			TEXT_LOOP(sourcelayer);
			{
				CopyText(&ctx, destlayer, text);
			}
			END_LOOP;
			POLYGON_LOOP(sourcelayer);
			{
				CopyPolygon(&ctx, destlayer, polygon);
			}
			END_LOOP;
		}
	}

	/* paste elements */
	if (PCB->PinOn && PCB->ElementOn) {
		PCB_ELEMENT_LOOP(PCB_PASTEBUFFER->Data);
		{
#ifdef DEBUG
			printf("In CopyPastebufferToLayout, pasting element %s\n", element->Name[1].TextString);
#endif
			if (PCB_FRONT(element) || PCB->InvisibleObjectsOn) {
				CopyElement(&ctx, element);
				changed = pcb_true;
			}
		}
		END_LOOP;
	}

	/* finally the vias */
	if (PCB->ViaOn) {
		changed |= (pinlist_length(&(PCB_PASTEBUFFER->Data->Via)) != 0);
		VIA_LOOP(PCB_PASTEBUFFER->Data);
		{
			CopyVia(&ctx, via);
		}
		END_LOOP;
	}

	if (changed) {
		pcb_draw();
		IncrementUndoSerialNumber();
	}

#ifdef DEBUG
	printf("  .... Leaving CopyPastebufferToLayout.\n");
#endif

	return (changed);
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

static int ActionPasteBuffer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = argc ? argv[0] : "";
	const char *sbufnum = argc > 1 ? argv[1] : "";
	const char *fmt = argc > 2 ? argv[2] : NULL;
	const char *forces = argc > 3 ? argv[3] : NULL;
	const char *name;
	static char *default_file = NULL;
	pcb_bool free_name = pcb_false;
	int force = (forces != NULL) && ((*forces == '1') || (*forces == 'y') || (*forces == 'Y'));

	pcb_notify_crosshair_change(pcb_false);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
			/* clear contents of paste buffer */
		case F_Clear:
			pcb_buffer_clear(PCB_PASTEBUFFER);
			break;

			/* copies objects to paste buffer */
		case F_AddSelected:
			pcb_buffer_add_selected(PCB_PASTEBUFFER, 0, 0, pcb_false);
			break;

			/* converts buffer contents into an element */
		case F_Convert:
			ConvertBufferToElement(PCB_PASTEBUFFER);
			break;

			/* break up element for editing */
		case F_Restore:
			SmashBufferElement(PCB_PASTEBUFFER);
			break;

			/* Mirror buffer */
		case F_Mirror:
			pcb_buffer_mirror(PCB_PASTEBUFFER);
			break;

		case F_Rotate:
			if (sbufnum) {
				pcb_buffer_rotate(PCB_PASTEBUFFER, (pcb_uint8_t) atoi(sbufnum));
				SetCrosshairRangeToBuffer();
			}
			break;

		case F_Save:
			if (elementlist_length(&PCB_PASTEBUFFER->Data->Element) == 0) {
				pcb_message(PCB_MSG_DEFAULT, _("Buffer has no elements!\n"));
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
				static pcb_coord_t oldx = 0, oldy = 0;
				pcb_coord_t x, y;
				pcb_bool absolute;

				if (argc == 1) {
					x = y = 0;
				}
				else if (argc == 3 || argc == 4) {
					x = pcb_get_value(PCB_ACTION_ARG(1), PCB_ACTION_ARG(3), &absolute, NULL);
					if (!absolute)
						x += oldx;
					y = pcb_get_value(PCB_ACTION_ARG(2), PCB_ACTION_ARG(3), &absolute, NULL);
					if (!absolute)
						y += oldy;
				}
				else {
					pcb_notify_crosshair_change(pcb_true);
					PCB_AFAIL(pastebuffer);
				}

				oldx = x;
				oldy = y;
				if (pcb_buffer_copy_to_layout(x, y))
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

	pcb_notify_crosshair_change(pcb_true);
	return 0;
}

/* --------------------------------------------------------------------------- */

pcb_hid_action_t buffer_action_list[] = {
	{"FreeRotateBuffer", 0, ActionFreeRotateBuffer,
	 freerotatebuffer_help, freerotatebuffer_syntax}
	,
	{"LoadFootprint", 0, pcb_load_footprint,
	 loadfootprint_help, loadfootprint_syntax}
	,
	{"PasteBuffer", 0, ActionPasteBuffer,
	 pastebuffer_help, pastebuffer_syntax}
};

PCB_REGISTER_ACTIONS(buffer_action_list, NULL)
