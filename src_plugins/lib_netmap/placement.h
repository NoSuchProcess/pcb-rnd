#include "board.h"
#include "data.h"
#include "ht_subc.h"

/* Build a hash of subcircuit-to-prototype. Effectively a local library
   of unique footprint prototypes */

typedef struct {
	htscp_t subcs;   /* value is a normalized subc prototype */
	pcb_board_t *pcb;
	pcb_data_t data; /* temp buffer to place prototype subcs in */
} pcb_placement_t;

void pcb_placement_init(pcb_placement_t *ctx, pcb_board_t *pcb);
void pcb_placement_uninit(pcb_placement_t *ctx);

/* Iterate all subcircuits in data and build ctx->subcs hash */
void pcb_placement_build(pcb_placement_t *ctx, pcb_data_t *data);

/* return subc's prototype or NULL */
#define pcb_placement_get(ctx, subc)    htscp_get((ctx)->subcs, (subc))
