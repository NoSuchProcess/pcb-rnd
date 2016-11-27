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
#include "search.h"

#include "obj_line_draw.h"

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Atomic[] = "Atomic(Save|Restore|Close|Block)";

static const char pcb_acth_Atomic[] = "Save or restore the undo serial number.";

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

int pcb_act_Atomic(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc != 1)
		PCB_ACT_FAIL(Atomic);

	switch (pcb_funchash_get(argv[0], NULL)) {
	case F_Save:
		pcb_undo_save_serial();
		break;
	case F_Restore:
		pcb_undo_restore_serial();
		break;
	case F_Close:
		pcb_undo_restore_serial();
		pcb_undo_inc_serial();
		break;
	case F_Block:
		pcb_undo_restore_serial();
		if (pcb_bumped)
			pcb_undo_inc_serial();
		break;
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Undo[] = "pcb_undo()\n" "pcb_undo(ClearList)";

static const char pcb_acth_Undo[] = "Undo recent changes.";

/* %start-doc actions Undo

The unlimited undo feature of @code{Pcb} allows you to recover from
most operations that materially affect you work.  Calling
@code{pcb_undo()} without any parameter recovers from the last (non-undo)
operation. @code{ClearList} is used to release the allocated
memory. @code{ClearList} is called whenever a new layout is started or
loaded. See also @code{Redo} and @code{Atomic}.

Note that undo groups operations by serial number; changes with the
same serial number will be undone (or redone) as a group.  See
@code{Atomic}.

%end-doc */

int pcb_act_Undo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (!function || !*function) {
		/* don't allow undo in the middle of an operation */
		if (conf_core.editor.mode != PCB_MODE_POLYGON_HOLE && pcb_crosshair.AttachedObject.State != PCB_CH_STATE_FIRST)
			return 1;
		if (pcb_crosshair.AttachedBox.State != PCB_CH_STATE_FIRST && conf_core.editor.mode != PCB_MODE_ARC)
			return 1;
		/* undo the last operation */

		pcb_notify_crosshair_change(pcb_false);
		if ((conf_core.editor.mode == PCB_MODE_POLYGON || conf_core.editor.mode == PCB_MODE_POLYGON_HOLE) && pcb_crosshair.AttachedPolygon.PointN) {
			pcb_polygon_go_to_prev_point();
			pcb_notify_crosshair_change(pcb_true);
			return 0;
		}
		/* move anchor point if undoing during line creation */
		if (conf_core.editor.mode == PCB_MODE_LINE) {
			if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND) {
				if (conf_core.editor.auto_drc)
					pcb_undo(pcb_true);						/* undo the connection find */
				pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
				pcb_crosshair_set_local_ref(0, 0, pcb_false);
				pcb_notify_crosshair_change(pcb_true);
				return 0;
			}
			if (pcb_crosshair.AttachedLine.State == PCB_CH_STATE_THIRD) {
				int type;
				void *ptr1, *ptr3, *ptrtmp;
				pcb_line_t *ptr2;
				/* this search is guaranteed to succeed */
				pcb_search_obj_by_location(PCB_TYPE_LINE | PCB_TYPE_RATLINE, &ptr1,
															 &ptrtmp, &ptr3, pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y, 0);
				ptr2 = (pcb_line_t *) ptrtmp;

				/* save both ends of line */
				pcb_crosshair.AttachedLine.Point2.X = ptr2->Point1.X;
				pcb_crosshair.AttachedLine.Point2.Y = ptr2->Point1.Y;
				if ((type = pcb_undo(pcb_true)))
					pcb_board_set_changed_flag(pcb_true);
				/* check that the undo was of the right type */
				if ((type & PCB_UNDO_CREATE) == 0) {
					/* wrong undo type, restore anchor points */
					pcb_crosshair.AttachedLine.Point2.X = pcb_crosshair.AttachedLine.Point1.X;
					pcb_crosshair.AttachedLine.Point2.Y = pcb_crosshair.AttachedLine.Point1.Y;
					pcb_notify_crosshair_change(pcb_true);
					return 0;
				}
				/* move to new anchor */
				pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X;
				pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y;
				/* check if an intermediate point was removed */
				if (type & PCB_UNDO_REMOVE) {
					/* this search should find the restored line */
					pcb_search_obj_by_location(PCB_TYPE_LINE | PCB_TYPE_RATLINE, &ptr1,
																 &ptrtmp, &ptr3, pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y, 0);
					ptr2 = (pcb_line_t *) ptrtmp;
					if (conf_core.editor.auto_drc) {
						/* undo loses PCB_FLAG_FOUND */
						PCB_FLAG_SET(PCB_FLAG_FOUND, ptr2);
						DrawLine(CURRENT, ptr2);
					}
					pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = ptr2->Point2.X;
					pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = ptr2->Point2.Y;
				}
				pcb_crosshair_grid_fit(pcb_crosshair.X, pcb_crosshair.Y);
				pcb_adjust_attached_objects();
				if (--pcb_added_lines == 0) {
					pcb_crosshair.AttachedLine.State = PCB_CH_STATE_SECOND;
					lastLayer = CURRENT;
				}
				else {
					/* this search is guaranteed to succeed too */
					pcb_search_obj_by_location(PCB_TYPE_LINE | PCB_TYPE_RATLINE, &ptr1,
																 &ptrtmp, &ptr3, pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y, 0);
					ptr2 = (pcb_line_t *) ptrtmp;
					lastLayer = (pcb_layer_t *) ptr1;
				}
				pcb_notify_crosshair_change(pcb_true);
				return 0;
			}
		}
		if (conf_core.editor.mode == PCB_MODE_ARC) {
			if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_SECOND) {
				pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
				pcb_notify_crosshair_change(pcb_true);
				return 0;
			}
			if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD) {
				void *ptr1, *ptr2, *ptr3;
				/* guaranteed to succeed */
				pcb_search_obj_by_location(PCB_TYPE_ARC, &ptr1, &ptr2, &ptr3,
															 pcb_crosshair.AttachedBox.Point1.X, pcb_crosshair.AttachedBox.Point1.Y, 0);
				pcb_arc_get_end((pcb_arc_t *) ptr2, 0, &pcb_crosshair.AttachedBox.Point2.X, &pcb_crosshair.AttachedBox.Point2.Y);
				pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X;
				pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y;
				pcb_adjust_attached_objects();
				if (--pcb_added_lines == 0)
					pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
			}
		}
		/* undo the last destructive operation */
		if (pcb_undo(pcb_true))
			pcb_board_set_changed_flag(pcb_true);
	}
	else if (function) {
		switch (pcb_funchash_get(function, NULL)) {
			/* clear 'undo objects' list */
		case F_ClearList:
			pcb_undo_clear_list(pcb_false);
			break;
		}
	}
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Redo[] = "pcb_redo()";

