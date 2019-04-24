/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
#include <string.h>
#include <ctype.h>
#include <genlist/gendlist.h>

#include "cli_history.h"
#include "lib_hid_common.h"
#include "safe_fs.h"
#include "paths.h"

#define CFG dialogs_conf.plugins.lib_hid_common.cli_history

typedef struct hist_t {
	gdl_elem_t lst;
	char cmd[1];
} hist_t;

/* Global CLI history: first entry is the oldest, last is the most recent one */
static gdl_list_t history;
static int loaded = 0;
static int cursor = -1; /* 0 means the last (most recent) entry, positive numbers are previous entries */

static hist_t *hist_append(const char *s)
{
	char *end;
	hist_t *h;
	size_t len = strlen(s);

	h = malloc(sizeof(hist_t) + len);
	memcpy(h->cmd, s, len+1);
	memset(&h->lst, 0, sizeof(h->lst));
	gdl_append(&history, h, lst);

	end = strpbrk(h->cmd, "\r\n");
	if (end != NULL)
		*end = '\0';

	return h;
}

static hist_t *hist_lookup(const char *cmd, int *idx)
{
	hist_t *h;
	int i;

	for(i = 0, h = gdl_first(&history); h != NULL; h = gdl_next(&history, h), i++) {
		if (strcmp(h->cmd, cmd) == 0) {
			*idx = i;
			return h;
		}
	}

	return NULL;
}

/* Trim list length to configured value; NOTE: shall not be called
   from conf change event because there is no way it could notify
   GUIs */
void pcb_clihist_trim(void *ctx, pcb_clihist_remove_cb_t *remove)
{
	while(gdl_length(&history) > CFG.slots) {
		hist_t *h = gdl_first(&history);
		if (h == NULL)
			return; /* corner case: slots < 0 */
		gdl_remove(&history, h, lst);
		if (remove != NULL)
			remove(ctx, 0);
		free(h);
	}
}

void pcb_clihist_append(const char *cmd, void *ctx, pcb_clihist_append_cb_t *append, pcb_clihist_remove_cb_t *remove)
{
	int idx;
	hist_t *h;

	if ((cmd == NULL) || (*cmd == '\0'))
		return;

	/* check for a duplicate entry; when found, move it to the end of the list */
	h = hist_lookup(cmd, &idx);
	if (h != NULL) {
		if (strcmp(cmd, h->cmd) == 0) {
			gdl_remove(&history, h, lst);
			if (remove != NULL)
				remove(ctx, idx);
			gdl_append(&history, h, lst);
			if (append != NULL)
				append(ctx, h->cmd);
			return;
		}
	}

	/* not a duplicate: append at the end */
	h = hist_append(cmd);
	if (append != NULL)
		append(ctx, h->cmd);

	pcb_clihist_trim(ctx, remove);
	pcb_clihist_reset();
}

void pcb_clihist_sync(void *ctx, pcb_clihist_append_cb_t *append)
{
	hist_t *h;
	for(h = gdl_first(&history); h != NULL; h = gdl_next(&history, h))
		append(ctx, h->cmd);
}

static const char *hist_back_cmd(int cnt)
{
	hist_t *h;

	if (cnt < 0)
		return NULL;

	for(h = gdl_last(&history); (cnt > 0); h = gdl_prev(&history, h), cnt--)
		if (h == NULL)
			return NULL;
	return h->cmd;
}

const char *pcb_clihist_prev(void)
{
	if (cursor < 0)
		cursor = 0;
	else
		cursor++;
	if (cursor >= gdl_length(&history))
		cursor = gdl_length(&history) - 1;
	return hist_back_cmd(cursor);
}

const char *pcb_clihist_next(void)
{
	cursor--;
	if (cursor < -1)
		cursor = -1;
	return hist_back_cmd(cursor);
}

void pcb_clihist_reset(void)
{
	cursor = -1;
}


void pcb_clihist_load(void)
{
	FILE *f;
	char *real_fn, *s, line[4096];

	if ((CFG.file == NULL) || (CFG.slots < 1))
		return;

	real_fn = pcb_build_fn(CFG.file);
	if (real_fn == NULL)
		return;
	f = pcb_fopen(real_fn, "r");
	free(real_fn);
	if (f == NULL)
		return;

	while((s = fgets(line, sizeof(line), f)) != NULL) {
		while(isspace(*s)) s++;
		if (*s == '\0')
			continue;
		hist_append(s);
	}
	fclose(f);
	pcb_clihist_trim(NULL, NULL);
	loaded = 1;
}

void pcb_clihist_save(void)
{
	FILE *f;
	hist_t *h;
	char *real_fn;

	if ((CFG.file == NULL) || (CFG.slots < 1) || (!loaded))
		return;

	real_fn = pcb_build_fn(CFG.file);
	if (real_fn == NULL)
		return;
	f = pcb_fopen(real_fn, "w");
	free(real_fn);
	if (f == NULL)
		return;

	for(h = gdl_first(&history); h != NULL; h = gdl_next(&history, h))
		fprintf(f, "%s\n", h->cmd);

	fclose(f);
}

void pcb_clihist_init()
{
	if (loaded)
		return;

	pcb_clihist_load();
}

void pcb_clihist_uninit(void)
{
	hist_t *h;
	for(h = gdl_first(&history); h != NULL; h = gdl_first(&history)) {
		gdl_remove(&history, h, lst);
		free(h);
	}
}
