
#include "config.h"

#include "plugins.h"
#include "plug_footprint.h"
#include "board.h"
#include "buffer.h"
#include "data.h"
#include "error.h"
#include "obj_subc.h"
#include "obj_subc_list.h"
#include "obj_subc_op.h"
#include "compat_misc.h"
#include "pcb-printf.h"
#include "operation.h"
#include "plug_io.h"
#include "safe_fs.h"

#define REQUIRE_PATH_PREFIX "board@"

static int fp_board_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
	const char *fpath;
	pcb_fplibrary_t *l;
	pcb_buffer_t buff;
	unsigned long int id;
	int old_dedup;
	pcb_subclist_dedup_initializer(dedup);

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
	old_dedup = pcb_subc_hash_ignore_uid;
	pcb_subc_hash_ignore_uid = 1;
	id = 0;
	PCB_SUBC_LOOP(buff.Data) {
		const char *ename;
		pcb_fplibrary_t *e;

		id++;
		pcb_subclist_dedup_skip(dedup, subc);

		ename = pcb_attribute_get(&subc->Attributes, "footprint");
		if (ename == NULL)
			ename = subc->refdes;
		if (ename == NULL)
			ename = "anonymous";
		e = pcb_fp_append_entry(l, ename, PCB_FP_FILE, NULL);

		/* remember location by ID - because of the dedup search by name is unsafe */
		if (e != NULL)
			e->data.fp.loc_info = pcb_strdup_printf("%s@%lu", path, id);

	} PCB_END_LOOP;

	pcb_subc_hash_ignore_uid = old_dedup;
	pcb_subclist_dedup_free(dedup);

	/* clean up buffer */
	pcb_buffer_clear(PCB, &buff);
	free(buff.Data);

	return 0;
}

static FILE *fp_board_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst)
{
	char *fpath, *ids, *end;
	unsigned long int id, req_id;
	pcb_buffer_t buff;
	FILE *f = NULL;
	pcb_opctx_t op;

	if (dst == NULL)
		return NULL;

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
	PCB_SUBC_LOOP(buff.Data) {
		id++;
		if (id == req_id) {
/*			if (strcmp(element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString, l->name)) */

TODO(": extend the API:")
			/* This is not pretty: we are saving the element to a file so we can return a FILE *.
			   Later on we should just return the footprint. */
			memset(&op, 0, sizeof(op));
			op.buffer.dst = dst;
			pcb_data_set_layer_parents(op.buffer.dst);
			pcb_subcop_add_to_buffer(&op, subc);

			return PCB_FP_FOPEN_IN_DST;
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
TODO(": remove file")
}


static pcb_plug_fp_t fp_board;

int pplg_check_ver_fp_board(int ver_needed) { return 0; }

void pplg_uninit_fp_board(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_board);
}

int pplg_init_fp_board(void)
{
	PCB_API_CHK_VER;
	fp_board.plugin_data = NULL;
	fp_board.load_dir = fp_board_load_dir;
	fp_board.fp_fopen = fp_board_fopen;
	fp_board.fp_fclose = fp_board_fclose;
	PCB_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_board);
	return 0;
}
