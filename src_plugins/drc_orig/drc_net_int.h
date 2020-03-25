#include "view.h"
#include "obj_common.h"

/* Check for DRC violations on a single net starting from the pad or pin
   sees if the connectivity changes when everything is bloated, or shrunk.
   If shrink or bloat is 0, that side of the test is skipped */
pcb_bool pcb_net_integrity(pcb_board_t *pcb, pcb_view_list_t *lst, pcb_any_obj_t *from, pcb_coord_t shrink, pcb_coord_t bloat);

