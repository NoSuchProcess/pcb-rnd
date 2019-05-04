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

/* Helpers for loading and handling lihata HID config files */

#ifndef PCB_HID_CFG_H
#define PCB_HID_CFG_H

#include <liblihata/dom.h>
#include <stdarg.h>
#include "global_typedefs.h"
#include "hid.h"

struct pcb_hid_cfg_s {
	lht_doc_t *doc;
};

/* Search and load the menu res for hidname; if not found, and embedded_fallback
   is not NULL, parse that string instead. Returns NULL on error */
pcb_hid_cfg_t *pcb_hid_cfg_load(pcb_hidlib_t *hidlib, const char *fn, int exact_fn, const char *embedded_fallback);

/* Generic, low level lihata loader */
lht_doc_t *pcb_hid_cfg_load_lht(pcb_hidlib_t *hidlib, const char *filename);
lht_doc_t *pcb_hid_cfg_load_str(const char *text);

/* Generic, low level lihata text value fetch */
const char *pcb_hid_cfg_text_value(lht_doc_t *doc, const char *path);

lht_node_t *pcb_hid_cfg_get_menu(pcb_hid_cfg_t *hr, const char *menu_path);
lht_node_t *pcb_hid_cfg_get_menu_at(pcb_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);

/* Create a new hash node under parent (optional) and create a flat subtree of
   text nodes from name,value varargs (NULL terminated). This is a shorthand
   for creating a menu item in a subtree list. If ins_after is not NULL and
   is under the same parent, the new menu is inserted after ins_after. */
lht_node_t *pcb_hid_cfg_create_hash_node(lht_node_t *parent, lht_node_t *ins_after, const char *name, ...);

/* Create a flat subtree of text nodes from name,value varargs (NULL
   terminated). This is a shorthand for creating a menu item in a
   subtree list. */
void pcb_hid_cfg_extend_hash_node(lht_node_t *node, ...);
void pcb_hid_cfg_extend_hash_nodev(lht_node_t *node, va_list ap);

/* Search a subtree in depth-first-search manner. Call cb on each node as
   descending. If cb returns non-zero, stop the search and return that value.
   Do all this recursively. */
int pcb_hid_cfg_dfs(lht_node_t *parent, int (*cb)(void *ctx, lht_node_t *n), void *ctx);

/* Report an error about a node */
void pcb_hid_cfg_error(const lht_node_t *node, const char *fmt, ...);


/* search all instances of an @anchor menu in the currently active GUI and
   call cb on the lihata node; path has 128 extra bytes available at the end */
void pcb_hid_cfg_map_anchor_menus(const char *name, void (*cb)(void *ctx, pcb_hid_cfg_t *cfg, lht_node_t *n, char *path), void *ctx);

/* remove all adjacent anchor menus with matching cookie below anode, the
   anchor node */
int pcb_hid_cfg_del_anchor_menus(lht_node_t *anode, const char *cookie);


#endif
