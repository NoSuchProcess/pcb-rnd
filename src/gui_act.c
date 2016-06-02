/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf_core.h"
#include "global.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "funchash_core.h"

#include "draw.h"
#include "search.h"
#include "crosshair.h"
#include "find.h"
#include "set.h"
#include "misc.h"
#include "stub_stroke.h"
#include "hid_actions.h"
#include "hid_init.h"

/* --------------------------------------------------------------------------- */

static const char display_syntax[] =
	"Display(NameOnPCB|Description|Value)\n"
	"Display(Grid|Redraw)\n"
	"Display(CycleClip|CycleCrosshair|ToggleAllDirections|ToggleStartDirection)\n"
	"Display(ToggleGrid|ToggleRubberBandMode|ToggleUniqueNames)\n"
	"Display(ToggleMask|ToggleName|ToggleClearLine|ToggleFullPoly)\n"
	"Display(ToggleSnapPin|ToggleSnapOffGridLine|ToggleHighlightOnPoint)\n"
	"Display(ToggleThindraw|ToggleThindrawPoly|ToggleOrthoMove|ToggleLocalRef)\n"
	"Display(ToggleCheckPlanes|ToggleShowDRC|ToggleAutoDRC)\n"
	"Display(ToggleLiveRoute|LockNames|OnlyNames)\n" "Display(Pinout|PinOrPadName)";

static const char display_help[] = "Several display-related actions.";

/* %start-doc actions Display

@table @code

@item NameOnPCB
@item Description
@item Value
Specify whether all elements show their name, description, or value.

@item Redraw
Redraw the whole board.

@item ToggleAllDirections
When clear, lines can be drawn at any angle.  When set, lines are
restricted to multiples of 45 degrees and requested lines may be
broken up according to the clip setting.

@item CycleClip
Changes the way lines are restricted to 45 degree increments.  The
various settings are: straight only, orthogonal then angled, and angled
then orthogonal.  If AllDirections is set, this action disables it.

@item CycleCrosshair
Changes crosshair drawing.  Crosshair may accept form of 4-ray,
8-ray and 12-ray cross.

@item ToggleRubberBandMode
If set, moving an object moves all the lines attached to it too.

@item ToggleStartDirection
If set, each time you set a point in a line, the Clip toggles between
orth-angle and angle-ortho.

@item ToggleUniqueNames
If set, you will not be permitted to change the name of an element to
match that of another element.

@item ToggleSnapPin
If set, pin centers and pad end points are treated as additional grid
points that the cursor can snap to.

@item ToggleSnapOffGridLine
If set, snap at some sensible point along a line.

@item ToggleHighlightOnPoint
If set, highlights lines and arcs when the crosshair is on one of their
two (end) points.

@item ToggleLocalRef
If set, the mark is automatically set to the beginning of any move, so
you can see the relative distance you've moved.

@item ToggleThindraw
If set, objects on the screen are drawn as outlines (lines are drawn
as center-lines).  This lets you see line endpoints hidden under pins,
for example.

@item ToggleThindrawPoly
If set, polygons on the screen are drawn as outlines.

@item ToggleShowDRC
If set, pending objects (i.e. lines you're in the process of drawing)
will be drawn with an outline showing how far away from other copper
you need to be.

@item ToggleLiveRoute
If set, the progress of the autorouter will be visible on the screen.

@item ToggleAutoDRC
If set, you will not be permitted to make connections which violate
the current DRC and netlist settings.

@item ToggleCheckPlanes
If set, lines and arcs aren't drawn, which usually leaves just the
polygons.  If you also disable all but the layer you're interested in,
this allows you to check for isolated regions.

@item ToggleOrthoMove
If set, the crosshair is only allowed to move orthogonally from its
previous position.  I.e. you can move an element or line up, down,
left, or right, but not up+left or down+right.

@item ToggleName
Selects whether the pinouts show the pin names or the pin numbers.

@item ToggleLockNames
If set, text will ignore left mouse clicks and actions that work on
objects under the mouse. You can still select text with a lasso (left
mouse drag) and perform actions on the selection.

@item ToggleOnlyNames
If set, only text will be sensitive for mouse clicks and actions that
work on objects under the mouse. You can still select other objects
with a lasso (left mouse drag) and perform actions on the selection.

@item ToggleMask
Turns the solder mask on or off.

@item ToggleClearLine
When set, the clear-line flag causes new lines and arcs to have their
``clear polygons'' flag set, so they won't be electrically connected
to any polygons they overlap.

@item ToggleFullPoly
When set, the full-poly flag causes new polygons to have their
``full polygon'' flag set, so all parts of them will be displayed
instead of only the biggest one.

@item ToggleGrid
Resets the origin of the current grid to be wherever the mouse pointer
is (not where the crosshair currently is).  If you provide two numbers
after this, the origin is set to that coordinate.

@item Grid
Toggles whether the grid is displayed or not.

@item Pinout
Causes the pinout of the element indicated by the cursor to be
displayed, usually in a separate window.

@item PinOrPadName
Toggles whether the names of pins, pads, or (yes) vias will be
displayed.  If the cursor is over an element, all of its pins and pads
are affected.

@end table

%end-doc */

