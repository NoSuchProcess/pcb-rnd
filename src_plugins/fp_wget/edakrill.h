#include "plug_footprint.h"
#include "fp_wget_conf.h"
int fp_edakrill_load_dir(pcb_plug_fp_t *ctx, const char *path, int force);
FILE *fp_edakrill_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst);
void fp_edakrill_fclose(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx);
void fp_edakrill_init(void);
void fp_edakrill_uninit(void);

