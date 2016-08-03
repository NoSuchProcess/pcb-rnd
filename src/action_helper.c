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

/* action routines for output window
 */

#include "config.h"

#include "conf_core.h"

#include "action_helper.h"
#include "buffer.h"
#include "change.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "insert.h"
#include "line.h"
#include "misc.h"
#include "move.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "undo.h"
#include "rtree.h"
#include "stub_stroke.h"
#include "funchash_core.h"
#include "hid_actions.h"

/* --------------------------------------------------------------------------- */

/* %start-doc actions 00delta

Many actions take a @code{delta} parameter as the last parameter,
which is an amount to change something.  That @code{delta} may include
units, as an additional parameter, such as @code{Action(Object,5,mm)}.
If no units are specified, the default is PCB's native units
(currently 1/100 mil).  Also, if the delta is prefixed by @code{+} or
@code{-}, the size is increased or decreased by that amount.
Otherwise, the size size is set to the given amount.

@example
Action(Object,5,mil)
Action(Object,+0.5,mm)
Action(Object,-1)
@end example

Actions which take a @code{delta} parameter which do not accept all
these options will specify what they do take.

%end-doc */

/* %start-doc actions 00objects

Many actions act on indicated objects on the board.  They will have
parameters like @code{ToggleObject} or @code{SelectedVias} to indicate
what group of objects they act on.  Unless otherwise specified, these
parameters are defined as follows:

@table @code

@item Object
@itemx ToggleObject
Affects the object under the mouse pointer.  If this action is invoked
from a menu or script, the user will be prompted to click on an
object, which is then the object affected.

@item Selected
@itemx SelectedObjects

Affects all objects which are currently selected.  At least, all
selected objects for which the given action makes sense.

@item SelectedPins
@itemx SelectedVias
@itemx Selected@var{Type}
@itemx @i{etc}
Affects all objects which are both selected and of the @var{Type} specified.

@end table

%end-doc */

/*  %start-doc actions 00macros

@macro pinshapes

Pins, pads, and vias can have various shapes.  All may be round.  Pins
and pads may be square (obviously "square" pads are usually
rectangular).  Pins and vias may be octagonal.  When you change a
shape flag of an element, you actually change all of its pins and
pads.

Note that the square flag takes precedence over the octagon flag,
thus, if both the square and octagon flags are set, the object is
square.  When the square flag is cleared, the pins and pads will be
either round or, if the octagon flag is set, octagonal.

@end macro

%end-doc */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static PointType InsertedPoint;
LayerTypePtr lastLayer;
static struct {
	PolygonTypePtr poly;
	LineType line;
} fake;

action_note_t Note;
int defer_updates = 0;
int defer_needs_update = 0;


static Cardinal polyIndex = 0;
bool saved_mode = false;

/* ---------------------------------------------------------------------------
 * some local routines
 */
static void AdjustAttachedBox(void);
/* ---------------------------------------------------------------------------
 * Clear warning color from pins/pads
 */
void ClearWarnings()
{
	conf_core.temp.rat_warn = false;
	ALLPIN_LOOP(PCB->Data);
	{
		if (TEST_FLAG(WARNFLAG, pin)) {
			CLEAR_FLAG(WARNFLAG, pin);
			DrawPin(pin);
		}
	}
	ENDALL_LOOP;
	VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(WARNFLAG, via)) {
			CLEAR_FLAG(WARNFLAG, via);
			DrawVia(via);
		}
	}
	END_LOOP;
	ALLPAD_LOOP(PCB->Data);
	{
		if (TEST_FLAG(WARNFLAG, pad)) {
			CLEAR_FLAG(WARNFLAG, pad);
			DrawPad(pad);
		}
	}
	ENDALL_LOOP;
	ALLLINE_LOOP(PCB->Data);
	{
		if (TEST_FLAG(WARNFLAG, line)) {
			CLEAR_FLAG(WARNFLAG, line);
			DrawLine(layer, line);
		}
	}
	ENDALL_LOOP;
	ALLARC_LOOP(PCB->Data);
	{
		if (TEST_FLAG(WARNFLAG, arc)) {
			CLEAR_FLAG(WARNFLAG, arc);
			DrawArc(layer, arc);
		}
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(PCB->Data);
	{
		if (TEST_FLAG(WARNFLAG, polygon)) {
			CLEAR_FLAG(WARNFLAG, polygon);
			DrawPolygon(layer, polygon);
		}
	}
	ENDALL_LOOP;

	Draw();
}

