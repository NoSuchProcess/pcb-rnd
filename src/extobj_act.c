/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "actions.h"
#include "funchash_core.h"

#include "extobj.h"

static const char pcb_acts_ExtobjConvFrom[] = "ExtobjConvFrom(object|selected|buffer, extotype)";
static const char pcb_acth_ExtobjConvFrom[] = "Create a new extended object of extotype by converting existing objects";
fgw_error_t pcb_act_ExtobjConvFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	pcb_extobj_t *eo;
	const char *eoname = NULL;
	int op;
	pcb_subc_t *sc;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, ExtobjConvFrom, op = fgw_keyword(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_STR, ExtobjConvFrom, eoname = argv[2].val.str);

	eo = pcb_extobj_lookup(eoname);
	if (eo == NULL) {
		pcb_message(PCB_MSG_ERROR, "ExtobjConvFrom: extended object '%s' is not available\n", eoname);
		PCB_ACT_IRES(-1);
		return 0;
	}

	switch(op) {
		case F_Selected:
			sc = pcb_extobj_conv_selected_objs(pcb, eo, pcb->Data, pcb->Data, 1);
			break;
		case F_Object:
		case F_Buffer:
		default:
			PCB_ACT_FAIL(ExtobjConvFrom);
	}

	if (sc == NULL) {
		pcb_message(PCB_MSG_ERROR, "ExtobjConvFrom: failed to create the extended object\n");
		PCB_ACT_IRES(-1);
		return 0;
	}

	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t pcb_extobj_action_list[] = {
	{"ExtobjConvFrom", pcb_act_ExtobjConvFrom, pcb_acth_ExtobjConvFrom, pcb_acts_ExtobjConvFrom}
};

PCB_REGISTER_ACTIONS_FUNC(pcb_extobj_action_list, NULL)
