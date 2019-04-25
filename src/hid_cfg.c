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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "config.h"
#include "hid.h"
#include "hid_cfg.h"
#include "error.h"
#include "paths.h"
#include "safe_fs.h"
#include "compat_misc.h"
#include "file_loaded.h"
#include "conf_core.h"

char hid_cfg_error_shared[1024];

typedef struct {
	pcb_hid_cfg_t *hr;
	pcb_create_menu_widget_t cb;
	void *cb_ctx;
	lht_node_t *parent;
	const pcb_menu_prop_t *props;
	int target_level;
	int err;
	lht_node_t *after;
} create_menu_ctx_t;

static lht_node_t *create_menu_cb(void *ctx, lht_node_t *node, const char *path, int rel_level)
{
	create_menu_ctx_t *cmc = ctx;
	lht_node_t *psub, *after;

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
			psub = pcb_hid_cfg_menu_field(cmc->parent, PCB_MF_SUBMENU, NULL);

		if (rel_level == cmc->target_level) {
			node = pcb_hid_cfg_create_hash_node(psub, cmc->after, name, "dyn", "1", "cookie", cmc->props->cookie, "a", cmc->props->accel, "tip", cmc->props->tip, "action", cmc->props->action, "checked", cmc->props->checked, "update_on", cmc->props->update_on, "foreground", cmc->props->foreground, "background", cmc->props->background, NULL);
			if (node != NULL)
				cmc->err = 0;
		}
		else
			node = pcb_hid_cfg_create_hash_node(psub, cmc->after, name, "dyn", "1", "cookie", cmc->props->cookie,  NULL);

		if (node == NULL)
			return NULL;

		if ((rel_level != cmc->target_level) || (cmc->props->action == NULL))
			lht_dom_hash_put(node, lht_dom_node_alloc(LHT_LIST, "submenu"));

		if (node->parent == NULL) {
			lht_dom_list_append(psub, node);
		}
		else {
			assert(node->parent == psub);
		}

		if ((cmc->after != NULL) && (cmc->after->parent->parent == cmc->parent))
			after = cmc->after;
		else
			after = NULL;

		if (cmc->cb(cmc->cb_ctx, path, name, (rel_level == 1), cmc->parent, after, node) != 0) {
			cmc->err = -1;
			return NULL;
		}
	}
	else {
		/* existing level */
		if ((node->type == LHT_TEXT) && (node->data.text.value[0] == '@')) {
			cmc->after = node;
			goto skip_parent;
		}
	}
	cmc->parent = node;

	skip_parent:;
	return node;
}