static void click_cb(hidval hv)
{
	if (Note.Click) {
		notify_crosshair_change(false);
		Note.Click = false;
		if (Note.Moving && !gui->shift_is_pressed()) {
			Note.Buffer = conf_core.editor.buffer_number;
			SetBufferNumber(MAX_BUFFER - 1);
			ClearBuffer(PASTEBUFFER);
			AddSelectedToBuffer(PASTEBUFFER, Note.X, Note.Y, true);
			SaveUndoSerialNumber();
			RemoveSelected();
			SaveMode();
			saved_mode = true;
			SetMode(PASTEBUFFER_MODE);
		}
		else if (Note.Hit && !gui->shift_is_pressed()) {
			BoxType box;

			SaveMode();
			saved_mode = true;
			SetMode(gui->control_is_pressed()? COPY_MODE : MOVE_MODE);
			Crosshair.AttachedObject.Ptr1 = Note.ptr1;
			Crosshair.AttachedObject.Ptr2 = Note.ptr2;
			Crosshair.AttachedObject.Ptr3 = Note.ptr3;
			Crosshair.AttachedObject.Type = Note.Hit;

			if (Crosshair.drags != NULL) {
				free(Crosshair.drags);
				Crosshair.drags = NULL;
			}
			Crosshair.dragx = Note.X;
			Crosshair.dragy = Note.Y;
			box.X1 = Note.X + SLOP * pixel_slop;
			box.X2 = Note.X - SLOP * pixel_slop;
			box.Y1 = Note.Y + SLOP * pixel_slop;
			box.Y2 = Note.Y - SLOP * pixel_slop;
			Crosshair.drags = ListBlock(&box, &Crosshair.drags_len);
			Crosshair.drags_current = 0;
			AttachForCopy(Note.X, Note.Y);
		}
		else {
			BoxType box;

			Note.Hit = 0;
			Note.Moving = false;
			SaveUndoSerialNumber();
			box.X1 = -MAX_COORD;
			box.Y1 = -MAX_COORD;
			box.X2 = MAX_COORD;
			box.Y2 = MAX_COORD;
			/* unselect first if shift key not down */
			if (!gui->shift_is_pressed() && SelectBlock(&box, false))
				SetChangedFlag(true);
			NotifyBlock();
			Crosshair.AttachedBox.Point1.X = Note.X;
			Crosshair.AttachedBox.Point1.Y = Note.Y;
		}
		notify_crosshair_change(true);
	}
}

void ReleaseMode(void)
{
	BoxType box;

	if (Note.Click) {
		BoxType box;

		box.X1 = -MAX_COORD;
		box.Y1 = -MAX_COORD;
		box.X2 = MAX_COORD;
		box.Y2 = MAX_COORD;

		Note.Click = false;					/* inhibit timer action */
		SaveUndoSerialNumber();
		/* unselect first if shift key not down */
		if (!gui->shift_is_pressed()) {
			if (SelectBlock(&box, false))
				SetChangedFlag(true);
			if (Note.Moving) {
				Note.Moving = 0;
				Note.Hit = 0;
				return;
			}
		}
		RestoreUndoSerialNumber();
		if (SelectObject())
			SetChangedFlag(true);
		Note.Hit = 0;
		Note.Moving = 0;
	}
	else if (Note.Moving) {
		RestoreUndoSerialNumber();
		NotifyMode();
		ClearBuffer(PASTEBUFFER);
		SetBufferNumber(Note.Buffer);
		Note.Moving = false;
		Note.Hit = 0;
	}
	else if (Note.Hit) {
		NotifyMode();
		Note.Hit = 0;
	}
	else if (conf_core.editor.mode == ARROW_MODE) {
		box.X1 = Crosshair.AttachedBox.Point1.X;
		box.Y1 = Crosshair.AttachedBox.Point1.Y;
		box.X2 = Crosshair.AttachedBox.Point2.X;
		box.Y2 = Crosshair.AttachedBox.Point2.Y;

		RestoreUndoSerialNumber();
		if (SelectBlock(&box, true))
			SetChangedFlag(true);
		else if (Bumped)
			IncrementUndoSerialNumber();
		Crosshair.AttachedBox.State = STATE_FIRST;
	}
	if (saved_mode)
		RestoreMode();
	saved_mode = false;
}

/* ---------------------------------------------------------------------------
 * set new coordinates if in 'RECTANGLE' mode
 * the cursor shape is also adjusted
 */
static void AdjustAttachedBox(void)
{
	if (conf_core.editor.mode == ARC_MODE) {
		Crosshair.AttachedBox.otherway = gui->shift_is_pressed();
		return;
	}
	switch (Crosshair.AttachedBox.State) {
	case STATE_SECOND:						/* one corner is selected */
		{
			/* update coordinates */
			Crosshair.AttachedBox.Point2.X = Crosshair.X;
			Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
			break;
		}
	}
}

/* ---------------------------------------------------------------------------
 * adjusts the objects which are to be created like attached lines...
 */
void AdjustAttachedObjects(void)
{
	PointTypePtr pnt;
	switch (conf_core.editor.mode) {
		/* update at least an attached block (selection) */
	case NO_MODE:
	case ARROW_MODE:
		if (Crosshair.AttachedBox.State) {
			Crosshair.AttachedBox.Point2.X = Crosshair.X;
			Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
		}
		break;

		/* rectangle creation mode */
	case RECTANGLE_MODE:
	case ARC_MODE:
		AdjustAttachedBox();
		break;

		/* polygon creation mode */
	case POLYGON_MODE:
	case POLYGONHOLE_MODE:
		AdjustAttachedLine();
		break;
		/* line creation mode */
	case LINE_MODE:
		if (PCB->RatDraw || PCB->Clipping == 0)
			AdjustAttachedLine();
		else
			AdjustTwoLine(PCB->Clipping - 1);
		break;
		/* point insertion mode */
	case INSERTPOINT_MODE:
		pnt = AdjustInsertPoint();
		if (pnt)
			InsertedPoint = *pnt;
		break;
	case ROTATE_MODE:
		break;
	}
}

/* ---------------------------------------------------------------------------
 * creates points of a line
 */
