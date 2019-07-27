/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <genht/htpp.h>
#include "board.h"
#include "netlist.h"

typedef struct dyn_net_s dyn_net_t;
typedef struct dyn_obj_s dyn_obj_t;

struct dyn_net_s {
	pcb_net_t net;
	dyn_net_t *next;
};

struct dyn_obj_s {
	pcb_any_obj_t *obj;
	dyn_obj_t *next;
};

typedef struct pcb_netmap_s {
	htpp_t o2n;   /* of (pcb_lib_menu_t *); tells the net for an object */
	htpp_t n2o;   /* of (dyn_obj_t *); tells the object list for a net */
	pcb_cardinal_t anon_cnt;
	pcb_board_t *pcb;
	pcb_net_t *curr_net;
	dyn_net_t *dyn_nets;
} pcb_netmap_t;

int pcb_netmap_init(pcb_netmap_t *map, pcb_board_t *pcb);
int pcb_netmap_uninit(pcb_netmap_t *map);

