/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include "toolbar.h"

typedef struct {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited;
} toolbar_ctx_t;

static toolbar_ctx_t toolbar;

static void toolbar_docked_create()
{
	int n;
	PCB_DAD_BEGIN_HBOX(toolbar.sub.dlg);
		PCB_DAD_COMPFLAG(toolbar.sub.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_LABEL(toolbar.sub.dlg, "TODO");
	PCB_DAD_END(toolbar.sub.dlg);
}


void pcb_toolbar_gui_init_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (PCB_HAVE_GUI_ATTR_DLG) {
		toolbar_docked_create();
		if (pcb_hid_dock_enter(&toolbar.sub, PCB_HID_DOCK_TOP_LEFT, "Toolbar") == 0)
			toolbar.sub_inited = 1;
	}
}

