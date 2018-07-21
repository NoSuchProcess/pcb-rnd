/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  2d drafting plugin
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <genvector/gds_char.h>
#include <genvector/vtp0.h>

#include "hid_attrib.h"
#include "hid_init.h"
#include "plugins.h"
#include "actions.h"
#include "funchash_core.h"
#include "search.h"

static const char *ddraft_cookie = "ddraft plugin";

#define EDGE_TYPES (PCB_OBJ_LINE | PCB_OBJ_ARC)
#define CUT_TYPES (PCB_OBJ_LINE | PCB_OBJ_ARC)

#include "trim.c"

static long do_trim(vtp0_t *edges, int kwobj)
{
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	long res = 0, n;

	switch(kwobj) {
		case F_Object:
			for(;;) {
				x = PCB_MAX_COORD;
				pcb_hid_get_coords("Select an object to cut or press esc", &x, &y, 1);
				if (x == PCB_MAX_COORD)
					break;

				type = pcb_search_screen(x, y, CUT_TYPES, &ptr1, &ptr2, &ptr3);
				if (type == 0) {
					pcb_message(PCB_MSG_ERROR, "Can't cut that object\n");
					continue;
				}
				for(n = 0; n < vtp0_len(edges); n++)
					res += pcb_trim((pcb_any_obj_t *)edges->array[n], (pcb_any_obj_t *)ptr2, x, y);
			}
			break;
		default:
			pcb_message(PCB_MSG_ERROR, "Invalid second argument\n");
			return -1;
	}

	return res;
}

static const char pcb_acts_trim[] = "trim([selected|found|object], [selected|found|object])\n";
static const char pcb_acth_trim[] = "Use one or more objects as cutting edge and trim other objects. First argument is the cutting edge";
static fgw_error_t pcb_act_trim(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int kwcut = F_Object, kwobj = F_Object;
	pcb_objtype_t type;
	void *ptr1, *ptr2, *ptr3;
	pcb_coord_t x, y;
	vtp0_t edges;

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, trim, kwcut = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_KEYWORD, trim, kwobj = fgw_keyword(&argv[2]));

	vtp0_init(&edges);

	if ((kwobj == kwcut) && (kwobj != F_Object)) {
		pcb_message(PCB_MSG_ERROR, "Both cutting edge and objects to cut can't be '%s'\n", kwcut == F_Selected ? "selected" : "found");
		goto err;
	}

	switch(kwcut) {
		case F_Object:
			pcb_hid_get_coords("Select cutting edge object", &x, &y, 0);
			type = pcb_search_screen(x, y, EDGE_TYPES, &ptr1, &ptr2, &ptr3);
			if (type == 0) {
				pcb_message(PCB_MSG_ERROR, "Invalid cutting edge object\n");
				goto err;
			}
			vtp0_append(&edges, ptr2);
			break;
		case F_Selected:
		case F_Found:
		default:
			pcb_message(PCB_MSG_ERROR, "Invalid first argument\n");
			goto err;
	}

	if (do_trim(&edges, kwobj) < 0)
		goto err;

	PCB_ACT_IRES(0);
	vtp0_uninit(&edges);
	return 0;

	err:;
	PCB_ACT_IRES(-1);
	vtp0_uninit(&edges);
	return 0;
}


static pcb_action_t ddraft_action_list[] = {
	{"trim", pcb_act_trim, pcb_acth_trim, pcb_acts_trim}
};

PCB_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)

int pplg_check_ver_ddraft(int ver_needed) { return 0; }

void pplg_uninit_ddraft(void)
{
	pcb_remove_actions_by_cookie(ddraft_cookie);
}

#include "dolists.h"
int pplg_init_ddraft(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(ddraft_action_list, ddraft_cookie)

	return 0;
}
