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
#include "global.h"
#include "data.h"
#include "action.h"
#include "change.h"
#include "error.h"

#include "crosshair.h"
#include "undo.h"
#include "polygon.h"
#include "set.h"
#include "search.h"
#include "draw.h"
#include "misc.h"

/* --------------------------------------------------------------------------- */

static const char atomic_syntax[] = "Atomic(Save|Restore|Close|Block)";

static const char atomic_help[] = "Save or restore the undo serial number.";

/* %start-doc actions Atomic

This action allows making multiple-action bindings into an atomic
operation that will be undone by a single Undo command.  For example,
to optimize rat lines, you'd delete the rats and re-add them.  To
group these into a single undo, you'd want the deletions and the
additions to have the same undo serial number.  So, you @code{Save},
delete the rats, @code{Restore}, add the rats - using the same serial
number as the deletes, then @code{Block}, which checks to see if the
deletions or additions actually did anything.  If not, the serial
number is set to the saved number, as there's nothing to undo.  If
something did happen, the serial number is incremented so that these
actions are counted as a single undo step.

@table @code

@item Save
Saves the undo serial number.

@item Restore
Returns it to the last saved number.

@item Close
Sets it to 1 greater than the last save.

@item Block
Does a Restore if there was nothing to undo, else does a Close.

@end table

%end-doc */

