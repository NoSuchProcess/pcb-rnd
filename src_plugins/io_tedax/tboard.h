#include "board.h"

int tedax_board_save(pcb_board_t *pcb, const char *fn);
int tedax_board_fsave(pcb_board_t *pcb, FILE *f);

int tedax_board_load(pcb_board_t *pcb, const char *fn, const char *blk_id, int silent);
int tedax_board_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, int silent);
