
#include "config.h"

#include "plugins.h"
#include "plug_footprint.h"

static int fp_board_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
}

static FILE *fp_board_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx)
{
}

static void fp_board_fclose(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
}


static pcb_plug_fp_t fp_board;

int pplg_check_ver_fp_board(int ver_needed) { return 0; }

void pplg_uninit_fp_board(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_board);
}

int pplg_init_fp_board(void)
{
	fp_board.plugin_data = NULL;
	fp_board.load_dir = fp_board_load_dir;
	fp_board.fopen = fp_board_fopen;
	fp_board.fclose = fp_board_fclose;
	PCB_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_board);
	return 0;
}
