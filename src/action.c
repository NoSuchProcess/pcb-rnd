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

#include "global.h"

#include "action.h"
#include "buffer.h"
#include "change.h"
#include "command.h"
#include "copy.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "file.h"
#include "find.h"
#include "hid.h"
#include "insert.h"
#include "line.h"
#include "mymem.h"
#include "misc.h"
#include "mirror.h"
#include "move.h"
#include "polygon.h"
/*#include "print.h"*/
#include "rats.h"
#include "remove.h"
#include "report.h"
#include "rotate.h"
#include "rubberband.h"
#include "search.h"
#include "select.h"
#include "set.h"
#include "thermal.h"
#include "undo.h"
#include "rtree.h"
#include "macro.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "rats_patch.h"
#include "portability.h"

#include <assert.h>
#include <stdlib.h>							/* rand() */


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
static bool saved_mode = false;
#ifdef HAVE_LIBSTROKE
static bool mid_stroke = false;
static BoxType StrokeBox;
#endif

/* ---------------------------------------------------------------------------
 * some local routines
 */
static void AdjustAttachedBox(void);
#ifdef HAVE_LIBSTROKE
static void FinishStroke(void);
extern void stroke_init(void);
extern void stroke_record(int x, int y);
extern int stroke_trans(char *s);
#endif

#ifdef HAVE_LIBSTROKE

/* ---------------------------------------------------------------------------
 * FinishStroke - try to recognize the stroke sent
 */
void FinishStroke(void)
{
	char msg[255];
	int type;
	unsigned long num;
	void *ptr1, *ptr2, *ptr3;

	mid_stroke = false;
	if (stroke_trans(msg)) {
		num = atoi(msg);
		switch (num) {
		case 456:
			if (Settings.Mode == LINE_MODE) {
				SetMode(LINE_MODE);
			}
			break;
		case 9874123:
		case 74123:
		case 987412:
		case 8741236:
		case 874123:
			RotateScreenObject(StrokeBox.X1, StrokeBox.Y1, SWAP_IDENT ? 1 : 3);
			break;
		case 7896321:
		case 786321:
		case 789632:
		case 896321:
			RotateScreenObject(StrokeBox.X1, StrokeBox.Y1, SWAP_IDENT ? 3 : 1);
			break;
		case 258:
			SetMode(LINE_MODE);
			break;
		case 852:
			SetMode(ARROW_MODE);
			break;
		case 1478963:
			ActionUndo("");
			break;
		case 147423:
		case 147523:
		case 1474123:
			Redo(true);
			break;
		case 148963:
		case 147863:
		case 147853:
		case 145863:
			SetMode(VIA_MODE);
			break;
		case 951:
		case 9651:
		case 9521:
		case 9621:
		case 9851:
		case 9541:
		case 96521:
		case 96541:
		case 98541:
			SetZoom(1000);						/* special zoom extents */
			break;
		case 159:
		case 1269:
		case 1259:
		case 1459:
		case 1569:
		case 1589:
		case 12569:
		case 12589:
		case 14589:
			{
				Coord x = (StrokeBox.X1 + StrokeBox.X2) / 2;
				Coord y = (StrokeBox.Y1 + StrokeBox.Y2) / 2;
				double z;
				/* XXX: PCB->MaxWidth and PCB->MaxHeight may be the wrong
				 *      divisors below. The old code WAS broken, but this
				 *      replacement has not been tested for correctness.
				 */
				z = 1 + log(fabs(StrokeBox.X2 - StrokeBox.X1) / PCB->MaxWidth) / log(2.0);
				z = MAX(z, 1 + log(fabs(StrokeBox.Y2 - StrokeBox.Y1) / PCB->MaxHeight) / log(2.0));
				SetZoom(z);

				CenterDisplay(x, y);
				break;
			}

		default:
			Message(_("Unknown stroke %s\n"), msg);
			break;
		}
	}
	else
		gui->beep();
}
#endif

/* ---------------------------------------------------------------------------
 * Clear warning color from pins/pads
 */
