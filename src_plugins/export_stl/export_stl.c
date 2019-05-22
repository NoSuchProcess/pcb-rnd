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
#include "hid_init.h"
#include "hid_attrib.h"
#include "plugins.h"

const char *stl_cookie = "export_stl HID";


static const char pcb_acts_ExportSTL[] = "ExportSTL()\n";
static const char pcb_acth_ExportSTL[] = "Export a three dimensional triangulated surface model in stl";
/*TODO: DOC: exportSTL.html */
fgw_error_t pcb_act_ExportSTL(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_FAIL(ExportSTL);
}

static pcb_action_t stl_action_list[] = {
	{"ExportSTL", pcb_act_ExportSTL, pcb_acth_ExportSTL, pcb_acts_ExportSTL}
};

PCB_REGISTER_ACTIONS(stl_action_list, stl_cookie)

int pplg_check_ver_export_stl(int ver_needed) { return 0; }

void pplg_uninit_export_stl(void)
{
	pcb_remove_actions_by_cookie(stl_cookie);
}

#include "dolists.h"

int pplg_init_export_stl(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(stl_action_list, stl_cookie)

	return 0;
}
