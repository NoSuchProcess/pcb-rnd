#include <stdlib.h>
#include <stdio.h>
#include "graph.h"
#include "load.h"
#include "solve.h"

#define strempty(s) ((s) == NULL ? "" : (s))

int main()
{
	int *best;
	gr_t *g;
	int n;

	g = load(stdin);
	if (g == NULL) {
		fprintf(stderr, "Failed to load input, exiting\n");
		exit(1);
	}
	best = solve(g);
	for(n = 0; best[n*2] != -1; n++)
		printf("%s-%s\n", strempty(g->node2name[best[n*2+0]]), strempty(g->node2name[best[n*2+1]]));

	return 0;
}
