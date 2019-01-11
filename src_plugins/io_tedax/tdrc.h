#include "board.h"

int tedax_drc_save(pcb_board_t *pcb, const char *drcid, const char *fn);
int tedax_drc_fsave(pcb_board_t *pcb, const char *drcid, FILE *f);

int tedax_drc_load(pcb_board_t *pcb, const char *fn);
int tedax_drc_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, int silent);
