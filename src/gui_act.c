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
#include "board.h"
#include "build_run.h"
#include "conf_core.h"
#include "data.h"
#include "action_helper.h"
#include "error.h"
#include "undo.h"
#include "funchash_core.h"

#include "draw.h"
#include "search.h"
#include "find.h"
#include "set.h"
#include "stub_stroke.h"
#include "hid_actions.h"
#include "hid_init.h"
#include "compat_nls.h"

#include "obj_elem_draw.h"
#include "obj_pinvia_draw.h"
#include "obj_pad_draw.h"


/* --------------------------------------------------------------------------- */
/* Toggle actions are kept for compatibility; new code should use the conf system instead */
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

static enum pcb_crosshair_shape_e CrosshairShapeIncrement(enum pcb_crosshair_shape_e shape)
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

static int ActionDisplay(int argc, const char **argv, pcb_coord_t childX, pcb_coord_t childY)
{
	const char *function, *str_dir;
	int id;
	int err = 0;

	function = PCB_ACTION_ARG(0);
	str_dir = PCB_ACTION_ARG(1);

	if (function && (!str_dir || !*str_dir)) {
		switch (id = pcb_funchash_get(function, NULL)) {

			/* redraw layout */
		case F_ClearAndRedraw:
		case F_Redraw:
			pcb_redraw();
			break;

			/* change the displayed name of elements */
		case F_Value:
		case F_NameOnPCB:
		case F_Description:
			PCB_ELEMENT_LOOP(PCB->Data);
			{
				EraseElementName(element);
			}
			END_LOOP;
			switch (id) {
			case F_Value:
				if (conf_core.editor.description || conf_core.editor.name_on_pcb) {
					conf_set_editor(description, 0);
					conf_set_editor(name_on_pcb, 0);
				}
				else
					conf_set_editor(name_on_pcb, 0); /* need to write once so the event is triggered */
				break;
			case F_NameOnPCB:
				if (conf_core.editor.description || !conf_core.editor.name_on_pcb) {
					conf_set_editor(description, 0);
					conf_set_editor(name_on_pcb, 1);
				}
				else
					conf_set_editor(name_on_pcb, 1); /* need to write once so the event is triggered */
				break;
			case F_Description:
				if (!conf_core.editor.description || conf_core.editor.name_on_pcb) {
					conf_set_editor(description, 1);
					conf_set_editor(name_on_pcb, 0);
				}
				else
					conf_set_editor(name_on_pcb, 0); /* need to write once so the event is triggered */
				break;
			}
			PCB_ELEMENT_LOOP(PCB->Data);
			{
				DrawElementName(element);
			}
			END_LOOP;
			pcb_draw();
			break;

			/* toggle line-adjust flag */
		case F_ToggleAllDirections:
			conf_toggle_editor(all_direction_lines);
			pcb_adjust_attached_objects();
			break;

		case F_CycleClip:
			pcb_notify_crosshair_change(pcb_false);
			if (conf_core.editor.all_direction_lines) {
				conf_toggle_editor(all_direction_lines);
				conf_setf(CFR_DESIGN,"editor/line_refraction",-1,"%d",0);
			}
			else {
				conf_setf(CFR_DESIGN,"editor/line_refraction",-1,"%d",(conf_core.editor.line_refraction +1) % 3);
			}
			pcb_adjust_attached_objects();
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_CycleCrosshair:
			pcb_notify_crosshair_change(pcb_false);
			Crosshair.shape = CrosshairShapeIncrement(Crosshair.shape);
			if (Crosshair_Shapes_Number == Crosshair.shape)
				Crosshair.shape = Basic_Crosshair_Shape;
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleRubberBandMode:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(rubber_band_mode);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleStartDirection:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(swap_start_direction);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleUniqueNames:
			conf_toggle_editor(unique_names);
			break;

		case F_ToggleSnapPin:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(snap_pin);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleSnapOffGridLine:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(snap_offgrid_line);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleHighlightOnPoint:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(highlight_on_point);
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleLocalRef:
			conf_toggle_editor(local_ref);
			break;

		case F_ToggleThindraw:
			conf_toggle_editor(thin_draw);
			pcb_redraw();
			break;

		case F_ToggleThindrawPoly:
			conf_toggle_editor(thin_draw_poly);
			pcb_redraw();
			break;

		case F_ToggleLockNames:
			conf_toggle_editor(lock_names);
			conf_set_editor(only_names, 0);
			break;

		case F_ToggleOnlyNames:
			conf_toggle_editor(only_names);
			conf_set_editor(lock_names, 0);
			break;

		case F_ToggleHideNames:
			conf_toggle_editor(hide_names);
			pcb_redraw();
			break;

		case F_ToggleStroke:
			conf_toggle_editor(enable_stroke);
			break;

		case F_ToggleShowDRC:
			conf_toggle_editor(show_drc);
			break;

		case F_ToggleLiveRoute:
			conf_toggle_editor(live_routing);
			break;

		case F_ToggleAutoDRC:
			pcb_notify_crosshair_change(pcb_false);
			conf_toggle_editor(auto_drc);
			if (conf_core.editor.auto_drc && conf_core.editor.mode == PCB_MODE_LINE) {
				if (pcb_reset_conns(pcb_true)) {
					IncrementUndoSerialNumber();
					pcb_draw();
				}
				if (Crosshair.AttachedLine.State != STATE_FIRST)
					pcb_lookup_conn(Crosshair.AttachedLine.Point1.X, Crosshair.AttachedLine.Point1.Y, pcb_true, 1, PCB_FLAG_FOUND);
			}
			pcb_notify_crosshair_change(pcb_true);
			break;

		case F_ToggleCheckPlanes:
			conf_toggle_editor(check_planes);
			pcb_redraw();
			break;

		case F_ToggleOrthoMove:
			conf_toggle_editor(orthogonal_moves);
			break;

		case F_ToggleName:
			conf_toggle_editor(show_number);
			pcb_redraw();
			break;

		case F_ToggleMask:
			conf_toggle_editor(show_mask);
			pcb_redraw();
			break;

		case F_ToggleClearLine:
			conf_toggle_editor(clear_line);
			break;

		case F_ToggleFullPoly:
			conf_toggle_editor(full_poly);
			break;

			/* shift grid alignment */
		case F_ToggleGrid:
			{
				pcb_coord_t oldGrid = PCB->Grid;

				PCB->Grid = 1;
				if (pcb_crosshair_move_absolute(Crosshair.X, Crosshair.Y))
					pcb_notify_crosshair_change(pcb_true);	/* first notify was in MoveCrosshairAbs */
				SetGrid(oldGrid, pcb_true);
			}
			break;

			/* toggle displaying of the grid */
		case F_Grid:
			conf_toggle_editor(draw_grid);
			pcb_redraw();
			break;

			/* display the pinout of an element */
		case F_Pinout:
			{
				pcb_element_t *element;
				void *ptrtmp;
				pcb_coord_t x, y;

				gui->get_coords(_("Click on an element"), &x, &y);
				if ((SearchScreen(x, y, PCB_TYPE_ELEMENT, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_TYPE_NONE) {
					element = (pcb_element_t *) ptrtmp;
					gui->show_item(element);
				}
				break;
			}

			/* toggle displaying of pin/pad/via names */
		case F_PinOrPadName:
			{
				void *ptr1, *ptr2, *ptr3;
				pcb_coord_t x, y;
				gui->get_coords(_("Click on an element"), &x, &y);

				switch (SearchScreen(x, y,
														 PCB_TYPE_ELEMENT | PCB_TYPE_PIN | PCB_TYPE_PAD |
														 PCB_TYPE_VIA, (void **) &ptr1, (void **) &ptr2, (void **) &ptr3)) {
				case PCB_TYPE_ELEMENT:
					PCB_PIN_LOOP((pcb_element_t *) ptr1);
					{
						if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, pin))
							ErasePinName(pin);
						else
							DrawPinName(pin);
						AddObjectToFlagUndoList(PCB_TYPE_PIN, ptr1, pin, pin);
						PCB_FLAG_TOGGLE(PCB_FLAG_DISPLAYNAME, pin);
					}
					END_LOOP;
					PCB_PAD_LOOP((pcb_element_t *) ptr1);
					{
						if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, pad))
							ErasePadName(pad);
						else
							DrawPadName(pad);
						AddObjectToFlagUndoList(PCB_TYPE_PAD, ptr1, pad, pad);
						PCB_FLAG_TOGGLE(PCB_FLAG_DISPLAYNAME, pad);
					}
					END_LOOP;
					SetChangedFlag(pcb_true);
					IncrementUndoSerialNumber();
					pcb_draw();
					break;

				case PCB_TYPE_PIN:
					if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, (pcb_pin_t *) ptr2))
						ErasePinName((pcb_pin_t *) ptr2);
					else
						DrawPinName((pcb_pin_t *) ptr2);
					AddObjectToFlagUndoList(PCB_TYPE_PIN, ptr1, ptr2, ptr3);
					PCB_FLAG_TOGGLE(PCB_FLAG_DISPLAYNAME, (pcb_pin_t *) ptr2);
					SetChangedFlag(pcb_true);
					IncrementUndoSerialNumber();
					pcb_draw();
					break;

				case PCB_TYPE_PAD:
					if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, (pcb_pad_t *) ptr2))
						ErasePadName((pcb_pad_t *) ptr2);
					else
						DrawPadName((pcb_pad_t *) ptr2);
					AddObjectToFlagUndoList(PCB_TYPE_PAD, ptr1, ptr2, ptr3);
					PCB_FLAG_TOGGLE(PCB_FLAG_DISPLAYNAME, (pcb_pad_t *) ptr2);
					SetChangedFlag(pcb_true);
					IncrementUndoSerialNumber();
					pcb_draw();
					break;
				case PCB_TYPE_VIA:
					if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, (pcb_pin_t *) ptr2))
						EraseViaName((pcb_pin_t *) ptr2);
					else
						DrawViaName((pcb_pin_t *) ptr2);
					AddObjectToFlagUndoList(PCB_TYPE_VIA, ptr1, ptr2, ptr3);
					PCB_FLAG_TOGGLE(PCB_FLAG_DISPLAYNAME, (pcb_pin_t *) ptr2);
					SetChangedFlag(pcb_true);
					IncrementUndoSerialNumber();
					pcb_draw();
					break;
				}
				break;
			}
		default:
			err = 1;
		}
	}
	else if (function && str_dir) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_ToggleGrid:
			if (argc > 2) {
				PCB->GridOffsetX = pcb_get_value(argv[1], NULL, NULL, NULL);
				PCB->GridOffsetY = pcb_get_value(argv[2], NULL, NULL, NULL);
				if (conf_core.editor.draw_grid)
					pcb_redraw();
			}
			break;

		default:
			err = 1;
			break;
		}
	}

	if (!err)
		return 0;

	PCB_AFAIL(display);
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

