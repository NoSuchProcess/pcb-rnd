#include "view.h"
#include "obj_common.h"

typedef struct pcb_net_int_s pcb_net_int_t;
typedef int (pcb_int_broken_cb_t)(pcb_net_int_t *ctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype);

struct pcb_net_int_s {
	pcb_board_t *pcb;
	pcb_find_t fa, fb;
	pcb_data_t *data;
	pcb_coord_t bloat, shrink;
	unsigned fast:1;
	unsigned shrunk:1;

	/* called on integrity check failure: either broken trace (shrunk==1) or
	   short circuit (shrunk==0); like 'find' callback; return
	   non-zero to break the search */
	pcb_int_broken_cb_t *broken_cb;
	void *cb_data;
};


/* Check for DRC violations on a single net starting from the pad or pin
   sees if the connectivity changes when everything is bloated, or shrunk.
   If shrink or bloat is 0, that side of the test is skipped */
pcb_bool pcb_net_integrity(pcb_board_t *pcb, pcb_any_obj_t *from, pcb_coord_t shrink, pcb_coord_t bloat, pcb_int_broken_cb_t *cb, void *cb_data);

