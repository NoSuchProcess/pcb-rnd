/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2022 Tibor 'Igor2' Palinkas
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
#include <librnd/core/actions.h>
#include "board.h"
#include "data.h"
#include "layer_vis.h"
#include "event.h"

static int have_gui, currly;
static int save_l_ons[PCB_MAX_LAYER], save_g_ons[PCB_MAX_LAYERGRP];

static void export_sess_pre(void)
{
	currly = PCB_CURRLID(PCB);
	have_gui = (rnd_gui != NULL) && rnd_gui->gui;
	if (have_gui) {
		pcb_hid_save_and_show_layer_ons(save_l_ons);
		pcb_hid_save_and_show_layergrp_ons(save_g_ons);
	}
}

static void export_sess_post(void)
{
	if (have_gui) {
		pcb_hid_restore_layer_ons(save_l_ons);
		pcb_hid_restore_layergrp_ons(save_g_ons);
		pcb_layervis_change_group_vis(&PCB->hidlib, currly, 1, 1);
		rnd_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	}
}

