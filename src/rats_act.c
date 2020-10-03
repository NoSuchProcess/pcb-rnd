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
#include "data.h"
#include <librnd/core/error.h>
#include "undo.h"
#include "find.h"
#include "remove.h"
#include "funchash_core.h"
#include "obj_rat.h"
#include <librnd/core/actions.h>
#include <librnd/core/funchash_core.h>
#include "netlist.h"
#include "draw.h"

#include "obj_rat_draw.h"


static const char pcb_acts_AddRats[] = "AddRats(AllRats|SelectedRats|Close, [manhattan])";
static const char pcb_acth_AddRats[] = "Add one or more rat lines to the board.";
/* DOC: addrats.html */
static fgw_error_t pcb_act_AddRats(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op, flgs = PCB_RATACC_PRECISE;
	const char *opts = NULL;
	pcb_rat_t *shorty;
	float len, small;

	RND_ACT_CONVARG(1, FGW_KEYWORD, AddRats, op = fgw_keyword(&argv[1]));

	TODO("toporouter: autorotuer: remove this when the old autorouter is removed");
	/* temporary hack for the old autorouter */
	RND_ACT_MAY_CONVARG(2, FGW_STR, AddRats, opts = argv[2].val.str);
	if ((opts != NULL) && (opts[0] == 'm'))
		flgs = PCB_RATACC_ONLY_MANHATTAN;

	if (conf_core.temp.rat_warn) {
		pcb_data_clear_flag(PCB->Data, PCB_FLAG_WARN, 1, 0);
		pcb_draw();
	}
	switch (op) {
		case F_AllRats:
			if (pcb_net_add_all_rats(PCB, flgs | PCB_RATACC_INFO) > 0)
				pcb_board_set_changed_flag(PCB, rnd_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (pcb_net_add_all_rats(PCB, flgs | PCB_RATACC_INFO | PCB_RATACC_ONLY_SELECTED) > 0)
				pcb_board_set_changed_flag(PCB, rnd_true);
			break;
		case F_Close:
			small = RND_SQUARE(RND_MAX_COORD);
			shorty = NULL;
			PCB_RAT_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
					continue;
				len = RND_SQUARE(line->Point1.X - line->Point2.X) + RND_SQUARE(line->Point1.Y - line->Point2.Y);
				if (len < small) {
					small = len;
					shorty = line;
				}
			}
			PCB_END_LOOP;
			if (shorty) {
				pcb_undo_add_obj_to_flag(shorty);
				PCB_FLAG_SET(PCB_FLAG_SELECTED, shorty);
				pcb_rat_invalidate_draw(shorty);
				pcb_draw();
				pcb_center_display(PCB, (shorty->Point2.X + shorty->Point1.X) / 2, (shorty->Point2.Y + shorty->Point1.Y) / 2);
			}
			break;

		default:
			RND_ACT_FAIL(AddRats);
	}
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Connection[] = "Connection(Find|ResetLinesAndPolygons|ResetPinsAndVias|Reset)";
static const char pcb_acth_Connection[] = "Searches connections of the object at the cursor position.";
/* DOC: connection.html */
static fgw_error_t pcb_act_Connection(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Connection, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Find:
			{
				rnd_coord_t x, y;
					unsigned long res;
					pcb_find_t fctx;
				rnd_hid_get_coords("Click on a connection", &x, &y, 0);
				memset(&fctx, 0, sizeof(fctx));
				fctx.flag_set = PCB_FLAG_FOUND;
				fctx.flag_chg_undoable = 1;
				fctx.consider_rats = !!conf_core.editor.conn_find_rat;
				res = pcb_find_from_xy(&fctx, PCB->Data, x, y);
				rnd_message(RND_MSG_INFO, "found %ld objects\n", res);
				pcb_find_free(&fctx);
				break;
			}

		case F_ResetLinesAndPolygons:
		case F_ResetLayerObjects:
			if (pcb_data_clear_obj_flag(PCB->Data, PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_TEXT, PCB_FLAG_FOUND, 1, 1) > 0) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;

		case F_ResetPinsViasAndPads:
		case F_ResetPadstacks:
			if (pcb_data_clear_obj_flag(PCB->Data, PCB_OBJ_PSTK, PCB_FLAG_FOUND, 1, 1) > 0) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;

		case F_Reset:
			if (pcb_data_clear_flag(PCB->Data, PCB_FLAG_FOUND, 1, 1) > 0) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;

		default:
			RND_ACT_FAIL(Connection);
	}

	RND_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_DeleteRats[] = "DeleteRats(AllRats|Selected|SelectedRats)";
static const char pcb_acth_DeleteRats[] = "Delete rat lines.";
static fgw_error_t pcb_act_DeleteRats(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	RND_ACT_CONVARG(1, FGW_KEYWORD, DeleteRats, op = fgw_keyword(&argv[1]));

	if (conf_core.temp.rat_warn) {
		pcb_data_clear_flag(PCB->Data, PCB_FLAG_WARN, 1, 0);
		pcb_draw();
	}
	switch(op) {
		case F_AllRats:
			if (pcb_rats_destroy(rnd_false))
				pcb_board_set_changed_flag(PCB, rnd_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (pcb_rats_destroy(rnd_true))
				pcb_board_set_changed_flag(PCB, rnd_true);
			break;

		default:
			RND_ACT_FAIL(DeleteRats);
	}

	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t rats_action_list[] = {
	{"AddRats", pcb_act_AddRats, pcb_acth_AddRats, pcb_acts_AddRats},
	{"Connection", pcb_act_Connection, pcb_acth_Connection, pcb_acts_Connection},
	{"DeleteRats", pcb_act_DeleteRats, pcb_acth_DeleteRats, pcb_acts_DeleteRats}
};

void pcb_rats_act_init2(void)
{
	RND_REGISTER_ACTIONS(rats_action_list, NULL);
}


