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

char hid_cfg_error_shared[1024];

lht_node_t *resource_create_menu(hid_cfg_t *hr, const char *name, const char *action, const char *mnemonic, const char *accel, const char *tip, int flags)
{
	/* TODO */
	abort();
}

lht_doc_t *hid_cfg_load_lht(const char *filename)
{
	FILE *f;
	lht_doc_t *doc;
	int error = 0;

	f = fopen(filename, "r");
	doc = lht_dom_init();
	lht_dom_loc_newfile(doc, filename);

	while(!(feof(f))) {
		lht_err_t err;
		int c = fgetc(f);
		err = lht_dom_parser_char(doc, c);
		if (err != LHTE_SUCCESS) {
			if (err != LHTE_STOP) {
				const char *fn;
				int line, col;
				lht_dom_loc_active(doc, &fn, &line, &col);
				Message("Resource error: %s (%s:%d.%d)*\n", lht_err_str(err), filename, line+1, col+1);
				error = 1;
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

hid_cfg_t *hid_cfg_load(const char *hidname, const char *embedded_fallback)
{
	lht_doc_t *doc;
	hid_cfg_t *hr;

//static const char *pcbmenu_paths_in[] = { "gpcb-menu.res", "gpcb-menu.res", PCBSHAREDIR "/gpcb-menu.res", NULL };
	char *hidfn = strdup(hidname); /* TODO: search for the file */

	doc = hid_cfg_load_lht(hidfn);
	if (doc == NULL)
		return NULL;

	hr = calloc(sizeof(hid_cfg_t), 1); /* make sure the cache is cleared */
	hr->doc = doc;

	return hr;
}

void hid_cfg_action(const lht_node_t *node)
{
	if (node == NULL)
		return;
	switch(node->type) {
		case LHT_TEXT:
			hid_parse_actions(node->data.text.value);
			break;
		case LHT_LIST:
			for(node = node->data.list.first; node != NULL; node = node->next)
				if (node->type == LHT_TEXT)
					hid_parse_actions(node->data.text.value);
			break;
	}
}

/************* "parsing" **************/

lht_node_t *hid_cfg_get_menu(hid_cfg_t *hr, const char *menu_path)
{
	lht_err_t err;
	return lht_tree_path(hr->doc, "/", menu_path, 1, &err);
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
	}
	if (field_name != NULL)
		*field_name = fieldstr;

	if (fieldstr == NULL)
		return NULL;

	return lht_tree_path_(submenu->doc, (lht_node_t *)submenu, fieldstr, 1, 0, &err);
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
		return NULL;
	}
	return 1;
}

