#ifndef PCB_HID_FLAGS_H
#define PCB_HID_FLAGS_H

/* Create a set of resources representing a single menu item
   If action is NULL, it's a drop-down item that has submenus.
   The callback is called after the new lihata node is created.
   NOTE: unlike other cookies, this cookie is strdup()'d. 
   */
typedef int (*pcb_create_menu_widget_t)(void *ctx, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item);
int pcb_hid_cfg_create_menu(pcb_hid_cfg_t *hr, const char *path, const pcb_menu_prop_t *props, pcb_create_menu_widget_t cb, void *cb_ctx);

/* Looks up an integer (usually boolean) value by conf path or by running
   an action (if name has a parenthesis). When an action is run, it has 0
   or 1 argument only and the return value of the action is returned.
   On error, returns -1. */
int pcb_hid_get_flag(const char *name);

/* Return non-zero if submenu has further submenus; generate pcb_message(PCB_MSG_ERROR, ) if
   there is a submenu field with the wrong lihata type */
int pcb_hid_cfg_has_submenus(const lht_node_t *submenu);

/* Fields are retrieved using this enum so that HIDs don't need to hardwire
   lihata node names */
typedef enum {
	PCB_MF_ACCELERATOR,
	PCB_MF_SUBMENU,
	PCB_MF_CHECKED,
	PCB_MF_UPDATE_ON,
	PCB_MF_SENSITIVE,
	PCB_MF_TIP,
	PCB_MF_ACTIVE,
	PCB_MF_ACTION,
	PCB_MF_FOREGROUND,
	PCB_MF_BACKGROUND,
	PCB_MF_FONT
} pcb_hid_cfg_menufield_t;

/* Return a field of a submenu and optionally fill in field_name with the
   field name expected in the lihata document (useful for error messages) */
lht_node_t *pcb_hid_cfg_menu_field(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field, const char **field_name);

/* Return a lihata node using a relative lihata path from parent - this is
   just a wrapper around lht_tree_path_ */
lht_node_t *pcb_hid_cfg_menu_field_path(const lht_node_t *parent, const char *path);

/* Return a text field of a submenu; return NULL and generate a pcb_message(PCB_MSG_ERROR, ) if
   the given field is not text */
const char *pcb_hid_cfg_menu_field_str(const lht_node_t *submenu, pcb_hid_cfg_menufield_t field);

/* Remove a path recursively; call gui_remove() on leaf paths until the subtree
   is consumed (should return 0 on success) */
int pcb_hid_cfg_remove_menu(pcb_hid_cfg_t *hr, const char *path, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx);
int pcb_hid_cfg_remove_menu_node(pcb_hid_cfg_t *hr, lht_node_t *root, int (*gui_remove)(void *ctx, lht_node_t *nd), void *ctx);

#endif
