/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "actions.h"
#include "data.h"
#include "action_helper.h"
#include "error.h"
#include "funchash_core.h"

#include "undo.h"
#include "undo_act.h"
#include "polygon.h"
#include "search.h"

#include "obj_line_draw.h"

#include "tool.h"

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

fgw_error_t pcb_act_Atomic(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	PCB_ACT_CONVARG(1, FGW_KEYWORD, Atomic, op = argv[1].val.nat_keyword);

	switch (op) {
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
	default:
		pcb_message(PCB_MSG_ERROR, "Invalid argument for Atomic()\n");
		PCB_ACT_IRES(-1);
		return 0;
	}
	PCB_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Undo[] = "undo()\n" "undo(ClearList)";

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

fgw_error_t pcb_act_Undo(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	const char *function = PCB_ACTION_ARG(0);
	if (!function || !*function) {
		pcb_notify_crosshair_change(pcb_false);
		if (pcb_tool_undo_act())
			if (pcb_undo(pcb_true) == 0)
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
	PCB_OLD_ACT_END;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Redo[] = "redo()";

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

fgw_error_t pcb_act_Redo(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_notify_crosshair_change(pcb_false);
	if (pcb_tool_redo_act())
		if (pcb_redo(pcb_true))
			pcb_board_set_changed_flag(pcb_true);
	pcb_notify_crosshair_change(pcb_true);
	return 0;
	PCB_OLD_ACT_END;
}


pcb_action_t undo_action_list[] = {
	{"Atomic", pcb_act_Atomic, pcb_acth_Atomic, pcb_acts_Atomic},
	{"Undo", pcb_act_Undo, pcb_acth_Undo, pcb_acts_Undo},
	{"Redo", pcb_act_Redo, pcb_acth_Redo, pcb_acts_Redo}
};

PCB_REGISTER_ACTIONS(undo_action_list, NULL)
