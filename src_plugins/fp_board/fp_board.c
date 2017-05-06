
#include "config.h"

#include "plugins.h"
#include "plug_footprint.h"
#include "board.h"
#include "buffer.h"
#include "data.h"
#include "error.h"
#include "obj_elem.h"
#include "obj_elem_list.h"
#include "compat_misc.h"
#include "pcb-printf.h"

#define REQUIRE_PATH_PREFIX "board@"

static int fp_board_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
	const char *fpath;
	pcb_fplibrary_t *l;
	pcb_buffer_t buff;
	elementlist_dedup_initializer(dedup);
	unsigned long int id;

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	fpath = path + strlen(REQUIRE_PATH_PREFIX);

	memset(&buff, 0, sizeof(buff));
	if (pcb_buffer_load_layout(PCB, &buff, fpath, NULL) != pcb_true) {
		pcb_message(PCB_MSG_ERROR, "Warning: failed to load %s\n", fpath);
		return -1;
	}

	l = pcb_fp_lib_search(&pcb_library, path);
	if (l == NULL)
		l = pcb_fp_mkdir_len(&pcb_library, path, -1);

	id = 0;
	PCB_ELEMENT_LOOP(buff.Data) {
		const char *ename;
		char *name;
		pcb_fplibrary_t *e;

		id++;
		elementlist_dedup_skip(dedup, element);

		ename = element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString;
		e = pcb_fp_append_entry(l, ename, PCB_FP_FILE, NULL);

		/* remember location by ID - because of the dedup search by name is unsafe */
		if (e != NULL)
			e->data.fp.loc_info = pcb_strdup_printf("%s@%lu", path, id);

	} PCB_END_LOOP;

	elementlist_dedup_free(dedup);

	pcb_buffer_clear(PCB, &buff);
	free(buff.Data);

	return 0;
}

static FILE *fp_board_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx)
{
	char *fpath, *id;

	if (strncmp(name, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return NULL;

	fpath = pcb_strdup(name + strlen(REQUIRE_PATH_PREFIX));
	id = strchr(fpath, '@');
	if (id == NULL) {
		free(fpath);
		return NULL;
	}
	*id = '\0';
	id++;
	printf("woops: '%s'@'%s'\n", fpath, id);


	return NULL;
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