static enum crosshair_shape CrosshairShapeIncrement(enum crosshair_shape shape)
{
	switch (shape) {
	case Basic_Crosshair_Shape:
		shape = Union_Jack_Crosshair_Shape;
		break;
	case Union_Jack_Crosshair_Shape:
		shape = Dozen_Crosshair_Shape;
		break;
	case Dozen_Crosshair_Shape:
		shape = Crosshair_Shapes_Number;
		break;
	case Crosshair_Shapes_Number:
		shape = Basic_Crosshair_Shape;
		break;
	}
	return shape;
}

static int ActionDisplay(int argc, char **argv, Coord childX, Coord childY)
{
	char *function, *str_dir;
	int id;
	int err = 0;

	function = ACTION_ARG(0);
	str_dir = ACTION_ARG(1);

	if (function && (!str_dir || !*str_dir)) {
		switch (id = funchash_get(function, NULL)) {

			/* redraw layout */
		case F_ClearAndRedraw:
		case F_Redraw:
			Redraw();
			break;

			/* change the displayed name of elements */
		case F_Value:
		case F_NameOnPCB:
		case F_Description:
			ELEMENT_LOOP(PCB->Data);
			{
				EraseElementName(element);
			}
			END_LOOP;
			CLEAR_FLAG(DESCRIPTIONFLAG | NAMEONPCBFLAG, PCB);
			switch (id) {
			case F_Value:
				break;
			case F_NameOnPCB:
				SET_FLAG(NAMEONPCBFLAG, PCB);
				break;
			case F_Description:
				SET_FLAG(DESCRIPTIONFLAG, PCB);
				break;
			}
			ELEMENT_LOOP(PCB->Data);
			{
				DrawElementName(element);
			}
			END_LOOP;
			Draw();
			break;

			/* toggle line-adjust flag */
		case F_ToggleAllDirections:
			TOGGLE_FLAG(ALLDIRECTIONFLAG, PCB);
			AdjustAttachedObjects();
			break;

		case F_CycleClip:
			notify_crosshair_change(false);
			if TEST_FLAG
				(ALLDIRECTIONFLAG, PCB) {
				TOGGLE_FLAG(ALLDIRECTIONFLAG, PCB);
				PCB->Clipping = 0;
				}
			else
				PCB->Clipping = (PCB->Clipping + 1) % 3;
			AdjustAttachedObjects();
			notify_crosshair_change(true);
			break;

		case F_CycleCrosshair:
			notify_crosshair_change(false);
			Crosshair.shape = CrosshairShapeIncrement(Crosshair.shape);
			if (Crosshair_Shapes_Number == Crosshair.shape)
				Crosshair.shape = Basic_Crosshair_Shape;
			notify_crosshair_change(true);
			break;

		case F_ToggleRubberBandMode:
			notify_crosshair_change(false);
			TOGGLE_FLAG(RUBBERBANDFLAG, PCB);
			notify_crosshair_change(true);
			break;

		case F_ToggleStartDirection:
			notify_crosshair_change(false);
			TOGGLE_FLAG(SWAPSTARTDIRFLAG, PCB);
			notify_crosshair_change(true);
			break;

		case F_ToggleUniqueNames:
			TOGGLE_FLAG(UNIQUENAMEFLAG, PCB);
			break;

		case F_ToggleSnapPin:
			notify_crosshair_change(false);
			TOGGLE_FLAG(SNAPPINFLAG, PCB);
			notify_crosshair_change(true);
			break;

		case F_ToggleSnapOffGridLine:
			notify_crosshair_change(false);
			TOGGLE_FLAG(SNAPOFFGRIDLINEFLAG, PCB);
			notify_crosshair_change(true);
			break;

		case F_ToggleHighlightOnPoint:
			notify_crosshair_change(false);
			TOGGLE_FLAG(HIGHLIGHTONPOINTFLAG, PCB);
			notify_crosshair_change(true);
			break;

		case F_ToggleLocalRef:
			TOGGLE_FLAG(LOCALREFFLAG, PCB);
			break;

		case F_ToggleThindraw:
			TOGGLE_FLAG(THINDRAWFLAG, PCB);
			Redraw();
			break;

		case F_ToggleThindrawPoly:
			TOGGLE_FLAG(THINDRAWPOLYFLAG, PCB);
			Redraw();
			break;

		case F_ToggleLockNames:
			TOGGLE_FLAG(LOCKNAMESFLAG, PCB);
			CLEAR_FLAG(ONLYNAMESFLAG, PCB);
			break;

		case F_ToggleOnlyNames:
			TOGGLE_FLAG(ONLYNAMESFLAG, PCB);
			CLEAR_FLAG(LOCKNAMESFLAG, PCB);
			break;

		case F_ToggleHideNames:
			TOGGLE_FLAG(HIDENAMESFLAG, PCB);
			Redraw();
			break;

		case F_ToggleStroke:
			TOGGLE_FLAG(ENABLESTROKEFLAG, PCB);
			conf_core.editor.enable_stroke = TEST_FLAG(ENABLESTROKEFLAG, PCB);
			break;

		case F_ToggleShowDRC:
			TOGGLE_FLAG(SHOWDRCFLAG, PCB);
			break;

		case F_ToggleLiveRoute:
			TOGGLE_FLAG(LIVEROUTEFLAG, PCB);
			break;

		case F_ToggleAutoDRC:
			notify_crosshair_change(false);
			TOGGLE_FLAG(AUTODRCFLAG, PCB);
			if (TEST_FLAG(AUTODRCFLAG, PCB) && conf_core.editor.mode == LINE_MODE) {
				if (ResetConnections(true)) {
					IncrementUndoSerialNumber();
					Draw();
				}
				if (Crosshair.AttachedLine.State != STATE_FIRST)
					LookupConnection(Crosshair.AttachedLine.Point1.X, Crosshair.AttachedLine.Point1.Y, true, 1, FOUNDFLAG);
			}
			notify_crosshair_change(true);
			break;

		case F_ToggleCheckPlanes:
			TOGGLE_FLAG(CHECKPLANESFLAG, PCB);
			Redraw();
			break;

		case F_ToggleOrthoMove:
			TOGGLE_FLAG(ORTHOMOVEFLAG, PCB);
			break;

		case F_ToggleName:
			TOGGLE_FLAG(SHOWNUMBERFLAG, PCB);
			Redraw();
			break;

		case F_ToggleMask:
			TOGGLE_FLAG(SHOWMASKFLAG, PCB);
			Redraw();
			break;

		case F_ToggleClearLine:
			TOGGLE_FLAG(CLEARNEWFLAG, PCB);
			break;

		case F_ToggleFullPoly:
			TOGGLE_FLAG(NEWFULLPOLYFLAG, PCB);
			break;

			/* shift grid alignment */
		case F_ToggleGrid:
			{
				Coord oldGrid = PCB->Grid;

				PCB->Grid = 1;
				if (MoveCrosshairAbsolute(Crosshair.X, Crosshair.Y))
					notify_crosshair_change(true);	/* first notify was in MoveCrosshairAbs */
				SetGrid(oldGrid, true);
			}
			break;

			/* toggle displaying of the grid */
		case F_Grid:
			conf_core.editor.draw_grid = !conf_core.editor.draw_grid;
			Redraw();
			break;

			/* display the pinout of an element */
		case F_Pinout:
			{
				ElementTypePtr element;
				void *ptrtmp;
				Coord x, y;

				gui->get_coords(_("Click on an element"), &x, &y);
				if ((SearchScreen(x, y, ELEMENT_TYPE, &ptrtmp, &ptrtmp, &ptrtmp)) != NO_TYPE) {
					element = (ElementTypePtr) ptrtmp;
					gui->show_item(element);
				}
				break;
			}

			/* toggle displaying of pin/pad/via names */
		case F_PinOrPadName:
			{
				void *ptr1, *ptr2, *ptr3;
				Coord x, y;
				gui->get_coords(_("Click on an element"), &x, &y);

				switch (SearchScreen(x, y,
														 ELEMENT_TYPE | PIN_TYPE | PAD_TYPE |
														 VIA_TYPE, (void **) &ptr1, (void **) &ptr2, (void **) &ptr3)) {
				case ELEMENT_TYPE:
					PIN_LOOP((ElementTypePtr) ptr1);
					{
						if (TEST_FLAG(DISPLAYNAMEFLAG, pin))
							ErasePinName(pin);
						else
							DrawPinName(pin);
						AddObjectToFlagUndoList(PIN_TYPE, ptr1, pin, pin);
						TOGGLE_FLAG(DISPLAYNAMEFLAG, pin);
					}
					END_LOOP;
					PAD_LOOP((ElementTypePtr) ptr1);
					{
						if (TEST_FLAG(DISPLAYNAMEFLAG, pad))
							ErasePadName(pad);
						else
							DrawPadName(pad);
						AddObjectToFlagUndoList(PAD_TYPE, ptr1, pad, pad);
						TOGGLE_FLAG(DISPLAYNAMEFLAG, pad);
					}
					END_LOOP;
					SetChangedFlag(true);
					IncrementUndoSerialNumber();
					Draw();
					break;

				case PIN_TYPE:
					if (TEST_FLAG(DISPLAYNAMEFLAG, (PinTypePtr) ptr2))
						ErasePinName((PinTypePtr) ptr2);
					else
						DrawPinName((PinTypePtr) ptr2);
					AddObjectToFlagUndoList(PIN_TYPE, ptr1, ptr2, ptr3);
					TOGGLE_FLAG(DISPLAYNAMEFLAG, (PinTypePtr) ptr2);
					SetChangedFlag(true);
					IncrementUndoSerialNumber();
					Draw();
					break;

				case PAD_TYPE:
					if (TEST_FLAG(DISPLAYNAMEFLAG, (PadTypePtr) ptr2))
						ErasePadName((PadTypePtr) ptr2);
					else
						DrawPadName((PadTypePtr) ptr2);
					AddObjectToFlagUndoList(PAD_TYPE, ptr1, ptr2, ptr3);
					TOGGLE_FLAG(DISPLAYNAMEFLAG, (PadTypePtr) ptr2);
					SetChangedFlag(true);
					IncrementUndoSerialNumber();
					Draw();
					break;
				case VIA_TYPE:
					if (TEST_FLAG(DISPLAYNAMEFLAG, (PinTypePtr) ptr2))
						EraseViaName((PinTypePtr) ptr2);
					else
						DrawViaName((PinTypePtr) ptr2);
					AddObjectToFlagUndoList(VIA_TYPE, ptr1, ptr2, ptr3);
					TOGGLE_FLAG(DISPLAYNAMEFLAG, (PinTypePtr) ptr2);
					SetChangedFlag(true);
					IncrementUndoSerialNumber();
					Draw();
					break;
				}
				break;
			}
		default:
			err = 1;
		}
	}
	else if (function && str_dir) {
		switch (funchash_get(function, NULL)) {
		case F_ToggleGrid:
			if (argc > 2) {
				PCB->GridOffsetX = GetValue(argv[1], NULL, NULL);
				PCB->GridOffsetY = GetValue(argv[2], NULL, NULL);
				if (conf_core.editor.draw_grid)
					Redraw();
			}
			break;

		default:
			err = 1;
			break;
		}
	}

	if (!err)
		return 0;

	AFAIL(display);
}

