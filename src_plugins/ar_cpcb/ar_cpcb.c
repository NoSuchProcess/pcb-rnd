/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  auto routing with c-pcb (selective import/export in c-pcb format)
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

#include "plugins.h"
#include "actions.h"

static const char *cpcb_cookie = "cpcb plugin";

static const char pcb_acts_import_cpcb[] = "ImportcpcbFrom(filename)";
static const char pcb_acth_import_cpcb[] = "Loads the auto-routed tracks from the specified c-pcb output.";
fgw_error_t pcb_act_import_cpcb(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}

static const char pcb_acts_export_cpcb[] = "ExportcpcbTo(filename)";
static const char pcb_acth_export_cpcb[] = "Dumps the current board in c-pcb format.";
fgw_error_t pcb_act_export_cpcb(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	return -1;
}


static pcb_action_t cpcb_action_list[] = {
	{"ImportcpcbFrom", pcb_act_import_cpcb, pcb_acth_import_cpcb, pcb_acts_import_cpcb},
	{"ExportcpcbTo", pcb_act_export_cpcb, pcb_acth_export_cpcb, pcb_acts_export_cpcb},
};

PCB_REGISTER_ACTIONS(cpcb_action_list, cpcb_cookie)

int pplg_check_ver_cpcb(int ver_needed) { return 0; }

void pplg_uninit_cpcb(void)
{
	pcb_remove_actions_by_cookie(cpcb_cookie);
}


#include "dolists.h"
int pplg_init_cpcb(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(cpcb_action_list, cpcb_cookie)

	return 0;
}
