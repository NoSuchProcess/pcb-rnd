/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "config.h"
#include "hid_cfg.h"
#include "error.h"
#include "paths.h"
#include "compat_misc.h"

char hid_cfg_error_shared[1024];

typedef struct {
	pcb_hid_cfg_t *hr;
	pcb_create_menu_widget_t cb;
	void *cb_ctx;
	lht_node_t *parent;
	const char *action;
	const char *mnemonic;
	const char *accel;
	const char *tip;
	const char *cookie;
	int target_level;
	int err;
} create_menu_ctx_t;

static lht_node_t *create_menu_cb(void *ctx, lht_node_t *node, const char *path, int rel_level)
{
	create_menu_ctx_t *cmc = ctx;
	lht_node_t *psub;

/*	printf(" CB: '%s' %p at %d->%d\n", path, node, rel_level, cmc->target_level);*/
	if (node == NULL) { /* level does not exist, create it */
		const char *name;
		name = strrchr(path, '/');
		if (name != NULL)
			name++;
		else
			name = path;

		if (rel_level <= 1) {
			/* creating a main menu */
			char *end, *name = pcb_strdup(path);
			for(end = name; *end == '/'; end++) ;
			end = strchr(end, '/');
			*end = '\0';
			psub = cmc->parent = pcb_hid_cfg_get_menu(cmc->hr, name);
			free(name);
		}
		else
			psub = pcb_hid_cfg_menu_field(cmc->parent, MF_SUBMENU, NULL);

		if (rel_level == cmc->target_level) {
			node = pcb_hid_cfg_create_hash_node(psub, name, "dyn", "1", "m", "cookie", cmc->cookie, cmc->mnemonic, "a", cmc->accel, "tip", cmc->tip, ((cmc->action != NULL) ? "action": NULL), cmc->action, NULL);
			if (node != NULL)
				cmc->err = 0;
		}
		else
			node = pcb_hid_cfg_create_hash_node(psub, name, "dyn", "1", "cookie", cmc->cookie,  NULL);

		if (node == NULL)
			return NULL;

		if ((rel_level != cmc->target_level) || (cmc->action == NULL))
			lht_dom_hash_put(node, lht_dom_node_alloc(LHT_LIST, "submenu"));

		if (node->parent == NULL)
			lht_dom_list_append(psub, node);
		else
			assert(node->parent == psub);

		if (cmc->cb(cmc->cb_ctx, path, name, (rel_level == 1), cmc->parent, node) != 0) {
			cmc->err = -1;
			return NULL;
		}
	}
	cmc->parent = node;
	return node;
}

int pcb_hid_cfg_create_menu(pcb_hid_cfg_t *hr, const char *path, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie, pcb_create_menu_widget_t cb, void *cb_ctx)
{
	const char *name;
	create_menu_ctx_t cmc;

	cmc.hr = hr;
	cmc.cb = cb;
	cmc.cb_ctx = cb_ctx;
	cmc.parent = NULL;
	cmc.action = action;
	cmc.mnemonic = mnemonic;
	cmc.accel = accel;
	cmc.tip = tip;
	cmc.cookie = cookie;
	cmc.err = -1;

	/* Allow creating new nodes only under certain main paths that correspond to menus */
	name = path;
	while(*name == '/') name++;

	if ((strncmp(name, "main_menu/", 10) == 0) || (strncmp(name, "popups/", 7) == 0)) {
		/* calculate target level */
		for(cmc.target_level = 0; *name != '\0'; name++) {
			if (*name == '/') {
				cmc.target_level++;
				while(*name == '/') name++;
				name--;
			}
		}

		/* descend and visit each level, create missing levels */

		pcb_hid_cfg_get_menu_at(hr, NULL, path, create_menu_cb, &cmc);
	}

	return cmc.err;
}

static int hid_cfg_remove_item(pcb_hid_cfg_t *hr, lht_node_t *item, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if (gui_remove(ctx, item) != 0)
		return -1;
	lht_tree_del(item);
	return 0;
}


static int hid_cfg_remove_menu_(pcb_hid_cfg_t *hr, lht_node_t *root, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if (root == NULL)
		return -1;

	if (root->type == LHT_HASH) {
		lht_node_t *psub, *n, *next;
		psub = pcb_hid_cfg_menu_field(root, MF_SUBMENU, NULL);
		if (psub != NULL) { /* remove a whole submenu with all children */
			int res = 0;
			for(n = psub->data.list.first; n != NULL; n = next) {
				next = n->next;
				if (hid_cfg_remove_menu_(hr, n, gui_remove, ctx) != 0)
					res = -1;
			}
			if (res == 0)
				res = hid_cfg_remove_item(hr, root, gui_remove, ctx);
			return res;
		}
	}

	if ((root->type != LHT_TEXT) && (root->type != LHT_HASH)) /* allow text for the sep */
		return -1;

	/* remove a simple menu item */
	return hid_cfg_remove_item(hr, root, gui_remove, ctx);
}

