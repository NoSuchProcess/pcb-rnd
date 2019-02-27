#ifndef PCB_DRILL_H
#define PCB_DRILL_H

#include "aperture.h"

typedef struct {
	pcb_coord_t diam;
	pcb_coord_t x;
	pcb_coord_t y;

	/* for slots */
	int is_slot;
	pcb_coord_t x2;
	pcb_coord_t y2;
} pending_drill_t;

#define GVT(x) vtpdr_ ## x
#define GVT_ELEM_TYPE pending_drill_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 2048
#define GVT_START_SIZE 32
#define GVT_FUNC
/*#define GVT_SET_NEW_BYTES_TO 0*/

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

typedef struct pcb_drill_ctx_s {
	vtpdr_t obj;
	aperture_list_t apr;
} pcb_drill_ctx_t;

void drill_init(pcb_drill_ctx_t *ctx);
void drill_uninit(pcb_drill_ctx_t *ctx);
void drill_export(pcb_board_t *pcb, const pcb_drill_ctx_t *ctx, int force_g85, const char *fn);
pending_drill_t *new_pending_drill(pcb_drill_ctx_t *ctx);

#endif