void NotifyLine(void)
{
	int type = NO_TYPE;
	void *ptr1, *ptr2, *ptr3;

	if (!Marked.status || conf_core.editor.local_ref)
		SetLocalRef(Crosshair.X, Crosshair.Y, true);
	switch (Crosshair.AttachedLine.State) {
	case STATE_FIRST:						/* first point */
		if (PCB->RatDraw && SearchScreen(Crosshair.X, Crosshair.Y, PAD_TYPE | PIN_TYPE, &ptr1, &ptr1, &ptr1) == NO_TYPE) {
			gui->beep();
			break;
		}
		if (conf_core.editor.auto_drc && conf_core.editor.mode == LINE_MODE) {
			type = SearchScreen(Crosshair.X, Crosshair.Y, PIN_TYPE | PAD_TYPE | VIA_TYPE, &ptr1, &ptr2, &ptr3);
			LookupConnection(Crosshair.X, Crosshair.Y, true, 1, FOUNDFLAG);
		}
		if (type == PIN_TYPE || type == VIA_TYPE) {
			Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = ((PinTypePtr) ptr2)->X;
			Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = ((PinTypePtr) ptr2)->Y;
		}
		else if (type == PAD_TYPE) {
			PadTypePtr pad = (PadTypePtr) ptr2;
			double d1 = Distance(Crosshair.X, Crosshair.Y, pad->Point1.X, pad->Point1.Y);
			double d2 = Distance(Crosshair.X, Crosshair.Y, pad->Point2.X, pad->Point2.Y);
			if (d2 < d1) {
				Crosshair.AttachedLine.Point1 = Crosshair.AttachedLine.Point2 = pad->Point2;
			}
			else {
				Crosshair.AttachedLine.Point1 = Crosshair.AttachedLine.Point2 = pad->Point1;
			}
		}
		else {
			Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = Crosshair.X;
			Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = Crosshair.Y;
		}
		Crosshair.AttachedLine.State = STATE_SECOND;
		break;

	case STATE_SECOND:
		/* fall through to third state too */
		lastLayer = CURRENT;
	default:											/* all following points */
		Crosshair.AttachedLine.State = STATE_THIRD;
		break;
	}
}

/* ---------------------------------------------------------------------------
 * create first or second corner of a marked block
 */
void NotifyBlock(void)
{
	notify_crosshair_change(false);
	switch (Crosshair.AttachedBox.State) {
	case STATE_FIRST:						/* setup first point */
		Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = Crosshair.X;
		Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = Crosshair.Y;
		Crosshair.AttachedBox.State = STATE_SECOND;
		break;

	case STATE_SECOND:						/* setup second point */
		Crosshair.AttachedBox.State = STATE_THIRD;
		break;
	}
	notify_crosshair_change(true);
}


/* ---------------------------------------------------------------------------
 *
 * does what's appropriate for the current mode setting. This normally
 * means creation of an object at the current crosshair location.
 *
 * new created objects are added to the create undo list of course
 */
