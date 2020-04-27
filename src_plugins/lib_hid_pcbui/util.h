#include "data.h"

/* calculate the bounding box of all selections/founds in data, return the number
   of selected objects. When returns 0, the dst box is infinitely large. */
rnd_cardinal_t pcb_get_selection_bbox(rnd_box_t *dst, const pcb_data_t *data);
rnd_cardinal_t pcb_get_found_bbox(rnd_box_t *dst, const pcb_data_t *data);

