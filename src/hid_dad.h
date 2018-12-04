/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
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

#ifndef PCB_HID_DAD_H
#define PCB_HID_DAD_H

#include <stddef.h>
#include <assert.h>
#include "compat_misc.h"
#include "hid_attrib.h"
#include "pcb-printf.h"


/*** Helpers for building dynamic attribute dialogs (DAD) ***/
#define PCB_DAD_DECL(table) \
	pcb_hid_attribute_t *table = NULL; \
	pcb_hid_attr_val_t *table ## _result = NULL; \
	int table ## _len = 0; \
	int table ## _alloced = 0; \
	void *table ## _hid_ctx = NULL; \
	pcb_dad_retovr_t table ## _ret_override = {0, 0, 0};

#define PCB_DAD_DECL_NOINIT(table) \
	pcb_hid_attribute_t *table; \
	pcb_hid_attr_val_t *table ## _result; \
	int table ## _len; \
	int table ## _alloced; \
	void *table ## _hid_ctx; \
	pcb_dad_retovr_t table ## _ret_override;

/* Free all resources allocated by DAD macros for table */
#define PCB_DAD_FREE(table) \
do { \
	int __n__; \
	if ((table ## _hid_ctx != NULL) && (!table ## _ret_override.already_freed)) \
		pcb_gui->attr_dlg_free(table ## _hid_ctx); \
	for(__n__ = 0; __n__ < table ## _len; __n__++) { \
		PCB_DAD_FREE_FIELD(table, __n__); \
	} \
	free(table); \
	free(table ## _result); \
	table = NULL; \
	table ## _result = NULL; \
	table ## _hid_ctx = NULL; \
	table ## _len = 0; \
	table ## _alloced = 0; \
} while(0)

#define PCB_DAD_NEW(table, title, descr, caller_data, modal, ev_cb) \
do { \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	table ## _hid_ctx = pcb_gui->attr_dlg_new(table, table ## _len, table ## _result, title, descr, caller_data, modal, ev_cb); \
} while(0)

#define PCB_DAD_RUN(table) pcb_hid_dad_run(table ## _hid_ctx, &table ## _ret_override)

/* failed is non-zero on cancel */
#define PCB_DAD_AUTORUN(table, title, descr, caller_data, failed) \
do { \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	table ## _ret_override.valid = 0; \
	table ## _ret_override.already_freed = 0; \
	failed = pcb_attribute_dialog(table, table ## _len, table ## _result, title, descr, caller_data); \
	if (table ## _ret_override.valid) \
		failed = table ## _ret_override.value; \
} while(0)

/* Return the index of the item currenty being edited */
#define PCB_DAD_CURRENT(table) (table ## _len - 1)

/* Begin a new box or table */
#define PCB_DAD_BEGIN(table, item_type) \
	PCB_DAD_ALLOC(table, item_type);

#define PCB_DAD_BEGIN_TABLE(table, cols) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_TABLE); \
	PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_table_cols, cols); \
} while(0)

#define PCB_DAD_BEGIN_TABBED(table, tabs) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_TABBED); \
	PCB_DAD_SET_ATTR_FIELD(table, enumerations, tabs); \
} while(0)

#define PCB_DAD_BEGIN_HBOX(table)      PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_HBOX)
#define PCB_DAD_BEGIN_VBOX(table)      PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_VBOX)
#define PCB_DAD_END(table)             PCB_DAD_BEGIN(table, PCB_HATT_END)
#define PCB_DAD_COMPFLAG(table, val)   PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_flags, val | (table[table ## _len-1].pcb_hatt_flags & PCB_HATF_TREE_COL))

#define PCB_DAD_LABEL(table, text) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_LABEL); \
	PCB_DAD_SET_ATTR_FIELD(table, name, pcb_strdup(text)); \
} while(0)

/* Add label usign printf formatting syntax: PCB_DAD_LABELF(tbl, ("a=%d", 12)); */
#define PCB_DAD_LABELF(table, printf_args) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_LABEL); \
	PCB_DAD_SET_ATTR_FIELD(table, name, pcb_strdup_printf printf_args); \
} while(0)

#define PCB_DAD_ENUM(table, choices) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_ENUM); \
	PCB_DAD_SET_ATTR_FIELD(table, enumerations, choices); \
} while(0)

