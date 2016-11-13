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

/* Helpers for loading and handling lihata HID config files */

#ifndef PCB_HID_CFG_H
#define PCB_HID_CFG_H

#include <liblihata/dom.h>
#include <stdarg.h>

typedef struct pcb_hid_cfg_s {
	lht_doc_t *doc;
} pcb_hid_cfg_t;

/* Create a set of resources representing a single menu item
   If action is NULL, it's a drop-down item that has submenus.
   The callback is called after the new lihata node is created.
   NOTE: unlike other cookies, this cookie is strdup()'d. 
   */
typedef int (*create_menu_widget_t)(void *ctx, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *menu_item);
int hid_cfg_create_menu(pcb_hid_cfg_t *hr, const char *path, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie, create_menu_widget_t cb, void *cb_ctx);

/* Remove a path recursively; call gui_remove() on leaf paths until the subtree
   is consumed (should return 0 on success) */
int hid_cfg_remove_menu(pcb_hid_cfg_t *hr, const char *path, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx);

/* Search and load the menu res for hidname; if not found, and embedded_fallback
   is not NULL, parse that string instead. Returns NULL on error */
pcb_hid_cfg_t *hid_cfg_load(const char *fn, int exact_fn, const char *embedded_fallback);

/* Generic, low level lihata loader */
lht_doc_t *hid_cfg_load_lht(const char *filename);
lht_doc_t *hid_cfg_load_str(const char *text);

/* Generic, low level lihata text value fetch */
const char *pcb_hid_cfg_text_value(lht_doc_t *doc, const char *path);

lht_node_t *hid_cfg_get_menu(pcb_hid_cfg_t *hr, const char *menu_path);
lht_node_t *hid_cfg_get_menu_at(pcb_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);


/* Fields are retrieved using this enum so that HIDs don't need to hardwire
   lihata node names */
typedef enum {
	MF_ACCELERATOR,
	MF_MNEMONIC,
	MF_SUBMENU,
	MF_CHECKED,
	MF_UPDATE_ON,
	MF_SENSITIVE,
	MF_TIP,
	MF_ACTIVE,
	MF_ACTION,
	MF_FOREGROUND,
	MF_BACKGROUND,
	MF_FONT
/*	MF_RADIO*/
} pcb_hid_cfg_menufield_t;

/* Return a field of a submenu and optionally fill in field_name with the
   field name expected in the lihata document (useful for error messages) */
lht_node_t *hid_cfg_menu_field(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field, const char **field_name);

/* Return a lihata node using a relative lihata path from parent - this is
   just a wrapper around lht_tree_path_ */
lht_node_t *hid_cfg_menu_field_path(const lht_node_t *parent, const char *path);

/* Return a text field of a submenu; return NULL and generate a pcb_message(PCB_MSG_DEFAULT, ) if
   the given field is not text */
const char *hid_cfg_menu_field_str(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field);

/* Return non-zero if submenu has further submenus; generate pcb_message(PCB_MSG_DEFAULT, ) if
   there is a submenu field with the wrong lihata type */
int hid_cfg_has_submenus(const lht_node_t *submenu);

/* Create a new hash node under parent (optional) and create a flat subtree of
   text nodes from name,value varargs (NULL terminated). This is a shorthand
   for creating a menu item in a subtree list. */
lht_node_t *hid_cfg_create_hash_node(lht_node_t *parent, const char *name, ...);

/* Create a flat subtree of text nodes from name,value varargs (NULL
   terminated). This is a shorthand for creating a menu item in a
   subtree list. */
void hid_cfg_extend_hash_node(lht_node_t *node, ...);
void hid_cfg_extend_hash_nodev(lht_node_t *node, va_list ap);

/* Search a subtree in depth-first-search manner. Call cb on each node as
   descending. If cb returns non-zero, stop the search and return that value.
   Do all this recursively. */
int hid_cfg_dfs(lht_node_t *parent, int (*cb)(void *ctx, lht_node_t *n), void *ctx);

/* Report an error about a node */
void hid_cfg_error(const lht_node_t *node, const char *fmt, ...);
#endif
