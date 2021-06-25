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

#include "obj_rat.h"
#include "obj_poly.h"
#include "obj_pstk.h"

#include <genht/htpp.h>
#include "../src_plugins/query/net_len.h"


typedef enum { /* bits */
	PCB_2NETMAPCTRL_RATS = 1      /* include rat lines */
} pcb_2netmap_control_t;

typedef struct pcb_2netmap_iseg_s pcb_2netmap_iseg_t;
struct pcb_2netmap_iseg_s {
	pcb_qry_netseg_len_t *seg;
	pcb_net_t *net;
	unsigned shorted:1; /* set if the segment connects two different nets */
	unsigned used:1;    /* already part of an output segment */
	char term[2];       /* 1 if ->seg's corresponding end is a terminal */
	void *mark;         /* for the A* */
	pcb_2netmap_iseg_t *next; /* in map */
	pcb_2netmap_iseg_t *path_next; /* in a temporary path while building oseg */
};

typedef struct pcb_2netmap_obj_s {
	rnd_coord_t x, y;   /* starting point coords */
	union {
		pcb_arc_t arc;
		pcb_line_t line;
		pcb_rat_t rat;
		pcb_pstk_t pstk;
		pcb_poly_t poly;
		pcb_text_t text;
		pcb_any_obj_t any;
	} o;

	/* internal/cache */
	unsigned char cc; /* ends used for connection */
} pcb_2netmap_obj_t;

typedef struct pcb_2netmap_oseg_s pcb_2netmap_oseg_t;
struct pcb_2netmap_oseg_s {
	vtp0_t objs;        /* of pcb_2netmap_obj_t ; these are not real board objects, they are just copies for the fields */
	pcb_net_t *net;
	unsigned shorted:1; /* set if the segment connects two different nets */
	pcb_2netmap_oseg_t *next;
};

typedef struct pcb_2netmap_s {
	pcb_2netmap_control_t ctrl;
	pcb_2netmap_oseg_t *osegs; /* output: head of a singly linked list */
	unsigned find_rats:1;      /* config: set to 1 if rats shall be included */

	/* internal */
	htpp_t o2n;   /* of (pcb_2netmap_iseg_t *); tells the net for an object */
	pcb_2netmap_iseg_t *isegs; /* head of a singly linked list */
	pcb_qry_exec_t *ec;
} pcb_2netmap_t;


int pcb_map_2nets_init(pcb_2netmap_t *map, pcb_board_t *pcb, pcb_2netmap_control_t how);
int pcb_map_2nets_uninit(pcb_2netmap_t *map);
