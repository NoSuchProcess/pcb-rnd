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

#include "board.h"
#include "conf_core.h"
#include "safe_fs.h"

static pcb_hidval_t infobar_timer;
static int infobar_timer_active = 0;
static double last_interval = -2;
static double last_date = -1;
static int infobar_gui_inited = 0;

static void pcb_infobar_brdchg_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_actionl("InfoBarFileChanged", "close");
	if ((PCB != NULL) && (PCB->hidlib.filename != NULL))
		last_date = pcb_file_mtime(PCB->hidlib.filename);
	else
		last_date = -1;
}


static void infobar_tick(pcb_hidval_t user_data)
{
	if (conf_core.rc.file_changed_interval > 0) {
		infobar_timer = pcb_gui->add_timer(infobar_tick, (conf_core.rc.file_changed_interval * 1000.0), user_data);
		last_interval = conf_core.rc.file_changed_interval;
		infobar_timer_active = 1;
	}
	else
		infobar_timer_active = 0;

	if (infobar_timer_active) { /* check for file change */
		if ((PCB != NULL) && (PCB->hidlib.filename != NULL)) {
			double last_chg = pcb_file_mtime(PCB->hidlib.filename);
			pcb_trace("check for file change %f %f!\n", last_date, last_chg);
			if (last_chg > last_date) {
				last_date = last_chg;
				pcb_actionl("InfoBarFileChanged", "open");
			}
		}
	}
}

static void pcb_infobar_update_conf(conf_native_t *cfg, int arr_idx)
{
	if ((!infobar_gui_inited) || (last_interval == conf_core.rc.file_changed_interval))
		return;
	if ((infobar_timer_active) && (pcb_gui != NULL) && (pcb_gui->stop_timer != NULL)) {
		pcb_gui->stop_timer(infobar_timer);
		infobar_timer_active = 0;
	}
	infobar_tick(infobar_timer);
}


static void pcb_infobar_gui_init_ev(void *user_data, int argc, pcb_event_arg_t argv[])
{
	infobar_gui_inited = 1;
	if (!infobar_timer_active)
		infobar_tick(infobar_timer);
}

