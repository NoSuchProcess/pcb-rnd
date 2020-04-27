#ifndef VTC0_H
#define VTC0_H

#include <librnd/config.h>
#include <stdlib.h>
#include <string.h>

#define GVT(x) vtc0_ ## x
#define GVT_ELEM_TYPE rnd_coord_t
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
