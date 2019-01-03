#include "data.h"
#include <genht/htsp.h>

int tedax_fp_save(pcb_data_t *data, const char *fn);
int tedax_fp_fsave(pcb_data_t *data, FILE *f);
int tedax_fp_load(pcb_data_t *data, const char *fn, int multi);

/* Save a single subc, with footprint header */
int tedax_fp_fsave_subc(pcb_subc_t *subc, const char *fpname, int lyrecipe, FILE *f);

int tedax_pstk_fsave(pcb_pstk_t *padstack, pcb_coord_t ox, pcb_coord_t oy, FILE *f);




