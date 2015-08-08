#include <stdlib.h>
#include <stdio.h>
#include "graph.h"
#include "load.h"
#include "solve.h"


int main()
{
	gr_t *g;
	g = load(stdin);
	if (g == NULL) {
		fprintf(stderr, "Failed to load input, exiting\n");
		exit(1);
	}
	solve(g);
	return 0;
}
