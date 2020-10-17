#include "data.h"
#include <genht/htsp.h>
#include "plug_io.h"

int tedax_fp_save(pcb_data_t *data, const char *fn, long subc_idx);
int tedax_fp_fsave(pcb_data_t *data, FILE *f, long subc_idx);
int tedax_fp_fsave_subc(pcb_subc_t *subc, FILE *f);
int tedax_fp_load(pcb_data_t *data, const char *fn, int multi, const char *blk_id, int silent, int searchlib);

/* parse a single footprint at current file pos; returns NULL on error */
pcb_subc_t *tedax_parse_1fp(pcb_data_t *data, FILE *fn, char *buff, int buff_size, char *argv[], int argv_size);

/* Save a single subc, with footprint header */
int tedax_fp_fsave_subc_(pcb_subc_t *subc, const char *fpname, int lyrecipe, FILE *f);

int tedax_pstk_fsave(pcb_pstk_t *padstack, rnd_coord_t ox, rnd_coord_t oy, FILE *f);

pcb_plug_fp_map_t *tedax_fp_map(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags);