static int ActionMode(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);

	if (function) {
		Note.X = Crosshair.X;
		Note.Y = Crosshair.Y;
		pcb_notify_crosshair_change(pcb_false);
		switch (pcb_funchash_get(function, NULL)) {
		case F_Arc:
			SetMode(PCB_MODE_ARC);
			break;
		case F_Arrow:
			SetMode(PCB_MODE_ARROW);
			break;
		case F_Copy:
			SetMode(PCB_MODE_COPY);
			break;
		case F_InsertPoint:
			SetMode(PCB_MODE_INSERT_POINT);
			break;
		case F_Line:
			SetMode(PCB_MODE_LINE);
			break;
		case F_Lock:
			SetMode(PCB_MODE_LOCK);
			break;
		case F_Move:
			SetMode(PCB_MODE_MOVE);
			break;
		case F_None:
			SetMode(PCB_MODE_NO);
			break;
		case F_Cancel:
			{
				int saved_mode = conf_core.editor.mode;
				SetMode(PCB_MODE_NO);
				SetMode(saved_mode);
			}
			break;
		case F_Escape:
			{
				switch (conf_core.editor.mode) {
				case PCB_MODE_VIA:
				case PCB_MODE_PASTE_BUFFER:
				case PCB_MODE_TEXT:
				case PCB_MODE_ROTATE:
				case PCB_MODE_REMOVE:
				case PCB_MODE_MOVE:
				case PCB_MODE_COPY:
				case PCB_MODE_INSERT_POINT:
				case PCB_MODE_RUBBERBAND_MOVE:
				case PCB_MODE_THERMAL:
				case PCB_MODE_LOCK:
					SetMode(PCB_MODE_NO);
					SetMode(PCB_MODE_ARROW);
					break;

				case PCB_MODE_LINE:
					if (Crosshair.AttachedLine.State == STATE_FIRST)
						SetMode(PCB_MODE_ARROW);
					else {
						SetMode(PCB_MODE_NO);
						SetMode(PCB_MODE_LINE);
					}
					break;

				case PCB_MODE_RECTANGLE:
					if (Crosshair.AttachedBox.State == STATE_FIRST)
						SetMode(PCB_MODE_ARROW);
					else {
						SetMode(PCB_MODE_NO);
						SetMode(PCB_MODE_RECTANGLE);
					}
					break;

				case PCB_MODE_POLYGON:
					if (Crosshair.AttachedLine.State == STATE_FIRST)
						SetMode(PCB_MODE_ARROW);
					else {
						SetMode(PCB_MODE_NO);
						SetMode(PCB_MODE_POLYGON);
					}
					break;

				case PCB_MODE_POLYGON_HOLE:
					if (Crosshair.AttachedLine.State == STATE_FIRST)
						SetMode(PCB_MODE_ARROW);
					else {
						SetMode(PCB_MODE_NO);
						SetMode(PCB_MODE_POLYGON_HOLE);
					}
					break;

				case PCB_MODE_ARC:
					if (Crosshair.AttachedBox.State == STATE_FIRST)
						SetMode(PCB_MODE_ARROW);
					else {
						SetMode(PCB_MODE_NO);
						SetMode(PCB_MODE_ARC);
					}
					break;

				case PCB_MODE_ARROW:
					break;

				default:
					break;
				}
			}
			break;

		case F_Notify:
			pcb_notify_mode();
			break;
		case F_PasteBuffer:
			SetMode(PCB_MODE_PASTE_BUFFER);
			break;
		case F_Polygon:
			SetMode(PCB_MODE_POLYGON);
			break;
		case F_PolygonHole:
			SetMode(PCB_MODE_POLYGON_HOLE);
			break;
		case F_Release:
			if ((mid_stroke) && (conf_core.editor.enable_stroke))
				stub_stroke_finish();
			else
				pcb_release_mode();
			break;
		case F_Remove:
			SetMode(PCB_MODE_REMOVE);
			break;
		case F_Rectangle:
			SetMode(PCB_MODE_RECTANGLE);
			break;
		case F_Rotate:
			SetMode(PCB_MODE_ROTATE);
			break;
		case F_Stroke:
			if (conf_core.editor.enable_stroke) {
				stub_stroke_start();
				break;
			}
			/* Handle middle mouse button restarts of drawing mode.  If not in
			   |  a drawing mode, middle mouse button will select objects.
			 */
			if (conf_core.editor.mode == PCB_MODE_LINE && Crosshair.AttachedLine.State != STATE_FIRST) {
				SetMode(PCB_MODE_LINE);
			}
			else if (conf_core.editor.mode == PCB_MODE_ARC && Crosshair.AttachedBox.State != STATE_FIRST)
				SetMode(PCB_MODE_ARC);
			else if (conf_core.editor.mode == PCB_MODE_RECTANGLE && Crosshair.AttachedBox.State != STATE_FIRST)
				SetMode(PCB_MODE_RECTANGLE);
			else if (conf_core.editor.mode == PCB_MODE_POLYGON && Crosshair.AttachedLine.State != STATE_FIRST)
				SetMode(PCB_MODE_POLYGON);
			else {
				SaveMode();
				saved_mode = pcb_true;
				SetMode(PCB_MODE_ARROW);
				pcb_notify_mode();
			}
			break;
		case F_Text:
			SetMode(PCB_MODE_TEXT);
			break;
		case F_Thermal:
			SetMode(PCB_MODE_THERMAL);
			break;
		case F_Via:
			SetMode(PCB_MODE_VIA);
			break;

		case F_Restore:						/* restore the last saved mode */
			RestoreMode();
			break;

		case F_Save:								/* save currently selected mode */
			SaveMode();
			break;
		}
		pcb_notify_crosshair_change(pcb_true);
		return 0;
	}

	PCB_AFAIL(mode);
}

