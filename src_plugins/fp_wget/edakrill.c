#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <genvector/gds_char.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "config.h"
#include "wget_common.h"
#include "edakrill.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "compat_misc.h"

#define REQUIRE_PATH_PREFIX "wget@edakrill"

#define ROOT_URL "http://www.repo.hu/projects/edakrill/"
#define FP_URL ROOT_URL "user/"

static const char *url_idx_md5 = ROOT_URL "tags.idx.md5";
static const char *url_idx_list = ROOT_URL "tags.idx";
static const char *gedasym_cache = "fp_wget_cache";
static const char *last_sum_fn = "fp_wget_cache/edakrill.last";

int fp_edakrill_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
{
	FILE *f;
	int fctx;
	char *md5_last, *md5_new;
	char line[1024];
	fp_get_mode mode;
	gds_t vpath;
	int vpath_base_len;
	fp_get_mode wmode = FP_WGET_OFFLINE;
	pcb_fplibrary_t *l;

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	gds_init(&vpath);
	gds_append_str(&vpath, REQUIRE_PATH_PREFIX);

	l = pcb_fp_mkdir_p(vpath.array);
	if (l != NULL)
		l->data.dir.backend = ctx;

	if (force || (conf_fp_wget.plugins.fp_wget.auto_update_edakrill))
		wmode &= ~FP_WGET_OFFLINE;

	if (fp_wget_open(url_idx_md5, gedasym_cache, &f, &fctx, wmode) != 0) {
		if (wmode & FP_WGET_OFFLINE) /* accept that we don't have the index in offline mode */
			goto quit;
		goto err;
	}

	md5_new = load_md5_sum(f);
	fp_wget_close(&f, &fctx);

	if (md5_new == NULL)
		goto err;

	f = fopen(last_sum_fn, "r");
	md5_last = load_md5_sum(f);
	if (f != NULL)
		fclose(f);

	printf("old='%s' new='%s'\n", md5_last, md5_new);

	if (!md5_cmp_free(last_sum_fn, md5_last, md5_new)) {
		printf("no chg.\n");
		mode = FP_WGET_OFFLINE; /* use the cache */
	}
	else
		mode = 0;

	if (fp_wget_open(url_idx_list, gedasym_cache, &f, &fctx, mode) != 0) {
		printf("failed to download the new list\n");
		remove(last_sum_fn); /* make sure it is downloaded next time */
		goto err;
	}

	gds_append(&vpath, '/');
	vpath_base_len = vpath.used;

	while(fgets(line, sizeof(line), f) != NULL) {
		char *end, *fn;

		if (*line == '#')
			continue;
		end = strchr(line, '|');
		if (end == NULL)
			continue;
		*end = '\0';

		/* split path and fn; path stays in vpath.array, fn is a ptr to the file name */
		gds_truncate(&vpath, vpath_base_len);
		gds_append_str(&vpath, line);
		end = vpath.array + vpath.used - 1;
		while((end > vpath.array) && (*end != '/')) { end--; vpath.used--; }
		*end = '\0';
		vpath.used--;
		end++;
		fn = end;

		/* add to the database */
		l = pcb_fp_mkdir_p(vpath.array);
		l = pcb_fp_append_entry(l, fn, PCB_FP_FILE, NULL);
		fn[-1] = '/';
		l->data.fp.loc_info = pcb_strdup(vpath.array);
	}
	fp_wget_close(&f, &fctx);

	printf("update!\n");

	quit:;
	gds_uninit(&vpath);
	return 0;

	err:;
	gds_uninit(&vpath);
	return -1;
}

#define FIELD_WGET_CTX 0

FILE *fp_edakrill_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx)
{
	gds_t s;
	FILE *f;

	if (strncmp(name, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) == 0)
		name+=strlen(REQUIRE_PATH_PREFIX);
	else
		return NULL;

	if (*name == '/')
		name++;

	gds_init(&s);
	gds_append_str(&s, FP_URL);
	gds_append_str(&s, name);

	fp_wget_open(s.array, gedasym_cache, &f, &(fctx->field[FIELD_WGET_CTX].i), FP_WGET_UPDATE);

	gds_uninit(&s);
	return f;
}

void fp_edakrill_fclose(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
	fp_wget_close(&f, &(fctx->field[FIELD_WGET_CTX].i));
}


static pcb_plug_fp_t fp_edakrill;

void fp_edakrill_uninit(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_edakrill);
}

void fp_edakrill_init(void)
{
	fp_edakrill.plugin_data = NULL;
	fp_edakrill.load_dir = fp_edakrill_load_dir;
	fp_edakrill.fopen = fp_edakrill_fopen;
	fp_edakrill.fclose = fp_edakrill_fclose;

	PCB_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_edakrill);
}
