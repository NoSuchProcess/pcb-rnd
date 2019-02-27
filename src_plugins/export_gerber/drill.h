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


#define DRILL_APR (is_plated ? &aprp : &apru)

void drill_init(void);
void drill_uninit(void);
int drill_sort(const void *va, const void *vb);
void drill_export(pcb_board_t *pcb, FILE *f, pending_drill_t **pd, pcb_cardinal_t *npd, pcb_cardinal_t *mpd, aperture_list_t *apl, int force_g85, const char *fn);
pending_drill_t *new_pending_drill(int is_plated);

extern aperture_list_t apru, aprp;
extern pending_drill_t *pending_udrills, *pending_pdrills;
extern pcb_cardinal_t n_pending_udrills, max_pending_udrills;
extern pcb_cardinal_t n_pending_pdrills, max_pending_pdrills;

#endif
