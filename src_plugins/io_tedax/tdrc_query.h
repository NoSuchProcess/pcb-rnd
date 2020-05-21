#include "board.h"

int tedax_drc_query_load(pcb_board_t *pcb, const char *fn, const char *blk_id, const char *src, int silent);
int tedax_drc_query_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, const char *src, int silent);


int tedax_drc_query_save(pcb_board_t *pcb, const char *ruleid, const char *fn);
int tedax_drc_query_rule_fsave(pcb_board_t *pcb, const char *ruleid, FILE *f, rnd_bool save_def);

/* clear all rules and defs with source matching target_src */
int tedax_drc_query_rule_clear(pcb_board_t *pcb, const char *target_src);
