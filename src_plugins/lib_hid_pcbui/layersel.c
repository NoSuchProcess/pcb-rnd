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

#include <genvector/vti0.h>

#include "hid.h"
#include "hid_cfg.h"
#include "hid_dad.h"

#include "board.h"
#include "event.h"
#include "layer.h"
#include "layer_grp.h"
#include "hidlib_conf.h"

#include "layersel.h"

typedef struct {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited;
} layersel_ctx_t;

static layersel_ctx_t layersel;

static void layersel_create_real(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	pcb_cardinal_t n;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		if (!(PCB_LAYER_IN_STACK(g->ltype)) || (g->ltype & PCB_LYT_SUBSTRATE))
			continue;
		PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
			/* vertical group name */
			PCB_DAD_LABEL(ls->sub.dlg, "grp");
			
			/* vert sep */
			PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
				PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_FRAME);
			PCB_DAD_END(ls->sub.dlg);

			PCB_DAD_BEGIN_VBOX(ls->sub.dlg);
				for(n = 0; n < g->len; n++) {
					pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[n]);
					PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
						PCB_DAD_LABEL(ls->sub.dlg, ly->name);
					PCB_DAD_END(ls->sub.dlg);
				}
			PCB_DAD_END(ls->sub.dlg);
		PCB_DAD_END(ls->sub.dlg);
	}
}

static void layersel_docked_create(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	PCB_DAD_BEGIN_VBOX(ls->sub.dlg);
		PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_EXPFILL | PCB_HATF_TIGHT /*| PCB_HATF_SCROLL*/);
		layersel_create_real(&layersel, pcb);
	PCB_DAD_END(ls->sub.dlg);
}


void pcb_layersel_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL)) {
		layersel_docked_create(&layersel, PCB);
		if (pcb_hid_dock_enter(&layersel.sub, PCB_HID_DOCK_LEFT, "layersel") == 0) {
			layersel.sub_inited = 1;
/*			layersel_pcb2dlg(&layersel, PCB);*/
		}
	}
}

void pcb_layersel_vis_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
/*	layersel_pcb2dlg(&layersel, PCB);*/
}

void pcb_layersel_stack_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
TODO("rebuild the whole widget");
/*	layersel_pcb2dlg(&layersel, PCB);*/
}
