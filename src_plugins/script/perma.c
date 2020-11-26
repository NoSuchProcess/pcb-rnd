/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>

#include <librnd/core/hidlib.h>
#include <librnd/core/hid_init.h>

static int perma_load(rnd_hidlib_t *hl, const char *dir, const char *id, const char *path_in, const char *lang)
{
	char spath[RND_PATH_MAX];
	const char *path;

	if (!rnd_is_path_abs(path_in)) {
		path = spath;
		rnd_snprintf(spath, sizeof(spath), "%s%c%s", dir, RND_DIR_SEPARATOR_C, path_in);
	}
	else
		path = path_in;

	return rnd_script_load(hl, id, path, lang);
}

static void perma_script_load_conf(rnd_hidlib_t *hl, const char *dir)
{
	char path[RND_PATH_MAX], *errmsg;
	lht_doc_t *doc;
	lht_node_t *n, *npath, *nlang;
	FILE *f;
	long succ = 0;

	rnd_snprintf(path, sizeof(path), "%s%c%s", dir, RND_DIR_SEPARATOR_C, "scripts.lht");
	f = rnd_fopen(NULL, path, "r");
	if (f == NULL)
		return; /* non-existing or unreadable file is no error */
	doc = lht_dom_load_stream(f, path, &errmsg);
	fclose(f);

	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to parse script config '%s':\n'%s'\n", path, errmsg);
		goto end;
	}

	n = doc->root;
	if ((n->type != LHT_LIST) || (strcmp(n->name, "pcb-rnd-perma-script-v1") != 0)) {
		rnd_message(RND_MSG_ERROR, "Failed to load script config '%s':\nroot node is not li:pcb-rnd-perma-script-v1\n", path);
		goto end;
	}

	for(n = n->data.list.first; n != NULL; n = n->next) {
		const char *id = n->name, *path_in, *lang = NULL;
		if (n->type != LHT_HASH) {
			rnd_message(RND_MSG_ERROR, "ignoring non-hash child '%s' in '%s'\n", n->name, path);
			continue;
		}

		npath = lht_dom_hash_get(n, "path");
		if ((npath == NULL) || (npath->type != LHT_TEXT)) {
			rnd_message(RND_MSG_ERROR, "ignoring '%s' in '%s': no path\n", n->name, path);
			continue;
		}
		path_in = npath->data.text.value;

		nlang = lht_dom_hash_get(n, "lang");
		if (nlang != NULL) {
			if (npath->type != LHT_TEXT) {
				rnd_message(RND_MSG_ERROR, "ignoring '%s' in '%s': invalid lang node type\n", n->name, path);
				continue;
			}
			lang = nlang->data.text.value;
		}
		else { /* guess from path */
			lang = rnd_script_guess_lang(NULL, path_in, 1);
			if (lang == NULL) {
				rnd_message(RND_MSG_ERROR, "ignoring '%s' in '%s': no lang specified and failed to guess/recognize the language\n", n->name, path);
				continue;
			}
		}

		if (perma_load(hl, dir, id, path_in, lang) == 0)
			succ++;
		else
			rnd_message(RND_MSG_ERROR, "failed to load script '%s' in '%s'\n", n->name, path);
	}

	rnd_message(RND_MSG_INFO, "Loaded %ld scripts from '%s'\n", succ, path);

	end:;
	lht_dom_uninit(doc);
}

static void perma_script_init(rnd_hidlib_t *hl)
{
	static int inited = 0;

	if (inited) return;

	perma_script_load_conf(hl, rnd_conf_userdir_path);
	perma_script_load_conf(hl, rnd_conf_sysdir_path);

	inited = 1;
}

static void script_mainloop_perma_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (rnd_hid_in_main_loop)
		perma_script_init(hidlib);
}