/* --------------------------------------------------------------------------- */

static const char mode_syntax[] =
	"Mode(Arc|Arrow|Copy|InsertPoint|Line|Lock|Move|None|PasteBuffer)\n"
	"Mode(Polygon|Rectangle|Remove|Rotate|Text|Thermal|Via)\n" "Mode(Notify|Release|Cancel|Stroke)\n" "Mode(Save|Restore)";

static const char mode_help[] = "Change or use the tool mode.";

/* %start-doc actions Mode

@table @code

@item Arc
@itemx Arrow
@itemx Copy
@itemx InsertPoint
@itemx Line
@itemx Lock
@itemx Move
@itemx None
@itemx PasteBuffer
@itemx Polygon
@itemx Rectangle
@itemx Remove
@itemx Rotate
@itemx Text
@itemx Thermal
@itemx Via
Select the indicated tool.

@item Notify
Called when you press the mouse button, or move the mouse.

@item Release
Called when you release the mouse button.

@item Cancel
Cancels any pending tool activity, allowing you to restart elsewhere.
For example, this allows you to start a new line rather than attach a
line to the previous line.

@item Escape
Similar to Cancel but calling this action a second time will return
to the Arrow tool.

@item Stroke
If your @code{pcb} was built with libstroke, this invokes the stroke
input method.  If not, this will restart a drawing mode if you were
drawing, else it will select objects.

@item Save
Remembers the current tool.

@item Restore
Restores the tool to the last saved tool.

@end table

%end-doc */

