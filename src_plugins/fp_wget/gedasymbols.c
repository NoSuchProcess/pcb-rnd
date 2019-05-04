/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017, 2018 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <genvector/gds_char.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "error.h"
#include "wget_common.h"
#include "gedasymbols.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "fp_wget_conf.h"
#include "globalconst.h"

#define REQUIRE_PATH_PREFIX "wget@gedasymbols"

#define CGI_URL "http://www.gedasymbols.org/scripts/global_list.cgi"
#define FP_URL "http://www.gedasymbols.org/"
#define FP_DL "?dl"
static const char *url_idx_md5 = CGI_URL "?md5";
static const char *url_idx_list = CGI_URL;


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
	char last_sum_fn[PCB_PATH_MAX];

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	pcb_snprintf(last_sum_fn, sizeof(last_sum_fn), "%s" PCB_DIR_SEPARATOR_S "gedasymbols.last", conf_fp_wget.plugins.fp_wget.cache_dir);

	gds_init(&vpath);
	gds_append_str(&vpath, REQUIRE_PATH_PREFIX);

	l = pcb_fp_mkdir_p(vpath.array);
	if (l != NULL)
		l->data.dir.backend = ctx;

	if (force || (conf_fp_wget.plugins.fp_wget.auto_update_gedasymbols))
		wmode &= ~FP_WGET_OFFLINE;

	if (fp_wget_open(url_idx_md5, conf_fp_wget.plugins.fp_wget.cache_dir, &f, &fctx, wmode) != 0) {
		if (wmode & FP_WGET_OFFLINE) /* accept that we don't have the index in offline mode */
			goto quit;
		goto err;
	}

	md5_new = load_md5_sum(f);
	fp_wget_close(&f, &fctx);

	if (md5_new == NULL)
		goto err;

	f = pcb_fopen(NULL, last_sum_fn, "r");
	md5_last = load_md5_sum(f);
	if (f != NULL)
		fclose(f);

/*	printf("old='%s' new='%s'\n", md5_last, md5_new);*/

	if (!md5_cmp_free(last_sum_fn, md5_last, md5_new)) {
/*		printf("no chg.\n");*/
		mode = FP_WGET_OFFLINE; /* use the cache */
	}
	else
		mode = 0;

	if (fp_wget_open(url_idx_list, conf_fp_wget.plugins.fp_wget.cache_dir, &f, &fctx, mode) != 0) {
		pcb_message(PCB_MSG_ERROR, "gedasymbols: failed to download the new list\n");
		pcb_remove(NULL, last_sum_fn); /* make sure it is downloaded next time */
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

	quit:;
	gds_uninit(&vpath);
	return 0;

	err:;
	gds_uninit(&vpath);
	return -1;
}

#define FIELD_WGET_CTX 0

FILE *fp_gedasymbols_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst)
{
	gds_t s;
	char tmp[1024];
	FILE *f = NULL;
	int from_path = (path != NULL) && (strcmp(path, REQUIRE_PATH_PREFIX) == 0);

	if (!from_path) {
		if (strncmp(name, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) == 0)
			name+=strlen(REQUIRE_PATH_PREFIX);
		else
			return NULL;
	}

	if (*name == '/')
		name++;

	if (from_path) {
		if (fp_wget_search(tmp, sizeof(tmp), name, !conf_fp_wget.plugins.fp_wget.auto_update_gedasymbols, url_idx_list, conf_fp_wget.plugins.fp_wget.cache_dir) != 0)
			goto bad;
		name = tmp;
	}

	gds_init(&s);
	gds_append_str(&s, FP_URL);
	gds_append_str(&s, name);
	gds_append_str(&s, FP_DL);

	fp_wget_open(s.array, conf_fp_wget.plugins.fp_wget.cache_dir, &f, &(fctx->field[FIELD_WGET_CTX].i), FP_WGET_UPDATE);

	gds_uninit(&s);

	bad:;
	fctx->backend = ctx;

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
	fp_gedasymbols.fp_fopen = fp_gedasymbols_fopen;
	fp_gedasymbols.fp_fclose = fp_gedasymbols_fclose;

	PCB_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_gedasymbols);
}
