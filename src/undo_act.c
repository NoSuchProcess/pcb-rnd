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

#include "board.h"
#include "data.h"
#include "action_helper.h"
#include "error.h"
#include "funchash_core.h"

#include "undo.h"
#include "polygon.h"
#include "set.h"
#include "search.h"

#include "obj_line_draw.h"

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

int ActionAtomic(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc != 1)
		PCB_AFAIL(atomic);

	switch (pcb_funchash_get(argv[0], NULL)) {
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

int ActionUndo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (!function || !*function) {
		/* don't allow undo in the middle of an operation */
		if (conf_core.editor.mode != PCB_MODE_POLYGON_HOLE && Crosshair.AttachedObject.State != STATE_FIRST)
			return 1;
		if (Crosshair.AttachedBox.State != STATE_FIRST && conf_core.editor.mode != PCB_MODE_ARC)
			return 1;
		/* undo the last operation */

		pcb_notify_crosshair_change(pcb_false);
		if ((conf_core.editor.mode == PCB_MODE_POLYGON || conf_core.editor.mode == PCB_MODE_POLYGON_HOLE) && Crosshair.AttachedPolygon.PointN) {
			pcb_polygon_go_to_prev_point();
			pcb_notify_crosshair_change(pcb_true);
			return 0;
		}
		/* move anchor point if undoing during line creation */
		if (conf_core.editor.mode == PCB_MODE_LINE) {
			if (Crosshair.AttachedLine.State == STATE_SECOND) {
				if (conf_core.editor.auto_drc)
					Undo(pcb_true);						/* undo the connection find */
				Crosshair.AttachedLine.State = STATE_FIRST;
				SetLocalRef(0, 0, pcb_false);
				pcb_notify_crosshair_change(pcb_true);
				return 0;
			}
			if (Crosshair.AttachedLine.State == STATE_THIRD) {
				int type;
				void *ptr1, *ptr3, *ptrtmp;
				pcb_line_t *ptr2;
				/* this search is guaranteed to succeed */
				pcb_search_obj_by_location(PCB_TYPE_LINE | PCB_TYPE_RATLINE, &ptr1,
															 &ptrtmp, &ptr3, Crosshair.AttachedLine.Point1.X, Crosshair.AttachedLine.Point1.Y, 0);
				ptr2 = (pcb_line_t *) ptrtmp;

				/* save both ends of line */
				Crosshair.AttachedLine.Point2.X = ptr2->Point1.X;
				Crosshair.AttachedLine.Point2.Y = ptr2->Point1.Y;
				if ((type = Undo(pcb_true)))
					SetChangedFlag(pcb_true);
				/* check that the undo was of the right type */
				if ((type & UNDO_CREATE) == 0) {
					/* wrong undo type, restore anchor points */
					Crosshair.AttachedLine.Point2.X = Crosshair.AttachedLine.Point1.X;
					Crosshair.AttachedLine.Point2.Y = Crosshair.AttachedLine.Point1.Y;
					pcb_notify_crosshair_change(pcb_true);
					return 0;
				}
				/* move to new anchor */
				Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X;
				Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y;
				/* check if an intermediate point was removed */
				if (type & UNDO_REMOVE) {
					/* this search should find the restored line */
					pcb_search_obj_by_location(PCB_TYPE_LINE | PCB_TYPE_RATLINE, &ptr1,
																 &ptrtmp, &ptr3, Crosshair.AttachedLine.Point2.X, Crosshair.AttachedLine.Point2.Y, 0);
					ptr2 = (pcb_line_t *) ptrtmp;
					if (conf_core.editor.auto_drc) {
						/* undo loses PCB_FLAG_FOUND */
						PCB_FLAG_SET(PCB_FLAG_FOUND, ptr2);
						DrawLine(CURRENT, ptr2);
					}
					Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = ptr2->Point2.X;
					Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = ptr2->Point2.Y;
				}
				pcb_crosshair_grid_fit(Crosshair.X, Crosshair.Y);
				pcb_adjust_attached_objects();
				if (--addedLines == 0) {
					Crosshair.AttachedLine.State = STATE_SECOND;
					lastLayer = CURRENT;
				}
				else {
					/* this search is guaranteed to succeed too */
					pcb_search_obj_by_location(PCB_TYPE_LINE | PCB_TYPE_RATLINE, &ptr1,
																 &ptrtmp, &ptr3, Crosshair.AttachedLine.Point1.X, Crosshair.AttachedLine.Point1.Y, 0);
					ptr2 = (pcb_line_t *) ptrtmp;
					lastLayer = (pcb_layer_t *) ptr1;
				}
				pcb_notify_crosshair_change(pcb_true);
				return 0;
			}
		}
		if (conf_core.editor.mode == PCB_MODE_ARC) {
			if (Crosshair.AttachedBox.State == STATE_SECOND) {
				Crosshair.AttachedBox.State = STATE_FIRST;
				pcb_notify_crosshair_change(pcb_true);
				return 0;
			}
			if (Crosshair.AttachedBox.State == STATE_THIRD) {
				void *ptr1, *ptr2, *ptr3;
				pcb_box_t *bx;
				/* guaranteed to succeed */
				pcb_search_obj_by_location(PCB_TYPE_ARC, &ptr1, &ptr2, &ptr3,
															 Crosshair.AttachedBox.Point1.X, Crosshair.AttachedBox.Point1.Y, 0);
				bx = pcb_arc_get_ends((pcb_arc_t *) ptr2);
				Crosshair.AttachedBox.Point1.X = Crosshair.AttachedBox.Point2.X = bx->X1;
				Crosshair.AttachedBox.Point1.Y = Crosshair.AttachedBox.Point2.Y = bx->Y1;
				pcb_adjust_attached_objects();
				if (--addedLines == 0)
					Crosshair.AttachedBox.State = STATE_SECOND;
			}
		}
		/* undo the last destructive operation */
		if (Undo(pcb_true))
			SetChangedFlag(pcb_true);
	}
	else if (function) {
		switch (pcb_funchash_get(function, NULL)) {
			/* clear 'undo objects' list */
		case F_ClearList:
			ClearUndoList(pcb_false);
			break;
		}
	}
	pcb_notify_crosshair_change(pcb_true);
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

int ActionRedo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (((conf_core.editor.mode == PCB_MODE_POLYGON ||
				conf_core.editor.mode == PCB_MODE_POLYGON_HOLE) && Crosshair.AttachedPolygon.PointN) || Crosshair.AttachedLine.State == STATE_SECOND)
		return 1;
	pcb_notify_crosshair_change(pcb_false);
	if (Redo(pcb_true)) {
		SetChangedFlag(pcb_true);
		if (conf_core.editor.mode == PCB_MODE_LINE && Crosshair.AttachedLine.State != STATE_FIRST) {
			pcb_line_t *line = linelist_last(&CURRENT->Line);
			Crosshair.AttachedLine.Point1.X = Crosshair.AttachedLine.Point2.X = line->Point2.X;
			Crosshair.AttachedLine.Point1.Y = Crosshair.AttachedLine.Point2.Y = line->Point2.Y;
			addedLines++;
		}
	}
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}


pcb_hid_action_t undo_action_list[] = {
	{"Atomic", 0, ActionAtomic,
	 atomic_help, atomic_syntax}
	,
	{"Undo", 0, ActionUndo,
	 undo_help, undo_syntax}
	,
	{"Redo", 0, ActionRedo,
	 redo_help, redo_syntax}
};

PCB_REGISTER_ACTIONS(undo_action_list, NULL)