void NotifyMode(void)
{
	void *ptr1, *ptr2, *ptr3;
	int type;

	if (conf_core.temp.rat_warn)
		ClearWarnings();
	switch (conf_core.editor.mode) {
	case ARROW_MODE:
		{
			int test;
			hidval hv;

			Note.Click = true;
			/* do something after click time */
			gui->add_timer(click_cb, conf_core.editor.click_time, hv);

			/* see if we clicked on something already selected
			 * (Note.Moving) or clicked on a MOVE_TYPE
			 * (Note.Hit)
			 */
			for (test = (SELECT_TYPES | MOVE_TYPES) & ~RATLINE_TYPE; test; test &= ~type) {
				type = SearchScreen(Note.X, Note.Y, test, &ptr1, &ptr2, &ptr3);
				if (!Note.Hit && (type & MOVE_TYPES) && !TEST_FLAG(LOCKFLAG, (PinTypePtr) ptr2)) {
					Note.Hit = type;
					Note.ptr1 = ptr1;
					Note.ptr2 = ptr2;
					Note.ptr3 = ptr3;
				}
				if (!Note.Moving && (type & SELECT_TYPES) && TEST_FLAG(SELECTEDFLAG, (PinTypePtr) ptr2))
					Note.Moving = true;
				if ((Note.Hit && Note.Moving) || type == NO_TYPE)
					break;
			}
			break;
		}

	case VIA_MODE:
		{
			PinTypePtr via;

			if (!PCB->ViaOn) {
				Message(_("You must turn via visibility on before\n" "you can place vias\n"));
				break;
			}
			if ((via = CreateNewVia(PCB->Data, Note.X, Note.Y,
															conf_core.design.via_thickness, 2 * conf_core.design.clearance,
															0, conf_core.design.via_drilling_hole, NULL, NoFlags())) != NULL) {
				AddObjectToCreateUndoList(VIA_TYPE, via, via, via);
				if (gui->shift_is_pressed())
					ChangeObjectThermal(VIA_TYPE, via, via, via, PCB->ThermStyle);
				IncrementUndoSerialNumber();
				DrawVia(via);
				Draw();
			}
			break;
		}

	case ARC_MODE:
		{
			switch (Crosshair.AttachedBox.State) {
			case STATE_FIRST:
				Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = Note.X;
				Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = Note.Y;
				Crosshair.AttachedBox.State = STATE_SECOND;
				break;

			case STATE_SECOND:
			case STATE_THIRD:
				{
					ArcTypePtr arc;
					Coord wx, wy;
					Angle sa, dir;

					wx = Note.X - Crosshair.AttachedBox.Point1.X;
					wy = Note.Y - Crosshair.AttachedBox.Point1.Y;
					if (XOR(Crosshair.AttachedBox.otherway, abs(wy) > abs(wx))) {
						Crosshair.AttachedBox.Point2.X = Crosshair.AttachedBox.Point1.X + abs(wy) * SGNZ(wx);
						sa = (wx >= 0) ? 0 : 180;
#ifdef ARC45
						if (abs(wy) / 2 >= abs(wx))
							dir = (SGNZ(wx) == SGNZ(wy)) ? 45 : -45;
						else
#endif
							dir = (SGNZ(wx) == SGNZ(wy)) ? 90 : -90;
					}
					else {
						Crosshair.AttachedBox.Point2.Y = Crosshair.AttachedBox.Point1.Y + abs(wx) * SGNZ(wy);
						sa = (wy >= 0) ? -90 : 90;
#ifdef ARC45
						if (abs(wx) / 2 >= abs(wy))
							dir = (SGNZ(wx) == SGNZ(wy)) ? -45 : 45;
						else
#endif
							dir = (SGNZ(wx) == SGNZ(wy)) ? -90 : 90;
						wy = wx;
					}
					if (abs(wy) > 0 && (arc = CreateNewArcOnLayer(CURRENT,
																												Crosshair.AttachedBox.Point2.X,
																												Crosshair.AttachedBox.Point2.Y,
																												abs(wy),
																												abs(wy),
																												sa,
																												dir,
																												conf_core.design.line_thickness,
																												2 * conf_core.design.clearance,
																												MakeFlags(conf_core.editor.clear_line ? CLEARLINEFLAG : 0)))) {
						BoxTypePtr bx;

						bx = GetArcEnds(arc);
						Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = bx->X2;
						Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = bx->Y2;
						AddObjectToCreateUndoList(ARC_TYPE, CURRENT, arc, arc);
						IncrementUndoSerialNumber();
						addedLines++;
						DrawArc(CURRENT, arc);
						Draw();
						Crosshair.AttachedBox.State = STATE_THIRD;
					}
					break;
				}
			}
			break;
		}
	case LOCK_MODE:
		{
			type = SearchScreen(Note.X, Note.Y, LOCK_TYPES, &ptr1, &ptr2, &ptr3);
			if (type == ELEMENT_TYPE) {
				ElementTypePtr element = (ElementTypePtr) ptr2;

				TOGGLE_FLAG(LOCKFLAG, element);
				PIN_LOOP(element);
				{
					TOGGLE_FLAG(LOCKFLAG, pin);
					CLEAR_FLAG(SELECTEDFLAG, pin);
				}
				END_LOOP;
				PAD_LOOP(element);
				{
					TOGGLE_FLAG(LOCKFLAG, pad);
					CLEAR_FLAG(SELECTEDFLAG, pad);
				}
				END_LOOP;
				CLEAR_FLAG(SELECTEDFLAG, element);
				/* always re-draw it since I'm too lazy
				 * to tell if a selected flag changed
				 */
				DrawElement(element);
				Draw();
				hid_actionl("Report", "Object", NULL);
			}
			else if (type != NO_TYPE) {
				TextTypePtr thing = (TextTypePtr) ptr3;
				TOGGLE_FLAG(LOCKFLAG, thing);
				if (TEST_FLAG(LOCKFLAG, thing)
						&& TEST_FLAG(SELECTEDFLAG, thing)) {
					/* this is not un-doable since LOCK isn't */
					CLEAR_FLAG(SELECTEDFLAG, thing);
					DrawObject(type, ptr1, ptr2);
					Draw();
				}
				hid_actionl("Report", "Object", NULL);
			}
			break;
		}
	case THERMAL_MODE:
		{
			if (((type = SearchScreen(Note.X, Note.Y, PIN_TYPES, &ptr1, &ptr2, &ptr3)) != NO_TYPE)
					&& !TEST_FLAG(HOLEFLAG, (PinTypePtr) ptr3)) {
				if (gui->shift_is_pressed()) {
					int tstyle = GET_THERM(INDEXOFCURRENT, (PinTypePtr) ptr3);
					tstyle++;
					if (tstyle > 5)
						tstyle = 1;
					ChangeObjectThermal(type, ptr1, ptr2, ptr3, tstyle);
				}
				else if (GET_THERM(INDEXOFCURRENT, (PinTypePtr) ptr3))
					ChangeObjectThermal(type, ptr1, ptr2, ptr3, 0);
				else
					ChangeObjectThermal(type, ptr1, ptr2, ptr3, PCB->ThermStyle);
			}
			break;
		}

	case LINE_MODE:
		/* do update of position */
		NotifyLine();
		if (Crosshair.AttachedLine.State != STATE_THIRD)
			break;

		/* Remove anchor if clicking on start point;
		 * this means we can't paint 0 length lines
		 * which could be used for square SMD pads.
		 * Instead use a very small delta, or change
		 * the file after saving.
		 */
		if (Crosshair.X == Crosshair.AttachedLine.Point1.X && Crosshair.Y == Crosshair.AttachedLine.Point1.Y) {
			SetMode(LINE_MODE);
			break;
		}

		if (PCB->RatDraw) {
			RatTypePtr line;
			if ((line = AddNet())) {
				addedLines++;
				AddObjectToCreateUndoList(RATLINE_TYPE, line, line, line);
				IncrementUndoSerialNumber();
				DrawRat(line);
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
				Draw();
			}
			break;
		}
		else
			/* create line if both ends are determined && length != 0 */
		{
			LineTypePtr line;
			int maybe_found_flag;

			if (PCB->Clipping
					&& Crosshair.AttachedLine.Point1.X ==
					Crosshair.AttachedLine.Point2.X
					&& Crosshair.AttachedLine.Point1.Y ==
					Crosshair.AttachedLine.Point2.Y
					&& (Crosshair.AttachedLine.Point2.X != Note.X || Crosshair.AttachedLine.Point2.Y != Note.Y)) {
				/* We will only need to paint the second line segment.
				   Since we only check for vias on the first segment,
				   swap them so the non-empty segment is the first segment. */
				Crosshair.AttachedLine.Point2.X = Note.X;
				Crosshair.AttachedLine.Point2.Y = Note.Y;
			}

			if (conf_core.editor.auto_drc
					&& !TEST_SILK_LAYER(CURRENT))
				maybe_found_flag = FOUNDFLAG;
			else
				maybe_found_flag = 0;

			if ((Crosshair.AttachedLine.Point1.X !=
					 Crosshair.AttachedLine.Point2.X || Crosshair.AttachedLine.Point1.Y != Crosshair.AttachedLine.Point2.Y)
					&& (line =
							CreateDrawnLineOnLayer(CURRENT,
																		 Crosshair.AttachedLine.Point1.X,
																		 Crosshair.AttachedLine.Point1.Y,
																		 Crosshair.AttachedLine.Point2.X,
																		 Crosshair.AttachedLine.Point2.Y,
																		 conf_core.design.line_thickness,
																		 2 * conf_core.design.clearance,
																		 MakeFlags(maybe_found_flag |
																							 (conf_core.editor.clear_line ? CLEARLINEFLAG : 0)))) != NULL) {
				PinTypePtr via;

				addedLines++;
				AddObjectToCreateUndoList(LINE_TYPE, CURRENT, line, line);
				DrawLine(CURRENT, line);
				/* place a via if vias are visible, the layer is
				   in a new group since the last line and there
				   isn't a pin already here */
				if (PCB->ViaOn && GetLayerGroupNumberByPointer(CURRENT) !=
						GetLayerGroupNumberByPointer(lastLayer) &&
						SearchObjectByLocation(PIN_TYPES, &ptr1, &ptr2, &ptr3,
																	 Crosshair.AttachedLine.Point1.X,
																	 Crosshair.AttachedLine.Point1.Y,
																	 conf_core.design.via_thickness / 2) ==
						NO_TYPE
						&& (via =
								CreateNewVia(PCB->Data,
														 Crosshair.AttachedLine.Point1.X,
														 Crosshair.AttachedLine.Point1.Y,
														 conf_core.design.via_thickness,
														 2 * conf_core.design.clearance, 0, conf_core.design.via_drilling_hole, NULL, NoFlags())) != NULL) {
					AddObjectToCreateUndoList(VIA_TYPE, via, via, via);
					DrawVia(via);
				}
				/* copy the coordinates */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
				IncrementUndoSerialNumber();
				lastLayer = CURRENT;
			}
			if (PCB->Clipping && (Note.X != Crosshair.AttachedLine.Point2.X || Note.Y != Crosshair.AttachedLine.Point2.Y)
					&& (line =
							CreateDrawnLineOnLayer(CURRENT,
																		 Crosshair.AttachedLine.Point2.X,
																		 Crosshair.AttachedLine.Point2.Y,
																		 Note.X, Note.Y,
																		 conf_core.design.line_thickness,
																		 2 * conf_core.design.clearance,
																		 MakeFlags((conf_core.editor.auto_drc ? FOUNDFLAG : 0) |
																							 (conf_core.editor.clear_line ? CLEARLINEFLAG : 0)))) != NULL) {
				addedLines++;
				AddObjectToCreateUndoList(LINE_TYPE, CURRENT, line, line);
				IncrementUndoSerialNumber();
				DrawLine(CURRENT, line);
				/* move to new start point */
				Crosshair.AttachedLine.Point1.X = Note.X;
				Crosshair.AttachedLine.Point1.Y = Note.Y;
				Crosshair.AttachedLine.Point2.X = Note.X;
				Crosshair.AttachedLine.Point2.Y = Note.Y;


				if (conf_core.editor.swap_start_direction) {
					PCB->Clipping ^= 3;
				}
			}
			if (conf_core.editor.orthogonal_moves) {
				/* set the mark to the new starting point so ortho works as expected and we can draw a perpendicular line from here */
				Marked.X = Note.X;
				Marked.Y = Note.Y;
			}
			Draw();
		}
		break;

	case RECTANGLE_MODE:
		/* do update of position */
		NotifyBlock();

		/* create rectangle if both corners are determined 
		 * and width, height are != 0
		 */
		if (Crosshair.AttachedBox.State == STATE_THIRD &&
				Crosshair.AttachedBox.Point1.X != Crosshair.AttachedBox.Point2.X &&
				Crosshair.AttachedBox.Point1.Y != Crosshair.AttachedBox.Point2.Y) {
			PolygonTypePtr polygon;

			int flags = CLEARPOLYFLAG;
			if (conf_core.editor.full_poly)
				flags |= FULLPOLYFLAG;
			if ((polygon = CreateNewPolygonFromRectangle(CURRENT,
																									 Crosshair.AttachedBox.Point1.X,
																									 Crosshair.AttachedBox.Point1.Y,
																									 Crosshair.AttachedBox.Point2.X,
																									 Crosshair.AttachedBox.Point2.Y, MakeFlags(flags))) != NULL) {
				AddObjectToCreateUndoList(POLYGON_TYPE, CURRENT, polygon, polygon);
				IncrementUndoSerialNumber();
				DrawPolygon(CURRENT, polygon);
				Draw();
			}

			/* reset state to 'first corner' */
			Crosshair.AttachedBox.State = STATE_FIRST;
		}
		break;

	case TEXT_MODE:
		{
			char *string;

			if ((string = gui->prompt_for(_("Enter text:"), "")) != NULL) {
				if (strlen(string) > 0) {
					TextTypePtr text;
					int flag = CLEARLINEFLAG;

					if (GetLayerGroupNumberByNumber(INDEXOFCURRENT) == GetLayerGroupNumberByNumber(solder_silk_layer))
						flag |= ONSOLDERFLAG;
					if ((text = CreateNewText(CURRENT, &PCB->Font, Note.X,
																		Note.Y, 0, conf_core.design.text_scale, string, MakeFlags(flag))) != NULL) {
						AddObjectToCreateUndoList(TEXT_TYPE, CURRENT, text, text);
						IncrementUndoSerialNumber();
						DrawText(CURRENT, text);
						Draw();
					}
				}
				free(string);
			}
			break;
		}

	case POLYGON_MODE:
		{
			PointTypePtr points = Crosshair.AttachedPolygon.Points;
			Cardinal n = Crosshair.AttachedPolygon.PointN;

			/* do update of position; use the 'LINE_MODE' mechanism */
			NotifyLine();

			/* check if this is the last point of a polygon */
			if (n >= 3 && points->X == Crosshair.AttachedLine.Point2.X && points->Y == Crosshair.AttachedLine.Point2.Y) {
				CopyAttachedPolygonToLayer();
				Draw();
				break;
			}

			/* create new point if it's the first one or if it's
			 * different to the last one
			 */
			if (!n || points[n - 1].X != Crosshair.AttachedLine.Point2.X || points[n - 1].Y != Crosshair.AttachedLine.Point2.Y) {
				CreateNewPointInPolygon(&Crosshair.AttachedPolygon, Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y);

				/* copy the coordinates */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
			}

			if (conf_core.editor.orthogonal_moves) {
				/* set the mark to the new starting point so ortho works */
				Marked.X = Note.X;
				Marked.Y = Note.Y;
			}

			break;
		}

	case POLYGONHOLE_MODE:
		{
			switch (Crosshair.AttachedObject.State) {
				/* first notify, lookup object */
			case STATE_FIRST:
				Crosshair.AttachedObject.Type =
					SearchScreen(Note.X, Note.Y, POLYGON_TYPE,
											 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);

				if (Crosshair.AttachedObject.Type == NO_TYPE) {
					Message("The first point of a polygon hole must be on a polygon.\n");
					break; /* don't start doing anything if clicket out of polys */
				}

				if (TEST_FLAG(LOCKFLAG, (PolygonTypePtr)
											Crosshair.AttachedObject.Ptr2)) {
					Message(_("Sorry, the object is locked\n"));
					Crosshair.AttachedObject.Type = NO_TYPE;
					break;
				}
				else
					Crosshair.AttachedObject.State = STATE_SECOND;
			/* fall thru: first click is also the first point of the poly hole */

				/* second notify, insert new point into object */
			case STATE_SECOND:
				{
					PointTypePtr points = Crosshair.AttachedPolygon.Points;
					Cardinal n = Crosshair.AttachedPolygon.PointN;
					POLYAREA *original, *new_hole, *result;
					FlagType Flags;

					/* do update of position; use the 'LINE_MODE' mechanism */
					NotifyLine();

					if (conf_core.editor.orthogonal_moves) {
						/* set the mark to the new starting point so ortho works */
						Marked.X = Note.X;
						Marked.Y = Note.Y;
					}

					/* check if this is the last point of a polygon */
					if (n >= 3 && points->X == Crosshair.AttachedLine.Point2.X && points->Y == Crosshair.AttachedLine.Point2.Y) {
						/* Create POLYAREAs from the original polygon
						 * and the new hole polygon */
						original = PolygonToPoly((PolygonType *) Crosshair.AttachedObject.Ptr2);
						new_hole = PolygonToPoly(&Crosshair.AttachedPolygon);

						/* Subtract the hole from the original polygon shape */
						poly_Boolean_free(original, new_hole, &result, PBO_SUB);

						/* Convert the resulting polygon(s) into a new set of nodes
						 * and place them on the page. Delete the original polygon.
						 */
						SaveUndoSerialNumber();
						Flags = ((PolygonType *) Crosshair.AttachedObject.Ptr2)->Flags;
						PolyToPolygonsOnLayer(PCB->Data, (LayerType *) Crosshair.AttachedObject.Ptr1, result, Flags);
						RemoveObject(POLYGON_TYPE,
												 Crosshair.AttachedObject.Ptr1, Crosshair.AttachedObject.Ptr2, Crosshair.AttachedObject.Ptr3);
						RestoreUndoSerialNumber();
						IncrementUndoSerialNumber();
						Draw();

						/* reset state of attached line */
						memset(&Crosshair.AttachedPolygon, 0, sizeof(PolygonType));
						Crosshair.AttachedLine.State = STATE_FIRST;
						Crosshair.AttachedObject.State = STATE_FIRST;
						addedLines = 0;

						break;
					}

					/* create new point if it's the first one or if it's
					 * different to the last one
					 */
					if (!n || points[n - 1].X != Crosshair.AttachedLine.Point2.X || points[n - 1].Y != Crosshair.AttachedLine.Point2.Y) {
						CreateNewPointInPolygon(&Crosshair.AttachedPolygon,
																		Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y);

						/* copy the coordinates */
						Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
						Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
					}
					break;
				}
			}
			break;
		}

	case PASTEBUFFER_MODE:
		{
			TextType estr[MAX_ELEMENTNAMES];
			ElementTypePtr e = 0;

			if (gui->shift_is_pressed()) {
				int type = SearchScreen(Note.X, Note.Y, ELEMENT_TYPE, &ptr1, &ptr2,
																&ptr3);
				if (type == ELEMENT_TYPE) {
					e = (ElementTypePtr) ptr1;
					if (e) {
						int i;

						memcpy(estr, e->Name, MAX_ELEMENTNAMES * sizeof(TextType));
						for (i = 0; i < MAX_ELEMENTNAMES; ++i)
							estr[i].TextString = estr[i].TextString ? strdup(estr[i].TextString) : NULL;
						RemoveElement(e);
					}
				}
			}
			if (CopyPastebufferToLayout(Note.X, Note.Y))
				SetChangedFlag(true);
			if (e) {
				int type = SearchScreen(Note.X, Note.Y, ELEMENT_TYPE, &ptr1, &ptr2,
																&ptr3);
				if (type == ELEMENT_TYPE && ptr1) {
					int i, save_n;
					e = (ElementTypePtr) ptr1;

					save_n = NAME_INDEX();

					for (i = 0; i < MAX_ELEMENTNAMES; i++) {
						if (i == save_n)
							EraseElementName(e);
						r_delete_entry(PCB->Data->name_tree[i], (BoxType *) & (e->Name[i]));
						memcpy(&(e->Name[i]), &(estr[i]), sizeof(TextType));
						e->Name[i].Element = e;
						SetTextBoundingBox(&PCB->Font, &(e->Name[i]));
						r_insert_entry(PCB->Data->name_tree[i], (BoxType *) & (e->Name[i]), 0);
						if (i == save_n)
							DrawElementName(e);
					}
				}
			}
			break;
		}

	case REMOVE_MODE:
		if ((type = SearchScreen(Note.X, Note.Y, REMOVE_TYPES, &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
			if (TEST_FLAG(LOCKFLAG, (LineTypePtr) ptr2)) {
				Message(_("Sorry, the object is locked\n"));
				break;
			}
			if (type == ELEMENT_TYPE) {
				RubberbandTypePtr ptr;
				int i;

				Crosshair.AttachedObject.RubberbandN = 0;
				LookupRatLines(type, ptr1, ptr2, ptr3);
				ptr = Crosshair.AttachedObject.Rubberband;
				for (i = 0; i < Crosshair.AttachedObject.RubberbandN; i++) {
					if (PCB->RatOn)
						EraseRat((RatTypePtr) ptr->Line);
					if (TEST_FLAG(RUBBERENDFLAG, ptr->Line))
						MoveObjectToRemoveUndoList(RATLINE_TYPE, ptr->Line, ptr->Line, ptr->Line);
					else
						TOGGLE_FLAG(RUBBERENDFLAG, ptr->Line);	/* only remove line once */
					ptr++;
				}
			}
			RemoveObject(type, ptr1, ptr2, ptr3);
			IncrementUndoSerialNumber();
			SetChangedFlag(true);
		}
		break;

	case ROTATE_MODE:
		RotateScreenObject(Note.X, Note.Y, gui->shift_is_pressed()? (SWAP_IDENT ? 1 : 3)
											 : (SWAP_IDENT ? 3 : 1));
		break;

		/* both are almost the same */
	case COPY_MODE:
	case MOVE_MODE:
pcb_trace("Move/copy: mode=%d state=%d {\n", conf_core.editor.mode, Crosshair.AttachedObject.State);
		switch (Crosshair.AttachedObject.State) {
			/* first notify, lookup object */
		case STATE_FIRST:
			{
				int types = (conf_core.editor.mode == COPY_MODE) ? COPY_TYPES : MOVE_TYPES;

				Crosshair.AttachedObject.Type =
					SearchScreen(Note.X, Note.Y, types,
											 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);
				if (Crosshair.AttachedObject.Type != NO_TYPE) {
					if (conf_core.editor.mode == MOVE_MODE && TEST_FLAG(LOCKFLAG, (PinTypePtr)
																											Crosshair.AttachedObject.Ptr2)) {
						Message(_("Sorry, the object is locked\n"));
						Crosshair.AttachedObject.Type = NO_TYPE;
					}
					else
						AttachForCopy(Note.X, Note.Y);
				}
				break;
			}

			/* second notify, move or copy object */
		case STATE_SECOND:
			if (conf_core.editor.mode == COPY_MODE)
				CopyObject(Crosshair.AttachedObject.Type,
									 Crosshair.AttachedObject.Ptr1,
									 Crosshair.AttachedObject.Ptr2,
									 Crosshair.AttachedObject.Ptr3, Note.X - Crosshair.AttachedObject.X, Note.Y - Crosshair.AttachedObject.Y);
			else {
				MoveObjectAndRubberband(Crosshair.AttachedObject.Type,
																Crosshair.AttachedObject.Ptr1,
																Crosshair.AttachedObject.Ptr2,
																Crosshair.AttachedObject.Ptr3,
																Note.X - Crosshair.AttachedObject.X, Note.Y - Crosshair.AttachedObject.Y);
				SetLocalRef(0, 0, false);
			}
			SetChangedFlag(true);

			/* reset identifiers */
			Crosshair.AttachedObject.Type = NO_TYPE;
			Crosshair.AttachedObject.State = STATE_FIRST;
			break;
		}
		pcb_trace("}\n");
		break;

		/* insert a point into a polygon/line/... */
	case INSERTPOINT_MODE:
		switch (Crosshair.AttachedObject.State) {
			/* first notify, lookup object */
		case STATE_FIRST:
			Crosshair.AttachedObject.Type =
				SearchScreen(Note.X, Note.Y, INSERT_TYPES,
										 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);

			if (Crosshair.AttachedObject.Type != NO_TYPE) {
				if (TEST_FLAG(LOCKFLAG, (PolygonTypePtr)
											Crosshair.AttachedObject.Ptr2)) {
					Message(_("Sorry, the object is locked\n"));
					Crosshair.AttachedObject.Type = NO_TYPE;
					break;
				}
				else {
					/* get starting point of nearest segment */
					if (Crosshair.AttachedObject.Type == POLYGON_TYPE) {
						fake.poly = (PolygonTypePtr) Crosshair.AttachedObject.Ptr2;
						polyIndex = GetLowestDistancePolygonPoint(fake.poly, Note.X, Note.Y);
						fake.line.Point1 = fake.poly->Points[polyIndex];
						fake.line.Point2 = fake.poly->Points[prev_contour_point(fake.poly, polyIndex)];
						Crosshair.AttachedObject.Ptr2 = &fake.line;

					}
					Crosshair.AttachedObject.State = STATE_SECOND;
					InsertedPoint = *AdjustInsertPoint();
				}
			}
			break;

			/* second notify, insert new point into object */
		case STATE_SECOND:
			if (Crosshair.AttachedObject.Type == POLYGON_TYPE)
				InsertPointIntoObject(POLYGON_TYPE,
															Crosshair.AttachedObject.Ptr1, fake.poly,
															&polyIndex, InsertedPoint.X, InsertedPoint.Y, false, false);
			else
				InsertPointIntoObject(Crosshair.AttachedObject.Type,
															Crosshair.AttachedObject.Ptr1,
															Crosshair.AttachedObject.Ptr2, &polyIndex, InsertedPoint.X, InsertedPoint.Y, false, false);
			SetChangedFlag(true);

			/* reset identifiers */
			Crosshair.AttachedObject.Type = NO_TYPE;
			Crosshair.AttachedObject.State = STATE_FIRST;
			break;
		}
		break;
	}
}

/* ---------------------------------------------------------------------------
 * !!! no action routine !!!
 *
 * event handler to set the cursor according to the X pointer position
 * called from inside main.c
 */
void EventMoveCrosshair(int ev_x, int ev_y)
{
	if (mid_stroke)
		stub_stroke_record(ev_x, ev_y);
	if (MoveCrosshairAbsolute(ev_x, ev_y)) {
		/* update object position and cursor location */
		AdjustAttachedObjects();
		notify_crosshair_change(true);
	}
}

/* --------------------------------------------------------------------------- */
/* helper: get route style size for a function and selected object type.
   size_id: 0=main size; 1=2nd size (drill); 2=clearance */
int get_style_size(int funcid, Coord * out, int type, int size_id)
{
	switch (funcid) {
	case F_Object:
		switch (type) {
		case ELEMENT_TYPE:					/* we'd set pin/pad properties, so fall thru */
		case VIA_TYPE:
		case PIN_TYPE:
			return get_style_size(F_SelectedVias, out, 0, size_id);
		case PAD_TYPE:
			return get_style_size(F_SelectedPads, out, 0, size_id);
		case LINE_TYPE:
			return get_style_size(F_SelectedLines, out, 0, size_id);
		case ARC_TYPE:
			return get_style_size(F_SelectedArcs, out, 0, size_id);
		}
		Message(_("Sorry, can't fetch the style of that object tpye (%x)\n"), type);
		return -1;
	case F_SelectedPads:
		if (size_id != 2)						/* don't mess with pad size */
			return -1;
	case F_SelectedVias:
	case F_SelectedPins:
	case F_SelectedObjects:
	case F_Selected:
	case F_SelectedElements:
		if (size_id == 0)
			*out = conf_core.design.via_thickness;
		else if (size_id == 1)
			*out = conf_core.design.via_drilling_hole;
		else
			*out = conf_core.design.clearance;
		break;
	case F_SelectedArcs:
	case F_SelectedLines:
		if (size_id == 2)
			*out = conf_core.design.clearance;
		else
			*out = conf_core.design.line_thickness;
		return 0;
	case F_SelectedTexts:
	case F_SelectedNames:
		Message(_("Sorry, can't change style of every selected object\n"));
		return -1;
	}
	return 0;
}

