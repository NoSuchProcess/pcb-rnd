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

#ifndef RND_HID_CFG_H
#define RND_HID_CFG_H

#include <liblihata/dom.h>
#include <stdarg.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/hid.h>

struct rnd_hid_cfg_s {
	lht_doc_t *doc;
};

/* Search and load the menu res for hidname; if not found, parse
   embedded_fallback instead (if it is NULL, use the application default
   instead; for explicit empty embedded: use ""). Returns NULL on error */
rnd_hid_cfg_t *rnd_hid_cfg_load(rnd_hidlib_t *hidlib, const char *fn, int exact_fn, const char *embedded_fallback);

/* Generic, low level lihata loader */
lht_doc_t *rnd_hid_cfg_load_lht(rnd_hidlib_t *hidlib, const char *filename);
lht_doc_t *rnd_hid_cfg_load_str(const char *text);

/* Generic, low level lihata text value fetch */
const char *rnd_hid_cfg_text_value(lht_doc_t *doc, const char *path);

lht_node_t *rnd_hid_cfg_get_menu(rnd_hid_cfg_t *hr, const char *menu_path);
lht_node_t *rnd_hid_cfg_get_menu_at(rnd_hid_cfg_t *hr, lht_node_t *at, const char *menu_path, lht_node_t *(*cb)(void *ctx, lht_node_t *node, const char *path, int rel_level), void *ctx);

/* Create a new hash node under parent (optional) and create a flat subtree of
   text nodes from name,value varargs (NULL terminated). This is a shorthand
   for creating a menu item in a subtree list. If ins_after is not NULL and
   is under the same parent, the new menu is inserted after ins_after. */
lht_node_t *rnd_hid_cfg_create_hash_node(lht_node_t *parent, lht_node_t *ins_after, const char *name, ...);

/* Create a flat subtree of text nodes from name,value varargs (NULL
   terminated). This is a shorthand for creating a menu item in a
   subtree list. */
void rnd_hid_cfg_extend_hash_node(lht_node_t *node, ...);
void rnd_hid_cfg_extend_hash_nodev(lht_node_t *node, va_list ap);

/* Search a subtree in depth-first-search manner. Call cb on each node as
   descending. If cb returns non-zero, stop the search and return that value.
   Do all this recursively. */
int rnd_hid_cfg_dfs(lht_node_t *parent, int (*cb)(void *ctx, lht_node_t *n), void *ctx);

/* Report an error about a node */
void rnd_hid_cfg_error(const lht_node_t *node, const char *fmt, ...);


/* search all instances of an @anchor menu in the currently active GUI and
   call cb on the lihata node; path has 128 extra bytes available at the end */
void rnd_hid_cfg_map_anchor_menus(const char *name, void (*cb)(void *ctx, rnd_hid_cfg_t *cfg, lht_node_t *n, char *path), void *ctx);

/* remove all adjacent anchor menus with matching cookie below anode, the
   anchor node */
int rnd_hid_cfg_del_anchor_menus(lht_node_t *anode, const char *cookie);


#endif
