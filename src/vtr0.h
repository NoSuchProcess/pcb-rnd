#ifndef VTR0_H
#define VTR0_H

#include "config.h"
#include <stdlib.h>
#include <string.h>

typedef struct pcb_range_s {
	rnd_coord_t begin, end;
	union {
		long l;
		rnd_coord_t c;
		double d;
		void *ptr;
	} data[4];
} pcb_range_t;

#define GVT(x) vtr0_ ## x
#define GVT_ELEM_TYPE pcb_range_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 4096
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

#endif