int ActionAtomic(int argc, char **argv, Coord x, Coord y)
{
	if (argc != 1)
		AFAIL(atomic);

	switch (GetFunctionID(argv[0])) {
	case F_Save:
		SaveUndoSerialNumber();
		break;
	case F_Restore:
		RestoreUndoSerialNumber();
		break;
	case F_Close:
		RestoreUndoSerialNumber();
		IncrementUndoSerialNumber();
		break;
	case F_Block:
		RestoreUndoSerialNumber();
		if (Bumped)
			IncrementUndoSerialNumber();
		break;
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char undo_syntax[] = "Undo()\n" "Undo(ClearList)";

static const char undo_help[] = "Undo recent changes.";

/* %start-doc actions Undo

The unlimited undo feature of @code{Pcb} allows you to recover from
most operations that materially affect you work.  Calling
@code{Undo()} without any parameter recovers from the last (non-undo)
operation. @code{ClearList} is used to release the allocated
memory. @code{ClearList} is called whenever a new layout is started or
loaded. See also @code{Redo} and @code{Atomic}.

Note that undo groups operations by serial number; changes with the
same serial number will be undone (or redone) as a group.  See
@code{Atomic}.

%end-doc */

int ActionUndo(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	if (!function || !*function) {
		/* don't allow undo in the middle of an operation */
		if (Settings.Mode != POLYGONHOLE_MODE && Crosshair.AttachedObject.State != STATE_FIRST)
			return 1;
		if (Crosshair.AttachedBox.State != STATE_FIRST && Settings.Mode != ARC_MODE)
			return 1;
		/* undo the last operation */

		notify_crosshair_change(false);
		if ((Settings.Mode == POLYGON_MODE || Settings.Mode == POLYGONHOLE_MODE) && Crosshair.AttachedPolygon.PointN) {
			GoToPreviousPoint();
			notify_crosshair_change(true);
			return 0;
		}
		/* move anchor point if undoing during line creation */
		if (Settings.Mode == LINE_MODE) {
			if (Crosshair.AttachedLine.State == STATE_SECOND) {
				if (TEST_FLAG(AUTODRCFLAG, PCB))
					Undo(true);						/* undo the connection find */
				Crosshair.AttachedLine.State = STATE_FIRST;
				SetLocalRef(0, 0, false);
				notify_crosshair_change(true);
				return 0;
			}
			if (Crosshair.AttachedLine.State == STATE_THIRD) {
				int type;
				void *ptr1, *ptr3, *ptrtmp;
				LineTypePtr ptr2;
				/* this search is guaranteed to succeed */
				SearchObjectByLocation(LINE_TYPE | RATLINE_TYPE, &ptr1,
															 &ptrtmp, &ptr3, Crosshair.AttachedLine.Point1.X, Crosshair.AttachedLine.Point1.Y, 0);
				ptr2 = (LineTypePtr) ptrtmp;

				/* save both ends of line */
				Crosshair.AttachedLine.Point2.X = ptr2->Point1.X;
				Crosshair.AttachedLine.Point2.Y = ptr2->Point1.Y;
				if ((type = Undo(true)))
					SetChangedFlag(true);
				/* check that the undo was of the right type */
				if ((type & UNDO_CREATE) == 0) {
					/* wrong undo type, restore anchor points */
					Crosshair.AttachedLine.Point2.X = Crosshair.AttachedLine.Point1.X;
					Crosshair.AttachedLine.Point2.Y = Crosshair.AttachedLine.Point1.Y;
					notify_crosshair_change(true);
					return 0;
				}
				/* move to new anchor */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
				/* check if an intermediate point was removed */
				if (type & UNDO_REMOVE) {
					/* this search should find the restored line */
					SearchObjectByLocation(LINE_TYPE | RATLINE_TYPE, &ptr1,
																 &ptrtmp, &ptr3, Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y, 0);
					ptr2 = (LineTypePtr) ptrtmp;
					if (TEST_FLAG(AUTODRCFLAG, PCB)) {
						/* undo loses FOUNDFLAG */
						SET_FLAG(FOUNDFLAG, ptr2);
						DrawLine(CURRENT, ptr2);
					}
					Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = ptr2->Point2.X;
					Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = ptr2->Point2.Y;
				}
				FitCrosshairIntoGrid(Crosshair.X, Crosshair.Y);
				AdjustAttachedObjects();
				if (--addedLines == 0) {
					Crosshair.AttachedLine.State = STATE_SECOND;
					lastLayer = CURRENT;
				}
				else {
					/* this search is guaranteed to succeed too */
					SearchObjectByLocation(LINE_TYPE | RATLINE_TYPE, &ptr1,
																 &ptrtmp, &ptr3, Crosshair.AttachedLine.Point1.X, Crosshair.AttachedLine.Point1.Y, 0);
					ptr2 = (LineTypePtr) ptrtmp;
					lastLayer = (LayerTypePtr) ptr1;
				}
				notify_crosshair_change(true);
				return 0;
			}
		}
		if (Settings.Mode == ARC_MODE) {
			if (Crosshair.AttachedBox.State == STATE_SECOND) {
				Crosshair.AttachedBox.State = STATE_FIRST;
				notify_crosshair_change(true);
				return 0;
			}
			if (Crosshair.AttachedBox.State == STATE_THIRD) {
				void *ptr1, *ptr2, *ptr3;
				BoxTypePtr bx;
				/* guaranteed to succeed */
				SearchObjectByLocation(ARC_TYPE, &ptr1, &ptr2, &ptr3,
															 Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point1.Y, 0);
				bx = GetArcEnds((ArcTypePtr) ptr2);
				Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = bx->X1;
				Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = bx->Y1;
				AdjustAttachedObjects();
				if (--addedLines == 0)
					Crosshair.AttachedBox.State = STATE_SECOND;
			}
		}
		/* undo the last destructive operation */
		if (Undo(true))
			SetChangedFlag(true);
	}
	else if (function) {
		switch (GetFunctionID(function)) {
			/* clear 'undo objects' list */
		case F_ClearList:
			ClearUndoList(false);
			break;
		}
	}
	notify_crosshair_change(true);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char redo_syntax[] = "Redo()";

static const char redo_help[] = "Redo recent \"undo\" operations.";

/* %start-doc actions Redo

This routine allows you to recover from the last undo command.  You
might want to do this if you thought that undo was going to revert
something other than what it actually did (in case you are confused
about which operations are un-doable), or if you have been backing up
through a long undo list and over-shoot your stopping point.  Any
change that is made since the undo in question will trim the redo
list.  For example if you add ten lines, then undo three of them you
could use redo to put them back, but if you move a line on the board
before performing the redo, you will lose the ability to "redo" the
three "undone" lines.

%end-doc */

int ActionRedo(int argc, char **argv, Coord x, Coord y)
{
	if (((Settings.Mode == POLYGON_MODE ||
				Settings.Mode == POLYGONHOLE_MODE) && Crosshair.AttachedPolygon.PointN) || Crosshair.AttachedLine.State == STATE_SECOND)
		return 1;
	notify_crosshair_change(false);
	if (Redo(true)) {
		SetChangedFlag(true);
		if (Settings.Mode == LINE_MODE && Crosshair.AttachedLine.State != STATE_FIRST) {
			LineType *line = linelist_last(&CURRENT->Line);
			Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = line->Point2.X;
			Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = line->Point2.Y;
			addedLines++;
		}
	}
	notify_crosshair_change(true);
	return 0;
}


HID_Action undo_action_list[] = {
	{"Atomic", 0, ActionAtomic,
	 atomic_help, atomic_syntax}
	,
	{"Undo", 0, ActionUndo,
	 undo_help, undo_syntax}
	,
	{"Redo", 0, ActionRedo,
	 redo_help, redo_syntax}
};

REGISTER_ACTIONS(undo_action_list)
