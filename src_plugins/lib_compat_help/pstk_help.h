#ifndef PCB_PSTK_COMPAT_H
#define PCB_PSTK_COMPAT_H

#include "obj_pstk.h"

/* create a new adstack that contains only a hole, but no shapes */
pcb_pstk_t *pcb_pstk_new_hole(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_bool plated);

#endif
