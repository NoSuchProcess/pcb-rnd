#include "find.h"
#include "obj_common.h"
#include "query_exec.h"

typedef struct pcb_net_int_s pcb_net_int_t;
typedef int (pcb_int_broken_cb_t)(pcb_net_int_t *ctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype);

struct pcb_net_int_s {
	pcb_board_t *pcb;
	pcb_find_t fa, fb;
	pcb_data_t *data;
	rnd_coord_t bloat, shrink;
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
rnd_bool pcb_net_integrity(pcb_board_t *pcb, pcb_any_obj_t *from, rnd_coord_t shrink, rnd_coord_t bloat, pcb_int_broken_cb_t *cb, void *cb_data);


/* Map the network of the from object. If it's a segment of a netlisted net,
   return the terminal object with the lowest ID from that segment. If it's
   an unlisted (floating) net, return the lowest ID copper object. (Worst
   case it is a floating net with only this one object - then the object
   is returned). The search is cached. */
pcb_any_obj_t *pcb_qry_parent_net_term(pcb_qry_exec_t *ec, pcb_any_obj_t *from);
