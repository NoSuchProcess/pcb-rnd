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

/* Load resources (menus, key and mouse action bindings) from lihata files */

#ifndef PCB_HID_RESOURCE_H
#define PCB_HID_RESOURCE_H

#include <liblihata/dom.h>
#include <genht/htip.h>

typedef struct hid_res_s {
	lht_doc_t *doc;
	struct {
		lht_node_t *mouse;
		htip_t *mouse_mask;
	} cache;
} hid_res_t;

/* For compatibility; TODO: REMOVE */
#define M_Mod(n)  (1u<<(n+1))

#define M_Mod0(n)  (1u<<(n))
typedef enum {
	M_Shift   = M_Mod0(0),
	M_Ctrl    = M_Mod0(1),
	M_Alt     = M_Mod0(2),
	M_Multi   = M_Mod0(3),
	/* M_Mod(3) is M_Mod0(4) */
	/* M_Mod(4) is M_Mod0(5) */
	M_Release = M_Mod0(6), /* there might be a random number of modkeys, but hopefully not this many */

	MB_LEFT   = M_Mod0(7),
	MB_MIDDLE = M_Mod0(8),
	MB_RIGHT  = M_Mod0(9),
	MB_UP     = M_Mod0(10), /* scroll wheel */
	MB_DOWN   = M_Mod0(11)  /* scroll wheel */
} hid_res_mod_t;

#define MB_ANY (MB_LEFT | MB_MIDDLE | MB_RIGHT | MB_UP | MB_DOWN)
#define M_ANY  (M_Release-1)



/* Create a set of resources representing a single menu item */
lht_node_t *resource_create_menu(hid_res_t *hr, const char *name, const char *action, const char *mnemonic, const char *accel, const char *tip, int flags);

/* Search and load the menu res for hidname; if not found, and embedded_fallback
   is not NULL, parse that string instead. Returns NULL on error */
hid_res_t *hid_res_load(const char *hidname, const char *embedded_fallback);

lht_node_t *hid_res_get_menu(hid_res_t *hr, const char *menu_path);

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
	MF_ACTION
} hid_res_menufield_t;

/* Return a field of a submenu and optionally fill in field_name with the
   field name expected in the lihata document (useful for error messages) */
lht_node_t *hid_res_menu_field(const lht_node_t *submenu, hid_res_menufield_t field, const char **field_name);

/* Return a text field of a submenu; return NULL and generate a Message() if
   the given field is not text */
const char *hid_res_menu_field_str(const lht_node_t *submenu, hid_res_menufield_t field);

/* Return non-zero if submenu has further submenus; generate Message() if
   there is a submenu field with the wrong lihata type */
int hid_res_has_submenus(const lht_node_t *submenu);

void hid_res_mouse_action(hid_res_t *hr, hid_res_mod_t button_and_mask);

/* Run an action node. The node is either a list of text nodes or a text node */
void hid_res_action(const lht_node_t *node);

/* Report an error about a node */
#define hid_res_error(node, ...) \
do { \
	extern char hid_res_error_shared[]; \
	char *__end__; \
	__end__ = hid_res_error_shared + sprintf(hid_res_error_shared, "Error in lihata node %s:%d.%d:", node->file_name, node->line, node->col); \
	__end__ += sprintf(__end__, __VA_ARGS__); \
	Message(hid_res_error_shared); \
} while(0)

#endif