int pcb_hid_cfg_create_menu(pcb_hid_cfg_t *hr, const char *path, const pcb_menu_prop_t *props, pcb_create_menu_widget_t cb, void *cb_ctx)
{
	const char *name;
	create_menu_ctx_t cmc;

	cmc.hr = hr;
	cmc.cb = cb;
	cmc.cb_ctx = cb_ctx;
	cmc.parent = NULL;
	cmc.props = props;
	cmc.err = -1;
	cmc.after = NULL;

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

int pcb_hid_cfg_remove_menu_node(pcb_hid_cfg_t *hr, lht_node_t *root, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	if (root == NULL)
		return -1;

	if (root->type == LHT_HASH) {
		lht_node_t *psub, *n, *next;
		psub = pcb_hid_cfg_menu_field(root, PCB_MF_SUBMENU, NULL);
		if (psub != NULL) { /* remove a whole submenu with all children */
			int res = 0;
			for(n = psub->data.list.first; n != NULL; n = next) {
				next = n->next;
				if (pcb_hid_cfg_remove_menu_node(hr, n, gui_remove, ctx) != 0)
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

int pcb_hid_cfg_remove_menu_cookie(pcb_hid_cfg_t *hr, const char *cookie, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx, int level, lht_node_t *curr)
{
	lht_err_t err;
	lht_dom_iterator_t it;

	if ((curr != hr->doc->root) && (level > 1)) {
		curr = lht_tree_path_(hr->doc, curr, "submenu", 1, 0, &err);
		if (curr == NULL)
			return 0;
	}

	for(curr = lht_dom_first(&it, curr); curr != NULL; curr = lht_dom_next(&it)) {
		lht_node_t *nck = lht_tree_path_(hr->doc, curr, "cookie", 1, 0, &err);

		if ((nck != NULL) && (strcmp(nck->data.text.value, cookie) == 0))
			pcb_hid_cfg_remove_menu_node(hr, curr, gui_remove, ctx);
		else
			pcb_hid_cfg_remove_menu_cookie(hr, cookie, gui_remove, ctx, level+1, curr);
	}

	/* always success: not finding any menu is not considered an error */
	return 0;
}

int pcb_hid_cfg_remove_menu(pcb_hid_cfg_t *hr, const char *path, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx)
{
	lht_node_t *nd = pcb_hid_cfg_get_menu_at(hr, NULL, path, NULL, NULL);

	if (nd == NULL)
		return pcb_hid_cfg_remove_menu_cookie(hr, path, gui_remove, ctx, 0, hr->doc->root);
	else
		return pcb_hid_cfg_remove_menu_node(hr, nd, gui_remove, ctx);
}

static int hid_cfg_load_error(lht_doc_t *doc, const char *filename, lht_err_t err)
{
	const char *fn;
	int line, col;
	lht_dom_loc_active(doc, &fn, &line, &col);
	pcb_message(PCB_MSG_ERROR, "Resource error: %s (%s:%d.%d)*\n", lht_err_str(err), filename, line+1, col+1);
	return 1;
}

lht_doc_t *pcb_hid_cfg_load_lht(const char *filename)
{
	FILE *f;
	lht_doc_t *doc;
	int error = 0;

	f = pcb_fopen(filename, "r");
	if (f == NULL)
		return NULL;

	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, filename);

	while(!(feof(f))) {
		lht_err_t err;
		int c = fgetc(f);
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				error = hid_cfg_load_error(doc, filename, err);
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

	/* override HID defaults with the configured path */
	if ((conf_core.rc.menu_file != NULL) && (*conf_core.rc.menu_file != '\0')) {
		fn = conf_core.rc.menu_file;
		exact_fn = (strchr(conf_core.rc.menu_file, '/') != NULL);
	}

	if (!exact_fn) {
		/* try different paths to find the menu file inventing its exact name */
		static const char *hid_cfg_paths_in[] = { "./", "~/.pcb-rnd/", PCBSHAREDIR "/", NULL };
		char **paths = NULL, **p;
		int fn_len = strlen(fn);

		doc = NULL;
		pcb_paths_resolve_all(hid_cfg_paths_in, paths, fn_len+32, pcb_false);
		for(p = paths; *p != NULL; p++) {
			if (doc == NULL) {
				char *end = *p + strlen(*p);
				sprintf(end, "pcb-menu-%s.lht", fn);
				doc = pcb_hid_cfg_load_lht(*p);
				if (doc != NULL)
					pcb_file_loaded_set_at("menu", "HID main", *p, "main menu system");
			}
			free(*p);
		}
		free(paths);
	}
	else
		doc = pcb_hid_cfg_load_lht(fn);

	if (doc == NULL) {
		doc = pcb_hid_cfg_load_str(embedded_fallback);
		pcb_file_loaded_set_at("menu", "HID main", "<internal>", "main menu system");
	}
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
	int level = 0, len = strlen(menu_path), iafter = 0;
	char *next_seg, *path;

 path = malloc(len+4); /* need a few bytes after the end for the ':' */
 strcpy(path, menu_path);

	next_seg = path;
	curr = (at == NULL) ? hr->doc->root : at;

	/* Have to descend manually because of the submenu nodes */
	for(;;) {
		char *next, *end;
		lht_dom_iterator_t it;

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
		
		*end = '\0';

		/* find next_seg in the current level */
		for(curr = lht_dom_first(&it, curr); curr != NULL; curr = lht_dom_next(&it)) {
			if (*next_seg == '@') {
				/* looking for an anon text node with the value matching the anchor name */
				if ((curr->type == LHT_TEXT) && (strcmp(curr->data.text.value, next_seg) == 0)) {
					iafter = 1;
					break;
				}
			}
			else {
				/* looking for a hash node */
				if (strcmp(curr->name, next_seg) == 0)
					break;
			}
		}

		if (cb != NULL)
			curr = cb(ctx, curr, path, level);

		if (next != NULL) /* restore previous / so that path is a full path */
			*next = '/';
		next_seg = next;
		if ((curr == NULL) || (next_seg == NULL))
			break;
		next_seg++;
		level++;
		if (iafter) {
			/* special case: insert after an @anchor and exit */
			if (cb != NULL)
				curr = cb(ctx, NULL, path, level);
			break;
		}
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
		case PCB_MF_ACCELERATOR:  fieldstr = "a"; break;
		case PCB_MF_SUBMENU:      fieldstr = "submenu"; break;
		case PCB_MF_CHECKED:      fieldstr = "checked"; break;
		case PCB_MF_UPDATE_ON:    fieldstr = "update_on"; break;
		case PCB_MF_SENSITIVE:    fieldstr = "sensitive"; break;
		case PCB_MF_TIP:          fieldstr = "tip"; break;
		case PCB_MF_ACTIVE:       fieldstr = "active"; break;
		case PCB_MF_ACTION:       fieldstr = "action"; break;
		case PCB_MF_FOREGROUND:   fieldstr = "foreground"; break;
		case PCB_MF_BACKGROUND:   fieldstr = "background"; break;
		case PCB_MF_FONT:         fieldstr = "font"; break;
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
	lht_node_t *n = pcb_hid_cfg_menu_field(submenu, PCB_MF_SUBMENU, &fldname);
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

lht_node_t *pcb_hid_cfg_create_hash_node(lht_node_t *parent, lht_node_t *ins_after, const char *name, ...)
{
	lht_node_t *n;
	va_list ap;

	if ((parent != NULL) && (parent->type != LHT_LIST))
		return NULL;

	/* ignore ins_after if we are already deeper in the tree */
	if ((ins_after != NULL) && (ins_after->parent != parent))
		ins_after = NULL;

	n = lht_dom_node_alloc(LHT_HASH, name);
	if (ins_after != NULL) {
		/* insert as next sibling below a @anchor */
		lht_dom_list_insert_after(ins_after, n);
	}
	else if (parent != NULL) {
		/* insert as last item under a parent node */
		lht_dom_list_append(parent, n);
	}

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

void pcb_hid_cfg_error(const lht_node_t *node, const char *fmt, ...)
{
	char *end;
	va_list ap;
	int len, maxlen = sizeof(hid_cfg_error_shared);

	len = pcb_snprintf(hid_cfg_error_shared, maxlen, "Error in lihata node %s:%d.%d:", node->file_name, node->line, node->col);
	end = hid_cfg_error_shared + len;
	maxlen -= len;
	va_start(ap, fmt);
	end += pcb_vsnprintf(end, maxlen, fmt, ap);
	va_end(ap);
	pcb_message(PCB_MSG_ERROR, hid_cfg_error_shared);
}

static void map_anchor_menus(vtp0_t *dst, lht_node_t *node, const char *name)
{
	lht_dom_iterator_t it;

	switch(node->type) {
		case LHT_HASH:
		case LHT_LIST:
			for(node = lht_dom_first(&it, node); node != NULL; node = lht_dom_next(&it))
				map_anchor_menus(dst, node, name);
			break;
		case LHT_TEXT:
			if ((node->parent != NULL) && (node->parent->type == LHT_LIST) && (strcmp(node->data.text.value, name) == 0))
				vtp0_append(dst, node);
			break;
		default:
			break;
	}
}

void pcb_hid_cfg_map_anchor_menus(const char *name, void (*cb)(void *ctx, pcb_hid_cfg_t *cfg, lht_node_t *n, char *path), void *ctx)
{
	vtp0_t anchors;

	if ((pcb_gui == NULL) || (pcb_gui->hid_cfg == NULL) || (pcb_gui->hid_cfg->doc == NULL) || (pcb_gui->hid_cfg->doc->root == NULL))
		return;

	/* extract anchors; don't do the callbacks from there because the tree
	   is going to be changed from the callbacks. It is however guaranteed
	   that anchors are not removed. */
	vtp0_init(&anchors);
	map_anchor_menus(&anchors, pcb_gui->hid_cfg->doc->root, name);

	/* iterate over all anchors extracted and call the callback */
	{
		char *path = NULL;
		int used = 0, alloced = 0, l0 = strlen(name) + 128;
		size_t an;

		for(an = 0; an < vtp0_len(&anchors); an++) {
			lht_node_t *n, *node = anchors.array[an];

			for(n = node->parent; n != NULL; n = n->parent) {
				int len;
				if (strcmp(n->name, "submenu") == 0)
					continue;
				len = strlen(n->name);
				if (used+len+2+l0 >= alloced) {
					alloced = used+len+2+l0 + 128;
					path = realloc(path, alloced);
				}
				memmove(path+len+1, path, used);
				memcpy(path, n->name, len);
				path[len] = '/';
				used += len+1;
			}
			memcpy(path+used, name, l0+1);
/*			pcb_trace("path='%s' used=%d\n", path, used);*/
			cb(ctx, pcb_gui->hid_cfg, node, path);
			used = 0;
		}
		free(path);
	}

	vtp0_uninit(&anchors);
}

int pcb_hid_cfg_del_anchor_menus(lht_node_t *node, const char *cookie)
{
	lht_node_t *nxt;

	if ((node->type != LHT_TEXT) || (node->data.text.value == NULL) || (node->data.text.value[0] != '@'))
		return -1;

	for(node = node->next; node != NULL; node = nxt) {
		lht_node_t *ncookie;

		if (node->type != LHT_HASH)
			break;
		ncookie = lht_dom_hash_get(node, "cookie");

/*		pcb_trace("  '%s' cookie='%s'\n", node->name, ncookie == NULL ? "NULL":ncookie->data.text.value );*/
		if ((ncookie == NULL) || (ncookie->type != LHT_TEXT) || (strcmp(ncookie->data.text.value, cookie) != 0))
			break;

		nxt = node->next;
		pcb_gui->remove_menu_node(node);
	}
	return 0;
}
