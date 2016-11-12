#include <stdlib.h>
#include <string.h>

typedef enum {
	LIB_INVALID,
	LIB_DIR,
	LIB_FOOTPRINT
} pcb_fplibrary_type_t;

typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,       /* used temporarily during the mapping - a finalized tree wouldn't have this */
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fptype_t;


typedef struct pcb_fplibrary_s pcb_fplibrary_t;

/* Elem=library_t; init=none */

/* all public symbols are wrapped in GVT() - see vt_t(7) */
#define GVT(x) vtlib_ ## x

/* Array elem type - see vt_t(7) */
#define GVT_ELEM_TYPE pcb_fplibrary_t

/* Type that represents array lengths - see vt_t(7) */
#define GVT_SIZE_TYPE size_t

/* Below this length, always double allocation size when the array grows */
#define GVT_DOUBLING_THRS 64

/* Initial array size when the first element is written */
#define GVT_START_SIZE 8

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

/* An element of a library: either a directory or a footprint */
struct pcb_fplibrary_s {
	char *name;            /* visible name */
	pcb_fplibrary_type_t type;
	pcb_fplibrary_t *parent;

	union {
		struct { /* type == LIB_DIR */
			vtlib_t children;
		} dir;
		struct { /* type == LIB_FOOTPRINT */
			char *loc_info;
			void *backend_data;
			pcb_fptype_t type;
			void **tags;        /* an array of void * tag IDs; last tag ID is NULL */
		} fp;
	} data;
} ;

