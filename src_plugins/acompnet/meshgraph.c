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
#include <stdlib.h>
#include <genht/hash.h>
#include "meshgraph.h"

#define INF_SCORE 9000000000.0

void pcb_msgr_init(pcb_meshgraph_t *gr)
{
	pcb_rtree_init(&gr->ntree);
	htip_init(&gr->id2node, longhash, longkeyeq);
	gr->next_id = 1;
}

long int pcb_msgr_add_node(pcb_meshgraph_t *gr, pcb_box_t *bbox)
{
	pcb_meshnode_t *nd = malloc(sizeof(pcb_meshnode_t));
	nd->bbox = *bbox;
	nd->id = gr->next_id;
	nd->came_from = 0;
	nd->gscore = INF_SCORE;
	nd->fscore = INF_SCORE;

	pcb_rtree_insert(&gr->ntree, nd, (pcb_rtree_box_t *)nd);
	htip_set(&gr->id2node, nd->id, nd);
	gr->next_id++;
	return nd->id;
}


void pcb_msgr_astar(pcb_meshgraph_t *gr, long int startid, long int endid)
{

}