int pcb_hid_cfg_remove_menu(pcb_hid_cfg_t *hr, const char *path, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	return hid_cfg_remove_menu_(hr, pcb_hid_cfg_get_menu_at(hr, NULL, path, NULL, NULL), gui_remove, ctx);
}


static int hid_cfg_load_error(lht_doc_t *doc, const char *filename, lht_err_t err)
{
	const char *fn;
	int line, col;
	lht_dom_loc_active(doc, &fn, &line, &col);
	pcb_message(PCB_MSG_DEFAULT, "Resource error: %s (%s:%d.%d)*\n", lht_err_str(err), filename, line+1, col+1);
	return 1;
}

lht_doc_t *pcb_hid_cfg_load_lht(const char *filename)
{
	FILE *f;
	lht_doc_t *doc;
	int error = 0;
	char *efn;

	resolve_path(filename, &efn, 0);

	f = fopen(efn, "r");
	if (f == NULL) {
		free(efn);
		return NULL;
	}
	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, efn);

	while(!(feof(f))) {
		lht_err_t err;
		int c = fgetc(f);
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				error = hid_cfg_load_error(doc, efn, err);
				break;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}

	if (error) {
		lht_dom_uninit(doc);
		doc = NULL;
	}
	fclose(f);

	free(efn);
	return doc;
}

lht_doc_t *pcb_hid_cfg_load_str(const char *text)
{
	lht_doc_t *doc;
	int error = 0;

	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, "embedded");

	while(*text != '\0') {
		lht_err_t err;
		int c = *text++;
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				error = hid_cfg_load_error(doc, "internal", err);
				break;
			}
			break; /* error or stop, do not read anymore (would get LHTE_STOP without any processing all the time) */
		}
	}

	if (error) {
		lht_dom_uninit(doc);
		doc = NULL;
	}

	return doc;
}

const char *pcb_hid_cfg_text_value(lht_doc_t *doc, const char *path)
{
	lht_node_t *n = lht_tree_path(doc, "/", path, 1, NULL);
	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		pcb_hid_cfg_error(n, "Error: node %s should be a text node\n", path);
		return NULL;
	}
	return n->data.text.value;
}

pcb_hid_cfg_t *pcb_hid_cfg_load(const char *fn, int exact_fn, const char *embedded_fallback)
{
	lht_doc_t *doc;
	pcb_hid_cfg_t *hr;

	if (!exact_fn) {
		/* try different paths to find the menu file inventing its exact name */
		static const char *hid_cfg_paths_in[] = { "./", PCBSHAREDIR "/", NULL };
		char **paths = NULL, **p;
		int fn_len = strlen(fn);

		doc = NULL;
		resolve_all_paths(hid_cfg_paths_in, paths, fn_len+32);
		for(p = paths; *p != NULL; p++) {
			if (doc == NULL) {
				char *end = *p + strlen(*p);
				sprintf(end, "pcb-menu-%s.lht", fn);
				doc = pcb_hid_cfg_load_lht(*p);
				if (doc != NULL)
					pcb_message(PCB_MSG_DEFAULT, "Loaded menu file '%s'\n", *p);
			}
			free(*p);
		}
		free(paths);
	}
	else
		doc = pcb_hid_cfg_load_lht(fn);

	if (doc == NULL)
		doc = pcb_hid_cfg_load_str(embedded_fallback);
	if (doc == NULL)
		return NULL;

	hr = calloc(sizeof(pcb_hid_cfg_t), 1); /* make sure the cache is cleared */
	hr->doc = doc;

	return hr;
}

/************* "parsing" **************/

lht_node_t *pcb_hid_cfg_get_menu_at(pcb_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx)
{
	lht_err_t err;
	lht_node_t *curr;
	int level = 0, len = strlen(menu_path);
	char *next_seg, *path;

 path = malloc(len+4); /* need a few bytes after the end for the ':' */
 strcpy(path, menu_path);

	next_seg = path;
	curr = (at == NULL) ? hr->doc->root : at;

	/* Have to descend manually because of the submenu nodes */
	for(;;) {
		char *next, *end, save;
		while(*next_seg == '/') next_seg++;
		if (curr != hr->doc->root) {
			if (level > 1) {
				curr = lht_tree_path_(hr->doc, curr, "submenu", 1, 0, &err);
				if (curr == NULL)
					break;
			}
		}
		next = end = strchr(next_seg, '/');
		if (end == NULL)
			end = next_seg + strlen(next_seg);
		
		if (level > 0)
			*end = ':';
		else
			*end = '\0';
		end++;
		save = *end;
		*end = '\0';
		
		curr = lht_tree_path_(hr->doc, curr, next_seg, 1, 0, &err);
		if (cb != NULL) {
			end[-1] = '\0';
			curr = cb(ctx, curr, path, level);
		}

		*end = save;
		if (next != NULL) /* restore previous / so that path is a full path */
			*next = '/';
		next_seg = next;
		if ((curr == NULL) || (next_seg == NULL))
			break;
		next_seg++;
		level++;
	}

	free(path);
	return curr;
}

