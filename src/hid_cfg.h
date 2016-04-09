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

/* Helpers for loading and handling lihata HID config files */

#ifndef PCB_HID_CFG_H
#define PCB_HID_CFG_H

#include <liblihata/dom.h>

typedef struct hid_cfg_s {
	lht_doc_t *doc;
} hid_cfg_t;

/* Create a set of resources representing a single menu item
   If action is NULL, it's a drop-down item that has submenus. */
lht_node_t *hid_cfg_create_menu(hid_cfg_t *hr, const char *path, const char *action, const char *mnemonic, const char *accel, const char *tip, lht_node_t **out_parent, int *is_main);

/* Search and load the menu res for hidname; if not found, and embedded_fallback
   is not NULL, parse that string instead. Returns NULL on error */
hid_cfg_t *hid_cfg_load(const char *fn, int exact_fn, const char *embedded_fallback);

/* Generic, low level lihata loader */
lht_doc_t *hid_cfg_load_lht(const char *filename);

/* Generic, low level lihata text value fetch */
const char *hid_cfg_text_value(lht_doc_t *doc, const char *path);

lht_node_t *hid_cfg_get_menu(hid_cfg_t *hr, const char *menu_path);
#warning TODO: this is broken
lht_node_t *hid_cfg_get_submenu(const lht_node_t *parent, const char *path);


/* Fields are retrieved using this enum so that HIDs don't need to hardwire
   lihata node names */
typedef enum {
	MF_ACCELERATOR,
	MF_MNEMONIC,
	MF_SUBMENU,
	MF_CHECKED,
	MF_SENSITIVE,
	MF_TIP,
	MF_ACTIVE,
	MF_ACTION,
	MF_FOREGROUND,
	MF_BACKGROUND,
	MF_FONT
/*	MF_RADIO*/
} hid_cfg_menufield_t;

/* Return a field of a submenu and optionally fill in field_name with the
   field name expected in the lihata document (useful for error messages) */
lht_node_t *hid_cfg_menu_field(const lht_node_t *submenu, hid_cfg_menufield_t field, const char **field_name);

/* Return a text field of a submenu; return NULL and generate a Message() if
   the given field is not text */
const char *hid_cfg_menu_field_str(const lht_node_t *submenu, hid_cfg_menufield_t field);

/* Return non-zero if submenu has further submenus; generate Message() if
   there is a submenu field with the wrong lihata type */
int hid_cfg_has_submenus(const lht_node_t *submenu);

/* Run an action node. The node is either a list of text nodes or a text node;
   returns non-zero on error, the first action that fails in a chain breaks the chain */
int hid_cfg_action(const lht_node_t *node);

/* Create a new hash node under parent (optional) and create a flat subtree of
   text nodes from name,value varargs (NULL terminated). This is a shorthand
   for creating a menu item in a subtree list. */
lht_node_t *hid_cfg_create_hash_node(lht_node_t *parent, const char *name, ...);

/* Report an error about a node */
#define hid_cfg_error(node, ...) \
do { \
	extern char hid_cfg_error_shared[]; \
	char *__end__; \
	__end__ = hid_cfg_error_shared + sprintf(hid_cfg_error_shared, "Error in lihata node %s:%d.%d:", node->file_name, node->line, node->col); \
	__end__ += sprintf(__end__, __VA_ARGS__); \
	Message(hid_cfg_error_shared); \
} while(0)

#endif
