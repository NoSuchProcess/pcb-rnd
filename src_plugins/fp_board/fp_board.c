
#include "config.h"

#include "plugins.h"
#include "plug_footprint.h"
#include "board.h"
#include "buffer.h"
#include "error.h"


#define REQUIRE_PATH_PREFIX "board@"




static int fp_board_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
	const char *fpath;
	pcb_fplibrary_t *l;
	pcb_buffer_t buff;

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	fpath = path + strlen(REQUIRE_PATH_PREFIX);

	if (pcb_buffer_load_layout(PCB, &buff, fpath, NULL) != pcb_true) {
		pcb_message(PCB_MSG_ERROR, "Warning: failed to load %s\n", fpath);
		return -1;
	}

	printf("OK!\n");

	l = pcb_fp_lib_search(&pcb_library, path);
	if (l == NULL)
		l = pcb_fp_mkdir_len(&pcb_library, path, -1);

/*	l = pcb_fp_append_entry(l, fn, PCB_FP_FILE, NULL);*/

	pcb_buffer_clear(PCB, &buff);
	free(buff.Data);

	return 0;
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
