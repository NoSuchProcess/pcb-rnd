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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <genht/htpp.h>
#include "board.h"

typedef struct pcb_netmap_s {
	htpp_t o2n, n2o;
	pcb_cardinal_t anon_cnt;
	pcb_board_t *pcb;
} pcb_netmap_t;

int pcb_netmap_init(pcb_netmap_t *map, pcb_board_t *pcb);
int pcb_netmap_uninit(pcb_netmap_t *map);

