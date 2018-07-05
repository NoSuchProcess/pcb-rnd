#include "data.h"

/* calculate the bounding box of all selections/founds in data, return the number
   of selected objects. When returns 0, the dst box is infinitely large. */
pcb_cardinal_t pcb_get_selection_bbox(pcb_box_t *dst, const pcb_data_t *data);
pcb_cardinal_t pcb_get_found_bbox(pcb_box_t *dst, const pcb_data_t *data);

