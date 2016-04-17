#include <stdlib.h>
#include <string.h>
#include "global_objs.h"

typedef void *ptr_t;

/* Elem=void *; init=none */

/* all public symbols are wrapped in GVT() - see vt_t(7) */
#define GVT(x) vtptr_ ## x

/* Array elem type - see vt_t(7) */
#define GVT_ELEM_TYPE ptr_t

/* Type that represents array lengths - see vt_t(7) */
#define GVT_SIZE_TYPE size_t

/* Below this length, always double allocation size when the array grows */
#define GVT_DOUBLING_THRS 256

/* Initial array size when the first element is written */
#define GVT_START_SIZE 32

/* Optional prefix for function definitions (e.g. static inline) */
#define GVT_FUNC


/* Include the actual header implementation */
#include <genvector/genvector_impl.h>

/* Memory allocator - see vt_allocation(7) */
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

/* clean up #defines */
#include <genvector/genvector_undef.h>
