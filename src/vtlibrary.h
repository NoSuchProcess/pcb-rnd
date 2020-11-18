#ifndef PCB_VTLIBRARY_H
#define PCB_VTLIBRARY_H
#include <stdlib.h>
#include <string.h>
#include <genvector/vtp0.h>

typedef enum {
	PCB_LIB_INVALID,
	PCB_LIB_DIR,
	PCB_LIB_FOOTPRINT
} pcb_fplibrary_type_t;

typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,       /* used temporarily during the mapping - a finalized tree wouldn't have this */
	PCB_FP_FILEDIR,   /* used temporarily during the mapping - a finalized tree wouldn't have this */
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fptype_t;


typedef struct pcb_fplibrary_s pcb_fplibrary_t;

/* An element of a library: either a directory or a footprint */
struct pcb_fplibrary_s {
	char *name;            /* visible name */
	pcb_fplibrary_type_t type;
	pcb_fplibrary_t *parent;

	union {
		struct { /* type == LIB_DIR */
			vtp0_t children; /* of (pcb_fplibrary_t *) */
			void *backend; /* pcb_plug_fp_t* */
		} dir;
		struct { /* type == LIB_FOOTPRINT */
			char *loc_info;
			void *backend_data;
			pcb_fptype_t type;

			/* an array of void * tag IDs; last tag ID is NULL; the array is
			   locally allocated but values are stored in a central hash and
			   must be allocated by pcb_fp_tag() and never free'd manually */
			void **tags;
		} fp;
	} data;
} ;

#endif
