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
#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "meshgraph.h"
#include <librnd/core/error.h>
#include "layer.h"
#include "route.h"

#define INF_SCORE 9000000000.0
#define SEARCHR RND_MM_TO_COORD(5)

void pcb_msgr_init(pcb_meshgraph_t *gr)
{
	rnd_rtree_init(&gr->ntree);
	htip_init(&gr->id2node, longhash, longkeyeq);
	gr->next_id = 1;
}

long int pcb_msgr_add_node(pcb_meshgraph_t *gr, rnd_box_t *bbox, int score)
{
	pcb_meshnode_t *nd = malloc(sizeof(pcb_meshnode_t));
	nd->bbox = *bbox;
	nd->id = gr->next_id;
	nd->came_from = 0;
	nd->gscore = INF_SCORE;
	nd->fscore = INF_SCORE;
	nd->iscore = score;

	rnd_rtree_insert(&gr->ntree, nd, (rnd_rtree_box_t *)nd);
	htip_set(&gr->id2node, nd->id, nd);
	gr->next_id++;
	return nd->id;
}

static double msgr_connect(pcb_meshnode_t *curr, pcb_meshnode_t *next)
{
	rnd_point_t np, cp;
	pcb_route_t route;
	pcb_route_init(&route);

	np.X = next->bbox.X1;
	np.Y = next->bbox.Y1;
	cp.X = curr->bbox.X1;
	cp.Y = curr->bbox.Y1;
	pcb_route_calculate(PCB, &route, &np, &cp, PCB_CURRLID(PCB), conf_core.design.line_thickness, conf_core.design.bloat, pcb_flag_make(PCB_FLAG_CLEARLINE), 0, 0);

	rnd_trace("size=%d\n", route.size);

	return curr->gscore + rnd_distance(curr->bbox.X1, curr->bbox.Y1, next->bbox.X1, next->bbox.Y1) * (next->iscore + 1.0);
}

static double msgr_heurist(pcb_meshnode_t *curr, pcb_meshnode_t *end)
{
	return rnd_distance(curr->bbox.X1, curr->bbox.Y1, end->bbox.X1, end->bbox.Y1);
}

int pcb_msgr_astar(pcb_meshgraph_t *gr, long int startid, long int endid)
{
	htip_t closed, open;
	htip_entry_t *e;
	pcb_meshnode_t *curr, *next, *end;

	curr = htip_get(&gr->id2node, startid);
	if (curr == NULL)
		return -1;

	end = htip_get(&gr->id2node, endid);
	if (end == NULL)
		return -1;

	htip_init(&closed, longhash, longkeyeq);
	htip_init(&open, longhash, longkeyeq);

	htip_set(&open, startid, curr);
	for(;;) {
		rnd_rtree_box_t sb;
		rnd_box_t *b;
		rnd_rtree_it_t it;
		double tentative_g, best;

TODO("should use a faster way for picking lowest fscore")
		/* get the best looking item from the open list */
		curr = NULL;
		best = INF_SCORE;
		for(e = htip_first(&open); e != NULL; e = htip_next(&open, e)) {
			next = e->value;
			if (next->fscore <= best) {
				best = next->fscore;
				curr = next;
			}
		}

		if (curr == NULL) {
rnd_trace("NO PATH\n");
			return 0;
		}
		htip_pop(&open, curr->id);
		if (curr->id == endid) {
rnd_trace("found path\n");
			return 1;
		}
		htip_set(&closed, curr->id, curr);
rnd_trace("first: %ld\n", curr->id);

		/* search potential neighbors */
		sb.x1 = curr->bbox.X1 - SEARCHR;
		sb.x2 = curr->bbox.X2 + SEARCHR;
		sb.y1 = curr->bbox.Y1 - SEARCHR;
		sb.y2 = curr->bbox.Y2 + SEARCHR;
		for(b = rnd_rtree_first(&it, &gr->ntree, &sb); b != NULL; b = rnd_rtree_next(&it)) {
			next = (pcb_meshnode_t *)b;
			if (htip_get(&closed, next->id) != NULL)
				continue;

			tentative_g = msgr_connect(curr, next);
			if (tentative_g < 0)
				continue;

			if (htip_get(&open, next->id) == NULL) /* not in open */
				htip_set(&open, next->id, next);
			else if (tentative_g > next->gscore)
				continue;

rnd_trace("add:   %ld\n", next->id);
			next->came_from = curr->id;
			next->gscore = tentative_g;
			next->fscore = msgr_heurist(curr, end);
		}
	}
}


