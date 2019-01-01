#include "board.h"

int tedax_boardmeta_save(pcb_board_t *pcb, const char *fn);
int tedax_boardmeta_fsave(pcb_board_t *pcb, FILE *f);
int tedax_board_save(pcb_board_t *pcb, const char *fn);
int tedax_board_fsave(pcb_board_t *pcb, FILE *f);

int tedax_boardmeta_load(pcb_board_t *pcb, const char *fn);
int tedax_boardmeta_fload(pcb_board_t *pcb, FILE *f);
int tedax_board_load(pcb_board_t *pcb, const char *fn);
int tedax_board_fload(pcb_board_t *pcb, FILE *f);