static int ActionMode(int argc, char **argv, Coord x, Coord y)
{
	char *function = ACTION_ARG(0);

	if (function) {
		Note.X = Crosshair.X;
		Note.Y = Crosshair.Y;
		notify_crosshair_change(false);
		switch (funchash_get(function, NULL)) {
		case F_Arc:
			SetMode(ARC_MODE);
			break;
		case F_Arrow:
			SetMode(ARROW_MODE);
			break;
		case F_Copy:
			SetMode(COPY_MODE);
			break;
		case F_InsertPoint:
			SetMode(INSERTPOINT_MODE);
			break;
		case F_Line:
			SetMode(LINE_MODE);
			break;
		case F_Lock:
			SetMode(LOCK_MODE);
			break;
		case F_Move:
			SetMode(MOVE_MODE);
			break;
		case F_None:
			SetMode(NO_MODE);
			break;
		case F_Cancel:
			{
				int saved_mode = conf_core.editor.mode;
				SetMode(NO_MODE);
				SetMode(saved_mode);
			}
			break;
		case F_Escape:
			{
				switch (conf_core.editor.mode) {
				case VIA_MODE:
				case PASTEBUFFER_MODE:
				case TEXT_MODE:
				case ROTATE_MODE:
				case REMOVE_MODE:
				case MOVE_MODE:
				case COPY_MODE:
				case INSERTPOINT_MODE:
				case RUBBERBANDMOVE_MODE:
				case THERMAL_MODE:
				case LOCK_MODE:
					SetMode(NO_MODE);
					SetMode(ARROW_MODE);
					break;

				case LINE_MODE:
					if (Crosshair.AttachedLine.State == STATE_FIRST)
						SetMode(ARROW_MODE);
					else {
						SetMode(NO_MODE);
						SetMode(LINE_MODE);
					}
					break;

				case RECTANGLE_MODE:
					if (Crosshair.AttachedBox.State == STATE_FIRST)
						SetMode(ARROW_MODE);
					else {
						SetMode(NO_MODE);
						SetMode(RECTANGLE_MODE);
					}
					break;

				case POLYGON_MODE:
					if (Crosshair.AttachedLine.State == STATE_FIRST)
						SetMode(ARROW_MODE);
					else {
						SetMode(NO_MODE);
						SetMode(POLYGON_MODE);
					}
					break;

				case POLYGONHOLE_MODE:
					if (Crosshair.AttachedLine.State == STATE_FIRST)
						SetMode(ARROW_MODE);
					else {
						SetMode(NO_MODE);
						SetMode(POLYGONHOLE_MODE);
					}
					break;

				case ARC_MODE:
					if (Crosshair.AttachedBox.State == STATE_FIRST)
						SetMode(ARROW_MODE);
					else {
						SetMode(NO_MODE);
						SetMode(ARC_MODE);
					}
					break;

				case ARROW_MODE:
					break;

				default:
					break;
				}
			}
			break;

		case F_Notify:
			NotifyMode();
			break;
		case F_PasteBuffer:
			SetMode(PASTEBUFFER_MODE);
			break;
		case F_Polygon:
			SetMode(POLYGON_MODE);
			break;
		case F_PolygonHole:
			SetMode(POLYGONHOLE_MODE);
			break;
		case F_Release:
			if ((mid_stroke) && (conf_core.editor.enable_stroke))
				stub_stroke_finish();
			else
				ReleaseMode();
			break;
		case F_Remove:
			SetMode(REMOVE_MODE);
			break;
		case F_Rectangle:
			SetMode(RECTANGLE_MODE);
			break;
		case F_Rotate:
			SetMode(ROTATE_MODE);
			break;
		case F_Stroke:
			if (conf_core.editor.enable_stroke) {
				stub_stroke_start();
				break;
			}
			/* Handle middle mouse button restarts of drawing mode.  If not in
			   |  a drawing mode, middle mouse button will select objects.
			 */
			if (conf_core.editor.mode == LINE_MODE && Crosshair.AttachedLine.State != STATE_FIRST) {
				SetMode(LINE_MODE);
			}
			else if (conf_core.editor.mode == ARC_MODE && Crosshair.AttachedBox.State != STATE_FIRST)
				SetMode(ARC_MODE);
			else if (conf_core.editor.mode == RECTANGLE_MODE && Crosshair.AttachedBox.State != STATE_FIRST)
				SetMode(RECTANGLE_MODE);
			else if (conf_core.editor.mode == POLYGON_MODE && Crosshair.AttachedLine.State != STATE_FIRST)
				SetMode(POLYGON_MODE);
			else {
				SaveMode();
				saved_mode = true;
				SetMode(ARROW_MODE);
				NotifyMode();
			}
			break;
		case F_Text:
			SetMode(TEXT_MODE);
			break;
		case F_Thermal:
			SetMode(THERMAL_MODE);
			break;
		case F_Via:
			SetMode(VIA_MODE);
			break;

		case F_Restore:						/* restore the last saved mode */
			RestoreMode();
			break;

		case F_Save:								/* save currently selected mode */
			SaveMode();
			break;
		}
		notify_crosshair_change(true);
		return 0;
	}

	AFAIL(mode);
}

