
#include "config.h"

#include "plugins.h"
#include "plug_footprint.h"
#include "board.h"
#include "buffer.h"
#include "data.h"
#include "error.h"
#include "obj_elem.h"
#include "obj_elem_list.h"
#include "obj_elem_op.h"
#include "compat_misc.h"
#include "pcb-printf.h"
#include "operation.h"
#include "plug_io.h"

#define REQUIRE_PATH_PREFIX "board@"

static int fp_board_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
	const char *fpath;
	pcb_fplibrary_t *l;
	pcb_buffer_t buff;
	unsigned long int id;
	elementlist_dedup_initializer(dedup);

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	fpath = path + strlen(REQUIRE_PATH_PREFIX);

	/* load file */
	memset(&buff, 0, sizeof(buff));
	if (pcb_buffer_load_layout(PCB, &buff, fpath, NULL) != pcb_true) {
		pcb_message(PCB_MSG_ERROR, "Warning: failed to load %s\n", fpath);
		return -1;
	}

	/* make sure lib dir is ready */
	l = pcb_fp_lib_search(&pcb_library, path);
	if (l == NULL)
		l = pcb_fp_mkdir_len(&pcb_library, path, -1);

	/* add unique elements */
	id = 0;
	PCB_ELEMENT_LOOP(buff.Data) {
		const char *ename;
		pcb_fplibrary_t *e;

		id++;
		elementlist_dedup_skip(dedup, element);

		ename = element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString;
		if (ename == NULL)
			ename = "anonymous";
		e = pcb_fp_append_entry(l, ename, PCB_FP_FILE, NULL);

		/* remember location by ID - because of the dedup search by name is unsafe */
		if (e != NULL)
			e->data.fp.loc_info = pcb_strdup_printf("%s@%lu", path, id);

	} PCB_END_LOOP;

	elementlist_dedup_free(dedup);

	/* clean up buffer */
	pcb_buffer_clear(PCB, &buff);
	free(buff.Data);

	return 0;
}

static FILE *fp_board_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx)
{
	char *fpath, *ids, *end;
	unsigned long int id, req_id;
	pcb_buffer_t buff;
	FILE *f = NULL;
	pcb_opctx_t op;
	char *tmp_name = ".board.fp";

	if (strncmp(name, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return NULL;

	/* split file name and ID */
	fpath = pcb_strdup(name + strlen(REQUIRE_PATH_PREFIX));
	ids = strchr(fpath, '@');
	if (ids == NULL)
		goto err;

	*ids = '\0';
	ids++;

	req_id = strtoul(ids, &end, 10);
	if (*end != '\0')
		goto err;

	/* load file */
	memset(&buff, 0, sizeof(buff));
	if (pcb_buffer_load_layout(PCB, &buff, fpath, NULL) != pcb_true) {
		pcb_message(PCB_MSG_ERROR, "Warning: failed to load %s\n", fpath);
		goto err;
	}

	/* find the reuqested footprint in the file */
	id = 0;
	PCB_ELEMENT_LOOP(buff.Data) {
		id++;
		if (id == req_id) {
/*			if (strcmp(element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString, l->name)) */
			pcb_buffer_t buff2;

#warning TODO: extend the API:
			/* This is not pretty: we are saving the element to a file so we can return a FILE *.
			   Later on we should just return the footprint. */
			memset(&op, 0, sizeof(op));
			op.buffer.dst = calloc(sizeof(pcb_data_t), 1);
			pcb_data_set_layer_parents(op.buffer.dst);
			AddElementToBuffer(&op, element);

			f = fopen(tmp_name, "w");
			memset(&buff2, 0, sizeof(buff2));
			buff2.Data = op.buffer.dst;
			pcb_write_buffer(f, &buff2, "pcb");
			fclose(f);

			pcb_data_free(op.buffer.dst);
			free(op.buffer.dst);

			f = fopen(tmp_name, "r");
			break;
		}
	} PCB_END_LOOP;

	/* clean up buffer */

	pcb_buffer_clear(PCB, &buff);
	free(buff.Data);
	return f;

err:;
		free(fpath);
		return NULL;
}

static void fp_board_fclose(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
	fclose(f);
#warning TODO: remove file
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
