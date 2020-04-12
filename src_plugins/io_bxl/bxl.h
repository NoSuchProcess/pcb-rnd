#ifndef PCB_BXL_H
#define PCB_BXL_H

#include "data.h"
#include "obj_subc.h"

typedef struct pcb_bxl_ctx_s {
	pcb_data_t *data;
	pcb_subc_t *sc;
	char in_target_fp; /* 1 if we are parsing the target footprint; else skip */
} pcb_bxl_ctx_t;

#endif
