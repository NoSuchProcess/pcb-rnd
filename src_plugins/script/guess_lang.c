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

#include <genht/htss.h>
#include <genht/ht_utils.h>
#include <puplug/util.h>

static int guess_lang_inited = 0;
static htss_t guess_lang_ext2lang;
static htss_t guess_lang_lang2eng;
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
	memcpy(guess_lang_eng, path, len);
	guess_lang_eng[len] = '\0';

	return 0;
}

static int guess_lang_line_split(pup_list_parse_pup_t *ctx, const char *fname, char *cmd, char *args)
{
	if (strcmp(cmd, "$script-ext") == 0) {
		char *lang = args, *ext = strpbrk(args, " \t");
		if (ext != NULL) {
			*ext = 0;
			ext++;
			while(isspace(*ext)) ext++;
		}
	}
	return 0;
}

static void rnd_script_guess_lang_init(void)
{
	if (!guess_lang_inited) {
		pup_list_parse_pup_t ctx = {0};
		const char *paths[2];

		htss_init(&guess_lang_ext2lang, strhash, strkeyeq);
		htss_init(&guess_lang_lang2eng, strhash, strkeyeq);

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
		genht_uninit_deep(htss, &guess_lang_ext2lang, {
			free(htent->key);
			free(htent->value);
		});
		guess_lang_inited = 0;
	}
}

const char *rnd_script_guess_lang(rnd_hidlib_t *hl, const char *fn, int is_filename)
{
	FILE *f;
	const char *res;

	rnd_script_guess_lang_init();

	if (!is_filename) {
		/* known special cases - these are pcb-rnd CLI conventions */
		if (strcmp(fn, "awk") == 0) fn = "mawk";
		if (strcmp(fn, "bas") == 0) fn = "fbas";
		if (strcmp(fn, "pas") == 0) fn = "fpas";
#ifdef RND_HAVE_SYS_FUNGW
		if (strcmp(fn, "ruby") == 0) fn = "mruby";
		if (strcmp(fn, "stt") == 0) fn = "estutter";
		if ((strcmp(fn, "js") == 0) || (strcmp(fn, "javascript") == 0)) fn = "duktape";
#endif

		/* find by engine name */
		if (htsp_get(&fgw_engines, fn) != NULL)
			return fn;
		return NULL;
	}

	/* find by file name: test parse */
	f = rnd_fopen(hl, fn, "r");
	res = fgw_engine_find(fn, f);
	if (f != NULL)
		fclose(f);

	return res;
}

