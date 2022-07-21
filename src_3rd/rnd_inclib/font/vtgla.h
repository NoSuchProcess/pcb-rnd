#define GVT(x) vtgla_ ## x
#define GVT_ELEM_TYPE rnd_glyph_atom_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 128
#define GVT_START_SIZE 8
#define GVT_SET_NEW_BYTES_TO 0
#define GVT_FUNC
#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

#ifndef RND_VTGLA_C
#include <genvector/genvector_undef.h>
#endif
