#ifndef EWIRE_H
#define EWIRE_H

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "sktypedefs.h"
#ifdef GVT_DONT_UNDEF
	#undef GVT_DONT_UNDEF
	#include "ewire_point.h"
	#define GVT_DONT_UNDEF
#else
	#include "ewire_point.h"
#endif

typedef struct {
	vtewire_point_t points;
	wire_t *wire;
} ewire_t;

typedef ewire_t* ewire_ptr_t;

void ewire_init(ewire_t *ew);
void ewire_uninit(ewire_t *ew);
void ewire_append_point(ewire_t *ew, spoke_t *sp, side_t side, int sp_slot);
void ewire_insert_point(ewire_t *ew, int after_i, spoke_t *sp, side_t side, int sp_slot);
void ewire_remove_point(ewire_t *ew, int i);

ewire_point_t *ewire_get_point_at_slot(ewire_t *ew, spoke_t *sp, int slot_num);


/* ewire ptr Vector */
#define GVT(x) vtewire_ ## x
#define GVT_ELEM_TYPE ewire_ptr_t
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

#define VTEWIRE_FOREACH(_loop_item_, _vt_) do { \
	int _i_; \
	for (_i_ = 0; _i_ < vtewire_len(_vt_); _i_++) { \
		wire_t *_loop_item_ = (_vt_)->array[_i_];

#define VTEWIRE_FOREACH_END() \
	} \
} while(0)

#endif
