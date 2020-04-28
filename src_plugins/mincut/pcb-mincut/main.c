#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "graph.h"
#include "load.h"
#include "solve.h"

#define strempty(s) ((s) == NULL ? "" : (s))

#undef strdup

char *rnd_strdup(const char *s) { return strdup(s); }
int rnd_rand() { return rand(); }

int main()
{
	int *best;
	gr_t *g;
	int n, cancel;

	g = load(stdin);
	if (g == NULL) {
		fprintf(stderr, "Failed to load input, exiting\n");
		exit(1);
	}
	best = pcb_mincut_solve(g, NULL, &cancel);
	for(n = 0; best[n*2] != -1; n++)
		printf("%s-%s\n", strempty(g->node2name[best[n*2+0]]), strempty(g->node2name[best[n*2+1]]));

	gr_free(g);
	free(best);

	return 0;
}
