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
#include "graph.h"
#include "safe_fs.h"

/* allocate a new graph of n nodes with no edges */
gr_t *gr_alloc(int n)
{
	gr_t *g = malloc(sizeof(gr_t));
	g->n = n;
	g->adj = calloc(n * n, sizeof(int));
	return g;
}

/* create a new graph by cloning graph g */
gr_t *gr_clone(gr_t *g)
{
	int size = g->n * g->n * sizeof(int);
	gr_t *o = malloc(sizeof(gr_t));

	o->n = g->n;
	o->adj = malloc(size);
	memcpy(o->adj, g->adj, size);
	o->node2name = g->node2name;
	return o;
}

/* free all resouces used by the graph */
void gr_free(gr_t *g)
{
	free(g->adj);
	free(g);
}

void gr_merge_nodes(gr_t *g, int n1, int n2)
{
	int n;
	gr_bound_chk(g, n1, n2);

	for(n = 0; n < g->n; n++) {
		if (n != n2)
			gr_add_(g, n, n1, gr_set_(g, n, n2, 0));
		else
			gr_add_(g, n1, n1, gr_set_(g, n2, n2, 0)/2);
	}
}

void gr_print(gr_t *g, FILE *f, const char *prefix)
{
	int x, y;

	fprintf(f, "%s      ", prefix);
	for(x = 0; x < g->n; x++)
		fprintf(f, "% 4d ", x);
	fprintf(f, "\n");

	for(y = 0; y < g->n; y++) {
		fprintf(f, "%s % 4d ", prefix, y);
		for(x = 0; x < g->n; x++)
			fprintf(f, "% 4d ", gr_get_(g, x, y));
		fprintf(f, "\n");
	}
	fprintf(f, "%s\n", prefix);
	if (g->node2name != NULL) {
	fprintf(f, "%snames:\n", prefix);
		for(x = 0; x < g->n; x++)
			fprintf(f, "%s % 4d=%s\n", prefix, x, g->node2name[x]);
	}
}

int gr_draw(gr_t *g, const char *name, const char *type)
{
	char *cmd;
	FILE *f;
	int x, y;

	if (type == NULL)
		type = "png";

	cmd = malloc(strlen(type)*2 + strlen(name) + 64);
	sprintf(cmd, "dot -T%s -o %s.%s", type, name, type);
	f = pcb_popen(NULL, cmd, "w");
	if (f == NULL)
		return -1;

	fprintf(f, "graph %s {\n", name);

	if (g->node2name != NULL)
		for(x = 0; x < g->n; x++)
			fprintf(f, "\t% 4d[label=\"%d\\n%s\"]\n", x, x, g->node2name[x] == NULL ? "NULL" : g->node2name[x]);

	for(y = 0; y < g->n; y++) {
		for(x = y; x < g->n; x++) {

#ifdef MULTI_EDGE
			int n;
			for(n = gr_get_(g, x, y); n > 0; n--)
				fprintf(f, "\t% 4d -- % 4d\n", x, y);
#else
			if (gr_get_(g, x, y) > 0)
				fprintf(f, "\t% 4d -- % 4d [label=\"*%d\"]\n", x, y, gr_get_(g, x, y));
#endif
		}
	}

	fprintf(f, "}\n");
	pcb_pclose(f);
	return 0;
}

int gr_node_edges(gr_t *g, int node)
{
	int n, sum;
	sum = 0;
	gr_bound_chk(g, 0, node);
	for(n = 0; n < g->n; n++)
		sum += gr_get_(g, n, node);
	return sum;
}
