#include "board.h"

int tedax_drc_query_load(pcb_board_t *pcb, const char *fn, const char *blk_id, const char *src, int silent);
int tedax_drc_query_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, const char *src, int silent);
