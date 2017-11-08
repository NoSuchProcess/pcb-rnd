#ifndef PCB_PSTK_COMPAT_H
#define PCB_PSTK_COMPAT_H

#include "obj_pstk.h"

typedef enum {
	PCB_PSTK_COMPAT_ROUND = 0,
	PCB_PSTK_COMPAT_SQUARE = 1,
	PCB_PSTK_COMPAT_SHAPED = 2, /* old pcb-rnd pin shapes */
	PCB_PSTK_COMPAT_SHAPED_END = 16, /* old pcb-rnd pin shapes */
	PCB_PSTK_COMPAT_OCTAGON
} pcb_pstk_compshape_t;

/* Create a padstack that emulates an old-style via - register proto as needed */
pcb_pstk_t *pcb_pstk_new_compat_via(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_coord_t pad_dia, pcb_coord_t clearance, pcb_coord_t mask, pcb_pstk_compshape_t shp);

#endif
