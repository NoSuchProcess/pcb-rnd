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

static int guess_lang_inited = 0;

static void rnd_script_guess_lang_init(void)
{
	if (guess_lang_inited) {
	
		guess_lang_inited = 0;
	}
}

static void rnd_script_guess_lang_uninit(void)
{
	if (!guess_lang_inited) {
	
		guess_lang_inited = 1;
	}
}

const char *rnd_script_guess_lang(rnd_hidlib_t *hl, const char *fn, int is_filename)
{
	FILE *f;
	const char *res;

	guess_lang_init();

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