void ClearWarnings()
{
	Settings.RatWarn = false;
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
			Note.Buffer = Settings.BufferNumber;
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

static void ReleaseMode(void)
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
	else if (Settings.Mode == ARROW_MODE) {
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
	if (Settings.Mode == ARC_MODE) {
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
	switch (Settings.Mode) {
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

	if (!Marked.status || TEST_FLAG(LOCALREFFLAG, PCB))
		SetLocalRef(Crosshair.X, Crosshair.Y, true);
	switch (Crosshair.AttachedLine.State) {
	case STATE_FIRST:						/* first point */
		if (PCB->RatDraw && SearchScreen(Crosshair.X, Crosshair.Y, PAD_TYPE | PIN_TYPE, &ptr1, &ptr1, &ptr1) == NO_TYPE) {
			gui->beep();
			break;
		}
		if (TEST_FLAG(AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE) {
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

	if (Settings.RatWarn)
		ClearWarnings();
	switch (Settings.Mode) {
	case ARROW_MODE:
		{
			int test;
			hidval hv;

			Note.Click = true;
			/* do something after click time */
			gui->add_timer(click_cb, CLICK_TIME, hv);

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
															Settings.ViaThickness, 2 * Settings.Keepaway,
															0, Settings.ViaDrillingHole, NULL, NoFlags())) != NULL) {
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
																												Settings.LineThickness,
																												2 * Settings.Keepaway,
																												MakeFlags(TEST_FLAG(CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)))) {
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

			if (TEST_FLAG(AUTODRCFLAG, PCB)
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
																		 Settings.LineThickness,
																		 2 * Settings.Keepaway,
																		 MakeFlags(maybe_found_flag |
																							 (TEST_FLAG(CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)))) != NULL) {
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
																	 Settings.ViaThickness / 2) ==
						NO_TYPE
						&& (via =
								CreateNewVia(PCB->Data,
														 Crosshair.AttachedLine.Point1.X,
														 Crosshair.AttachedLine.Point1.Y,
														 Settings.ViaThickness,
														 2 * Settings.Keepaway, 0, Settings.ViaDrillingHole, NULL, NoFlags())) != NULL) {
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
																		 Settings.LineThickness,
																		 2 * Settings.Keepaway,
																		 MakeFlags((TEST_FLAG
																								(AUTODRCFLAG,
																								 PCB) ? FOUNDFLAG : 0) |
																							 (TEST_FLAG(CLEARNEWFLAG, PCB) ? CLEARLINEFLAG : 0)))) != NULL) {
				addedLines++;
				AddObjectToCreateUndoList(LINE_TYPE, CURRENT, line, line);
				IncrementUndoSerialNumber();
				DrawLine(CURRENT, line);
				/* move to new start point */
				Crosshair.AttachedLine.Point1.X = Note.X;
				Crosshair.AttachedLine.Point1.Y = Note.Y;
				Crosshair.AttachedLine.Point2.X = Note.X;
				Crosshair.AttachedLine.Point2.Y = Note.Y;
				if (TEST_FLAG(SWAPSTARTDIRFLAG, PCB)) {
					PCB->Clipping ^= 3;
				}
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
			if (TEST_FLAG(NEWFULLPOLYFLAG, PCB))
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
																		Note.Y, 0, Settings.TextScale, string, MakeFlags(flag))) != NULL) {
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

				if (Crosshair.AttachedObject.Type != NO_TYPE) {
					if (TEST_FLAG(LOCKFLAG, (PolygonTypePtr)
												Crosshair.AttachedObject.Ptr2)) {
						Message(_("Sorry, the object is locked\n"));
						Crosshair.AttachedObject.Type = NO_TYPE;
						break;
					}
					else
						Crosshair.AttachedObject.State = STATE_SECOND;
				}
				break;

				/* second notify, insert new point into object */
			case STATE_SECOND:
				{
					PointTypePtr points = Crosshair.AttachedPolygon.Points;
					Cardinal n = Crosshair.AttachedPolygon.PointN;
					POLYAREA *original, *new_hole, *result;
					FlagType Flags;

					/* do update of position; use the 'LINE_MODE' mechanism */
					NotifyLine();

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

					save_n = NAME_INDEX(PCB);

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
		switch (Crosshair.AttachedObject.State) {
			/* first notify, lookup object */
		case STATE_FIRST:
			{
				int types = (Settings.Mode == COPY_MODE) ? COPY_TYPES : MOVE_TYPES;

				Crosshair.AttachedObject.Type =
					SearchScreen(Note.X, Note.Y, types,
											 &Crosshair.AttachedObject.Ptr1, &Crosshair.AttachedObject.Ptr2, &Crosshair.AttachedObject.Ptr3);
				if (Crosshair.AttachedObject.Type != NO_TYPE) {
					if (Settings.Mode == MOVE_MODE && TEST_FLAG(LOCKFLAG, (PinTypePtr)
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
			if (Settings.Mode == COPY_MODE)
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


/* -------------------------------------------------------------------------- */

static const char drc_syntax[] = "DRC()";

static const char drc_help[] = "Invoke the DRC check.";

/* %start-doc actions DRC

Note that the design rule check uses the current board rule settings,
not the current style settings.

%end-doc */

static int ActionDRCheck(int argc, char **argv, Coord x, Coord y)
{
	int count;

	if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview) {
		Message(_("%m+Rules are minspace %$mS, minoverlap %$mS "
							"minwidth %$mS, minsilk %$mS\n"
							"min drill %$mS, min annular ring %$mS\n"),
						Settings.grid_unit->allow, PCB->Bloat, PCB->Shrink, PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
	}
	count = DRCAll();
	if (gui->drc_gui == NULL || gui->drc_gui->log_drc_overview) {
		if (count == 0)
			Message(_("No DRC problems found.\n"));
		else if (count > 0)
			Message(_("Found %d design rule errors.\n"), count);
		else
			Message(_("Aborted DRC after %d design rule errors.\n"), -count);
	}
	return 0;
}

/* ---------------------------------------------------------------- */
static const char cycledrag_syntax[] = "CycleDrag()\n";

static const char cycledrag_help[] = "Cycle through which object is being dragged";

#define close_enough(a, b) ((((a)-(b)) > 0) ? ((a)-(b) < (SLOP * pixel_slop)) : ((a)-(b) > -(SLOP * pixel_slop)))
static int CycleDrag(int argc, char **argv, Coord x, Coord y)
{
	void *ptr1, *ptr2, *ptr3;
	int over = 0;

	if (Crosshair.drags == NULL)
		return NULL;

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

static const char dumplibrary_syntax[] = "DumpLibrary()";

static const char dumplibrary_help[] = "Display the entire contents of the libraries.";

/* %start-doc actions DumpLibrary


%end-doc */

static int ActionDumpLibrary(int argc, char **argv, Coord x, Coord y)
{
	int i, j;

	printf("**** Do not count on this format.  It will change ****\n\n");
	printf("MenuN   = %d\n", Library.MenuN);
	printf("MenuMax = %d\n", Library.MenuMax);
	for (i = 0; i < Library.MenuN; i++) {
		printf("Library #%d:\n", i);
		printf("    EntryN    = %d\n", Library.Menu[i].EntryN);
		printf("    EntryMax  = %d\n", Library.Menu[i].EntryMax);
		printf("    Name      = \"%s\"\n", UNKNOWN(Library.Menu[i].Name));
		printf("    directory = \"%s\"\n", UNKNOWN(Library.Menu[i].directory));
		printf("    Style     = \"%s\"\n", UNKNOWN(Library.Menu[i].Style));
		printf("    flag      = %d\n", Library.Menu[i].flag);

		for (j = 0; j < Library.Menu[i].EntryN; j++) {
			printf("    #%4d: ", j);
			printf("newlib: \"%s\"\n", UNKNOWN(Library.Menu[i].Entry[j].ListEntry));
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */

static const char flip_syntax[] = "Flip(Object|Selected|SelectedElements)";

static const char flip_help[] = "Flip an element to the opposite side of the board.";

/* %start-doc actions Flip

Note that the location of the element will be symmetric about the
cursor location; i.e. if the part you are pointing at will still be at
the same spot once the element is on the other side.  When flipping
multiple elements, this retains their positions relative to each
other, not their absolute positions on the board.

%end-doc */

static int ActionFlip(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	ElementTypePtr element;
	void *ptrtmp;
	int err = 0;

	if (function) {
		switch (GetFunctionID(function)) {
		case F_Object:
			if ((SearchScreen(x, y, ELEMENT_TYPE, &ptrtmp, &ptrtmp, &ptrtmp)) != NO_TYPE) {
				element = (ElementTypePtr) ptrtmp;
				ChangeElementSide(element, 2 * Crosshair.Y - PCB->MaxHeight);
				IncrementUndoSerialNumber();
				Draw();
			}
			break;
		case F_Selected:
		case F_SelectedElements:
			ChangeSelectedElementSide();
			break;
		default:
			err = 1;
			break;
		}
		if (!err)
			return 0;
	}

	AFAIL(flip);
}

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



/* ---------------------------------------------------------------------------
 * !!! no action routine !!!
 *
 * event handler to set the cursor according to the X pointer position
 * called from inside main.c
 */
void EventMoveCrosshair(int ev_x, int ev_y)
{
#ifdef HAVE_LIBSTROKE
	if (mid_stroke) {
		StrokeBox.X2 = ev_x;
		StrokeBox.Y2 = ev_y;
		stroke_record(ev_x, ev_y);
		return;
	}
#endif /* HAVE_LIBSTROKE */
	if (MoveCrosshairAbsolute(ev_x, ev_y)) {
		/* update object position and cursor location */
		AdjustAttachedObjects();
		notify_crosshair_change(true);
	}
}

/* --------------------------------------------------------------------------- */

static const char disperseelements_syntax[] = "DisperseElements(All|Selected)";

static const char disperseelements_help[] = "Disperses elements.";

/* %start-doc actions DisperseElements

Normally this is used when starting a board, by selecting all elements
and then dispersing them.  This scatters the elements around the board
so that you can pick individual ones, rather than have all the
elements at the same 0,0 coordinate and thus impossible to choose
from.

%end-doc */

#define GAP MIL_TO_COORD(100)

static int ActionDisperseElements(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	Coord minx = GAP, miny = GAP, maxy = GAP, dx, dy;
	int all = 0, bad = 0;

	if (!function || !*function) {
		bad = 1;
	}
	else {
		switch (GetFunctionID(function)) {
		case F_All:
			all = 1;
			break;

		case F_Selected:
			all = 0;
			break;

		default:
			bad = 1;
		}
	}

	if (bad) {
		AFAIL(disperseelements);
	}


	ELEMENT_LOOP(PCB->Data);
	{
		/* 
		 * If we want to disperse selected elements, maybe we need smarter
		 * code here to avoid putting components on top of others which
		 * are not selected.  For now, I'm assuming that this is typically
		 * going to be used either with a brand new design or a scratch
		 * design holding some new components
		 */
		if (!TEST_FLAG(LOCKFLAG, element) && (all || TEST_FLAG(SELECTEDFLAG, element))) {

			/* figure out how much to move the element */
			dx = minx - element->BoundingBox.X1;

			/* snap to the grid */
			dx -= (element->MarkX + dx) % PCB->Grid;

			/* 
			 * and add one grid size so we make sure we always space by GAP or
			 * more
			 */
			dx += PCB->Grid;

			/* Figure out if this row has room.  If not, start a new row */
			if (GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth) {
				miny = maxy + GAP;
				minx = GAP;
			}

			/* figure out how much to move the element */
			dx = minx - element->BoundingBox.X1;
			dy = miny - element->BoundingBox.Y1;

			/* snap to the grid */
			dx -= (element->MarkX + dx) % PCB->Grid;
			dx += PCB->Grid;
			dy -= (element->MarkY + dy) % PCB->Grid;
			dy += PCB->Grid;

			/* move the element */
			MoveElementLowLevel(PCB->Data, element, dx, dy);

			/* and add to the undo list so we can undo this operation */
			AddObjectToMoveUndoList(ELEMENT_TYPE, NULL, NULL, element, dx, dy);

			/* keep track of how tall this row is */
			minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
			if (maxy < element->BoundingBox.Y2) {
				maxy = element->BoundingBox.Y2;
			}
		}

	}
	END_LOOP;

	/* done with our action so increment the undo # */
	IncrementUndoSerialNumber();

	Redraw();
	SetChangedFlag(true);

	return 0;
}

#undef GAP

/* --------------------------------------------------------------------------- */

static const char display_syntax[] =
	"Display(NameOnPCB|Description|Value)\n"
	"Display(Grid|Redraw)\n"
	"Display(CycleClip|CycleCrosshair|Toggle45Degree|ToggleStartDirection)\n"
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

@item Toggle45Degree
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

	function = ARG(0);
	str_dir = ARG(1);

	if (function && (!str_dir || !*str_dir)) {
		switch (id = GetFunctionID(function)) {

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

		case F_ToggleMinCut:
			TOGGLE_FLAG(ENABLEMINCUTFLAG, PCB);
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
			if (TEST_FLAG(AUTODRCFLAG, PCB) && Settings.Mode == LINE_MODE) {
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
			Settings.DrawGrid = !Settings.DrawGrid;
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

				switch (SearchScreen(Crosshair.X, Crosshair.Y,
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
		switch (GetFunctionID(function)) {
		case F_ToggleGrid:
			if (argc > 2) {
				PCB->GridOffsetX = GetValue(argv[1], NULL, NULL);
				PCB->GridOffsetY = GetValue(argv[2], NULL, NULL);
				if (Settings.DrawGrid)
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
	char *function = ARG(0);

	if (function) {
		Note.X = Crosshair.X;
		Note.Y = Crosshair.Y;
		notify_crosshair_change(false);
		switch (GetFunctionID(function)) {
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
				int saved_mode = Settings.Mode;
				SetMode(NO_MODE);
				SetMode(saved_mode);
			}
			break;
		case F_Escape:
			{
				switch (Settings.Mode) {
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
#ifndef HAVE_LIBSTROKE
		case F_Release:
			ReleaseMode();
			break;
#else
		case F_Release:
			if (mid_stroke)
				FinishStroke();
			else
				ReleaseMode();
			break;
#endif
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
#ifdef HAVE_LIBSTROKE
			mid_stroke = true;
			StrokeBox.X1 = Crosshair.X;
			StrokeBox.Y1 = Crosshair.Y;
			break;
#else
			/* Handle middle mouse button restarts of drawing mode.  If not in
			   |  a drawing mode, middle mouse button will select objects.
			 */
			if (Settings.Mode == LINE_MODE && Crosshair.AttachedLine.State != STATE_FIRST) {
				SetMode(LINE_MODE);
			}
			else if (Settings.Mode == ARC_MODE && Crosshair.AttachedBox.State != STATE_FIRST)
				SetMode(ARC_MODE);
			else if (Settings.Mode == RECTANGLE_MODE && Crosshair.AttachedBox.State != STATE_FIRST)
				SetMode(RECTANGLE_MODE);
			else if (Settings.Mode == POLYGON_MODE && Crosshair.AttachedLine.State != STATE_FIRST)
				SetMode(POLYGON_MODE);
			else {
				SaveMode();
				saved_mode = true;
				SetMode(ARROW_MODE);
				NotifyMode();
			}
			break;
#endif
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

/* --------------------------------------------------------------------------- */

static const char removeselected_syntax[] = "RemoveSelected()";

static const char removeselected_help[] = "Removes any selected objects.";

/* %start-doc actions RemoveSelected

%end-doc */

static int ActionRemoveSelected(int argc, char **argv, Coord x, Coord y)
{
	if (RemoveSelected())
		SetChangedFlag(true);
	return 0;
}

#ifdef BA_TODO
/* --------------------------------------------------------------------------- */

static const char renumber_syntax[] = "Renumber()\n" "Renumber(filename)";

static const char renumber_help[] =
	"Renumber all elements.  The changes will be recorded to filename\n"
	"for use in backannotating these changes to the schematic.";

/* %start-doc actions Renumber

%end-doc */

static int ActionRenumber(int argc, char **argv, Coord x, Coord y)
{
	bool changed = false;
	ElementTypePtr *element_list;
	ElementTypePtr *locked_element_list;
	unsigned int i, j, k, cnt, lock_cnt;
	unsigned int tmpi;
	size_t sz;
	char *tmps;
	char *name;
	FILE *out;
	static char *default_file = NULL;
	size_t cnt_list_sz = 100;
	struct _cnt_list {
		char *name;
		unsigned int cnt;
	} *cnt_list;
	char **was, **is, *pin;
	unsigned int c_cnt = 0;
	int unique, ok;
	int free_name = 0;

	if (argc < 1) {
		/*
		 * We deal with the case where name already exists in this
		 * function so the GUI doesn't need to deal with it 
		 */
		name = gui->fileselect(_("Save Renumber Annotation File As ..."),
													 _("Choose a file to record the renumbering to.\n"
														 "This file may be used to back annotate the\n"
														 "change to the schematics.\n"), default_file, ".eco", "eco", 0);

		free_name = 1;
	}
	else
		name = argv[0];

	if (default_file) {
		free(default_file);
		default_file = NULL;
	}

	if (name && *name) {
		default_file = strdup(name);
	}

	if ((out = fopen(name, "r"))) {
		fclose(out);
		if (!gui->confirm_dialog(_("File exists!  Ok to overwrite?"), 0)) {
			if (free_name && name)
				free(name);
			return 0;
		}
	}

	if ((out = fopen(name, "w")) == NULL) {
		Message(_("Could not open %s\n"), name);
		if (free_name && name)
			free(name);
		return 1;
	}

	if (free_name && name)
		free(name);

	fprintf(out, "*COMMENT* PCB Annotation File\n");
	fprintf(out, "*FILEVERSION* 20061031\n");

	/*
	 * Make a first pass through all of the elements and sort them out
	 * by location on the board.  While here we also collect a list of
	 * locked elements.
	 *
	 * We'll actually renumber things in the 2nd pass.
	 */
	element_list = (ElementType **) calloc(PCB->Data->ElementN, sizeof(ElementTypePtr));
	locked_element_list = (ElementType **) calloc(PCB->Data->ElementN, sizeof(ElementTypePtr));
	was = (char **) calloc(PCB->Data->ElementN, sizeof(char *));
	is = (char **) calloc(PCB->Data->ElementN, sizeof(char *));
	if (element_list == NULL || locked_element_list == NULL || was == NULL || is == NULL) {
		fprintf(stderr, "calloc() failed in %s\n", __FUNCTION__);
		exit(1);
	}


	cnt = 0;
	lock_cnt = 0;
	ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(LOCKFLAG, element->Name) || TEST_FLAG(LOCKFLAG, element)) {
			/* 
			 * add to the list of locked elements which we won't try to
			 * renumber and whose reference designators are now reserved.
			 */
			pcb_fprintf(out,
									"*WARN* Element \"%s\" at %$md is locked and will not be renumbered.\n",
									UNKNOWN(NAMEONPCB_NAME(element)), element->MarkX, element->MarkY);
			locked_element_list[lock_cnt] = element;
			lock_cnt++;
		}

		else {
			/* count of devices which will be renumbered */
			cnt++;

			/* search for correct position in the list */
			i = 0;
			while (element_list[i] && element->MarkY > element_list[i]->MarkY)
				i++;

			/* 
			 * We have found the position where we have the first element that
			 * has the same Y value or a lower Y value.  Now move forward if
			 * needed through the X values
			 */
			while (element_list[i]
						 && element->MarkY == element_list[i]->MarkY && element->MarkX > element_list[i]->MarkX)
				i++;

			for (j = cnt - 1; j > i; j--) {
				element_list[j] = element_list[j - 1];
			}
			element_list[i] = element;
		}
	}
	END_LOOP;


	/* 
	 * Now that the elements are sorted by board position, we go through
	 * and renumber them.
	 */

	/* 
	 * turn off the flag which requires unique names so it doesn't get
	 * in our way.  When we're done with the renumber we will have unique
	 * names.
	 */
	unique = TEST_FLAG(UNIQUENAMEFLAG, PCB);
	CLEAR_FLAG(UNIQUENAMEFLAG, PCB);

	cnt_list = (struct _cnt_list *) calloc(cnt_list_sz, sizeof(struct _cnt_list));
	for (i = 0; i < cnt; i++) {
		/* If there is no refdes, maybe just spit out a warning */
		if (NAMEONPCB_NAME(element_list[i])) {
			/* figure out the prefix */
			tmps = strdup(NAMEONPCB_NAME(element_list[i]));
			j = 0;
			while (tmps[j] && (tmps[j] < '0' || tmps[j] > '9')
						 && tmps[j] != '?')
				j++;
			tmps[j] = '\0';

			/* check the counter for this prefix */
			for (j = 0; cnt_list[j].name && (strcmp(cnt_list[j].name, tmps) != 0)
					 && j < cnt_list_sz; j++);

			/* grow the list if needed */
			if (j == cnt_list_sz) {
				cnt_list_sz += 100;
				cnt_list = (struct _cnt_list *) realloc(cnt_list, cnt_list_sz);
				if (cnt_list == NULL) {
					fprintf(stderr, "realloc failed() in %s\n", __FUNCTION__);
					exit(1);
				}
				/* zero out the memory that we added */
				for (tmpi = j; tmpi < cnt_list_sz; tmpi++) {
					cnt_list[tmpi].name = NULL;
					cnt_list[tmpi].cnt = 0;
				}
			}

			/* 
			 * start a new counter if we don't have a counter for this
			 * prefix 
			 */
			if (!cnt_list[j].name) {
				cnt_list[j].name = strdup(tmps);
				cnt_list[j].cnt = 0;
			}

			/*
			 * check to see if the new refdes is already used by a
			 * locked element
			 */
			do {
				ok = 1;
				cnt_list[j].cnt++;
				free(tmps);

				/* space for the prefix plus 1 digit plus the '\0' */
				sz = strlen(cnt_list[j].name) + 2;

				/* and 1 more per extra digit needed to hold the number */
				tmpi = cnt_list[j].cnt;
				while (tmpi > 10) {
					sz++;
					tmpi = tmpi / 10;
				}
				tmps = (char *) malloc(sz * sizeof(char));
				sprintf(tmps, "%s%d", cnt_list[j].name, cnt_list[j].cnt);

				/* 
				 * now compare to the list of reserved (by locked
				 * elements) names 
				 */
				for (k = 0; k < lock_cnt; k++) {
					if (strcmp(UNKNOWN(NAMEONPCB_NAME(locked_element_list[k])), tmps) == 0) {
						ok = 0;
						break;
					}
				}

			}
			while (!ok);

			if (strcmp(tmps, NAMEONPCB_NAME(element_list[i])) != 0) {
				fprintf(out, "*RENAME* \"%s\" \"%s\"\n", NAMEONPCB_NAME(element_list[i]), tmps);

				/* add this rename to our table of renames so we can update the netlist */
				was[c_cnt] = strdup(NAMEONPCB_NAME(element_list[i]));
				is[c_cnt] = strdup(tmps);
				c_cnt++;

				AddObjectToChangeNameUndoList(ELEMENT_TYPE, NULL, NULL, element_list[i], NAMEONPCB_NAME(element_list[i]));

				ChangeObjectName(ELEMENT_TYPE, element_list[i], NULL, NULL, tmps);
				changed = true;

				/* we don't free tmps in this case because it is used */
			}
			else
				free(tmps);
		}
		else {
			pcb_fprintf(out, "*WARN* Element at %$md has no name.\n", element_list[i]->MarkX, element_list[i]->MarkY);
		}

	}

	fclose(out);

	/* restore the unique flag setting */
	if (unique)
		SET_FLAG(UNIQUENAMEFLAG, PCB);

	if (changed) {

		/* update the netlist */
		AddNetlistLibToUndoList(&(PCB->NetlistLib));

		/* iterate over each net */
		for (i = 0; i < PCB->NetlistLib.MenuN; i++) {

			/* iterate over each pin on the net */
			for (j = 0; j < PCB->NetlistLib.Menu[i].EntryN; j++) {

				/* figure out the pin number part from strings like U3-21 */
				tmps = strdup(PCB->NetlistLib.Menu[i].Entry[j].ListEntry);
				for (k = 0; tmps[k] && tmps[k] != '-'; k++);
				tmps[k] = '\0';
				pin = tmps + k + 1;

				/* iterate over the list of changed reference designators */
				for (k = 0; k < c_cnt; k++) {
					/*
					 * if the pin needs to change, change it and quit
					 * searching in the list. 
					 */
					if (strcmp(tmps, was[k]) == 0) {
						free(PCB->NetlistLib.Menu[i].Entry[j].ListEntry);
						PCB->NetlistLib.Menu[i].Entry[j].ListEntry = (char *) malloc((strlen(is[k]) + strlen(pin) + 2) * sizeof(char));
						sprintf(PCB->NetlistLib.Menu[i].Entry[j].ListEntry, "%s-%s", is[k], pin);
						k = c_cnt;
					}

				}
				free(tmps);
			}
		}
		for (k = 0; k < c_cnt; k++) {
			free(was[k]);
			free(is[k]);
		}

		NetlistChanged(0);
		IncrementUndoSerialNumber();
		SetChangedFlag(true);
	}

	free(locked_element_list);
	free(element_list);
	free(cnt_list);
	return 0;
}
#endif

/* --------------------------------------------------------------------------- */

static const char ripup_syntax[] = "RipUp(All|Selected|Element)";

static const char ripup_help[] = "Ripup auto-routed tracks, or convert an element to parts.";

/* %start-doc actions RipUp

@table @code

@item All
Removes all lines and vias which were created by the autorouter.

@item Selected
Removes all selected lines and vias which were created by the
autorouter.

@item Element
Converts the element under the cursor to parts (vias and lines).  Note
that this uses the highest numbered paste buffer.

@end table

%end-doc */

static int ActionRipUp(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	bool changed = false;

	if (function) {
		switch (GetFunctionID(function)) {
		case F_All:
			ALLLINE_LOOP(PCB->Data);
			{
				if (TEST_FLAG(AUTOFLAG, line) && !TEST_FLAG(LOCKFLAG, line)) {
					RemoveObject(LINE_TYPE, layer, line, line);
					changed = true;
				}
			}
			ENDALL_LOOP;
			ALLARC_LOOP(PCB->Data);
			{
				if (TEST_FLAG(AUTOFLAG, arc) && !TEST_FLAG(LOCKFLAG, arc)) {
					RemoveObject(ARC_TYPE, layer, arc, arc);
					changed = true;
				}
			}
			ENDALL_LOOP;
			VIA_LOOP(PCB->Data);
			{
				if (TEST_FLAG(AUTOFLAG, via) && !TEST_FLAG(LOCKFLAG, via)) {
					RemoveObject(VIA_TYPE, via, via, via);
					changed = true;
				}
			}
			END_LOOP;

			if (changed) {
				IncrementUndoSerialNumber();
				SetChangedFlag(true);
			}
			break;
		case F_Selected:
			VISIBLELINE_LOOP(PCB->Data);
			{
				if (TEST_FLAGS(AUTOFLAG | SELECTEDFLAG, line)
						&& !TEST_FLAG(LOCKFLAG, line)) {
					RemoveObject(LINE_TYPE, layer, line, line);
					changed = true;
				}
			}
			ENDALL_LOOP;
			if (PCB->ViaOn)
				VIA_LOOP(PCB->Data);
			{
				if (TEST_FLAGS(AUTOFLAG | SELECTEDFLAG, via)
						&& !TEST_FLAG(LOCKFLAG, via)) {
					RemoveObject(VIA_TYPE, via, via, via);
					changed = true;
				}
			}
			END_LOOP;
			if (changed) {
				IncrementUndoSerialNumber();
				SetChangedFlag(true);
			}
			break;
		case F_Element:
			{
				void *ptr1, *ptr2, *ptr3;

				if (SearchScreen(Crosshair.X, Crosshair.Y, ELEMENT_TYPE, &ptr1, &ptr2, &ptr3) != NO_TYPE) {
					Note.Buffer = Settings.BufferNumber;
					SetBufferNumber(MAX_BUFFER - 1);
					ClearBuffer(PASTEBUFFER);
					CopyObjectToBuffer(PASTEBUFFER->Data, PCB->Data, ELEMENT_TYPE, ptr1, ptr2, ptr3);
					SmashBufferElement(PASTEBUFFER);
					PASTEBUFFER->X = 0;
					PASTEBUFFER->Y = 0;
					SaveUndoSerialNumber();
					EraseObject(ELEMENT_TYPE, ptr1, ptr1);
					MoveObjectToRemoveUndoList(ELEMENT_TYPE, ptr1, ptr2, ptr3);
					RestoreUndoSerialNumber();
					CopyPastebufferToLayout(0, 0);
					SetBufferNumber(Note.Buffer);
					SetChangedFlag(true);
				}
			}
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char delete_syntax[] = "Delete(Object|Selected)\n" "Delete(AllRats|SelectedRats)";

static const char delete_help[] = "Delete stuff.";

/* %start-doc actions Delete

%end-doc */

static int ActionDelete(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	int id = GetFunctionID(function);

	Note.X = Crosshair.X;
	Note.Y = Crosshair.Y;

	if (id == -1) {								/* no arg */
		if (RemoveSelected() == false)
			id = F_Object;
	}

	switch (id) {
	case F_Object:
		SaveMode();
		SetMode(REMOVE_MODE);
		NotifyMode();
		RestoreMode();
		break;
	case F_Selected:
		RemoveSelected();
		break;
	case F_AllRats:
		if (DeleteRats(false))
			SetChangedFlag(true);
		break;
	case F_SelectedRats:
		if (DeleteRats(true))
			SetChangedFlag(true);
		break;
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
	char *function = ARG(0);
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
	else if (GetFunctionID(function) == F_Center) {
		notify_mark_change(false);
		Marked.status = true;
		Marked.X = Crosshair.X;
		Marked.Y = Crosshair.Y;
		notify_mark_change(true);
	}
	return 0;
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
			*out = Settings.ViaThickness;
		else if (size_id == 1)
			*out = Settings.ViaDrillingHole;
		else
			*out = Settings.Keepaway;
		break;
	case F_SelectedArcs:
	case F_SelectedLines:
		if (size_id == 2)
			*out = Settings.Keepaway;
		else
			*out = Settings.LineThickness;
		return 0;
	case F_SelectedTexts:
	case F_SelectedNames:
		Message(_("Sorry, can't change style of every selected object\n"));
		return -1;
	}
	return 0;
}

/* ---------------------------------------------------------------------------  */

static const char minmaskgap_syntax[] = "MinMaskGap(delta)\n" "MinMaskGap(Selected, delta)";

static const char minmaskgap_help[] = "Ensures the mask is a minimum distance from pins and pads.";

/* %start-doc actions MinMaskGap

Checks all specified pins and/or pads, and increases the mask if
needed to ensure a minimum distance between the pin or pad edge and
the mask edge.

%end-doc */

static int ActionMinMaskGap(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	char *delta = ARG(1);
	char *units = ARG(2);
	bool absolute;
	Coord value;
	int flags;

	if (!function)
		return 1;
	if (strcasecmp(function, "Selected") == 0)
		flags = SELECTEDFLAG;
	else {
		units = delta;
		delta = function;
		flags = 0;
	}
	value = 2 * GetValue(delta, units, &absolute);

	SaveUndoSerialNumber();
	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (!TEST_FLAGS(flags, pin))
				continue;
			if (pin->Mask < pin->Thickness + value) {
				ChangeObjectMaskSize(PIN_TYPE, element, pin, 0, pin->Thickness + value, 1);
				RestoreUndoSerialNumber();
			}
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			if (!TEST_FLAGS(flags, pad))
				continue;
			if (pad->Mask < pad->Thickness + value) {
				ChangeObjectMaskSize(PAD_TYPE, element, pad, 0, pad->Thickness + value, 1);
				RestoreUndoSerialNumber();
			}
		}
		END_LOOP;
	}
	END_LOOP;
	VIA_LOOP(PCB->Data);
	{
		if (!TEST_FLAGS(flags, via))
			continue;
		if (via->Mask && via->Mask < via->Thickness + value) {
			ChangeObjectMaskSize(VIA_TYPE, via, 0, 0, via->Thickness + value, 1);
			RestoreUndoSerialNumber();
		}
	}
	END_LOOP;
	RestoreUndoSerialNumber();
	IncrementUndoSerialNumber();
	return 0;
}

/* ---------------------------------------------------------------------------  */

static const char mincleargap_syntax[] = "MinClearGap(delta)\n" "MinClearGap(Selected, delta)";

static const char mincleargap_help[] = "Ensures that polygons are a minimum distance from objects.";

/* %start-doc actions MinClearGap

Checks all specified objects, and increases the polygon clearance if
needed to ensure a minimum distance between their edges and the
polygon edges.

%end-doc */

static int ActionMinClearGap(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	char *delta = ARG(1);
	char *units = ARG(2);
	bool absolute;
	Coord value;
	int flags;

	if (!function)
		return 1;
	if (strcasecmp(function, "Selected") == 0)
		flags = SELECTEDFLAG;
	else {
		units = delta;
		delta = function;
		flags = 0;
	}
	value = 2 * GetValue(delta, units, &absolute);

	SaveUndoSerialNumber();
	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (!TEST_FLAGS(flags, pin))
				continue;
			if (pin->Clearance < value) {
				ChangeObjectClearSize(PIN_TYPE, element, pin, 0, value, 1);
				RestoreUndoSerialNumber();
			}
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			if (!TEST_FLAGS(flags, pad))
				continue;
			if (pad->Clearance < value) {
				ChangeObjectClearSize(PAD_TYPE, element, pad, 0, value, 1);
				RestoreUndoSerialNumber();
			}
		}
		END_LOOP;
	}
	END_LOOP;
	VIA_LOOP(PCB->Data);
	{
		if (!TEST_FLAGS(flags, via))
			continue;
		if (via->Clearance < value) {
			ChangeObjectClearSize(VIA_TYPE, via, 0, 0, value, 1);
			RestoreUndoSerialNumber();
		}
	}
	END_LOOP;
	ALLLINE_LOOP(PCB->Data);
	{
		if (!TEST_FLAGS(flags, line))
			continue;
		if (line->Clearance < value) {
			ChangeObjectClearSize(LINE_TYPE, layer, line, 0, value, 1);
			RestoreUndoSerialNumber();
		}
	}
	ENDALL_LOOP;
	ALLARC_LOOP(PCB->Data);
	{
		if (!TEST_FLAGS(flags, arc))
			continue;
		if (arc->Clearance < value) {
			ChangeObjectClearSize(ARC_TYPE, layer, arc, 0, value, 1);
			RestoreUndoSerialNumber();
		}
	}
	ENDALL_LOOP;
	RestoreUndoSerialNumber();
	IncrementUndoSerialNumber();
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char morphpolygon_syntax[] = "MorphPolygon(Object|Selected)";

static const char morphpolygon_help[] = "Converts dead polygon islands into separate polygons.";

/* %start-doc actions MorphPolygon 

If a polygon is divided into unconnected "islands", you can use
this command to convert the otherwise disappeared islands into
separate polygons. Be sure the cursor is over a portion of the
polygon that remains visible. Very small islands that may flake
off are automatically deleted.

%end-doc */

static int ActionMorphPolygon(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	if (function) {
		switch (GetFunctionID(function)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = SearchScreen(x, y, POLYGON_TYPE, &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
					MorphPolygon((LayerType *) ptr1, (PolygonType *) ptr3);
					Draw();
					IncrementUndoSerialNumber();
				}
				break;
			}
		case F_Selected:
		case F_SelectedObjects:
			ALLPOLYGON_LOOP(PCB->Data);
			{
				if (TEST_FLAG(SELECTEDFLAG, polygon))
					MorphPolygon(layer, polygon);
			}
			ENDALL_LOOP;
			Draw();
			IncrementUndoSerialNumber();
			break;
		}
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
	char *function = ARG(0);
	if (function && PCB->ElementOn) {
		switch (GetFunctionID(function)) {
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

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
void ActionBell(char *volume)
{
	gui->beep();
}

/* --------------------------------------------------------------------------- */

static const char pastebuffer_syntax[] =
	"PasteBuffer(AddSelected|Clear|1..MAX_BUFFER)\n"
	"PasteBuffer(Rotate, 1..3)\n" "PasteBuffer(Convert|Save|Restore|Mirror)\n" "PasteBuffer(ToLayout, X, Y, units)";

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
Saves any elements in the current buffer to the indicated file.

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

static int ActionPasteBuffer(int argc, char **argv, Coord x, Coord y)
{
	char *function = argc ? argv[0] : (char *) "";
	char *sbufnum = argc > 1 ? argv[1] : (char *) "";
	char *name;
	static char *default_file = NULL;
	int free_name = 0;

	notify_crosshair_change(false);
	if (function) {
		switch (GetFunctionID(function)) {
			/* clear contents of paste buffer */
		case F_Clear:
			ClearBuffer(PASTEBUFFER);
			break;

			/* copies objects to paste buffer */
		case F_AddSelected:
			AddSelectedToBuffer(PASTEBUFFER, 0, 0, false);
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
				RotateBuffer(PASTEBUFFER, (BYTE) atoi(sbufnum));
				SetCrosshairRangeToBuffer();
			}
			break;

		case F_Save:
			if (PASTEBUFFER->Data->ElementN == 0) {
				Message(_("Buffer has no elements!\n"));
				break;
			}
			free_name = 0;
			if (argc <= 1) {
				name = gui->fileselect(_("Save Paste Buffer As ..."),
															 _("Choose a file to save the contents of the\n"
																 "paste buffer to.\n"), default_file, ".fp", "footprint", 0);

				if (default_file) {
					free(default_file);
					default_file = NULL;
				}
				if (name && *name) {
					default_file = strdup(name);
				}
				free_name = 1;
			}

			else
				name = argv[1];

			{
				FILE *exist;

				if ((exist = fopen(name, "r"))) {
					fclose(exist);
					if (gui->confirm_dialog(_("File exists!  Ok to overwrite?"), 0))
						SaveBufferElements(name);
				}
				else
					SaveBufferElements(name);

				if (free_name && name)
					free(name);
			}
			break;

		case F_ToLayout:
			{
				static Coord oldx = 0, oldy = 0;
				Coord x, y;
				bool absolute;

				if (argc == 1) {
					x = y = 0;
				}
				else if (argc == 3 || argc == 4) {
					x = GetValue(ARG(1), ARG(3), &absolute);
					if (!absolute)
						x += oldx;
					y = GetValue(ARG(2), ARG(3), &absolute);
					if (!absolute)
						y += oldy;
				}
				else {
					notify_crosshair_change(true);
					AFAIL(pastebuffer);
				}

				oldx = x;
				oldy = y;
				if (CopyPastebufferToLayout(x, y))
					SetChangedFlag(true);
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

	notify_crosshair_change(true);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char polygon_syntax[] = "Polygon(Close|PreviousPoint)";

static const char polygon_help[] = "Some polygon related stuff.";

/* %start-doc actions Polygon

Polygons need a special action routine to make life easier.

@table @code

@item Close
Creates the final segment of the polygon.  This may fail if clipping
to 45 degree lines is switched on, in which case a warning is issued.

@item PreviousPoint
Resets the newly entered corner to the previous one. The Undo action
will call Polygon(PreviousPoint) when appropriate to do so.

@end table

%end-doc */

static int ActionPolygon(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	if (function && Settings.Mode == POLYGON_MODE) {
		notify_crosshair_change(false);
		switch (GetFunctionID(function)) {
			/* close open polygon if possible */
		case F_Close:
			ClosePolygon();
			break;

			/* go back to the previous point */
		case F_PreviousPoint:
			GoToPreviousPoint();
			break;
		}
		notify_crosshair_change(true);
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
	char *str = ARG(0);
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

static const char moveobject_syntax[] = "MoveObject(X,Y,dim)";

static const char moveobject_help[] = "Moves the object under the crosshair.";

/* %start-doc actions MoveObject

The @code{X} and @code{Y} are treated like @code{delta} is for many
other objects.  For each, if it's prefixed by @code{+} or @code{-},
then that amount is relative.  Otherwise, it's absolute.  Units can be
@code{mil} or @code{mm}; if unspecified, units are PCB's internal
units, currently 1/100 mil.

%end-doc */

static int ActionMoveObject(int argc, char **argv, Coord x, Coord y)
{
	char *x_str = ARG(0);
	char *y_str = ARG(1);
	char *units = ARG(2);
	Coord nx, ny;
	bool absolute1, absolute2;
	void *ptr1, *ptr2, *ptr3;
	int type;

	ny = GetValue(y_str, units, &absolute1);
	nx = GetValue(x_str, units, &absolute2);

	type = SearchScreen(x, y, MOVE_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == NO_TYPE) {
		Message(_("Nothing found under crosshair\n"));
		return 1;
	}
	if (absolute1)
		nx -= x;
	if (absolute2)
		ny -= y;
	Crosshair.AttachedObject.RubberbandN = 0;
	if (TEST_FLAG(RUBBERBANDFLAG, PCB))
		LookupRubberbandLines(type, ptr1, ptr2, ptr3);
	if (type == ELEMENT_TYPE)
		LookupRatLines(type, ptr1, ptr2, ptr3);
	MoveObjectAndRubberband(type, ptr1, ptr2, ptr3, nx, ny);
	SetChangedFlag(true);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char movetocurrentlayer_syntax[] = "MoveToCurrentLayer(Object|SelectedObjects)";

static const char movetocurrentlayer_help[] = "Moves objects to the current layer.";

/* %start-doc actions MoveToCurrentLayer

Note that moving an element from a component layer to a solder layer,
or from solder to component, won't automatically flip it.  Use the
@code{Flip()} action to do that.

%end-doc */

static int ActionMoveToCurrentLayer(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	if (function) {
		switch (GetFunctionID(function)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = SearchScreen(x, y, MOVETOLAYER_TYPES, &ptr1, &ptr2, &ptr3)) != NO_TYPE)
					if (MoveObjectToLayer(type, ptr1, ptr2, ptr3, CURRENT, false))
						SetChangedFlag(true);
				break;
			}

		case F_SelectedObjects:
		case F_Selected:
			if (MoveSelectedObjectsToLayer(CURRENT))
				SetChangedFlag(true);
			break;
		}
	}
	return 0;
}


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
		Settings.LineThickness = ((LineTypePtr) ptr2)->Thickness;
		Settings.Keepaway = ((LineTypePtr) ptr2)->Clearance / 2;
		layer = (LayerTypePtr) ptr1;
		if (Settings.Mode != LINE_MODE)
			SetMode(LINE_MODE);
		notify_crosshair_change(true);
		hid_action("RouteStylesChanged");
		break;

	case ARC_TYPE:
		notify_crosshair_change(false);
		Settings.LineThickness = ((ArcTypePtr) ptr2)->Thickness;
		Settings.Keepaway = ((ArcTypePtr) ptr2)->Clearance / 2;
		layer = (LayerTypePtr) ptr1;
		if (Settings.Mode != ARC_MODE)
			SetMode(ARC_MODE);
		notify_crosshair_change(true);
		hid_action("RouteStylesChanged");
		break;

	case POLYGON_TYPE:
		layer = (LayerTypePtr) ptr1;
		break;

	case VIA_TYPE:
		notify_crosshair_change(false);
		Settings.ViaThickness = ((PinTypePtr) ptr2)->Thickness;
		Settings.ViaDrillingHole = ((PinTypePtr) ptr2)->DrillingHole;
		Settings.Keepaway = ((PinTypePtr) ptr2)->Clearance / 2;
		if (Settings.Mode != VIA_MODE)
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

static const char executefile_syntax[] = "ExecuteFile(filename)";

static const char executefile_help[] = "Run actions from the given file.";

/* %start-doc actions ExecuteFile

Lines starting with @code{#} are ignored.

%end-doc */

int ActionExecuteFile(int argc, char **argv, Coord x, Coord y)
{
	FILE *fp;
	char *fname;
	char line[256];
	int n = 0;
	char *sp;

	if (argc != 1)
		AFAIL(executefile);

	fname = argv[0];

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, _("Could not open actions file \"%s\".\n"), fname);
		return 1;
	}

	defer_updates = 1;
	defer_needs_update = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		n++;
		sp = line;

		/* eat the trailing newline */
		while (*sp && *sp != '\r' && *sp != '\n')
			sp++;
		*sp = '\0';

		/* eat leading spaces and tabs */
		sp = line;
		while (*sp && (*sp == ' ' || *sp == '\t'))
			sp++;

		/* 
		 * if we have anything left and its not a comment line
		 * then execute it
		 */

		if (*sp && *sp != '#') {
			/*Message ("%s : line %-3d : \"%s\"\n", fname, n, sp); */
			hid_parse_actions(sp);
		}
	}

	defer_updates = 0;
	if (defer_needs_update) {
		IncrementUndoSerialNumber();
		gui->invalidate_all();
	}
	fclose(fp);
	return 0;
}

/* --------------------------------------------------------------------------- */

static int ActionPSCalib(int argc, char **argv, Coord x, Coord y)
{
	HID *ps = hid_find_exporter("ps");
	ps->calibrate(0.0, 0.0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static ElementType *element_cache = NULL;

static ElementType *find_element_by_refdes(char *refdes)
{
	if (element_cache && NAMEONPCB_NAME(element_cache)
			&& strcmp(NAMEONPCB_NAME(element_cache), refdes) == 0)
		return element_cache;

	ELEMENT_LOOP(PCB->Data);
	{
		if (NAMEONPCB_NAME(element)
				&& strcmp(NAMEONPCB_NAME(element), refdes) == 0) {
			element_cache = element;
			return element_cache;
		}
	}
	END_LOOP;
	return NULL;
}

static AttributeType *lookup_attr(AttributeListTypePtr list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(list->List[i].name, name) == 0)
			return &list->List[i];
	return NULL;
}

static void delete_attr(AttributeListTypePtr list, AttributeType * attr)
{
	int idx = attr - list->List;
	if (idx < 0 || idx >= list->Number)
		return;
	if (list->Number - idx > 1)
		memmove(attr, attr + 1, (list->Number - idx - 1) * sizeof(AttributeType));
	list->Number--;
}

/* ---------------------------------------------------------------- */
static const char elementlist_syntax[] = "ElementList(Start|Done|Need,<refdes>,<footprint>,<value>)";

static const char elementlist_help[] = "Adds the given element if it doesn't already exist.";

/* %start-doc actions elementlist

@table @code

@item Start
Indicates the start of an element list; call this before any Need
actions.

@item Need
Searches the board for an element with a matching refdes.

If found, the value and footprint are updated.

If not found, a new element is created with the given footprint and value.

@item Done
Compares the list of elements needed since the most recent
@code{start} with the list of elements actually on the board.  Any
elements that weren't listed are selected, so that the user may delete
them.

@end table

%end-doc */

static int number_of_footprints_not_found;

static int parse_layout_attribute_units(char *name, int def)
{
	const char *as = AttributeGet(PCB, name);
	if (!as)
		return def;
	return GetValue(as, NULL, NULL);
}

static int ActionElementList(int argc, char **argv, Coord x, Coord y)
{
	ElementType *e = NULL;
	char *refdes, *value, *footprint, *old;
	char *args[3];
	char *function = argv[0];

#ifdef DEBUG
	printf("Entered ActionElementList, executing function %s\n", function);
#endif

	if (strcasecmp(function, "start") == 0) {
		ELEMENT_LOOP(PCB->Data);
		{
			CLEAR_FLAG(FOUNDFLAG, element);
		}
		END_LOOP;
		element_cache = NULL;
		number_of_footprints_not_found = 0;
		return 0;
	}

	if (strcasecmp(function, "done") == 0) {
		ELEMENT_LOOP(PCB->Data);
		{
			if (TEST_FLAG(FOUNDFLAG, element)) {
				CLEAR_FLAG(FOUNDFLAG, element);
			}
			else if (!EMPTY_STRING_P(NAMEONPCB_NAME(element))) {
				/* Unnamed elements should remain untouched */
				SET_FLAG(SELECTEDFLAG, element);
			}
		}
		END_LOOP;
		if (number_of_footprints_not_found > 0)
			gui->confirm_dialog("Not all requested footprints were found.\n" "See the message log for details", "Ok", NULL);
		return 0;
	}

	if (strcasecmp(function, "need") != 0)
		AFAIL(elementlist);

	if (argc != 4)
		AFAIL(elementlist);

	argc--;
	argv++;

	refdes = ARG(0);
	footprint = ARG(1);
	value = ARG(2);

	args[0] = footprint;
	args[1] = refdes;
	args[2] = value;

#ifdef DEBUG
	printf("  ... footprint = %s\n", footprint);
	printf("  ... refdes = %s\n", refdes);
	printf("  ... value = %s\n", value);
#endif

	e = find_element_by_refdes(refdes);

	if (!e) {
		Coord nx, ny, d;

#ifdef DEBUG
		printf("  ... Footprint not on board, need to add it.\n");
#endif
		/* Not on board, need to add it. */
		if (LoadFootprint(argc, args, x, y)) {
			number_of_footprints_not_found++;
			return 1;
		}

		nx = PCB->MaxWidth / 2;
		ny = PCB->MaxHeight / 2;
		d = MIN(PCB->MaxWidth, PCB->MaxHeight) / 10;

		nx = parse_layout_attribute_units("import::newX", nx);
		ny = parse_layout_attribute_units("import::newY", ny);
		d = parse_layout_attribute_units("import::disperse", d);

		if (d > 0) {
			nx += rand() % (d * 2) - d;
			ny += rand() % (d * 2) - d;
		}

		if (nx < 0)
			nx = 0;
		if (nx >= PCB->MaxWidth)
			nx = PCB->MaxWidth - 1;
		if (ny < 0)
			ny = 0;
		if (ny >= PCB->MaxHeight)
			ny = PCB->MaxHeight - 1;

		/* Place components onto center of board. */
		if (CopyPastebufferToLayout(nx, ny))
			SetChangedFlag(true);
	}

	else if (e && strcmp(DESCRIPTION_NAME(e), footprint) != 0) {
#ifdef DEBUG
		printf("  ... Footprint on board, but different from footprint loaded.\n");
#endif
		int er, pr, i;
		Coord mx, my;
		ElementType *pe;

		/* Different footprint, we need to swap them out.  */
		if (LoadFootprint(argc, args, x, y)) {
			number_of_footprints_not_found++;
			return 1;
		}

		er = ElementOrientation(e);
		pe = PASTEBUFFER->Data->Element->data;
		if (!FRONT(e))
			MirrorElementCoordinates(PASTEBUFFER->Data, pe, pe->MarkY * 2 - PCB->MaxHeight);
		pr = ElementOrientation(pe);

		mx = e->MarkX;
		my = e->MarkY;

		if (er != pr)
			RotateElementLowLevel(PASTEBUFFER->Data, pe, pe->MarkX, pe->MarkY, (er - pr + 4) % 4);

		for (i = 0; i < MAX_ELEMENTNAMES; i++) {
			pe->Name[i].X = e->Name[i].X - mx + pe->MarkX;
			pe->Name[i].Y = e->Name[i].Y - my + pe->MarkY;
			pe->Name[i].Direction = e->Name[i].Direction;
			pe->Name[i].Scale = e->Name[i].Scale;
		}

		RemoveElement(e);

		if (CopyPastebufferToLayout(mx, my))
			SetChangedFlag(true);
	}

	/* Now reload footprint */
	element_cache = NULL;
	e = find_element_by_refdes(refdes);

	old = ChangeElementText(PCB, PCB->Data, e, NAMEONPCB_INDEX, strdup(refdes));
	if (old)
		free(old);
	old = ChangeElementText(PCB, PCB->Data, e, VALUE_INDEX, strdup(value));
	if (old)
		free(old);

	SET_FLAG(FOUNDFLAG, e);

#ifdef DEBUG
	printf(" ... Leaving ActionElementList.\n");
#endif

	return 0;
}

/* ---------------------------------------------------------------- */
static const char elementsetattr_syntax[] = "ElementSetAttr(refdes,name[,value])";

static const char elementsetattr_help[] = "Sets or clears an element-specific attribute.";

/* %start-doc actions elementsetattr

If a value is specified, the named attribute is added (if not already
present) or changed (if it is) to the given value.  If the value is
not specified, the given attribute is removed if present.

%end-doc */

static int ActionElementSetAttr(int argc, char **argv, Coord x, Coord y)
{
	ElementType *e = NULL;
	char *refdes, *name, *value;
	AttributeType *attr;

	if (argc < 2) {
		AFAIL(elementsetattr);
	}

	refdes = argv[0];
	name = argv[1];
	value = ARG(2);

	ELEMENT_LOOP(PCB->Data);
	{
		if (NSTRCMP(refdes, NAMEONPCB_NAME(element)) == 0) {
			e = element;
			break;
		}
	}
	END_LOOP;

	if (!e) {
		Message(_("Cannot change attribute of %s - element not found\n"), refdes);
		return 1;
	}

	attr = lookup_attr(&e->Attributes, name);

	if (attr && value) {
		free(attr->value);
		attr->value = strdup(value);
	}
	if (attr && !value) {
		delete_attr(&e->Attributes, attr);
	}
	if (!attr && value) {
		CreateNewAttribute(&e->Attributes, name, value);
	}

	return 0;
}

/* ------------------------------------------------------------ */

static const char attributes_syntax[] = "Attributes(Layout|Layer|Element)\n" "Attributes(Layer,layername)";

static const char attributes_help[] =
	"Let the user edit the attributes of the layout, current or given\n" "layer, or selected element.";

/* %start-doc actions Attributes

This just pops up a dialog letting the user edit the attributes of the
pcb, an element, or a layer.

%end-doc */


static int ActionAttributes(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	char *layername = ARG(1);
	char *buf;

	if (!function)
		AFAIL(attributes);

	if (!gui->edit_attributes) {
		Message(_("This GUI doesn't support Attribute Editing\n"));
		return 1;
	}

	switch (GetFunctionID(function)) {
	case F_Layout:
		{
			gui->edit_attributes("Layout Attributes", &(PCB->Attributes));
			return 0;
		}

	case F_Layer:
		{
			LayerType *layer = CURRENT;
			if (layername) {
				int i;
				layer = NULL;
				for (i = 0; i < max_copper_layer; i++)
					if (strcmp(PCB->Data->Layer[i].Name, layername) == 0) {
						layer = &(PCB->Data->Layer[i]);
						break;
					}
				if (layer == NULL) {
					Message(_("No layer named %s\n"), layername);
					return 1;
				}
			}
			buf = (char *) malloc(strlen(layer->Name) + strlen("Layer X Attributes"));
			sprintf(buf, "Layer %s Attributes", layer->Name);
			gui->edit_attributes(buf, &(layer->Attributes));
			free(buf);
			return 0;
		}

	case F_Element:
		{
			int n_found = 0;
			ElementType *e = NULL;
			ELEMENT_LOOP(PCB->Data);
			{
				if (TEST_FLAG(SELECTEDFLAG, element)) {
					e = element;
					n_found++;
				}
			}
			END_LOOP;
			if (n_found > 1) {
				Message(_("Too many elements selected\n"));
				return 1;
			}
			if (n_found == 0) {
				void *ptrtmp;
				gui->get_coords(_("Click on an element"), &x, &y);
				if ((SearchScreen(x, y, ELEMENT_TYPE, &ptrtmp, &ptrtmp, &ptrtmp)) != NO_TYPE)
					e = (ElementTypePtr) ptrtmp;
				else {
					Message(_("No element found there\n"));
					return 1;
				}
			}

			if (NAMEONPCB_NAME(e)) {
				buf = (char *) malloc(strlen(NAMEONPCB_NAME(e)) + strlen("Element X Attributes"));
				sprintf(buf, "Element %s Attributes", NAMEONPCB_NAME(e));
			}
			else {
				buf = strdup("Unnamed Element Attributes");
			}
			gui->edit_attributes(buf, &(e->Attributes));
			free(buf);
			break;
		}

	default:
		AFAIL(attributes);
	}

	return 0;
}

/* ---------------------------------------------------------------- */
static const char manageplugins_syntax[] = "ManagePlugins()\n";

static const char manageplugins_help[] = "Manage plugins dialog.";

static int ManagePlugins(int argc, char **argv, Coord x, Coord y)
{
	plugin_info_t *i;
	int nump = 0, numb = 0;
	DynamicStringType str;

	memset(&str, 0, sizeof(str));

	for (i = plugins; i != NULL; i = i->next)
		if (i->dynamic)
			nump++;
		else
			numb++;

	DSAddString(&str, "Plugins loaded:\n");
	if (nump > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (i->dynamic) {
				DSAddCharacter(&str, ' ');
				DSAddString(&str, i->name);
				DSAddCharacter(&str, ' ');
				DSAddString(&str, i->path);
				DSAddCharacter(&str, '\n');
			}
		}
	}
	else
		DSAddString(&str, " (none)\n");

	DSAddString(&str, "\n\nBuildins:\n");
	if (numb > 0) {
		for (i = plugins; i != NULL; i = i->next) {
			if (!i->dynamic) {
				DSAddCharacter(&str, ' ');
				DSAddString(&str, i->name);
				DSAddCharacter(&str, '\n');
			}
		}
	}
	else
		DSAddString(&str, " (none)\n");

	DSAddString(&str, "\n\nNOTE: this is the alpha version, can only list plugins/buildins\n");
	gui->report_dialog("Manage plugins", str.Data);
	free(str.Data);
	return 0;
}

/* ---------------------------------------------------------------- */
static const char replacefootprint_syntax[] = "ReplaceFootprint()\n";

static const char replacefootprint_help[] = "Replace the footprint of the selected components with the footprint specified.";

static int ReplaceFootprint(int argc, char **argv, Coord x, Coord y)
{
	char *a[4];
	char *fpname;
	int found = 0;

	/* check if we have elements selected and quit if not */
	ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(SELECTEDFLAG, element)) {
			found = 1;
			break;
		}
	}
	END_LOOP;

	if (!(found)) {
		Message("ReplaceFootprint works on selected elements, please select elements first!\n");
		return 1;
	}

	/* fetch the name of the new footprint */
	if (argc == 0) {
		fpname = gui->prompt_for("Footprint name", "");
		if (fpname == NULL) {
			Message("No footprint name supplied\n");
			return 1;
		}
	}
	else
		fpname = argv[0];

	/* check if the footprint is available */
	a[0] = fpname;
	a[1] = NULL;
	if (LoadFootprint(1, a, x, y) != 0) {
		Message("Can't load footprint %s\n", fpname);
		return 1;
	}


	/* action: replace selected elements */
	ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(SELECTEDFLAG, element)) {
			a[0] = fpname;
			a[1] = element->Name[1].TextString;
			a[2] = element->Name[2].TextString;
			a[3] = NULL;
			LoadFootprint(3, a, element->MarkX, element->MarkY);
			CopyPastebufferToLayout(element->MarkX, element->MarkY);
			rats_patch_append_optimize(PCB, RATP_CHANGE_ATTRIB, a[1], "footprint", fpname);
			RemoveElement(element);
		}
	}
	END_LOOP;

	fprintf(stderr, "rp2\n");
}


/* --------------------------------------------------------------------------- */

HID_Action action_action_list[] = {
	{"Attributes", 0, ActionAttributes,
	 attributes_help, attributes_syntax}
	,
	{"Delete", 0, ActionDelete,
	 delete_help, delete_syntax}
	,
	{"DisperseElements", 0, ActionDisperseElements,
	 disperseelements_help, disperseelements_syntax}
	,
	{"Display", 0, ActionDisplay,
	 display_help, display_syntax}
	,
	{"DRC", 0, ActionDRCheck,
	 drc_help, drc_syntax}
	,
	{"CycleDrag", 0, CycleDrag,
	 cycledrag_help, cycledrag_syntax}
	,
	{"DumpLibrary", 0, ActionDumpLibrary,
	 dumplibrary_help, dumplibrary_syntax}
	,
	{"ExecuteFile", 0, ActionExecuteFile,
	 executefile_help, executefile_syntax}
	,
	{"Flip", N_("Click on Object or Flip Point"), ActionFlip,
	 flip_help, flip_syntax}
	,
	{"MarkCrosshair", 0, ActionMarkCrosshair,
	 markcrosshair_help, markcrosshair_syntax}
	,
	{"Message", 0, ActionMessage,
	 message_help, message_syntax}
	,
	{"MinMaskGap", 0, ActionMinMaskGap,
	 minmaskgap_help, minmaskgap_syntax}
	,
	{"MinClearGap", 0, ActionMinClearGap,
	 mincleargap_help, mincleargap_syntax}
	,
	{"Mode", 0, ActionMode,
	 mode_help, mode_syntax}
	,
	{"MorphPolygon", 0, ActionMorphPolygon,
	 morphpolygon_help, morphpolygon_syntax}
	,
	{"PasteBuffer", 0, ActionPasteBuffer,
	 pastebuffer_help, pastebuffer_syntax}
	,
	{"RemoveSelected", 0, ActionRemoveSelected,
	 removeselected_help, removeselected_syntax}
	,
#ifdef BA_TODO
	{"Renumber", 0, ActionRenumber,
	 renumber_help, renumber_syntax}
	,
#endif
	{"RipUp", 0, ActionRipUp,
	 ripup_help, ripup_syntax}
	,
	{"ToggleHideName", 0, ActionToggleHideName,
	 togglehidename_help, togglehidename_syntax}
	,
	{"SetSame", N_("Select item to use attributes from"), ActionSetSame,
	 setsame_help, setsame_syntax}
	,
	{"Polygon", 0, ActionPolygon,
	 polygon_help, polygon_syntax}
	,
	{"RouteStyle", 0, ActionRouteStyle,
	 routestyle_help, routestyle_syntax}
	,
	{"MoveObject", N_("Select an Object"), ActionMoveObject,
	 moveobject_help, moveobject_syntax}
	,
	{"MoveToCurrentLayer", 0, ActionMoveToCurrentLayer,
	 movetocurrentlayer_help, movetocurrentlayer_syntax}
	,
	{"pscalib", 0, ActionPSCalib}
	,
	{"ElementList", 0, ActionElementList,
	 elementlist_help, elementlist_syntax}
	,
	{"ElementSetAttr", 0, ActionElementSetAttr,
	 elementsetattr_help, elementsetattr_syntax}
	,
	{"ManagePlugins", 0, ManagePlugins,
	 manageplugins_help, manageplugins_syntax}
	,
	{"ReplaceFootprint", 0, ReplaceFootprint,
	 replacefootprint_help, replacefootprint_syntax}
	,
};

REGISTER_ACTIONS(action_action_list)
