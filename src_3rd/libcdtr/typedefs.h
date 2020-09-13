#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef int coord_t;

typedef struct {
	coord_t x;
	coord_t y;
} pos_t;

typedef struct point_s point_t;
typedef struct edge_s edge_t;
typedef struct triangle_s triangle_t;

typedef struct pointlist_node_s pointlist_node_t;
typedef struct edgelist_node_s edgelist_node_t;
typedef struct trianglelist_node_s trianglelist_node_t;

#endif
