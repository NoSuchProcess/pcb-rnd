#include "board.h"
#include "data.h"
#include "ht_subc.h"

typedef struct {
	htscp_t subcs;
	pcb_board_t *pcb;
	pcb_data_t data; /* temp buffer to place prototype subcs in */
} pcb_placement_t;

void pcb_placement_init(pcb_placement_t *ctx, pcb_board_t *pcb);
void pcb_placement_uninit(pcb_placement_t *ctx);
void pcb_placement_build(pcb_placement_t *ctx, pcb_data_t *data);
