/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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
#include "event.h"

static const char *pcb_evnames[] = {
	"pcbev_new_pstk",
	"pcbev_route_styles_changed",
	"pcbev_netlist_changed",
	"pcbev_layers_changed",
	"pcbev_layer_changed_grp",
	"pcbev_layervis_changed",
	"pcbev_library_changed",
	"pcbev_font_changed",
	"pcbev_undo_post",
	"pcbev_rubber_reset",
	"pcbev_rubber_move",
	"pcbev_rubber_move_draw",
	"pcbev_rubber_rotate90",
	"pcbev_rubber_rotate",
	"pcbev_rubber_lookup_lines",
	"pcbev_rubber_lookup_rats",
	"pcbev_rubber_constrain_main_line",
	"pcbev_draw_crosshair_chatt",
	"pcbev_drc_run",
	"pcbev_net_indicate_short"
};

void pcb_event_init_app(void)
{
	pcb_event_app_reg(PCB_EVENT_last, pcb_evnames, sizeof(pcb_evnames));
}
