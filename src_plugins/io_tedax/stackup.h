#include "board.h"
#include <genht/htsp.h>
#include <genvector/vtp0.h>

typedef struct {
	htsp_t n2g; /* tedax name to layer group pointer */
	vtp0_t g2n; /* group ID to tedax layer name */
} tedax_stackup_t;


int tedax_stackup_save(pcb_board_t *pcb, const char *fn);
int tedax_stackup_fsave(tedax_stackup_t *ctx, pcb_board_t *pcb, FILE *f);

int tedax_stackup_load(pcb_board_t *pcb, const char *fn);
int tedax_stackup_fload(tedax_stackup_t *ctx, pcb_board_t *pcb, FILE *f);