lht_node_t *pcb_hid_cfg_get_menu(pcb_hid_cfg_t *hr, const char *menu_path)
{
	return pcb_hid_cfg_get_menu_at(hr, NULL, menu_path, NULL, NULL);
}

lht_node_t *pcb_hid_cfg_menu_field(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field, const char **field_name)
{
	lht_err_t err;
	const char *fieldstr = NULL;

	switch(field) {
		case MF_ACCELERATOR:  fieldstr = "a"; break;
		case MF_MNEMONIC:     fieldstr = "m"; break;
		case MF_SUBMENU:      fieldstr = "submenu"; break;
		case MF_CHECKED:      fieldstr = "checked"; break;
		case MF_UPDATE_ON:    fieldstr = "update_on"; break;
		case MF_SENSITIVE:    fieldstr = "sensitive"; break;
		case MF_TIP:          fieldstr = "tip"; break;
		case MF_ACTIVE:       fieldstr = "active"; break;
		case MF_ACTION:       fieldstr = "action"; break;
		case MF_FOREGROUND:   fieldstr = "foreground"; break;
		case MF_BACKGROUND:   fieldstr = "background"; break;
		case MF_FONT:         fieldstr = "font"; break;
/*		case MF_RADIO:        fieldstr = "radio"; break; */
	}
	if (field_name != NULL)
		*field_name = fieldstr;

	if (fieldstr == NULL)
		return NULL;

	return lht_tree_path_(submenu->doc, submenu, fieldstr, 1, 0, &err);
}

const char *pcb_hid_cfg_menu_field_str(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field)
{
	const char *fldname;
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, field, &fldname);

	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		pcb_hid_cfg_error(submenu, "Error: field %s should be a text node\n", fldname);
		return NULL;
	}
	return n->data.text.value;
}

int pcb_hid_cfg_has_submenus(const lht_node_t *submenu)
{
	const char *fldname;
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, MF_SUBMENU, &fldname);
	if (n == NULL)
		return 0;
	if (n->type != LHT_LIST) {
		pcb_hid_cfg_error(submenu, "Error: field %s should be a list (of submenus)\n", fldname);
		return 0;
	}
	return 1;
}


void pcb_hid_cfg_extend_hash_nodev(lht_node_t *node, va_list ap)
{
	for(;;) {
		char *cname, *cval;
		lht_node_t *t;

		cname = va_arg(ap, char *);
		if (cname == NULL)
			break;
		cval = va_arg(ap, char *);

		if (cval != NULL) {
			t = lht_dom_node_alloc(LHT_TEXT, cname);
			t->data.text.value = pcb_strdup(cval);
			lht_dom_hash_put(node, t);
		}
	}
}

void pcb_hid_cfg_extend_hash_node(lht_node_t *node, ...)
{
	va_list ap;
	va_start(ap, node);
	pcb_hid_cfg_extend_hash_nodev(node, ap);
	va_end(ap);
}

lht_node_t *pcb_hid_cfg_create_hash_node(lht_node_t *parent, const char *name, ...)
{
	lht_node_t *n;
	va_list ap;

	if ((parent != NULL) && (parent->type != LHT_LIST))
		return NULL;

	n = lht_dom_node_alloc(LHT_HASH, name);
	if (parent != NULL)
		lht_dom_list_append(parent, n);

	va_start(ap, name);
	pcb_hid_cfg_extend_hash_nodev(n, ap);
	va_end(ap);

	return n;
}

lht_node_t *pcb_hid_cfg_menu_field_path(const lht_node_t *parent, const char *path)
{
	return lht_tree_path_(parent->doc, parent, path, 1, 0, NULL);
}

int pcb_hid_cfg_dfs(lht_node_t *parent, int (*cb)(void *ctx, lht_node_t *n), void *ctx)
{
	lht_dom_iterator_t it;
	lht_node_t *n;

	for(n = lht_dom_first(&it, parent); n != NULL; n = lht_dom_next(&it)) {
		int ret;
		ret = cb(ctx, n);
		if (ret != 0)
			return ret;
		if (n->type != LHT_TEXT) {
			ret = pcb_hid_cfg_dfs(n, cb, ctx);
			if (ret != 0)
				return ret;
		}
	}
	return 0;
}

/* extern char hid_cfg_error_shared[]; */
void pcb_hid_cfg_error(const lht_node_t *node, const char *fmt, ...)
{
	char *end;
	va_list ap;

	end = hid_cfg_error_shared + sprintf(hid_cfg_error_shared, "Error in lihata node %s:%d.%d:", node->file_name, node->line, node->col);
	va_start(ap, fmt);
	end += vsprintf(end, fmt, ap);
	va_end(ap);
	pcb_message(PCB_MSG_DEFAULT, hid_cfg_error_shared);
}
