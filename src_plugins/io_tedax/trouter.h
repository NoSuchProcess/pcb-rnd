#include "board.h"

int tedax_route_req_save(pcb_board_t *pcb, const char *fn, int cfg_argc, fgw_arg_t *cfg_argv);
int tedax_route_req_fsave(pcb_board_t *pcb, FILE *f, int cfg_argc, fgw_arg_t *cfg_argv);

int tedax_route_res_fload(FILE *fn, const char *blk_id, int silent);
int tedax_route_res_load(const char *fname, const char *blk_id, int silent);


void *tedax_route_conf_keys_load(const char *fname, const char *blk_id, int silent);