#define PCB_DAD_BOOL(table, label) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_BOOL); \
	PCB_DAD_SET_ATTR_FIELD(table, name, pcb_strdup(label)); \
} while(0)

#define PCB_DAD_INTEGER(table, label) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_INTEGER); \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_REAL(table, label) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_REAL); \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_COORD(table, label) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_COORD); \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_STRING(table) \
	PCB_DAD_ALLOC(table, PCB_HATT_STRING); \

#define PCB_DAD_BUTTON(table, text) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_BUTTON); \
	table[table ## _len - 1].default_val.str_value = text; \
} while(0)

#define PCB_DAD_BUTTON_CLOSE(table, text, retval) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_BUTTON); \
	table[table ## _len - 1].default_val.str_value = text; \
	table[table ## _len - 1].default_val.int_value = retval; \
	table[table ## _len - 1].enumerations = (const char **)(&table ## _ret_override); \
	PCB_DAD_CHANGE_CB(table, pcb_hid_dad_close_cb); \
} while(0)

#define PCB_DAD_BUTTON_CLOSES(table, buttons) \
do { \
	pcb_hid_dad_buttons_t *__n__; \
	PCB_DAD_BEGIN_HBOX(table); \
	PCB_DAD_COMPFLAG(table, PCB_HATF_EXPFILL); \
	PCB_DAD_BEGIN_HBOX(table); \
	PCB_DAD_COMPFLAG(table, PCB_HATF_EXPFILL); \
	PCB_DAD_END(table); \
	for(__n__ = buttons; __n__->label != NULL; __n__++) { \
		PCB_DAD_BUTTON_CLOSE(table, __n__->label, __n__->retval); \
	} \
	PCB_DAD_END(table); \
} while(0)


#define PCB_DAD_PROGRESS(table) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_PROGRESS); \
} while(0)

#define PCB_DAD_PREVIEW(table, expose_cb, mouse_cb, free_cb, initial_view_box, min_sizex_px_,  min_sizey_px_, user_ctx_) \
do { \
	pcb_hid_preview_t *prv = calloc(sizeof(pcb_hid_preview_t), 1); \
	prv->user_ctx = user_ctx_; \
	prv->user_expose_cb = expose_cb; \
	prv->user_mouse_cb = mouse_cb; \
	prv->user_free_cb = free_cb; \
	prv->min_sizex_px = min_sizex_px_; \
	prv->min_sizey_px = min_sizey_px_; \
	if ((initial_view_box) != NULL) { \
		prv->initial_view.X1 = ((pcb_box_t *)(initial_view_box))->X1; \
		prv->initial_view.Y1 = ((pcb_box_t *)(initial_view_box))->Y1; \
		prv->initial_view.X2 = ((pcb_box_t *)(initial_view_box))->X2; \
		prv->initial_view.Y2 = ((pcb_box_t *)(initial_view_box))->Y2; \
		prv->initial_view_valid = 1; \
	} \
	PCB_DAD_ALLOC(table, PCB_HATT_PREVIEW); \
	prv->attrib = &table[table ## _len-1]; \
	PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)prv); \
} while(0)

#define pcb_dad_preview_zoomto(attr, view) \
do { \
	pcb_hid_preview_t *prv = (pcb_hid_preview_t *)((attr)->enumerations); \
	if (prv->hid_zoomto_cb != NULL) \
		prv->hid_zoomto_cb((attr), prv->hid_ctx, view); \
} while(0)


#define PCB_DAD_PICTURE(table, xpm) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_PICTURE); \
	table[table ## _len - 1].enumerations = xpm; \
} while(0)

#define PCB_DAD_PICBUTTON(table, xpm) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_PICBUTTON); \
	table[table ## _len - 1].enumerations = xpm; \
} while(0)

