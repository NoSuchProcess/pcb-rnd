#include "board.h"

int tedax_route_req_save(pcb_board_t *pcb, const char *fn);
int tedax_route_req_fsave(pcb_board_t *pcb, FILE *f);

int tedax_route_res_fload(FILE *fn, const char *blk_id, int silent);
int tedax_route_res_load(const char *fname, const char *blk_id, int silent);