static const char pcb_acth_Redo[] = "Redo recent \"undo\" operations.";

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

int pcb_act_Redo(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (((conf_core.editor.mode == PCB_MODE_POLYGON ||
				conf_core.editor.mode == PCB_MODE_POLYGON_HOLE) && pcb_crosshair.AttachedPolygon.PointN) || pcb_crosshair.AttachedLine.State == PCB_CH_STATE_SECOND)
		return 1;
	pcb_notify_crosshair_change(pcb_false);
	if (pcb_redo(pcb_true)) {
		pcb_board_set_changed_flag(pcb_true);
		if (conf_core.editor.mode == PCB_MODE_LINE && pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST) {
			pcb_line_t *line = linelist_last(&CURRENT->Line);
			pcb_crosshair.AttachedLine.Point1.X = pcb_crosshair.AttachedLine.Point2.X = line->Point2.X;
			pcb_crosshair.AttachedLine.Point1.Y = pcb_crosshair.AttachedLine.Point2.Y = line->Point2.Y;
			pcb_added_lines++;
		}
	}
	pcb_notify_crosshair_change(pcb_true);
	return 0;
}


pcb_hid_action_t undo_action_list[] = {
	{"Atomic", 0, pcb_act_Atomic,
	 pcb_acth_Atomic, pcb_acts_Atomic}
	,
	{"Undo", 0, pcb_act_Undo,
	 pcb_acth_Undo, pcb_acts_Undo}
	,
	{"Redo", 0, pcb_act_Redo,
	 pcb_acth_Redo, pcb_acts_Redo}
};

PCB_REGISTER_ACTIONS(undo_action_list, NULL)
