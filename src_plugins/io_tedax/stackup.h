#include "board.h"
#include <genht/htsp.h>
#include <genvector/vtp0.h>

typedef struct {
	htsp_t n2g; /* tedax name to layer group pointer */
	vtp0_t g2n; /* group ID to tedax layer name */
	unsigned include_grp_id:1; /* whether to include group IDs in the name as prefix */
} tedax_stackup_t;


void tedax_stackup_init(tedax_stackup_t *ctx);
void tedax_stackup_uninit(tedax_stackup_t *ctx);

int tedax_stackup_save(pcb_board_t *pcb, const char *stackid, const char *fn);
int tedax_stackup_fsave(tedax_stackup_t *ctx, pcb_board_t *pcb, const char *stackid, FILE *f, pcb_layer_type_t lyt);

int tedax_stackup_load(pcb_board_t *pcb, const char *fn, const char *blk_id, int silent);
int tedax_stackup_fload(tedax_stackup_t *ctx, pcb_board_t *pcb, FILE *f, const char *blk_id, int silent);
