#ifndef POINT_H
#define POINT_H

#include <stdlib.h>
#include <string.h>

#include "typedefs.h"

struct point_s {
	pos_t pos;
	edgelist_node_t *adj_edges;
	trianglelist_node_t *adj_triangles;

	void *data;
};

typedef point_t* point_ptr_t;


/* List */
#define LST(x) pointlist_ ## x
#define LST_ITEM_T point_ptr_t
#define LST_DONT_TYPEDEF_NODE

#include "list/list.h"

#ifndef LST_DONT_UNDEF
	#undef LST
	#undef LST_ITEM_T
	#undef LST_DONT_TYPEDEF_NODE
#endif

#define POINTLIST_FOREACH(_loop_item_, _list_) do { \
	pointlist_node_t *_node_ = _list_; \
	while (_node_ != NULL) { \
		point_t *_loop_item_ = _node_->item;

#define POINTLIST_FOREACH_END() \
		_node_ = _node_->next; \
	} \
} while(0)


/* Vector */
#define GVT(x) vtpoint_ ## x
#define GVT_ELEM_TYPE point_ptr_t
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

#define VTPOINT_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vtpoint_len(_vt_); _i_++) { \
		point_t *_loop_item_ = (_vt_)->array[_i_];

#define VTPOINT_FOREACH_END() \
	} \
} while(0)

#endif