#define PCB_DAD_BEGIN_HPANE(table) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_HPANE); \
	table[table ## _len - 1].default_val.real_value = 0.5; \
} while(0)

#define PCB_DAD_BEGIN_VPANE(table) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_VPANE); \
	table[table ## _len - 1].default_val.real_value = 0.5; \
} while(0)

#define PCB_DAD_TREE(table, cols, first_col_is_tree, opt_header) \
do { \
	pcb_hid_tree_t *tree = calloc(sizeof(pcb_hid_tree_t), 1); \
	htsp_init(&tree->paths, strhash, strkeyeq); \
	PCB_DAD_ALLOC(table, PCB_HATT_TREE); \
	tree->attrib = &table[table ## _len-1]; \
	tree->hdr = opt_header; \
	PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_table_cols, cols); \
	PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_flags, first_col_is_tree ? PCB_HATF_TREE_COL : 0); \
	PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)tree); \
} while(0)

#define PCB_DAD_TREE_APPEND(table, row_after, cells) \
	pcb_dad_tree_append(&table[table ## _len-1], row_after, cells)

#define PCB_DAD_TREE_APPEND_UNDER(table, parent_row, cells) \
	pcb_dad_tree_append_under(&table[table ## _len-1], parent_row, cells)

#define PCB_DAD_TREE_INSERT(table, row_before, cells) \
	pcb_dad_tree_insert(&table[table ## _len-1], row_before, cells)

/* set the named tree user callback to func_or_data; name is automatically
   appended with user_, any field prefixed with user_ in pcb_hid_tree_t
   can be set */
#define PCB_DAD_TREE_SET_CB(table, name, func_or_data) \
do { \
	pcb_hid_tree_t *__tree__ = (pcb_hid_tree_t *)table[table ## _len-1].enumerations; \
	__tree__->user_ ## name = func_or_data; \
} while(0)

#define PCB_DAD_DUP_ATTR(table, attr) \
do { \
	PCB_DAD_ALLOC(table, 0); \
	memcpy(&table[table ## _len-1], (attr), sizeof(pcb_hid_attribute_t)); \
} while(0)


/* Set properties of the current item */
#define PCB_DAD_MINVAL(table, val)       PCB_DAD_SET_ATTR_FIELD(table, min_val, val)
#define PCB_DAD_MAXVAL(table, val)       PCB_DAD_SET_ATTR_FIELD(table, max_val, val)
#define PCB_DAD_DEFAULT(table, val)      PCB_DAD_SET_ATTR_FIELD_VAL(table, default_val, val)
#define PCB_DAD_MINMAX(table, min, max)  (PCB_DAD_SET_ATTR_FIELD(table, min_val, min),PCB_DAD_SET_ATTR_FIELD(table, max_val, max))
#define PCB_DAD_CHANGE_CB(table, cb)     PCB_DAD_SET_ATTR_FIELD(table, change_cb, cb)
#define PCB_DAD_HELP(table, val)         PCB_DAD_SET_ATTR_FIELD(table, help_text, val)

/* safe way to call gui->attr_dlg_set_value() - resets the unused fields */
#define PCB_DAD_SET_VALUE(hid_ctx, wid, field, val) \
	do { \
		pcb_hid_attr_val_t __val__; \
		memset(&__val__, 0, sizeof(__val__)); \
		__val__.field = val; \
		pcb_gui->attr_dlg_set_value(hid_ctx, wid, &__val__); \
	} while(0)

/*** DAD internals (do not use directly) ***/
/* Allocate a new item at the end of the attribute table; updates stored
   attribute pointers of existing items (e.g. previews, trees) as the base
   address of the table may have changed. */
#define PCB_DAD_ALLOC(table, item_type) \
	do { \
		if (table ## _len >= table ## _alloced) { \
			pcb_hid_preview_t *__prv__; \
			pcb_hid_tree_t *__tree__; \
			int __n__; \
			table ## _alloced += 16; \
			table = realloc(table, sizeof(table[0]) * table ## _alloced); \
			for(__n__ = 0; __n__ < table ## _len; __n__++) { \
				switch(table[__n__].type) { \
					case PCB_HATT_PREVIEW: \
						__prv__ = (pcb_hid_preview_t *)table[__n__].enumerations; \
						__prv__->attrib = &table[__n__]; \
						break; \
					case PCB_HATT_TREE: \
						__tree__ = (pcb_hid_tree_t *)table[__n__].enumerations; \
						__tree__->attrib = &table[__n__]; \
						break; \
					default: break; \
				} \
			} \
		} \
		memset(&table[table ## _len], 0, sizeof(table[0])); \
		table[table ## _len].type = item_type; \
		table ## _len++; \
	} while(0)

#define PCB_DAD_SET_ATTR_FIELD(table, field, value) \
	table[table ## _len - 1].field = (value)

#define PCB_DAD_SET_ATTR_FIELD_VAL(table, field, val) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			assert(0); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_UNIT: \
		case PCB_HATT_BEGIN_TABBED: \
			table[table ## _len - 1].field.int_value = (int)val; \
			break; \
		case PCB_HATT_COORD: \
			table[table ## _len - 1].field.coord_value = (pcb_coord_t)val; \
			break; \
		case PCB_HATT_REAL: \
		case PCB_HATT_PROGRESS: \
		case PCB_HATT_BEGIN_HPANE: \
		case PCB_HATT_BEGIN_VPANE: \
			table[table ## _len - 1].field.real_value = (double)val; \
			break; \
		case PCB_HATT_STRING: \
		case PCB_HATT_PATH: \
		case PCB_HATT_BUTTON: \
		case PCB_HATT_TREE: \
			table[table ## _len - 1].field.str_value = (char *)val; \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_END: \
		case PCB_HATT_PREVIEW: \
		case PCB_HATT_PICTURE: \
		case PCB_HATT_PICBUTTON: \
			assert(0); \
	} \
} while(0)

#define PCB_DAD_FREE_FIELD(table, field) \
do { \
	switch(table[field].type) { \
		case PCB_HATT_LABEL: \
			free((char *)table[field].name); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_COORD: \
		case PCB_HATT_UNIT: \
		case PCB_HATT_REAL: \
		case PCB_HATT_PROGRESS: \
		case PCB_HATT_STRING: \
		case PCB_HATT_PATH: \
		case PCB_HATT_BUTTON: \
		case PCB_HATT_PICTURE: \
		case PCB_HATT_PICBUTTON: \
			break; \
		case PCB_HATT_TREE: \
			pcb_dad_tree_free(&table[field]); \
			break; \
		case PCB_HATT_PREVIEW: \
			{ \
				pcb_hid_preview_t *prv = (pcb_hid_preview_t *)table[field].enumerations; \
				if (prv->user_free_cb != NULL) \
					prv->user_free_cb(&table[field], prv->user_ctx, prv->hid_ctx); \
				if (prv->hid_free_cb != NULL) \
					prv->hid_free_cb(&table[field], prv->hid_ctx); \
			} \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_HPANE: \
		case PCB_HATT_BEGIN_VPANE: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_BEGIN_TABBED: \
		case PCB_HATT_END: \
			break; \
	} \
} while(0)

#define PCB_DAD_ALLOC_RESULT(table) \
do { \
	free(table ## _result); \
	table ## _result = calloc(PCB_DAD_CURRENT(table)+1, sizeof(pcb_hid_attr_val_t)); \
} while(0)

/* Internal: free all rows and caches and the tree itself */
void pcb_dad_tree_free(pcb_hid_attribute_t *attr);

/* internal: retval override for the auto-close buttons */
typedef struct {
	char valid;
	char already_freed;
	int value;
} pcb_dad_retovr_t;

typedef struct {
	const char *label;
	int retval;
} pcb_hid_dad_buttons_t;

void pcb_hid_dad_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
int pcb_hid_dad_run(void *hid_ctx, pcb_dad_retovr_t *retovr);

#endif