/* ---------------------------------------------------------------- */
static const char cycledrag_syntax[] = "CycleDrag()\n";

static const char cycledrag_help[] = "Cycle through which object is being dragged";

#define close_enough(a, b) ((((a)-(b)) > 0) ? ((a)-(b) < (SLOP * pixel_slop)) : ((a)-(b) > -(SLOP * pixel_slop)))
static int ActionCycleDrag(int argc, char **argv, Coord x, Coord y)
{
	void *ptr1, *ptr2, *ptr3;
	int over = 0;

	if (Crosshair.drags == NULL)
		return 0;

	do {
		Crosshair.drags_current++;
		if (Crosshair.drags_current >= Crosshair.drags_len) {
			Crosshair.drags_current = 0;
			over++;
		}

		if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], LINE_TYPE) != NO_TYPE) {
			/* line has two endpoints, check which one is close to the original x;y */
			LineType *l = ptr2;
			long int d1, d2;
			if (close_enough(Note.X, l->Point1.X) && close_enough(Note.Y, l->Point1.Y)) {
				Crosshair.AttachedObject.Type = LINEPOINT_TYPE;
				Crosshair.AttachedObject.Ptr1 = ptr1;
				Crosshair.AttachedObject.Ptr2 = ptr2;
				Crosshair.AttachedObject.Ptr3 = &l->Point1;
				return 0;
			}
			if (close_enough(Note.X, l->Point2.X) && close_enough(Note.Y, l->Point2.Y)) {
				Crosshair.AttachedObject.Type = LINEPOINT_TYPE;
				Crosshair.AttachedObject.Ptr1 = ptr1;
				Crosshair.AttachedObject.Ptr2 = ptr2;
				Crosshair.AttachedObject.Ptr3 = &l->Point2;
				return 0;
			}
		}
		else if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], VIA_TYPE) != NO_TYPE) {
			Crosshair.AttachedObject.Type = VIA_TYPE;
			Crosshair.AttachedObject.Ptr1 = ptr1;
			Crosshair.AttachedObject.Ptr2 = ptr2;
			Crosshair.AttachedObject.Ptr3 = ptr3;
			return 0;
		}
		else if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], PAD_TYPE) != NO_TYPE) {
			Crosshair.AttachedObject.Type = ELEMENT_TYPE;
			Crosshair.AttachedObject.Ptr1 = ptr1;
			Crosshair.AttachedObject.Ptr2 = ptr1;
			Crosshair.AttachedObject.Ptr3 = ptr1;
			return 0;
		}
		else if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], ARC_TYPE) != NO_TYPE) {
			Crosshair.AttachedObject.Type = ARC_TYPE;
			Crosshair.AttachedObject.Ptr1 = ptr1;
			Crosshair.AttachedObject.Ptr2 = ptr2;
			Crosshair.AttachedObject.Ptr3 = ptr3;
			return 0;
		}

	} while (over <= 1);

	return -1;
}

