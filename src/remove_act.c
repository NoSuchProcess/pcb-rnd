/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
#include "data.h"
#include <librnd/core/actions.h>
#include <librnd/core/tool.h>
#include "remove.h"
#include "board.h"
#include "funchash_core.h"

static const char pcb_acts_Delete[] = 
	"Delete(Object [,idpath])\n"
	"Delete(Selected)\n"
	"Delete(AllRats|SelectedRats)";
static const char pcb_acth_Delete[] = "Delete stuff.";
static fgw_error_t pcb_act_Delete(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int id;

	RND_PCB_ACT_CONVARG(1, FGW_KEYWORD, Delete, id = fgw_keyword(&argv[1]));

	if (id == -1) { /* no arg */
		if (pcb_remove_selected(rnd_false) == rnd_false)
			id = F_Object;
	}

	switch(id) {
	case F_Object:
		if (argc > 2) { /* delete by idpath */
			pcb_idpath_t *idp;
			pcb_any_obj_t *obj;
			RND_PCB_ACT_CONVARG(2, FGW_IDPATH, Delete, idp = fgw_idpath(&argv[2]));
			if ((idp == NULL) || !fgw_ptr_in_domain(&rnd_fgw, &argv[2], RND_PTR_DOMAIN_IDPATH))
				return FGW_ERR_PTR_DOMAIN;
			obj = pcb_idpath2obj(PCB, idp);
			if ((obj == NULL) || ((obj->type & PCB_OBJ_CLASS_REAL) == 0)) {
				RND_ACT_IRES(-1);
				return 0;
			}
			pcb_remove_object(obj->type, obj->parent.any, obj, obj);
		}
		else { /* interactive remove */
			rnd_hidlib_t *hidlib = RND_ACT_HIDLIB;
			rnd_hid_get_coords("Click on object to delete", &hidlib->tool_x, &hidlib->tool_y, 0);
			pcb_tool_save(RND_ACT_HIDLIB);
			pcb_tool_select_by_name(RND_ACT_HIDLIB, "remove");
			pcb_tool_do_press(RND_ACT_HIDLIB);
			pcb_tool_restore(RND_ACT_HIDLIB);
		}
		break;
	case F_Selected:
		pcb_remove_selected(rnd_false);
		break;
	case F_AllRats:
		if (pcb_rats_destroy(rnd_false))
			pcb_board_set_changed_flag(rnd_true);
		break;
	case F_SelectedRats:
		if (pcb_rats_destroy(rnd_true))
			pcb_board_set_changed_flag(rnd_true);
		break;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_RemoveSelected[] = "pcb_remove_selected()";
static const char pcb_acth_RemoveSelected[] = "Removes any selected objects.";
static fgw_error_t pcb_act_RemoveSelected(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (pcb_remove_selected(rnd_false))
		pcb_board_set_changed_flag(rnd_true);
	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t remove_action_list[] = {
	{"Delete", pcb_act_Delete, pcb_acth_Delete, pcb_acts_Delete},
	{"PcbDelete", pcb_act_Delete, NULL, "Alias to Delete()"},
	{"RemoveSelected", pcb_act_RemoveSelected, pcb_acth_RemoveSelected, pcb_acts_RemoveSelected}
};

void pcb_remove_act_init2(void)
{
	RND_REGISTER_ACTIONS(remove_action_list, NULL);
}
