#include "data.h"
#include <genht/htsp.h>

int tedax_fp_save(pcb_data_t *data, const char *fn);
int tedax_fp_fsave(pcb_data_t *data, FILE *f);
int tedax_fp_load(pcb_data_t *data, const char *fn, int multi);

int tedax_pstk_fsave(pcb_pstk_t *padstack, pcb_coord_t ox, pcb_coord_t oy, FILE *f);




