#ifndef PCB_VTPADSTACK_T_H
#define PCB_VTPADSTACK_T_H

/* Elem=pcb_pstk_proto_t; init=none */

#include "obj_pstk_shape.h"

/* all public symbols are wrapped in GVT() - see vt_t(7) */
#define GVT(x) pcb_vtpadstack_tshape_ ## x

/* Array elem type - see vt_t(7) */
#define GVT_ELEM_TYPE pcb_pstk_tshape_t

/* Type that represents array lengths - see vt_t(7) */
#define GVT_SIZE_TYPE size_t

/* Below this length, always double allocation size when the array grows */
#define GVT_DOUBLING_THRS 32

/* Initial array size when the first element is written */
#define GVT_START_SIZE 6

/* Optional prefix for function definitions (e.g. static inline) */
#define GVT_FUNC

/* Enable this to set all new bytes ever allocated to this value - see
   vt_set_new_bytes_to(7) */
#define GVT_SET_NEW_BYTES_TO 0


/* Include the actual header implementation */
#include <genvector/genvector_impl.h>

/* Memory allocator - see vt_allocation(7) */
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

/* clean up #defines */
#include <genvector/genvector_undef.h>

#endif
