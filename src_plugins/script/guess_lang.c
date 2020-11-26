/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
 *
 *  This module, debug, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <ctype.h>
#include <assert.h>

#include <genvector/vtp0.h>
#include <genht/htss.h>
#include <genht/htsp.h>
#include <genht/ht_utils.h>
#include <puplug/util.h>

static int guess_lang_inited = 0;
static htsp_t guess_lang_ext2lang; /* of (vtp0_t *) listing (char *) languages */
static htss_t guess_lang_lang2eng;
static htss_t guess_lang_alias;
static htsp_t guess_lang_engs; /* value is the same as key, value is not free'd */
static char *guess_lang_eng;

static int guess_lang_open(pup_list_parse_pup_t *ctx, const char *path, const char *basename)
{
	int len;

	if (strncmp(basename, "fungw_", 6) != 0)
		return 1;

	len = strlen(basename);
	guess_lang_eng = malloc(len+1);
	if (len > 4)
		len -= 4;
	memcpy(guess_lang_eng, basename, len);
	guess_lang_eng[len] = '\0';

	htsp_set(&guess_lang_engs, guess_lang_eng, guess_lang_eng);

	return 0;
}

static int guess_lang_line_split(pup_list_parse_pup_t *ctx, const char *fname, char *cmd, char *args)
{
	if (strcmp(cmd, "$script-ext") == 0) {
		char *eng, *lang = args, *ext = strpbrk(args, " \t");

		if (ext != NULL) {
			*ext = 0;
			ext++;
			while(isspace(*ext)) ext++;
		}

		eng = htss_get(&guess_lang_lang2eng, lang);
		if (eng != NULL) {
			if (strcmp(eng, guess_lang_eng) != 0) {
				rnd_message(RND_MSG_ERROR, "Script setup error: language %s is claimed by both '%s' and '%s' fungw bindings (accepted %s)\n", lang, eng, guess_lang_eng, eng);
				return 0;
			}
		}
		else {
/*printf("ADD: '%s' -> '%s'\n", lang, guess_lang_eng);*/
			htss_set(&guess_lang_lang2eng, rnd_strdup(lang), rnd_strdup(guess_lang_eng));
		}

		if (ext != NULL) { /* append language to the extension's lang vector */
			vtp0_t *langs = htsp_get(&guess_lang_ext2lang, ext);
			if (langs == NULL) {
				langs = calloc(sizeof(vtp0_t), 1);
				htsp_set(&guess_lang_ext2lang, rnd_strdup(ext), langs);
			}
			vtp0_append(langs, rnd_strdup(lang));
/*printf("APP: '%s' -> '%s'\n", ext, lang);*/
		}
	}
	else if (strcmp(cmd, "$lang-alias") == 0) {
		char *lang = args, *alias = strpbrk(args, " \t");

		if (alias != NULL) {
			*alias = 0;
			alias++;
			while(isspace(*alias)) alias++;
			if (htss_get(&guess_lang_alias, alias) == NULL)
				htss_set(&guess_lang_alias, rnd_strdup(alias), rnd_strdup(lang));
		}
	}
	return 0;
}

static void rnd_script_guess_lang_init(void)
{
	if (!guess_lang_inited) {
		pup_list_parse_pup_t ctx = {0};
		const char *paths[2];

		htsp_init(&guess_lang_ext2lang, strhash, strkeyeq);
		htss_init(&guess_lang_lang2eng, strhash, strkeyeq);
		htss_init(&guess_lang_alias, strhash, strkeyeq);
		htsp_init(&guess_lang_engs, strhash, strkeyeq);

		ctx.open = guess_lang_open;
		ctx.line_split = guess_lang_line_split;

		paths[0] = FGW_CFG_PUPDIR;
		paths[1] = NULL;
		pup_list_parse_pups(&ctx, paths);

		guess_lang_inited = 1;
	}
}

static void rnd_script_guess_lang_uninit(void)
{
	if (guess_lang_inited) {
		int n;

		genht_uninit_deep(htsp, &guess_lang_ext2lang, {
			vtp0_t *langs = htent->value;
			for(n = 0; n < langs->used; n++)
				free(langs->array[n]);
			vtp0_uninit(langs);
			free(htent->key);
			free(langs);
		});
		genht_uninit_deep(htss, &guess_lang_lang2eng, {
			free(htent->key);
			free(htent->value);
		});
		genht_uninit_deep(htss, &guess_lang_alias, {
			free(htent->key);
			free(htent->value);
		});
		genht_uninit_deep(htsp, &guess_lang_engs, {
			free(htent->key);
		});
		guess_lang_inited = 0;
	}
}

const char *rnd_script_lang2eng(const char **lang)
{
	const char *eng, *alang = htss_get(&guess_lang_alias, *lang);

	if (alang != NULL)
		*lang = alang;

	eng = htss_get(&guess_lang_lang2eng, *lang);
	if (eng == NULL) {
		/* last resort: maybe *lang is an eng, prefixewd with fungw_ */
		char name[RND_PATH_MAX];
		rnd_snprintf(name, RND_PATH_MAX, "fungw_%s", *lang);
		eng = htsp_get(&guess_lang_engs, name);
	}
	return eng;
}

const char *rnd_script_guess_lang(rnd_hidlib_t *hl, const char *fn, int is_filename)
{
	rnd_script_guess_lang_init();

	if (!is_filename) {
		const char *eng = rnd_script_lang2eng(&fn);

		/* find by engine name */
		if (eng != NULL)
			return fn;
		return NULL;
	}
	else {
		const char *ext;
		vtp0_t *langs;

		ext = strrchr(fn, '.');
		if (ext == NULL)
			return NULL;

		langs = htsp_get(&guess_lang_ext2lang, ext);
		if (langs == NULL)
			return NULL;

		assert(langs->used > 0);

		return langs->array[0];
	}
}