/* ---------------------------------------------------------------- */
static const char cycledrag_syntax[] = "CycleDrag()\n";

static const char cycledrag_help[] = "Cycle through which object is being dragged";

#define close_enough(a, b) ((((a)-(b)) > 0) ? ((a)-(b) < (SLOP * pixel_slop)) : ((a)-(b) > -(SLOP * pixel_slop)))
static int ActionCycleDrag(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
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

		if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], PCB_TYPE_LINE) != PCB_TYPE_NONE) {
			/* line has two endpoints, check which one is close to the original x;y */
			pcb_line_t *l = ptr2;
			if (close_enough(Note.X, l->Point1.X) && close_enough(Note.Y, l->Point1.Y)) {
				Crosshair.AttachedObject.Type = PCB_TYPE_LINE_POINT;
				Crosshair.AttachedObject.Ptr1 = ptr1;
				Crosshair.AttachedObject.Ptr2 = ptr2;
				Crosshair.AttachedObject.Ptr3 = &l->Point1;
				return 0;
			}
			if (close_enough(Note.X, l->Point2.X) && close_enough(Note.Y, l->Point2.Y)) {
				Crosshair.AttachedObject.Type = PCB_TYPE_LINE_POINT;
				Crosshair.AttachedObject.Ptr1 = ptr1;
				Crosshair.AttachedObject.Ptr2 = ptr2;
				Crosshair.AttachedObject.Ptr3 = &l->Point2;
				return 0;
			}
		}
		else if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], PCB_TYPE_VIA) != PCB_TYPE_NONE) {
			Crosshair.AttachedObject.Type = PCB_TYPE_VIA;
			Crosshair.AttachedObject.Ptr1 = ptr1;
			Crosshair.AttachedObject.Ptr2 = ptr2;
			Crosshair.AttachedObject.Ptr3 = ptr3;
			return 0;
		}
		else if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], PCB_TYPE_PAD) != PCB_TYPE_NONE) {
			Crosshair.AttachedObject.Type = PCB_TYPE_ELEMENT;
			Crosshair.AttachedObject.Ptr1 = ptr1;
			Crosshair.AttachedObject.Ptr2 = ptr1;
			Crosshair.AttachedObject.Ptr3 = ptr1;
			return 0;
		}
		else if (SearchObjectByID(PCB->Data, &ptr1, &ptr2, &ptr3, Crosshair.drags[Crosshair.drags_current], PCB_TYPE_ARC) != PCB_TYPE_NONE) {
			Crosshair.AttachedObject.Type = PCB_TYPE_ARC;
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

static const char message_syntax[] = "pcb_message(message)";

static const char message_help[] = "Writes a message to the log window.";

/* %start-doc actions Message

This action displays a message to the log window.  This action is primarily
provided for use by other programs which may interface with PCB.  If
multiple arguments are given, each one is sent to the log window
followed by a newline.

%end-doc */

static int ActionMessage(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int i;

	if (argc < 1)
		PCB_AFAIL(message);

	for (i = 0; i < argc; i++) {
		pcb_message(PCB_MSG_DEFAULT, argv[i]);
		pcb_message(PCB_MSG_DEFAULT, "\n");
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

static int ActionToggleHideName(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function && PCB->ElementOn) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = SearchScreen(x, y, PCB_TYPE_ELEMENT, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
					AddObjectToFlagUndoList(type, ptr1, ptr2, ptr3);
					EraseElementName((pcb_element_t *) ptr2);
					PCB_FLAG_TOGGLE(PCB_FLAG_HIDENAME, (pcb_element_t *) ptr2);
					DrawElementName((pcb_element_t *) ptr2);
					pcb_draw();
					IncrementUndoSerialNumber();
				}
				break;
			}
		case F_SelectedElements:
		case F_Selected:
			{
				pcb_bool changed = pcb_false;
				PCB_ELEMENT_LOOP(PCB->Data);
				{
					if ((PCB_FLAG_TEST(PCB_FLAG_SELECTED, element) || PCB_FLAG_TEST(PCB_FLAG_SELECTED, &NAMEONPCB_TEXT(element)))
							&& (PCB_FRONT(element) || PCB->InvisibleObjectsOn)) {
						AddObjectToFlagUndoList(PCB_TYPE_ELEMENT, element, element, element);
						EraseElementName(element);
						PCB_FLAG_TOGGLE(PCB_FLAG_HIDENAME, element);
						DrawElementName(element);
						changed = pcb_true;
					}
				}
				END_LOOP;
				if (changed) {
					pcb_draw();
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

static int ActionMarkCrosshair(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (!function || !*function) {
		if (Marked.status) {
			pcb_notify_mark_change(pcb_false);
			Marked.status = pcb_false;
			pcb_notify_mark_change(pcb_true);
		}
		else {
			pcb_notify_mark_change(pcb_false);
			Marked.status = pcb_false;
			Marked.status = pcb_true;
			Marked.X = Crosshair.X;
			Marked.Y = Crosshair.Y;
			pcb_notify_mark_change(pcb_true);
		}
	}
	else if (pcb_funchash_get(function, NULL) == F_Center) {
		pcb_notify_mark_change(pcb_false);
		Marked.status = pcb_true;
		Marked.X = Crosshair.X;
		Marked.Y = Crosshair.Y;
		pcb_notify_mark_change(pcb_true);
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char routestyle_syntax[] = "RouteStyle(1|2|3|4)";

static const char routestyle_help[] = "Copies the indicated routing style into the current sizes.";

/* %start-doc actions RouteStyle

%end-doc */

static int ActionRouteStyle(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *str = PCB_ACTION_ARG(0);
	pcb_route_style_t *rts;
	int number;

	if (str) {
		char *end;
		number = strtol(str, &end, 10);

		if (*end != '\0') { /* if not an integer, find by name */
			int n;
			number = -1;
			for(n = 0; n < vtroutestyle_len(&PCB->RouteStyle); n++) {
				rts = &PCB->RouteStyle.array[n];
				if (strcasecmp(rts->name, str) == 0) {
					number = n + 1;
					break;
				}
			}
		}

		if (number > 0 && number <= vtroutestyle_len(&PCB->RouteStyle)) {
			rts = &PCB->RouteStyle.array[number - 1];
			SetLineSize(rts->Thick);
			SetViaSize(rts->Diameter, pcb_true);
			SetViaDrillingHole(rts->Hole, pcb_true);
			SetClearanceWidth(rts->Clearance);
			pcb_hid_action("RouteStylesChanged");
		}
		else
			pcb_message(PCB_MSG_DEFAULT, "Error: invalid route style name or index\n");
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char createmenu_syntax[] = "CreateMenu(path | path, action, mnemonic, accel, tooltip, cookie)";
static const char createmenu_help[] = "Creates a new menu, popup (only path specified) or submenu (at least path and action are specified)";

/* %start-doc actions RouteStyle

%end-doc */

static int ActionCreateMenu(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (gui == NULL) {
		pcb_message(PCB_MSG_DEFAULT, "Error: can't create menu, there's no GUI hid loaded\n");
		return 1;
	}

	if (argc > 0) {
		gui->create_menu(argv[0], (argc > 1) ? argv[1] : NULL, (argc > 2) ? argv[2] : NULL, (argc > 3) ? argv[3] : NULL, (argc > 4) ? argv[4] : NULL, (argc > 5) ? argv[5] : NULL);
		return 0;
	}

	PCB_AFAIL(message);
}

/* --------------------------------------------------------------------------- */

static const char removemenu_syntax[] = "RemoveMenu(path|cookie)";
static const char removemenu_help[] = "Recursively removes a new menu, popup (only path specified) or submenu. ";

/* %start-doc actions RouteStyle

%end-doc */

static int ActionRemoveMenu(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (gui == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't remove menu, there's no GUI hid loaded\n");
		return 1;
	}

	if (gui->remove_menu == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't remove menu, the GUI doesn't support it\n");
		return 1;
	}

	if (argc > 0) {
		if (gui->remove_menu(argv[0]) != 0)
			pcb_message(PCB_MSG_ERROR, "failed to remove some of the menu items\n");
		return 0;
	}

	PCB_AFAIL(message);
}

/* --------------------------------------------------------------------------- */

static const char setsame_syntax[] = "SetSame()";

static const char setsame_help[] = "Sets current layer and sizes to match indicated item.";

/* %start-doc actions SetSame

When invoked over any line, arc, polygon, or via, this changes the
current layer to be the layer that item is on, and changes the current
sizes (thickness, clearance, drill, etc) according to that item.

%end-doc */
static void set_same_(pcb_coord_t Thick, pcb_coord_t Diameter, pcb_coord_t Hole, pcb_coord_t Clearance, char *Name)
{
	int known;
	known = pcb_route_style_lookup(&PCB->RouteStyle, Thick, Diameter, Hole, Clearance, Name);
	if (known < 0) {
		/* unknown style, set properties */
		if (Thick != 0)     { pcb_custom_route_style.Thick     = Thick;     conf_set_design("design/line_thickness", "%$mS", Thick); }
		if (Clearance != 0) { pcb_custom_route_style.Clearance = Clearance; conf_set_design("design/clearance", "%$mS", Clearance); }
		if (Diameter != 0)  { pcb_custom_route_style.Diameter  = Diameter;  conf_set_design("design/via_thickness", "%$mS", Diameter); }
		if (Hole != 0)      { pcb_custom_route_style.Hole      = Hole;      conf_set_design("design/via_drilling_hole", "%$mS", Hole); }
	}
	else
		pcb_use_route_style_idx(&PCB->RouteStyle, known);
}

static int ActionSetSame(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	pcb_layer_t *layer = CURRENT;

	type = SearchScreen(x, y, CLONE_TYPES, &ptr1, &ptr2, &ptr3);
/* set layer current and size from line or arc */
	switch (type) {
	case PCB_TYPE_LINE:
		pcb_notify_crosshair_change(pcb_false);
		set_same_(((pcb_line_t *) ptr2)->Thickness, 0, 0, ((pcb_line_t *) ptr2)->Clearance / 2, NULL);
		layer = (pcb_layer_t *) ptr1;
		if (conf_core.editor.mode != PCB_MODE_LINE)
			SetMode(PCB_MODE_LINE);
		pcb_notify_crosshair_change(pcb_true);
		pcb_hid_action("RouteStylesChanged");
		break;

	case PCB_TYPE_ARC:
		pcb_notify_crosshair_change(pcb_false);
		set_same_(((pcb_arc_t *) ptr2)->Thickness, 0, 0, ((pcb_arc_t *) ptr2)->Clearance / 2, NULL);
		layer = (pcb_layer_t *) ptr1;
		if (conf_core.editor.mode != PCB_MODE_ARC)
			SetMode(PCB_MODE_ARC);
		pcb_notify_crosshair_change(pcb_true);
		pcb_hid_action("RouteStylesChanged");
		break;

	case PCB_TYPE_POLYGON:
		layer = (pcb_layer_t *) ptr1;
		break;

	case PCB_TYPE_VIA:
		pcb_notify_crosshair_change(pcb_false);
		set_same_(0, ((pcb_pin_t *) ptr2)->Thickness, ((pcb_pin_t *) ptr2)->DrillingHole, ((pcb_pin_t *) ptr2)->Clearance / 2, NULL);
		if (conf_core.editor.mode != PCB_MODE_VIA)
			SetMode(PCB_MODE_VIA);
		pcb_notify_crosshair_change(pcb_true);
		pcb_hid_action("RouteStylesChanged");
		break;

	default:
		return 1;
	}
	if (layer != CURRENT) {
		ChangeGroupVisibility(GetLayerNumber(PCB->Data, layer), pcb_true, pcb_true);
		pcb_redraw();
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char switchhid_syntax[] = "SwitchHID(lesstif|gtk|batch)";

static const char switchhid_help[] = "Switch to another HID.";

/* %start-doc actions SwitchHID

Switch to another HID.

%end-doc */

static int ActionSwitchHID(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_hid_t *ng = pcb_hid_find_gui(argv[0]);
	int chg;

	if (ng == NULL) {
		pcb_message(PCB_MSG_DEFAULT, "No such HID.");
		return 1;
	}

	next_gui = ng;
	chg = PCB->Changed;
	pcb_quit_app();
	PCB->Changed = chg;

	return 0;
}


/* --------------------------------------------------------------------------- */

/* This action is provided for CLI convience */
static const char fullscreen_syntax[] = "FullScreen(on|off|toggle)\n";

static const char fullscreen_help[] = "Hide widgets to get edit area full screen";

static int FullScreen(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *op = argv == NULL ? NULL : argv[0];

	if ((op == NULL) || (strcmp(op, "toggle") == 0))
		conf_setf(CFR_DESIGN, "editor/fullscreen", -1, "%d", !conf_core.editor.fullscreen, POL_OVERWRITE);
	else if (strcmp(op, "on") == 0)
		conf_set(CFR_DESIGN, "editor/fullscreen", -1, "1", POL_OVERWRITE);
	else if (strcmp(op, "off") == 0)
		conf_set(CFR_DESIGN, "editor/fullscreen", -1, "0", POL_OVERWRITE);


	return 0;
}



pcb_hid_action_t gui_action_list[] = {
	{"Display", 0, ActionDisplay,
	 display_help, display_syntax}
	,
	{"CycleDrag", 0, ActionCycleDrag,
	 cycledrag_help, cycledrag_syntax}
	,
	{"FullScreen", 0, FullScreen, fullscreen_help, fullscreen_syntax}
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
	{"RemoveMenu", 0, ActionRemoveMenu,
	 removemenu_help, removemenu_syntax}
	,
	{"SwitchHID", 0, ActionSwitchHID,
	 switchhid_help, switchhid_syntax}
};

PCB_REGISTER_ACTIONS(gui_action_list, NULL)
