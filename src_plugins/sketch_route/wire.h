#ifndef WIRE_H
#define WIRE_H

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "cdt/typedefs.h"
#include "sktypedefs.h"


typedef struct {
	point_t *p;
	side_t side;
  wirelist_node_t *wire_node;
} wire_point_t;

struct wire_s {
	int point_num;
	int point_max;
	wire_point_t *points;
	pcb_coord_t thickness;
	pcb_coord_t clearance;
};

typedef wire_t* wire_ptr_t;

void wire_init(wire_t *w);
void wire_uninit(wire_t *w);
void wire_push_point(wire_t *w, point_t *p, int side);
void wire_pop_point(wire_t *w);
void wire_copy(wire_t *dst, wire_t *src);

int wire_is_node_connected_with_point(wirelist_node_t *node, point_t *p);
int wire_is_coincident_at_node(wirelist_node_t *node, point_t *p1, point_t *p2);
int wire_node_position_at_point(wirelist_node_t *node, point_t *p); /* counting from the inside */


/* List */
#define LST(x) wirelist_ ## x
#define LST_ITEM_T wire_ptr_t
#define LST_DONT_TYPEDEF_NODE

#include "cdt/list/list.h"

#ifndef LST_DONT_UNDEF
	#undef LST
	#undef LST_ITEM_T
	#undef LST_DONT_TYPEDEF_NODE
#endif

#define WIRELIST_FOREACH(_loop_item_, _list_) do { \
	wirelist_node_t *_node_ = _list_; \
	while (_node_ != NULL) { \
		wire_t *_loop_item_ = _node_->item;

#define WIRELIST_FOREACH_END() \
		_node_ = _node_->next; \
	} \
} while(0)


/* Vector */
#define GVT(x) vtwire_ ## x
#define GVT_ELEM_TYPE wire_ptr_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0
#define GVT_ELEM_CONSTRUCTOR
#define GVT_ELEM_DESTRUCTOR
#define GVT_ELEM_COPY

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
int GVT(constructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem);
void GVT(destructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem);
#include <genvector/genvector_undef.h>

#define VTWIRE_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vtwire_len(_vt_); _i_++) { \
		wire_t *_loop_item_ = (_vt_)->array[_i_];

#define VTWIRE_FOREACH_END() \
	} \
} while(0)

#endif
