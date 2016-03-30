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
#include <genht/hash.h>

#include "global.h"
#include "hid.h"
#include "hid_resource.h"
#include "hid_actions.h"
#include "hid_resource.h"
#include "error.h"

static void hid_res_flush(hid_res_t *hr);

char hid_res_error_shared[1024];

/* split value into a list of '-' separated words; examine each word
   and set the bitmask of modifiers */
static hid_res_mod_t parse_mods(const char *value)
{
	hid_res_mod_t m = 0;
	int press = 0;

	for(;;) {
		if (strncmp(value, "shift", 5) == 0)        m |= M_Shift;
		else if (strncmp(value, "ctrl", 4) == 0)    m |= M_Ctrl;
		else if (strncmp(value, "alt", 3) == 0)     m |= M_Alt;
		else if (strncmp(value, "release", 7) == 0) m |= M_Release;
		else if (strncmp(value, "press", 5) == 0)   press = 1;
		else
			Message("Unkown modifier: %s\n", value);
		/* skip to next word */
		value = strchr(value, '-');
		if (value == NULL)
			break;
		value++;
	}

	if (press && (m & M_Release))
		Message("Bogus modifier: both press and release\n");

	return m;
}

static hid_res_mod_t button_name2mask(const char *name)
{
	/* All mouse-related resources must be named.  The name is the
	   mouse button number.  */
	if (!name)
		return 0;
	else if (strcasecmp(name, "left") == 0)   return MB_LEFT;
	else if (strcasecmp(name, "middle") == 0) return MB_MIDDLE;
	else if (strcasecmp(name, "right") == 0)  return MB_RIGHT;
	else if (strcasecmp(name, "up") == 0)     return MB_UP;
	else if (strcasecmp(name, "down") == 0)   return MB_DOWN;
	else {
		Message("Error: unknown mouse button: %s\n", name);
		return 0;
	}
}


static int keyeq_int(htip_key_t a, htip_key_t b)   { return a == b; }
static unsigned int keyhash_int(htip_key_t a)      { return murmurhash32(a & 0xFFFF); }

static void build_mouse_cache(hid_res_t *hr)
{
	lht_node_t *btn, *m;

	if (hr->cache.mouse == NULL)
		hr->cache.mouse = hid_res_get_menu(hr, "/mouse");

	if (hr->cache.mouse == NULL) {
		Message("Warning: no /mouse section in the resource file - mouse is disabled\n");
		return;
	}

	if (hr->cache.mouse->type != LHT_LIST) {
		hid_res_error(hr->cache.mouse, "Warning: should be a list - mouse is disabled\n");
		return;
	}

	if (hr->cache.mouse_mask == NULL)
		hr->cache.mouse_mask = htip_alloc(keyhash_int, keyeq_int);
	else
		htip_clear(hr->cache.mouse_mask);

	for(btn = hr->cache.mouse->data.list.first; btn != NULL; btn = btn->next) {
		hid_res_mod_t btn_mask = button_name2mask(btn->name);
		if (btn_mask == 0) {
			hid_res_error(btn, "unknown mouse button");
			continue;
		}
		if (btn->type != LHT_LIST) {
			hid_res_error(btn, "needs to be a list");
			continue;
		}
		for(m = btn->data.list.first; m != NULL; m = m->next) {
			hid_res_mod_t mod_mask = parse_mods(m->name);
			htip_set(hr->cache.mouse_mask, btn_mask|mod_mask, m);
		}
	}
}

static lht_node_t *find_best_action(hid_res_t *hr, hid_res_mod_t button_and_mask)
{
	lht_node_t *n;

	if (hr->cache.mouse_mask == NULL)
		return NULL;

	/* look for exact mod match */
	n = htip_get(hr->cache.mouse_mask, button_and_mask);
	if (n != NULL)
		return n;

	if (button_and_mask & M_Release) {
		/* look for plain release for the given button */
		n = htip_get(hr->cache.mouse_mask, (button_and_mask & M_ANY) | M_Release);
		if (n != NULL)
			return n;
	}

	return NULL;
}

void hid_res_mouse_action(hid_res_t *hr, hid_res_mod_t button_and_mask)
{
	hid_res_action(find_best_action(hr, button_and_mask));
}

lht_node_t *resource_create_menu(hid_res_t *hr, const char *name, const char *action, const char *mnemonic, const char *accel, const char *tip, int flags)
{
	/* TODO */
	abort();
}

lht_doc_t *hid_res_load_lht(const char *filename)
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

const char *hid_res_text_value(lht_doc_t *doc, const char *path)
{
	lht_node_t *n = lht_tree_path(doc, "/", path, 1, NULL);
	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		hid_res_error(n, "Error: node %s should be a text node\n", path);
		return NULL;
	}
	return n->data.text.value;
}

hid_res_t *hid_res_load(const char *hidname, const char *embedded_fallback)
{
	lht_doc_t *doc;
	hid_res_t *hr;

//static const char *pcbmenu_paths_in[] = { "gpcb-menu.res", "gpcb-menu.res", PCBSHAREDIR "/gpcb-menu.res", NULL };
	char *hidfn = strdup(hidname); /* TODO: search for the file */

	doc = hid_res_load_lht(hidfn);
	if (doc == NULL)
		return NULL;

	hr = calloc(sizeof(hid_res_t), 1); /* make sure the cache is cleared */
	hr->doc = doc;
	hid_res_flush(hr);

	return hr;
}

void hid_res_action(const lht_node_t *node)
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

lht_node_t *hid_res_get_menu(hid_res_t *hr, const char *menu_path)
{
	lht_err_t err;
	return lht_tree_path(hr->doc, "/", menu_path, 1, &err);
}

lht_node_t *hid_res_menu_field(const lht_node_t *submenu, hid_res_menufield_t field, const char **field_name)
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

const char *hid_res_menu_field_str(const lht_node_t *submenu, hid_res_menufield_t field)
{
	const char *fldname;
	lht_node_t *n = hid_res_menu_field(submenu, field, &fldname);

	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		hid_res_error(submenu, "Error: field %s should be a text node\n", fldname);
		return NULL;
	}
	return n->data.text.value;
}

int hid_res_has_submenus(const lht_node_t *submenu)
{
	const char *fldname;
	lht_node_t *n = hid_res_menu_field(submenu, MF_SUBMENU, &fldname);
	if (n == NULL)
		return 0;
	if (n->type != LHT_LIST) {
		hid_res_error(submenu, "Error: field %s should be a list (of submenus)\n", fldname);
		return NULL;
	}
	return 1;
}

/************* cache **************/
/* reset the cache of a resource tree; should be called any time the tree
   changes */
static void hid_res_flush(hid_res_t *hr)
{
	build_mouse_cache(hr);
}
