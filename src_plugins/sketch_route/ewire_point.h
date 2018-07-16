#ifndef EWIRE_POINT_H
#define EWIRE_POINT_H

#include <stdlib.h>
#include <string.h>

#include "sktypedefs.h"

typedef struct{
	spoke_t *sp; /* set this to cdt point in case of terminal (side = SIDE_TERM) */
	side_t side;
	int sp_slot;
	wirelist_node_t *w_node;
} ewire_point_t;


/* ewire_point Vector */
#define GVT(x) vtewire_point_ ## x
#define GVT_ELEM_TYPE ewire_point_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 256
#define GVT_START_SIZE 8
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

#define VTEWIRE_POINT_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vtewire_point_len(_vt_); _i_++) { \
		ewire_point_t *_loop_item_ = &(_vt_)->array[_i_];

#define VTEWIRE_POINT_FOREACH_END() \
	} \
} while(0)

#endif
