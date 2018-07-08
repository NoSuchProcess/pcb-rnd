#ifndef WIRE_H
#define WIRE_H

#include <stdlib.h>
#include <string.h>

#ifdef GVT_DONT_UNDEF
  #undef GVT_DONT_UNDEF
  #include "cdt/point.h"
  #define GVT_DONT_UNDEF
#else
  #include "cdt/point.h"
#endif

typedef struct {
	point_t *p;
	enum {
		SIDE_LEFT = (1<<0),
		SIDE_RIGHT = (1<<1),
		SIDE_TERM = (1<<1)|(1<<0)
	} side;
} wire_point_t;

typedef struct {
	int point_num;
	int point_max;
	wire_point_t *points;
} wire_t;

typedef wire_t* wire_ptr_t;

void wire_init(wire_t *w);
void wire_uninit(wire_t *w);
void wire_push_point(wire_t *w, point_t *p, int side);
void wire_pop_point(wire_t *w);
void wire_copy(wire_t *dst, wire_t *src);
void wire_print(wire_t *w, const char *tab);


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
