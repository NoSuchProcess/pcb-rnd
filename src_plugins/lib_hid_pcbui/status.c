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

#include "hid.h"
#include "hid_dad.h"
#include "conf_core.h"

#include "status.h"

typedef struct {
	pcb_hid_dad_subdialog_t stsub, rdsub; /* st is for the bottom status line, rd is for the top readouts */
	int stsub_inited, rdsub_inited;
} status_ctx_t;

static status_ctx_t status;

static void status_pcb2dlg()
{
	if (status.stsub_inited) {
	
	}

	if (status.rdsub_inited) {
	
	}
}

static void status_docked_create_st()
{
	PCB_DAD_BEGIN_HBOX(status.stsub.dlg);
		PCB_DAD_COMPFLAG(status.stsub.dlg, PCB_HATF_EXPFILL | PCB_HATF_TIGHT);
		PCB_DAD_LABEL(status.stsub.dlg, "TODO");
	PCB_DAD_END(status.stsub.dlg);
}


void pcb_status_gui_init_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL)) {
		status_docked_create_st();
		if (pcb_hid_dock_enter(&status.stsub, PCB_HID_DOCK_BOTTOM, "status") == 0) {
			status.stsub_inited = 1;
		}
		
		status_pcb2dlg();
	}
}

void pcb_status_update_conf(conf_native_t *cfg, int arr_idx)
{
	status_pcb2dlg();
}
