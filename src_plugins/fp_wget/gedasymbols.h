#include "plug_footprint.h"
int fp_gedasymbols_load_dir(plug_fp_t *ctx, const char *path);
FILE *fp_gedasymbols_fopen(plug_fp_t *ctx, const char *path, const char *name, fp_fopen_ctx_t *fctx);
void fp_gedasymbols_fclose(plug_fp_t *ctx, FILE * f, fp_fopen_ctx_t *fctx);

