#ifndef PCB_VTROUTESTYLE_H
#define PCB_VTROUTESTYLE_H
#include <stdlib.h>
#include <string.h>
#include "unit.h"

/* Elem=RouteStyle; init=0 */

typedef struct {
	pcb_coord_t Thick,       /* line thickness */
	  Diameter,        /* via diameter */
	  Hole,            /* via drill hole */
	  Clearance;       /* min. separation from other nets */
	char name[32];     /* fixed length name to save malloc/free */
/*	int index;*/
} RouteStyleType, *RouteStyleTypePtr;

/* all public symbols are wrapped in GVT() - see vt_t(7) */
#define GVT(x) vtroutestyle_ ## x
#define HAVE_VTROUTESTYLE_T

/* Array elem type - see vt_t(7) */
#define GVT_ELEM_TYPE RouteStyleType

/* Type that represents array lengths - see vt_t(7) */
#define GVT_SIZE_TYPE size_t

/* Below this length, always double allocation size when the array grows */
#define GVT_DOUBLING_THRS 16

/* Initial array size when the first element is written */
#define GVT_START_SIZE 4

/* Optional terminator; when present, it is always appended at the end - see
   vt_term(7)*/
/* #define GVT_TERM '\0' */

/* Optional prefix for function definitions (e.g. static inline) */
#define GVT_FUNC

/* Enable this to set all new bytes ever allocated to this value - see
   vt_set_new_bytes_to(7) */
#define GVT_SET_NEW_BYTES_TO 0

/* Enable GVT_INIT_ELEM_FUNC and an user configured function is called
   for each new element allocated (even between used and alloced).
   See vt_init_elem(7) */
/*#define GVT_INIT_ELEM_FUNC*/

/* Enable GVT_ELEM_CONSTRUCTOR and an user configured function is called
   for each element that is getting within the range of ->used.
   See vt_construction(7) */
/*#define GVT_ELEM_CONSTRUCTOR */

/* Enable GVT_ELEM_DESTRUCTOR and an user configured function is called
   for each element that was once constructed and now getting beyond ->used.
   See vt_construction(7) */
/*#define GVT_ELEM_DESTRUCTOR */

/* Enable GVT_ELEM_COPY and an user configured function is called
   for copying elements into the array.
   See vt_construction(7) */
/*#define GVT_ELEM_COPY */

/* Optional extra fields in the vector struct - see vt_user_fields(7) */
/* #define GVT_USER_FIELDS int foo; char bar[12]; */

/* Include the actual header implementation */
#include <genvector/genvector_impl.h>

/* Memory allocator - see vt_allocation(7) */
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)

/* clean up #defines */
#include <genvector/genvector_undef.h>

#endif
