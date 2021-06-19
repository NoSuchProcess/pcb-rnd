/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#include "board.h"

#include <genht/htpp.h>
#include "../src_plugins/query/net_len.h"


typedef enum { /* bits */
	PCB_2NETMAPCTRL_RATS = 1      /* include rat lines */
} pcb_2netmap_control_t;

typedef struct pcb_2netmap_seg_s {
	pcb_qry_netseg_len_t *seg;
	pcb_net_t *net;
	unsigned shorted:1; /* set if the segment connects two different nets */
} pcb_2netmap_seg_t;

typedef struct pcb_2netmap_s {
	pcb_2netmap_control_t ctrl;
	htpp_t o2n;   /* of (pcb_2netmap_seg_t *); tells the net for an object */

	/* internal */
	pcb_qry_exec_t *ec;
} pcb_2netmap_t;


int pcb_map_2nets_init(pcb_2netmap_t *map, pcb_board_t *pcb, pcb_2netmap_control_t how);
int pcb_map_2nets_uninit(pcb_2netmap_t *map);
