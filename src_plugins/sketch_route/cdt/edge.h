#ifndef EDGE_H
#define EDGE_H

#include <stdlib.h>
#include <string.h>

#include "typedefs.h"

struct edge_s {
	point_t *endp[2];
	triangle_t *adj_t[2];

	int is_constrained;
};

typedef edge_t* edge_ptr_t;


/* List */
#define LST(x) edgelist_ ## x
#define LST_ITEM_T edge_ptr_t

#include <list/list.h>

#ifndef LST_DONT_UNDEF
	#undef LST
	#undef LST_ITEM_T
#endif

#define EDGELIST_FOREACH(_loop_item_, _list_) do { \
	edgelist_node_t *_node_ = _list_; \
	while (_node_ != NULL) { \
		edge_t *_loop_item_ = _node_->item;

#define EDGELIST_FOREACH_END() \
		_node_ = _node_->next; \
	} \
} while(0)


/* Vector */
#define GVT(x) vtedge_ ## x
#define GVT_ELEM_TYPE edge_ptr_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0
#define GVT_ELEM_CONSTRUCTOR
#define GVT_ELEM_DESTRUCTOR

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
int GVT(constructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem);
void GVT(destructor)(GVT(t) *vect, GVT_ELEM_TYPE *elem);
#include <genvector/genvector_undef.h>

#define VTEDGE_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vtedge_len(_vt_); _i_++) { \
		edge_t *_loop_item_ = (_vt_)->array[_i_];

#define VTEDGE_FOREACH_END() \
	} \
} while(0)

#endif
