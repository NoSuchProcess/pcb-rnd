#include "query_exec.h"

typedef struct {
	vtp0_t objs;
	unsigned has_junction:1;
	unsigned has_nontrace:1;
	pcb_any_obj_t *junction[3]; /* object with a junction on it before and after the list; 0..1 are ordered as objs; 2 is a spare field for internal use */
	pcb_any_obj_t *junc_at[3];  /* our last object (part of the seg) that faces the given junction */
	unsigned hub:1;             /* when set, this segment is a junction hub with more than 2 connected segments */
	rnd_coord_t len;
	int num_vias; /* number of functional vias, a.k.a. layer group changes */
} pcb_qry_netseg_len_t;

pcb_qry_netseg_len_t *pcb_qry_parent_net_lenseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from);

/* Return a segment (up to the first junction) starting from an object */
pcb_qry_netseg_len_t *pcb_qry_parent_net_len_mapseg(pcb_qry_exec_t *ec, pcb_any_obj_t *from);


void pcb_qry_lenseg_free_fields(pcb_qry_netseg_len_t *ns);

