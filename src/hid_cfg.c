/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
#include <liblihata/lihata.h>
#include <liblihata/tree.h>

#include "global.h"
#include "hid.h"
#include "hid_cfg.h"
#include "hid_actions.h"
#include "hid_cfg.h"
#include "error.h"
#include "paths.h"

char hid_cfg_error_shared[1024];

lht_node_t *hid_cfg_create_menu(hid_cfg_t *hr, const char *path_, const char *action, const char *mnemonic, const char *accel, const char *tip)
{
	lht_node_t *parent, *new_sub;
	char *name, *path = strdup(path_);

	if (strchr(path+1, '/') == NULL) {
		/* a new main menu */
		parent = NULL;
		name = path;
		while (*name == '/')
			name++;
	}
	else {
		/* submenu under an existing menu */
		name = strrchr(path, '/');
		*name = '\0';
		name++;
		parent = hid_cfg_get_menu(hr, path);
	}

	new_sub = hid_cfg_create_hash_node(parent, name, "m", mnemonic, "a", accel, "tip", tip, ((action != NULL) ? "action": NULL), action, NULL);

	if (action == NULL)
		lht_dom_hash_put(new_sub, lht_dom_node_alloc(LHT_LIST, "submenu"));

	free(path);
	return new_sub;
}

static int hid_cfg_load_error(lht_doc_t *doc, const char *filename, lht_err_t err)
{
	const char *fn;
	int line, col;
	lht_dom_loc_active(doc, &fn, &line, &col);
	Message("Resource error: %s (%s:%d.%d)*\n", lht_err_str(err), filename, line+1, col+1);
	return 1;
}

lht_doc_t *hid_cfg_load_lht(const char *filename)
{
	FILE *f;
	lht_doc_t *doc;
	int error = 0;

	f = fopen(filename, "r");
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

lht_doc_t *hid_cfg_load_str(const char *text)
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

const char *hid_cfg_text_value(lht_doc_t *doc, const char *path)
{
	lht_node_t *n = lht_tree_path(doc, "/", path, 1, NULL);
	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		hid_cfg_error(n, "Error: node %s should be a text node\n", path);
		return NULL;
	}
	return n->data.text.value;
}

hid_cfg_t *hid_cfg_load(const char *fn, int exact_fn, const char *embedded_fallback)
{
	lht_doc_t *doc;
	hid_cfg_t *hr;

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
				doc = hid_cfg_load_lht(*p);
				if (doc != NULL)
					Message("Loaded menu file '%s'\n", *p);
			}
			free(*p);
		}
		free(paths);
	}
	else
		doc = hid_cfg_load_lht(fn);

	if (doc == NULL)
		doc = hid_cfg_load_str(embedded_fallback);
	if (doc == NULL)
		return NULL;

	hr = calloc(sizeof(hid_cfg_t), 1); /* make sure the cache is cleared */
	hr->doc = doc;

	return hr;
}

int hid_cfg_action(const lht_node_t *node)
{
	if (node == NULL)
		return -1;
	switch(node->type) {
		case LHT_TEXT:
			return hid_parse_actions(node->data.text.value);
		case LHT_LIST:
			for(node = node->data.list.first; node != NULL; node = node->next) {
				if (node->type == LHT_TEXT) {
					if (hid_parse_actions(node->data.text.value) != 0)
						return -1;
				}
				else
					return -1;
			}
			return 0;
	}
	return -1;
}

/************* "parsing" **************/

lht_node_t *hid_cfg_get_menu(hid_cfg_t *hr, const char *menu_path)
{
	lht_err_t err;
	lht_node_t *curr;
	int level = 0, len = strlen(menu_path);
	char *next_seg, *path;

 path = malloc(len+4); /* need a few bytes after the end for the ':' */
 strcpy(path, menu_path);

	next_seg = path;
	curr = hr->doc->root;

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
		
/*		printf("step: %p '%s'\n", curr, next_seg);*/
		curr = lht_tree_path_(hr->doc, curr, next_seg, 1, 0, &err);
/*		printf("      %p\n", curr);*/
		*end = save;
		next_seg = next;
		if ((curr == NULL) || (next_seg == NULL))
			break;
		next_seg++;
		level++;
	}

	free(path);
	return curr;
}

lht_node_t *hid_cfg_menu_field(const lht_node_t *submenu, hid_cfg_menufield_t field, const char **field_name)
{
	lht_node_t *n;
	lht_err_t err;
	const char *fieldstr = NULL;

	switch(field) {
		case MF_ACCELERATOR:  fieldstr = "a"; break;
		case MF_MNEMONIC:     fieldstr = "m"; break;
		case MF_SUBMENU:      fieldstr = "submenu"; break;
		case MF_CHECKED:      fieldstr = "checked"; break;
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

const char *hid_cfg_menu_field_str(const lht_node_t *submenu, hid_cfg_menufield_t field)
{
	const char *fldname;
	lht_node_t *n = hid_cfg_menu_field(submenu, field, &fldname);

	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		hid_cfg_error(submenu, "Error: field %s should be a text node\n", fldname);
		return NULL;
	}
	return n->data.text.value;
}

int hid_cfg_has_submenus(const lht_node_t *submenu)
{
	const char *fldname;
	lht_node_t *n = hid_cfg_menu_field(submenu, MF_SUBMENU, &fldname);
	if (n == NULL)
		return 0;
	if (n->type != LHT_LIST) {
		hid_cfg_error(submenu, "Error: field %s should be a list (of submenus)\n", fldname);
		return 0;
	}
	return 1;
}


lht_node_t *hid_cfg_create_hash_node(lht_node_t *parent, const char *name, ...)
{
	lht_node_t *n;
	va_list ap;

	if ((parent != NULL) && (parent->type != LHT_LIST))
		return NULL;

	n = lht_dom_node_alloc(LHT_HASH, name);
	if (parent != NULL)
		lht_dom_list_append(parent, n);

	va_start(ap, name);
	for(;;) {
		char *cname, *cval;
		lht_node_t *t;

		cname = va_arg(ap, char *);
		if (cname == NULL)
			break;
		cval = va_arg(ap, char *);

		t = lht_dom_node_alloc(LHT_TEXT, cname);
		t->data.text.value = strdup(cval);
		lht_dom_hash_put(n, t);
	}
	va_end(ap);

	return n;
}

lht_node_t *hid_cfg_get_submenu(const lht_node_t *parent, const char *path)
{
	return lht_tree_path_(parent->doc, parent, path, 1, 0, NULL);
}
