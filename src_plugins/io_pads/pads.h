#include "board.h"

typedef struct pcb_pads_ctx_s {
	pcb_board_t *pcb;
	unsigned in_error:1;
} pcb_pads_ctx_t;