#undef close_enough

/* -------------------------------------------------------------------------- */

static const char message_syntax[] = "Message(message)";

static const char message_help[] = "Writes a message to the log window.";

/* %start-doc actions Message

This action displays a message to the log window.  This action is primarily
provided for use by other programs which may interface with PCB.  If
multiple arguments are given, each one is sent to the log window
followed by a newline.

%end-doc */

static int ActionMessage(int argc, char **argv, Coord x, Coord y)
{
	int i;

	if (argc < 1)
		AFAIL(message);

	for (i = 0; i < argc; i++) {
		Message(argv[i]);
		Message("\n");
	}

	return 0;
}

/* --------------------------------------------------------------------------- */

static const char togglehidename_syntax[] = "ToggleHideName(Object|SelectedElements)";

static const char togglehidename_help[] = "Toggles the visibility of element names.";

/* %start-doc actions ToggleHideName

If names are hidden you won't see them on the screen and they will not
appear on the silk layer when you print the layout.

%end-doc */

static int ActionToggleHideName(int argc, char **argv, Coord x, Coord y)
{
	char *function = ACTION_ARG(0);
	if (function && PCB->ElementOn) {
		switch (funchash_get(function, NULL)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = SearchScreen(x, y, ELEMENT_TYPE, &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
					AddObjectToFlagUndoList(type, ptr1, ptr2, ptr3);
					EraseElementName((ElementTypePtr) ptr2);
					TOGGLE_FLAG(HIDENAMEFLAG, (ElementTypePtr) ptr2);
					DrawElementName((ElementTypePtr) ptr2);
					Draw();
					IncrementUndoSerialNumber();
				}
				break;
			}
		case F_SelectedElements:
		case F_Selected:
			{
				bool changed = false;
				ELEMENT_LOOP(PCB->Data);
				{
					if ((TEST_FLAG(SELECTEDFLAG, element) || TEST_FLAG(SELECTEDFLAG, &NAMEONPCB_TEXT(element)))
							&& (FRONT(element) || PCB->InvisibleObjectsOn)) {
						AddObjectToFlagUndoList(ELEMENT_TYPE, element, element, element);
						EraseElementName(element);
						TOGGLE_FLAG(HIDENAMEFLAG, element);
						DrawElementName(element);
						changed = true;
					}
				}
				END_LOOP;
				if (changed) {
					Draw();
					IncrementUndoSerialNumber();
				}
			}
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char markcrosshair_syntax[] = "MarkCrosshair()\n" "MarkCrosshair(Center)";

static const char markcrosshair_help[] = "Set/Reset the Crosshair mark.";

/* %start-doc actions MarkCrosshair

The ``mark'' is a small X-shaped target on the display which is
treated like a second origin (the normal origin is the upper let
corner of the board).  The GUI will display a second set of
coordinates for this mark, which tells you how far you are from it.

If no argument is given, the mark is toggled - disabled if it was
enabled, or enabled at the current cursor position of disabled.  If
the @code{Center} argument is given, the mark is moved to the current
cursor location.

%end-doc */

static int ActionMarkCrosshair(int argc, char **argv, Coord x, Coord y)
{
	char *function = ACTION_ARG(0);
	if (!function || !*function) {
		if (Marked.status) {
			notify_mark_change(false);
			Marked.status = false;
			notify_mark_change(true);
		}
		else {
			notify_mark_change(false);
			Marked.status = false;
			Marked.status = true;
			Marked.X = Crosshair.X;
			Marked.Y = Crosshair.Y;
			notify_mark_change(true);
		}
	}
	else if (funchash_get(function, NULL) == F_Center) {
		notify_mark_change(false);
		Marked.status = true;
		Marked.X = Crosshair.X;
		Marked.Y = Crosshair.Y;
		notify_mark_change(true);
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char routestyle_syntax[] = "RouteStyle(1|2|3|4)";

static const char routestyle_help[] = "Copies the indicated routing style into the current sizes.";

/* %start-doc actions RouteStyle

%end-doc */

static int ActionRouteStyle(int argc, char **argv, Coord x, Coord y)
{
	char *str = ACTION_ARG(0);
	RouteStyleType *rts;
	int number;

	if (str) {
		number = atoi(str);
		if (number > 0 && number <= NUM_STYLES) {
			rts = &PCB->RouteStyle[number - 1];
			SetLineSize(rts->Thick);
			SetViaSize(rts->Diameter, true);
			SetViaDrillingHole(rts->Hole, true);
			SetKeepawayWidth(rts->Keepaway);
			hid_action("RouteStylesChanged");
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char createmenu_syntax[] = "CreateMenu(path | path, action, mnemonic, accel, tooltip, cookie)";
static const char createmenu_help[] = "Creates a new menu, popup (only path specified) or submenu (at least path and action are specified)";

/* %start-doc actions RouteStyle

%end-doc */

static int ActionCreateMenu(int argc, char **argv, Coord x, Coord y)
{
	if (gui == NULL) {
		Message("Error: can't create menu, there's no GUI hid loaded\n");
		return 1;
	}

	if (argc > 0) {
		gui->create_menu(argv[0], (argc > 1) ? argv[1] : NULL, (argc > 2) ? argv[2] : NULL, (argc > 3) ? argv[3] : NULL, (argc > 4) ? argv[4] : NULL, (argc > 5) ? argv[5] : NULL);
		return 0;
	}

	AFAIL(message);
}

/* --------------------------------------------------------------------------- */

static const char setsame_syntax[] = "SetSame()";

static const char setsame_help[] = "Sets current layer and sizes to match indicated item.";

/* %start-doc actions SetSame

When invoked over any line, arc, polygon, or via, this changes the
current layer to be the layer that item is on, and changes the current
sizes (thickness, keepaway, drill, etc) according to that item.

%end-doc */

static int ActionSetSame(int argc, char **argv, Coord x, Coord y)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	LayerTypePtr layer = CURRENT;

	type = SearchScreen(x, y, CLONE_TYPES, &ptr1, &ptr2, &ptr3);
/* set layer current and size from line or arc */
	switch (type) {
	case LINE_TYPE:
		notify_crosshair_change(false);
		conf_core.design.line_thickness = ((LineTypePtr) ptr2)->Thickness;
		conf_core.design.keepaway = ((LineTypePtr) ptr2)->Clearance / 2;
		layer = (LayerTypePtr) ptr1;
		if (conf_core.editor.mode != LINE_MODE)
			SetMode(LINE_MODE);
		notify_crosshair_change(true);
		hid_action("RouteStylesChanged");
		break;

	case ARC_TYPE:
		notify_crosshair_change(false);
		conf_core.design.line_thickness = ((ArcTypePtr) ptr2)->Thickness;
		conf_core.design.keepaway = ((ArcTypePtr) ptr2)->Clearance / 2;
		layer = (LayerTypePtr) ptr1;
		if (conf_core.editor.mode != ARC_MODE)
			SetMode(ARC_MODE);
		notify_crosshair_change(true);
		hid_action("RouteStylesChanged");
		break;

	case POLYGON_TYPE:
		layer = (LayerTypePtr) ptr1;
		break;

	case VIA_TYPE:
		notify_crosshair_change(false);
		conf_core.design.via_thickness = ((PinTypePtr) ptr2)->Thickness;
		conf_core.design.via_drilling_hole = ((PinTypePtr) ptr2)->DrillingHole;
		conf_core.design.keepaway = ((PinTypePtr) ptr2)->Clearance / 2;
		if (conf_core.editor.mode != VIA_MODE)
			SetMode(VIA_MODE);
		notify_crosshair_change(true);
		hid_action("RouteStylesChanged");
		break;

	default:
		return 1;
	}
	if (layer != CURRENT) {
		ChangeGroupVisibility(GetLayerNumber(PCB->Data, layer), true, true);
		Redraw();
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char switchhid_syntax[] = "SwitchHID(lesstif|gtk|batch)";

static const char switchhid_help[] = "Switch to another HID.";

/* %start-doc actions SwitchHID

Switch to another HID.

%end-doc */

static int ActionSwitchHID(int argc, char **argv, Coord x, Coord y)
{
	HID *ng = hid_find_gui(argv[0]);
	int chg;

	if (ng == NULL) {
		Message("No such HID.");
		return 1;
	}

	next_gui = ng;
	chg = PCB->Changed;
	QuitApplication();
	PCB->Changed = chg;

	return 0;
}


HID_Action gui_action_list[] = {
	{"Display", 0, ActionDisplay,
	 display_help, display_syntax}
	,
	{"CycleDrag", 0, ActionCycleDrag,
	 cycledrag_help, cycledrag_syntax}
	,
	{"MarkCrosshair", 0, ActionMarkCrosshair,
	 markcrosshair_help, markcrosshair_syntax}
	,
	{"Message", 0, ActionMessage,
	 message_help, message_syntax}
	,
	{"Mode", 0, ActionMode,
	 mode_help, mode_syntax}
	,
	{"ToggleHideName", 0, ActionToggleHideName,
	 togglehidename_help, togglehidename_syntax}
	,
	{"SetSame", N_("Select item to use attributes from"), ActionSetSame,
	 setsame_help, setsame_syntax}
	,
	{"RouteStyle", 0, ActionRouteStyle,
	 routestyle_help, routestyle_syntax}
	,
	{"CreateMenu", 0, ActionCreateMenu,
	 createmenu_help, createmenu_syntax}
	,
	{"SwitchHID", 0, ActionSwitchHID,
	 switchhid_help, switchhid_syntax}
};

REGISTER_ACTIONS(gui_action_list, NULL)
