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

#ifndef GRAPH_H
#define GRAPH_H
#include <stdio.h>
#include <assert.h>
#include "../../../config.h"

typedef struct gr_s {
	int n;     /* number of nodes */
	int *adj;  /* adjacency matrix of n*n ints */
	char **node2name; /* optional node names */
} gr_t;

/* allocate a new graph of n nodes with no edges */
gr_t *gr_alloc(int n);

/* create a new graph by cloning graph g */
gr_t *gr_clone(gr_t *g);

/* free all resouces used by the graph */
void gr_free(gr_t *g);


static inline PCB_FUNC_UNUSED void gr_bound_chk(gr_t *g, int n1, int n2)
{
	assert((n1 >= 0) && (n1 < g->n));
	assert((n2 >= 0) && (n2 < g->n));
}

/* return number of edges between nodes n1 and n2 - without checks */
static inline PCB_FUNC_UNUSED int gr_get_(gr_t *g, int n1, int n2)
{
	return g->adj[n1 * g->n + n2];
}

/* return number of edges between nodes n1 and n2 - with checks */
static inline PCB_FUNC_UNUSED int gr_get(gr_t *g, int n1, int n2)
{
	gr_bound_chk(g, n1, n2);
	return gr_get_(g, n1, n2);
}

/* return old number of edges between nodes n1 and n2 and change it to newnum - no check*/
static inline PCB_FUNC_UNUSED int gr_set_(gr_t *g, int n1, int n2, int newnum)
{
	int old;
	old = g->adj[n1 * g->n + n2];
	g->adj[n1 * g->n + n2] = newnum;
	g->adj[n2 * g->n + n1] = newnum;
	return old;
}

/* return old number of edges between nodes n1 and n2 and increase it by newnum - no check*/
static inline PCB_FUNC_UNUSED int gr_add_(gr_t *g, int n1, int n2, int newnum)
{
	int old;
	old = g->adj[n1 * g->n + n2];
	g->adj[n1 * g->n + n2] += newnum;
	g->adj[n2 * g->n + n1] += newnum;
	return old;
}

/* return old number of edges between nodes n1 and n2 and change it to newnum - check*/
static inline PCB_FUNC_UNUSED int gr_set(gr_t *g, int n1, int n2, int newnum)
{
	gr_bound_chk(g, n1, n2);
	return gr_set_(g, n1, n2, newnum);
}

/* merge two nodes n1 and n2 (contraction) */
void gr_merge_nodes(gr_t *g, int n1, int n2);

/* print the connection matrix */
void gr_print(gr_t *g, FILE *f, const char *prefix);

/* draw graph using graphviz/dot */
int gr_draw(gr_t *g, const char *name, const char *type);

/* return total number of edges of a node */
int gr_node_edges(gr_t *g, int node);
#endif
