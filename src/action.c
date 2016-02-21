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
bool saved_mode = false;
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

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
void ActionBell(char *volume)
{
	gui->beep();
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

HID_Action action_action_list[] = {
	{"DRC", 0, ActionDRCheck,
	 drc_help, drc_syntax}
	,
	{"DumpLibrary", 0, ActionDumpLibrary,
	 dumplibrary_help, dumplibrary_syntax}
	,
	{"ExecuteFile", 0, ActionExecuteFile,
	 executefile_help, executefile_syntax}
	,
#ifdef BA_TODO
	{"Renumber", 0, ActionRenumber,
	 renumber_help, renumber_syntax}
	,
#endif
	{"SetSame", N_("Select item to use attributes from"), ActionSetSame,
	 setsame_help, setsame_syntax}
	,
	{"RouteStyle", 0, ActionRouteStyle,
	 routestyle_help, routestyle_syntax}
	,
};

REGISTER_ACTIONS(action_action_list)
