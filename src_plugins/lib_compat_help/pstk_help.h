#ifndef PCB_PSTK_HELP_H
#define PCB_PSTK_HELP_H

#include "obj_pstk.h"
#include <genvector/vtp0.h>

/* create a new adstack that contains only a hole, but no shapes */
pcb_pstk_t *pcb_pstk_new_hole(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_bool plated);

/* Convert a vector of (pcb_any_obj_t *) into zero or more padstacks. Remove
   objects that are converted  from both data and objs. New padstacks are
   placed back in objs. Quiet controls log messages. Return the number
   of padstacks created. Return -1 on error.
   WARNING: O(n^2) loops, assuming there are only a dozen of objects passed. */
int pcb_pstk_vect2pstk_thr(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet); /* thru-hole pins only */
int pcb_pstk_vect2pstk_smd(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet); /* smd pads only */

/* Same as above, but convert both; returns -1 only if both failed */
int pcb_pstk_vect2pstk(pcb_data_t *data, vtp0_t *objs, pcb_bool_t quiet);

#endif
