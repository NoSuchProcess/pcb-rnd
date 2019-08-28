#include "board.h"

int tedax_etest_save(pcb_board_t *pcb, const char *etestid, const char *fn);
int tedax_etest_fsave(pcb_board_t *pcb, const char *etestid, FILE *f);

void tedax_etest_init(void);
void tedax_etest_uninit(void);

