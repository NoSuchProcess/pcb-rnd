#include "graph.h"

/* returns a list of object ID pairs (each nth and n+1th element) terminated
   by a -1;-1. Cutting these vertices would separate g. */
int *pcb_mincut_solve(gr_t *g, int (*progress)(long so_far, long total, const char *msg), int *cancel);
