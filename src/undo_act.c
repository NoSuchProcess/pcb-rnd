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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
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
#include <librnd/core/actions.h>
#include "data.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include "funchash_core.h"

#include "undo.h"
#include "undo_act.h"
#include "polygon.h"
#include "search.h"

#include "obj_line_draw.h"

#include <librnd/core/tool.h>

static const char pcb_acts_Atomic[] = "Atomic(Save|Restore|Close|Block)";
static const char pcb_acth_Atomic[] = "Save or restore the undo serial number.";
/* DOC: atomic.html */
fgw_error_t pcb_act_Atomic(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	RND_ACT_CONVARG(1, FGW_KEYWORD, Atomic, op = fgw_keyword(&argv[1]));

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
	case F_Freeze:
		pcb_undo_freeze_serial();
		break;
	case F_UnFreeze:
	case F_Thaw:
		pcb_undo_unfreeze_serial();
		break;
	default:
		rnd_message(RND_MSG_ERROR, "Invalid argument for Atomic()\n");
		RND_ACT_IRES(-1);
		return 0;
	}
	RND_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Undo[] = "undo()\n" "undo(ClearList|FreezeSerial|UnfreezeSerial|IncSerial|GetSerial|Above)";

static const char pcb_acth_Undo[] = "Undo recent changes.";

/* DOC: undo.html */

fgw_error_t pcb_act_Undo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *function = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, Undo, function = argv[1].val.str);
	if (!function || !*function) {
		rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
		if (rnd_tool_undo_act(RND_ACT_HIDLIB))
			if (pcb_undo(rnd_true) == 0)
				pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_true);
	}
	else if (function) {
		rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
		if (rnd_strcasecmp(function, "ClearList") == 0)
			pcb_undo_clear_list(rnd_false);
		else if (rnd_strcasecmp(function, "FreezeSerial") == 0)
			pcb_undo_freeze_serial();
		else if (rnd_strcasecmp(function, "UnFreezeSerial") == 0)
			pcb_undo_unfreeze_serial();
		else if (rnd_strcasecmp(function, "IncSerial") == 0)
			pcb_undo_inc_serial();
		else if (rnd_strcasecmp(function, "GetSerial") == 0) {
			res->type = FGW_LONG;
			res->val.nat_long = pcb_undo_serial();
			return 0;
		}
		else if (rnd_strcasecmp(function, "GetNum") == 0) {
			res->type = FGW_LONG;
			res->val.nat_long = pcb_num_undo();
			return 0;
		}
		else if (rnd_strcasecmp(function, "Above") == 0) {
			long ser;
			RND_ACT_CONVARG(2, FGW_LONG, Undo, ser = argv[2].val.nat_long);
			pcb_undo_above(ser);
		}

	}
	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
	RND_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Redo[] = "redo()";

static const char pcb_acth_Redo[] = "Redo recent \"undo\" operations.";

/* DOC: redo.html */

fgw_error_t pcb_act_Redo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
	if (rnd_tool_redo_act(RND_ACT_HIDLIB))
		if (pcb_redo(rnd_true))
			pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_true);
	rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t undo_action_list[] = {
	{"Atomic", pcb_act_Atomic, pcb_acth_Atomic, pcb_acts_Atomic},
	{"Undo", pcb_act_Undo, pcb_acth_Undo, pcb_acts_Undo},
	{"Redo", pcb_act_Redo, pcb_acth_Redo, pcb_acts_Redo}
};

void pcb_undo_act_init2(void)
{
	RND_REGISTER_ACTIONS(undo_action_list, NULL);
}


