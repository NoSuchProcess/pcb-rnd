#ifndef PCB_ACOMPNET_MESHGRAPH_H
#define PCB_ACOMPNET_MESHGRAPH_H
#include <librnd/poly/rtree.h>
#include <librnd/core/box.h>
#include <genht/htip.h>

typedef struct {
	rnd_box_t bbox;
	long int id;
	long int came_from;
	double gscore, fscore;
	int iscore; /* input score: how much we prefer to use this node */
} pcb_meshnode_t;


typedef struct {
	pcb_rtree_t ntree;
	htip_t id2node;
	long int next_id;
} pcb_meshgraph_t;

void pcb_msgr_init(pcb_meshgraph_t *gr);
long int pcb_msgr_add_node(pcb_meshgraph_t *gr, rnd_box_t *bbox, int score);
int pcb_msgr_astar(pcb_meshgraph_t *gr, long int startid, long int endid);

#endif
