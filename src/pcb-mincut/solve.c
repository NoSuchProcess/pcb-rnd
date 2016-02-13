/*    pcb-mincut, a prototype project demonstrating how to highlight shorts
 *    Copyright (C) 2012 Tibor 'Igor2' Palinkas
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "solve.h"

/* Karger's algorithm as described in the wikipedia article
   http://en.wikipedia.org/wiki/Karger%27s_algorithm
*/

#define BAD 1000000

/*#define DEBUG_MERGES*/
/*#define DEBUG_TAGS*/
/*#define DEBUG_SOLVE*/

typedef struct {
	gr_t *g;
	int *avail;    /* nodes IDs still avaialble for merging */
	int *neigh;    /* neighbor list */
	int *tag;
	int num_avail; /* number of nodes still avaialble for merging */
} sstate_t;

static int pick_del(sstate_t *st)
{
	int idx, ret, size;
	idx = rand() % st->num_avail;
	ret = st->avail[idx];
	size = (st->num_avail-idx-1) * sizeof(int);
	if (size > 0)
		memmove(&st->avail[idx], &st->avail[idx+1], size);
	st->num_avail--;
	return ret;
}

static int pick_neigh(sstate_t *st, int node)
{
	int n, num_neigh;

	num_neigh = 0;
	for(n = 0; n < st->g->n; n++) {
		if ((n != node) && (gr_get_(st->g, n, node) > 0)) {
			st->neigh[num_neigh] = n;
			num_neigh++;
		}
	}
	return st->neigh[rand() % num_neigh];
}

static void retag(sstate_t *st, int from, int to)
{
	int n;
	for(n = 0; n < st->g->n; n++)
		if (st->tag[n] == from)
			st->tag[n] = to;
}


/* clone graph and do a randon contraction */
int solve_(gr_t *g_, int *cuts)
{
	sstate_t st;
	int n, result, tags;
	static int solution = -1;
#ifdef DEBUG_MERGES
	int cnt = 0;
	char fn[512];
#endif

	solution++;
	st.g = gr_clone(g_);
	st.avail = malloc(sizeof(int) * st.g->n);
	st.neigh = malloc(sizeof(int) * st.g->n);
	st.tag = malloc(sizeof(int) * st.g->n);

#define FREE_ALL() \
	gr_free(st.g); \
	free(st.avail); \
	free(st.neigh); \
	free(st.tag);

	for(n = 2; n < st.g->n; n++)
		st.tag[n] = -1;
	st.tag[0] = 0;
	st.tag[1] = 1;
	tags = 2;

	st.num_avail = 0;
	for(n = 0; n < st.g->n; n++) {
		if (gr_node_edges(st.g, n) > 0) {
			st.avail[st.num_avail] = n;
			st.num_avail++;
		}
	}

	while(st.num_avail > 2) {
		int n1, n2;
		n2 = pick_del(&st);
		n1 = pick_neigh(&st, n2);
#ifndef DEBUG_MERGES
#ifdef DEBUG_SOLVE
			printf("Merge %d (%s) into %d (%s)\n", n2, st.g->node2name[n2], n1, st.g->node2name[n1]);
#endif
#endif
		assert(n2 != n1);

		/* propagate tags */
		if ((st.tag[n1] != -1) && (st.tag[n2] == -1))
			st.tag[n2] = st.tag[n1];
		else if ((st.tag[n1] == -1) && (st.tag[n2] != -1))
			st.tag[n1] = st.tag[n2];
		else if ((st.tag[n1] == -1) && (st.tag[n2] == -1))
			st.tag[n1] = st.tag[n2] = tags++;
		else if ((st.tag[n1] != -1) && (st.tag[n2] != -1)) {
			if ((st.tag[n1] > 1) && (st.tag[n2] <= 1))
				retag(&st, st.tag[n1], st.tag[n2]);
			else if ((st.tag[n2] > 1) && (st.tag[n1] <= 1))
				retag(&st, st.tag[n2], st.tag[n1]);
			else {
				/* tag collision means we won't be able to distinguish between our
				   two groups and our cut won't resolve the short anyway */
#ifdef DEBUG_TAGS
				printf("Tag collision!\n");
#endif
				FREE_ALL();
				return BAD;
			}
		}

		gr_merge_nodes(st.g, n1, n2);

#ifdef DEBUG_MERGES
			sprintf(fn, "contraction_%02d_%02d", solution, cnt);
			cnt++;
			gr_draw(st.g, fn, "png");
			printf("Merge %d into %d, result in %s leaving %d available nodes\n", n2, n1, fn, st.num_avail);
#endif
	}

#ifdef DEBUG_SOLVE
	{
		char fn[128];
		sprintf(fn, "contraction_%02d", solution);
		gr_draw(st.g, fn, "png");
	}
#endif

	result = gr_get(st.g, st.avail[0], st.avail[1]);

#ifdef DEBUG_TAGS
	{
		int t, n;
		printf("Groups:\n");
		for(t = 0; t < 2; t++) {
			printf("  [%d] is", t);
			for(n = 0; n < st.g->n; n++) {
				if (st.tag[n] == t)
					printf(" %s", st.g->node2name[n]);
			}
			printf("\n");
		}
	}
#endif
	{
		int x, y, num_cuts = 0;
		for(y = 0; y < st.g->n; y++)
			for(x = y+1; x < st.g->n; x++)
				if ((gr_get_(g_, x, y) > 0) && (st.tag[x] != st.tag[y])) {
#ifdef DEBUG_TAGS
					printf("CUT %s-%s\n", st.g->node2name[x], st.g->node2name[y]);
#endif
					cuts[num_cuts*2+0] = x;
					cuts[num_cuts*2+1] = y;
					num_cuts++;
				}
		cuts[num_cuts*2+0] = -1;
		cuts[num_cuts*2+1] = -1;
	}
	FREE_ALL();
	return result;
}

#define strempty(s) ((s) == NULL ? "" : (s))
int *solve(gr_t *g)
{
	int n, best, res, till, cuts_size;
	double nd;
	int *cuts, *best_cuts;

	/* count how many nodes we really have - the ones not cut down from the graph by the preprocessor */
	nd = 0;
	for(n = 0; n < g->n; n++)
		if (gr_node_edges(g, n) > 0)
			nd++;

	till = (int)(nd * (nd-1.0) / 2.0 * log(nd))+1;
#ifdef DEBUG_SOLVE
	printf("Running solver at most %d times for %d relevant nodes\n", till, (int)nd);
#endif

	cuts_size = ((nd * nd) + 1) * sizeof(int);
	cuts = malloc(cuts_size);
	best_cuts = malloc(cuts_size);

	best = BAD;
	for(n = 0; n < till; n++) {
		res = solve_(g, cuts);
#ifdef DEBUG_SOLVE
		printf("solution %d=%d\n", n, res);
#endif
		if (res < best) {
			best = res;
			memcpy(best_cuts, cuts, cuts_size);
		}
		if (best == 1) /* we won't find a better solution ever */
			break;
	}

#ifdef DEBUG_SOLVE
	printf("Best solution cuts %d edge%s:", best, best == 1 ? "" : "s");
	for(n = 0; best_cuts[n*2] != -1; n++) {
		printf(" %d:%s-%d:%s", best_cuts[n*2+0], strempty(g->node2name[best_cuts[n*2+0]]), best_cuts[n*2+1], strempty(g->node2name[best_cuts[n*2+1]]));
	}
	printf("\n");
#endif
	if (best == BAD) {
		free(best_cuts);
		free(cuts);
		return NULL;
	}
	free(cuts);
	return best_cuts;
}


