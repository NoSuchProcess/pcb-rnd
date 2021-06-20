#include "query_exec.h"

typedef struct {
	vtp0_t objs;
	unsigned has_junction:1;
	unsigned has_nontrace:1;
	pcb_any_obj_t *junction[2]; /* object with a junction on it before and after the list */
	rnd_coord_t ex[2], ey[2]; /* extend from the first and last object to the junction point */
	rnd_coord_t len;
	int num_vias; /* number of functional vias, a.k.a. layer group changes */
} pcb_qry_netseg_len_t;

pcb_qry_netseg_len_t *pcb_qry_parent_net_lenseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from);

/* Return a segment (up to the first junction) starting from an object */
pcb_qry_netseg_len_t *pcb_qry_parent_net_len_mapseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from);


void pcb_qry_lenseg_free_fields(pcb_qry_netseg_len_t *ns);

