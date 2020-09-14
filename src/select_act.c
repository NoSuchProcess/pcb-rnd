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
#include "funchash_core.h"

#include <librnd/core/tool.h>
#include "select.h"
#include "draw.h"
#include "remove.h"
#include "move.h"
#include "tool_logic.h"
#include <librnd/core/grid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/actions.h>


static const char pcb_acts_Select[] =
	"Select(Object, [idpath])\n"
	"Select(ToggleObject)\n"
	"Select(All|Block|Connection|Invert)\n"
	"Select(Convert)";
static const char pcb_acth_Select[] = "Toggles or sets the selection.";
/* DOC: select.html */
static fgw_error_t pcb_act_Select(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int op;
	RND_ACT_CONVARG(1, FGW_KEYWORD, Select, op = fgw_keyword(&argv[1]));

	switch(op) {

			/* select a single object */
		case F_Object:
			if (argc > 2) { /* select by idpath */
				pcb_idpath_t *idp;
				pcb_any_obj_t *obj;
				RND_ACT_CONVARG(2, FGW_IDPATH, Select, idp = fgw_idpath(&argv[2]));
				if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[2], RND_PTR_DOMAIN_IDPATH))
					return FGW_ERR_PTR_DOMAIN;
				obj = pcb_idpath2obj(PCB, idp);
				if ((obj == NULL) || ((obj->type & PCB_OBJ_CLASS_REAL) == 0)) {
					RND_ACT_IRES(-1);
					return 0;
				}
				pcb_undo_add_obj_to_flag(obj);
				PCB_FLAG_SET(PCB_FLAG_SELECTED, obj);
				pcb_draw_invalidate(obj);
				pcb_board_set_changed_flag(pcb, rnd_true);
			}
			else {
		case F_ToggleObject:
				if (pcb_select_object(PCB)) {
					pcb_board_set_changed_flag(pcb, rnd_true);
					rnd_gui->invalidate_all(rnd_gui);
				}
			}
			break;

			/* all objects in block */
		case F_Block:
			{
				rnd_box_t box;

				box.X1 = MIN(pcb_crosshair.AttachedBox.Point1.X, pcb_crosshair.AttachedBox.Point2.X);
				box.Y1 = MIN(pcb_crosshair.AttachedBox.Point1.Y, pcb_crosshair.AttachedBox.Point2.Y);
				box.X2 = MAX(pcb_crosshair.AttachedBox.Point1.X, pcb_crosshair.AttachedBox.Point2.X);
				box.Y2 = MAX(pcb_crosshair.AttachedBox.Point1.Y, pcb_crosshair.AttachedBox.Point2.Y);
				rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
				pcb_tool_notify_block();
				if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD && pcb_select_block(PCB, &box, rnd_true, rnd_true, rnd_false)) {
					pcb_board_set_changed_flag(pcb, rnd_true);
					pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
				}
				rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
				rnd_gui->invalidate_all(rnd_gui);
				break;
			}

			/* select all visible(?) objects */
		case F_All:
			{
				rnd_box_t box;

				box.X1 = -RND_MAX_COORD;
				box.Y1 = -RND_MAX_COORD;
				box.X2 = RND_MAX_COORD;
				box.Y2 = RND_MAX_COORD;
				if (pcb_select_block(PCB, &box, rnd_true, rnd_true, rnd_false)) {
					pcb_board_set_changed_flag(pcb, rnd_true);
					rnd_gui->invalidate_all(rnd_gui);
				}
				break;
			}

		case F_Invert:
			{
				rnd_box_t box;

				box.X1 = -RND_MAX_COORD;
				box.Y1 = -RND_MAX_COORD;
				box.X2 = RND_MAX_COORD;
				box.Y2 = RND_MAX_COORD;
				if (pcb_select_block(PCB, &box, rnd_true, rnd_true, rnd_true)) {
					pcb_board_set_changed_flag(pcb, rnd_true);
					rnd_gui->invalidate_all(rnd_gui);
				}
				break;
			}

			/* all found connections */
		case F_Connection:
			if (pcb_select_connection(PCB, rnd_true)) {
				pcb_draw();
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb, rnd_true);
			}
			break;

		case F_Convert:
		case F_ConvertSubc:
			{
				rnd_coord_t x, y;
				pcb_crosshair_note.Buffer = conf_core.editor.buffer_number;
				pcb_buffer_set_number(PCB_MAX_BUFFER - 1);
				pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
				rnd_hid_get_coords("Select the Subcircuit's Origin (mark) Location", &x, &y, 0);
				x = rnd_grid_fit(x, RND_ACT_HIDLIB->grid, RND_ACT_HIDLIB->grid_ox);
				y = rnd_grid_fit(y, RND_ACT_HIDLIB->grid, RND_ACT_HIDLIB->grid_oy);
				pcb_buffer_add_selected(PCB, PCB_PASTEBUFFER, x, y, rnd_true, rnd_false);
				pcb_undo_save_serial();
				pcb_remove_selected(0);
				pcb_subc_convert_from_buffer(PCB_PASTEBUFFER);
				pcb_undo_restore_serial();
				pcb_buffer_copy_to_layout(PCB, x, y, rnd_false);
				pcb_buffer_set_number(pcb_crosshair_note.Buffer);
			}
			break;

		default:
			RND_ACT_FAIL(Select);
			break;
	}

	RND_ACT_IRES(0);
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_Unselect[] = "Unselect(All|Block|Connection)\n";
static const char pcb_acth_Unselect[] = "Unselects the object at the pointer location or the specified objects.";
/* DOC: unselect.html */
static fgw_error_t pcb_act_Unselect(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int op;
	RND_ACT_CONVARG(1, FGW_KEYWORD, Unselect, op = fgw_keyword(&argv[1]));

	switch(op) {

			/* all objects in block */
		case F_Block:
			{
				rnd_box_t box;

				box.X1 = MIN(pcb_crosshair.AttachedBox.Point1.X, pcb_crosshair.AttachedBox.Point2.X);
				box.Y1 = MIN(pcb_crosshair.AttachedBox.Point1.Y, pcb_crosshair.AttachedBox.Point2.Y);
				box.X2 = MAX(pcb_crosshair.AttachedBox.Point1.X, pcb_crosshair.AttachedBox.Point2.X);
				box.Y2 = MAX(pcb_crosshair.AttachedBox.Point1.Y, pcb_crosshair.AttachedBox.Point2.Y);
				rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_false);
				pcb_tool_notify_block();
				if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD && pcb_select_block(PCB, &box, rnd_false, rnd_true, rnd_false)) {
					pcb_board_set_changed_flag(pcb, rnd_true);
					pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
				}
				rnd_hid_notify_crosshair_change(RND_ACT_HIDLIB, rnd_true);
				break;
			}

			/* unselect all visible objects */
		case F_All:
			{
				rnd_box_t box;

				box.X1 = -RND_MAX_COORD;
				box.Y1 = -RND_MAX_COORD;
				box.X2 = RND_MAX_COORD;
				box.Y2 = RND_MAX_COORD;
				if (pcb_select_block(PCB, &box, rnd_false, rnd_false, rnd_false))
					pcb_board_set_changed_flag(pcb, rnd_true);
				break;
			}

			/* all found connections */
		case F_Connection:
			if (pcb_select_connection(PCB, rnd_false)) {
				pcb_draw();
				pcb_undo_inc_serial();
				pcb_board_set_changed_flag(pcb, rnd_true);
			}
			break;

		default:
			RND_ACT_FAIL(Unselect);
			break;

	}
	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t select_action_list[] = {
	{"Select", pcb_act_Select, pcb_acth_Select, pcb_acts_Select},
	{"Unselect", pcb_act_Unselect, pcb_acth_Unselect, pcb_acts_Unselect}
};

void pcb_select_act_init2(void)
{
	RND_REGISTER_ACTIONS(select_action_list, NULL);
}


