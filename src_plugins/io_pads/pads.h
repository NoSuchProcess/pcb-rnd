#include "board.h"

typedef struct pcb_pads_ctx_s {
	pcb_board_t *pcb;
	double coord_scale; /* multiply input integer coord values to get pcb-rnd nanometer */
	unsigned in_error:1;
} pcb_pads_ctx_t;