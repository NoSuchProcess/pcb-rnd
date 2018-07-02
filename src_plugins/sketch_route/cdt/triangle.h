#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <stdlib.h>
#include <string.h>

#include "typedefs.h"

struct triangle_s {
	point_t *p[3];
	edge_t *e[3];
  triangle_t *adj_t[3];
};

typedef triangle_t* triangle_ptr_t;


/* List */
#define LST(x) trianglelist_ ## x
#define LST_ITEM_T triangle_ptr_t
#define LST_DONT_TYPEDEF_NODE

#include "list/list.h"

#ifndef LST_DONT_UNDEF
	#undef LST
	#undef LST_ITEM_T
	#undef LST_DONT_TYPEDEF_NODE
#endif

#define TRIANGLELIST_FOREACH(_loop_item_, _list_) do { \
	trianglelist_node_t *_node_ = _list_; \
	while (_node_ != NULL) { \
		triangle_t *_loop_item_ = _node_->item;

#define TRIANGLELIST_FOREACH_END() \
		_node_ = _node_->next; \
	} \
} while(0)


/* Vector */
#define GVT(x) vttriangle_ ## x
#define GVT_ELEM_TYPE triangle_ptr_t
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

#define VTTRIANGLE_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vttriangle_len(_vt_); _i_++) { \
		triangle_t *_loop_item_ = (_vt_)->array[_i_];

#define VTTRIANGLE_FOREACH_END() \
	} \
} while(0)

#endif
