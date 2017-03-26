#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <genvector/gds_char.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "config.h"
#include "wget_common.h"
#include "gedasymbols.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "compat_misc.h"

#define REQUIRE_PATH_PREFIX "wget@gedasymbols"

#define CGI_URL "http://www.gedasymbols.org/scripts/global_list.cgi"
#define FP_URL "http://www.gedasymbols.org/"
#define FP_DL "?dl"
static const char *url_idx_md5 = CGI_URL "?md5";
static const char *url_idx_list = CGI_URL;
static const char *gedasym_cache = "fp_wget_cache";
static const char *last_sum_fn = "fp_wget_cache/gedasymbols.last";

static char *load_md5_sum(FILE *f)
{
	char *s, sum[64];

	if (f == NULL)
		return NULL;

	*sum = '\0';
	fgets(sum, sizeof(sum), f);
	sum[sizeof(sum)-1] = '\0';

	for(s = sum;; s++) {
		if ((*s == '\0') || (isspace(*s))) {
			if ((s - sum) == 32) {
				*s = '\0';
				return pcb_strdup(sum);
			}
			else
				return NULL;
		}
		if (isdigit(*s))
			continue;
		if ((*s >= 'a') && (*s <= 'f'))
			continue;
		if ((*s >= 'A') && (*s <= 'F'))
			continue;
		return NULL;
	}
}

static int md5_cmp_free(const char *last_fn, char *md5_last, char *md5_new)
{
	int changed = 0;

	if ((md5_last == NULL) || (strcmp(md5_last, md5_new) != 0)) {
		FILE *f;
		f = fopen(last_fn, "w");
		fputs(md5_new, f);
		fclose(f);
		changed = 1;
	}
	if (md5_last != NULL)
		free(md5_last);
	free(md5_new);
	return changed;
}

int fp_gedasymbols_load_dir(pcb_plug_fp_t *ctx, const char *path, int force)
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

	if (force || (conf_fp_wget.plugins.fp_wget.auto_update_gedasymbols))
		wmode &= ~FP_WGET_OFFLINE;

	if (fp_wget_open(url_idx_md5, gedasym_cache, &f, &fctx, wmode) != 0)
		return -1;
	md5_new = load_md5_sum(f);
	fp_wget_close(&f, &fctx);

	if (md5_new == NULL)
		return -1;

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
		return -1;
	}

	gds_init(&vpath);
	gds_append_str(&vpath, REQUIRE_PATH_PREFIX);

	l = pcb_fp_mkdir_p(vpath.array);
	l->data.dir.backend = ctx;

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
	return 0;
}

#define FIELD_WGET_CTX 0

FILE *fp_gedasymbols_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx)
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
	gds_append_str(&s, FP_DL);

	fp_wget_open(s.array, gedasym_cache, &f, &(fctx->field[FIELD_WGET_CTX].i), FP_WGET_UPDATE);

	gds_uninit(&s);
	return f;
}

void fp_gedasymbols_fclose(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx)
{
	fp_wget_close(&f, &(fctx->field[FIELD_WGET_CTX].i));
}


static pcb_plug_fp_t fp_gedasymbols;

void fp_gedasymbols_uninit(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_gedasymbols);
}

void fp_gedasymbols_init(void)
{
	fp_gedasymbols.plugin_data = NULL;
	fp_gedasymbols.load_dir = fp_gedasymbols_load_dir;
	fp_gedasymbols.fopen = fp_gedasymbols_fopen;
	fp_gedasymbols.fclose = fp_gedasymbols_fclose;

	PCB_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_gedasymbols);
}
