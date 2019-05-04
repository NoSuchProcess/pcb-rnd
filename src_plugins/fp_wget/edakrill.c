/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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
#include "edakrill.h"
#include "plugins.h"
#include "plug_footprint.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "fp_wget_conf.h"
#include "globalconst.h"

#define REQUIRE_PATH_PREFIX "wget@edakrill"

#define ROOT_URL "http://www.repo.hu/projects/edakrill/"
#define FP_URL ROOT_URL "user/"

static const char *url_idx_md5 = ROOT_URL "tags.idx.md5";
static const char *url_idx_list = ROOT_URL "tags.idx";

struct {
	char *name;
	char *fname;
	int fp;
	long date;
	void **tags;
	int tags_used, tags_alloced;
} krill;

static void tag_add_(const char *kv)
{
	if (krill.tags_used >= krill.tags_alloced) {
		krill.tags_alloced += 16;
		krill.tags = realloc(krill.tags, sizeof(void *) * krill.tags_alloced);
	}
	krill.tags[krill.tags_used] = (void *)(kv == NULL ? NULL : pcb_fp_tag(kv, 1));
	krill.tags_used++;
}

static void tag_add(const char *key, const char *val)
{
	char *next, *tmp;
	for(; val != NULL; val = next) {
		next = strchr(val, ',');
		if (next != NULL) {
			*next = '\0';
			next++;
		}
		while(*val == ' ') val++;
		if (*val == '\0')
			break;
		tmp = pcb_strdup_printf("%s:%s", key, val);
		tag_add_(tmp);
		free(tmp);
	}
}

static void krill_flush(pcb_plug_fp_t *ctx, gds_t *vpath, int base_len)
{
	if ((krill.fp) && (krill.fname != NULL)) {
		char *end, *fn;
		pcb_fplibrary_t *l;

/*		printf("Krill %s: %s [%ld]\n", krill.name, krill.fname, krill.date);*/

		/* split path and fn; path stays in vpath.array, fn is a ptr to the file name */
		gds_truncate(vpath, base_len);
		gds_append_str(vpath, krill.fname);
		end = vpath->array + vpath->used - 1;
		while((end > vpath->array) && (*end != '/')) { end--; vpath->used--; }
		*end = '\0';
		vpath->used--;
		end++;
		fn = end;

		/* add to the database */
		l = pcb_fp_mkdir_p(vpath->array);
		if (krill.tags != NULL)
			tag_add_(NULL);
		l = pcb_fp_append_entry(l, fn, PCB_FP_FILE, krill.tags);
		fn[-1] = '/';
		l->data.fp.loc_info = pcb_strdup(vpath->array);
		
		krill.tags = NULL;
		krill.tags_used = 0;
	}

	krill.tags_used = 0;

	free(krill.name);
	free(krill.fname);
	free(krill.tags);
	krill.name = NULL;
	krill.fname = NULL;
	krill.fp = 0;
	krill.date = 0;
	krill.tags = NULL;
	krill.tags_alloced = 0;
}

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
	char last_sum_fn[PCB_PATH_MAX];

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	pcb_snprintf(last_sum_fn, sizeof(last_sum_fn), "%s" PCB_DIR_SEPARATOR_S "edakrill.last", conf_fp_wget.plugins.fp_wget.cache_dir);

	gds_init(&vpath);
	gds_append_str(&vpath, REQUIRE_PATH_PREFIX);

	l = pcb_fp_mkdir_p(vpath.array);
	if (l != NULL)
		l->data.dir.backend = ctx;

	if (force || (conf_fp_wget.plugins.fp_wget.auto_update_edakrill))
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
		pcb_message(PCB_MSG_ERROR, "edakrill: failed to download the new list\n");
		pcb_remove(NULL, last_sum_fn); /* make sure it is downloaded next time */
		goto err;
	}

	gds_append(&vpath, '/');
	vpath_base_len = vpath.used;

	while(fgets(line, sizeof(line), f) != NULL) {
		char *end;
		if ((*line == '#') || (line[1] != ' '))
			continue;

		end = line + strlen(line) - 1;
		*end = '\0';
		if (*line == 'f') {
			krill_flush(ctx, &vpath, vpath_base_len);
			krill.name = pcb_strdup(line+2);
		}
		if (strncmp(line, "t type=", 7) == 0) {
			if (strcmp(line+7, "footprint") == 0)
				krill.fp = 1;
		}
		if (*line == 't') {
			char *val, *key = line+2;
			val = strchr(key, '=');
			if (val != NULL) {
				*val = '\0';
				val++;
				if ((strcmp(key, "auto/file") != 0) && (strcmp(key, "type") != 0)) {
					tag_add(key, val);
				}
			}
		}
		if (*line == 'm') {
			end = strstr(line, ".cnv.fp ");
			if (end != NULL) {
				end += 7;
				*end = '\0';
				end++;
				krill.fname = pcb_strdup(line+2);
				krill.date = strtol(end, NULL, 10);
			}
		}

	}
	krill_flush(ctx, &vpath, vpath_base_len);
	fp_wget_close(&f, &fctx);

	quit:;
	gds_uninit(&vpath);
	return 0;

	err:;
	gds_uninit(&vpath);
	return -1;
}

#define FIELD_WGET_CTX 0

FILE *fp_edakrill_fopen(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst)
{
	gds_t s;
	char tmp[1024];
	FILE *f;
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
		if (fp_wget_search(tmp, sizeof(tmp), name, !conf_fp_wget.plugins.fp_wget.auto_update_edakrill, url_idx_list, conf_fp_wget.plugins.fp_wget.cache_dir) != 0)
			goto bad;
		name = tmp;
	}

	gds_init(&s);
	gds_append_str(&s, FP_URL);
	gds_append_str(&s, name);

	fp_wget_open(s.array, conf_fp_wget.plugins.fp_wget.cache_dir, &f, &(fctx->field[FIELD_WGET_CTX].i), FP_WGET_UPDATE);

	gds_uninit(&s);

	bad:;
	fctx->backend = ctx;

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
	fp_edakrill.fp_fopen = fp_edakrill_fopen;
	fp_edakrill.fp_fclose = fp_edakrill_fclose;

	PCB_HOOK_REGISTER(pcb_plug_fp_t, pcb_plug_fp_chain, &fp_edakrill);
}
