#ifndef DELAY_CLEARANCE
#define DELAY_CLEARANCE

#include "board.h"

typedef enum {
	PCB_DLCL_TRACE,
	PCB_DLCL_SMD_TERM,
	PCB_DLCL_TRH_TERM,
	PCB_DLCL_VIA,
	PCB_DLCL_POLY,
	PCB_DLCL_max
} pcb_dlcl_t;

void pcb_dlcr_apply(pcb_board_t *pcb, rnd_coord_t clr[PCB_DLCL_max]);

#endif

