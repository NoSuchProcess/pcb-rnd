#ifndef PCB_DRILL_H
#define PCB_DRILL_H

#include "aperture.h"

void pcb_drill_export_excellon(pcb_board_t *pcb, pcb_drill_ctx_t *ctx, int force_g85, const char *fn);

int pplg_check_ver_export_excellon(int ver_needed);
void pplg_uninit_export_excellon(void);
int pplg_init_export_excellon(void);

#endif
