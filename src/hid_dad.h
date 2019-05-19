/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2018,2019 Tibor 'Igor2' Palinkas
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
#include "global_typedefs.h"

#include "hid_dad_spin.h"

#include <genlist/gendlist.h>

/*** Helpers for building dynamic attribute dialogs (DAD) ***/
#define PCB_DAD_DECL(table) \
	pcb_hid_attribute_t *table = NULL; \
	pcb_hid_attr_val_t *table ## _result = NULL; \
	int table ## _append_lock = 0; \
	int table ## _len = 0; \
	int table ## _alloced = 0; \
	void *table ## _hid_ctx = NULL; \
	int table ## _defx = 0, table ## _defy = 0; \
	pcb_dad_retovr_t *table ## _ret_override;

#define PCB_DAD_DECL_NOINIT(table) \
	pcb_hid_attribute_t *table; \
	pcb_hid_attr_val_t *table ## _result; \
	int table ## _append_lock; \
	int table ## _len; \
	int table ## _alloced; \
	void *table ## _hid_ctx; \
	int table ## _defx, table ## _defy; \
	pcb_dad_retovr_t *table ## _ret_override;

/* Free all resources allocated by DAD macros for table */
#define PCB_DAD_FREE(table) \
do { \
	int __n__; \
	if ((table ## _hid_ctx != NULL) && (table ## _ret_override != NULL)) \
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
	table ## _append_lock = 0; \
	if ((table ## _ret_override != NULL) && (table ## _ret_override->dont_free == 0)) {\
		free(table ## _ret_override); \
		table ## _ret_override = NULL; \
	} \
} while(0)

#define PCB_DAD_NEW(id, table, title, caller_data, modal, ev_cb) \
do { \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	table ## _ret_override = calloc(sizeof(pcb_dad_retovr_t), 1); \
	table ## _append_lock = 1; \
	table ## _hid_ctx = pcb_gui->attr_dlg_new(id, table, table ## _len, table ## _result, title, caller_data, modal, ev_cb, table ## _defx, table ## _defy); \
} while(0)

/* Sets the default window size (that is only a hint) - NOTE: must be called
   before PCB_DAD_NEW() */
#define PCB_DAD_DEFSIZE(table, width, height) \
do { \
	table ## _defx = width; \
	table ## _defy = height; \
} while(0)

#define PCB_DAD_RUN(table) pcb_hid_dad_run(table ## _hid_ctx, table ## _ret_override)

/* failed is zero on success and -1 on error (e.g. cancel) or an arbitrary
   value set by ret_override (e.g. on close buttons) */
#define PCB_DAD_AUTORUN(id, table, title, caller_data, failed) \
do { \
	int __ok__; \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	table ## _ret_override = calloc(sizeof(pcb_dad_retovr_t), 1); \
	table ## _ret_override->dont_free++; \
	table ## _ret_override->valid = 0; \
	table ## _append_lock = 1; \
	__ok__ = pcb_attribute_dialog_(id,table, table ## _len, table ## _result, title, caller_data, (void **)&(table ## _ret_override), table ## _defx, table ## _defy, &table ## _hid_ctx); \
	failed = (__ok__ == 0) ? -1 : 0; \
	if (table ## _ret_override->valid) \
		failed = table ## _ret_override->value; \
	table ## _ret_override->dont_free--; \
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

#define PCB_DAD_UNIT(table) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_UNIT); \
} while(0)

#include "brave.h"

#define PCB_DAD_COORD(table, label) \
do { \
	if (pcb_brave & PCB_BRAVE_OLD_SPINBOX) { \
		PCB_DAD_ALLOC(table, PCB_HATT_COORD); \
	} \
	else {\
		PCB_DAD_SPIN_COORD(table); \
	} \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_STRING(table) \
	PCB_DAD_ALLOC(table, PCB_HATT_STRING); \

#define PCB_DAD_TEXT(table, user_ctx_) \
do { \
	pcb_hid_text_t *txt = calloc(sizeof(pcb_hid_text_t), 1); \
	txt->user_ctx = user_ctx_; \
	PCB_DAD_ALLOC(table, PCB_HATT_TEXT); \
	PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)txt); \
} while(0)

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

/* Draw a set of close buttons without trying to fill a while hbox row */
#define PCB_DAD_BUTTON_CLOSES_NAKED(table, buttons) \
do { \
	pcb_hid_dad_buttons_t *__n__; \
	PCB_DAD_BEGIN_HBOX(table); \
	PCB_DAD_COMPFLAG(table, PCB_HATF_EXPFILL); \
	PCB_DAD_END(table); \
	for(__n__ = buttons; __n__->label != NULL; __n__++) { \
		PCB_DAD_BUTTON_CLOSE(table, __n__->label, __n__->retval); \
	} \
} while(0)

/* Draw a set of close buttons, adding a new hbox that tries to fill a whole row */
#define PCB_DAD_BUTTON_CLOSES(table, buttons) \
do { \
	PCB_DAD_BEGIN_HBOX(table); \
	PCB_DAD_BUTTON_CLOSES_NAKED(table, buttons); \
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
		prv->hid_zoomto_cb((attr), prv->hid_wdata, view); \
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


#define PCB_DAD_COLOR(table) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_COLOR); \
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
	PCB_DAD_UPDATE_INTERNAL(table, table ## _len-1); \
} while(0)


/* Set properties of the current item */
#define PCB_DAD_MINVAL(table, val)       PCB_DAD_SET_ATTR_FIELD(table, min_val, val)
#define PCB_DAD_MAXVAL(table, val)       PCB_DAD_SET_ATTR_FIELD(table, max_val, val)
#define PCB_DAD_MINMAX(table, min, max)  (PCB_DAD_SET_ATTR_FIELD(table, min_val, min),PCB_DAD_SET_ATTR_FIELD(table, max_val, max))
#define PCB_DAD_CHANGE_CB(table, cb)     PCB_DAD_SET_ATTR_FIELD(table, change_cb, cb)
#define PCB_DAD_RIGHT_CB(table, cb)      PCB_DAD_SET_ATTR_FIELD(table, right_cb, cb)
#define PCB_DAD_ENTER_CB(table, cb)      PCB_DAD_SET_ATTR_FIELD(table, enter_cb, cb)
#define PCB_DAD_HELP(table, val)         PCB_DAD_SET_ATTR_FIELD(table, help_text, val)

#define PCB_DAD_DEFAULT_PTR(table, val) \
	do {\
		switch(table[table ## _len - 1].type) { \
			case PCB_HATT_BEGIN_COMPOUND: \
			case PCB_HATT_END: \
				{ \
					pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)table[table ## _len - 1].enumerations; \
					if ((cmp != NULL) && (cmp->set_val_ptr != NULL)) \
						cmp->set_val_ptr(&table[table ## _len - 1], (void *)(val)); \
					else \
						assert(0); \
				} \
				break; \
			default: \
				PCB_DAD_SET_ATTR_FIELD_PTR(table, default_val, val); \
		} \
	} while(0)

#define PCB_DAD_DEFAULT_NUM(table, val) \
	do {\
		switch(table[table ## _len - 1].type) { \
			case PCB_HATT_BEGIN_COMPOUND: \
			case PCB_HATT_END: \
				{ \
					pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)table[table ## _len - 1].enumerations; \
					if ((cmp != NULL) && (cmp->set_val_num != NULL)) \
						cmp->set_val_num(&table[table ## _len - 1], (long)(val), (double)(val), (pcb_coord_t)(val)); \
					else \
						assert(0); \
				} \
				break; \
			default: \
				PCB_DAD_SET_ATTR_FIELD_NUM(table, default_val, val); \
		} \
	} while(0);

/* safe way to call gui->attr_dlg_set_value() - resets the unused fields */
#define PCB_DAD_SET_VALUE(hid_ctx, wid, field, val) \
	do { \
		pcb_hid_attr_val_t __val__; \
		memset(&__val__, 0, sizeof(__val__)); \
		__val__.field = val; \
		pcb_gui->attr_dlg_set_value(hid_ctx, wid, &__val__); \
	} while(0)

/*** DAD internals (do not use directly) ***/

/* Update widget internals after a potential attr pointer change */
#define PCB_DAD_UPDATE_INTERNAL(table, widx) \
	do { \
		pcb_hid_preview_t *__prv__; \
		pcb_hid_tree_t *__tree__; \
		switch(table[(widx)].type) { \
			case PCB_HATT_PREVIEW: \
				__prv__ = (pcb_hid_preview_t *)table[(widx)].enumerations; \
				__prv__->attrib = &table[(widx)]; \
				break; \
			case PCB_HATT_TREE: \
				__tree__ = (pcb_hid_tree_t *)table[(widx)].enumerations; \
				__tree__->attrib = &table[(widx)]; \
				break; \
			default: break; \
		} \
	} while(0)

/* Allocate a new item at the end of the attribute table; updates stored
   attribute pointers of existing items (e.g. previews, trees) as the base
   address of the table may have changed. */
#define PCB_DAD_ALLOC(table, item_type) \
	do { \
		assert(table ## _append_lock == 0); \
		if (table ## _len >= table ## _alloced) { \
			int __n__; \
			table ## _alloced += 16; \
			table = realloc(table, sizeof(table[0]) * table ## _alloced); \
			for(__n__ = 0; __n__ < table ## _len; __n__++) { \
				PCB_DAD_UPDATE_INTERNAL(table, __n__); \
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
		case PCB_HATT_TEXT: \
		case PCB_HATT_BUTTON: \
		case PCB_HATT_TREE: \
			table[table ## _len - 1].field.str_value = (char *)val; \
			break; \
		case PCB_HATT_COLOR: \
			table[table ## _len - 1].field.clr_value = *(pcb_color_t *)val; \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_BEGIN_COMPOUND: \
		case PCB_HATT_END: \
		case PCB_HATT_PREVIEW: \
		case PCB_HATT_PICTURE: \
		case PCB_HATT_PICBUTTON: \
			assert(0); \
	} \
} while(0)

#define PCB_DAD_SET_ATTR_FIELD_NUM(table, field, val) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			assert(0); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
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
		case PCB_HATT_TEXT: \
		case PCB_HATT_BUTTON: \
		case PCB_HATT_TREE: \
		case PCB_HATT_COLOR: \
		case PCB_HATT_UNIT: \
			assert(!"please use the _PTR() variant instead of the _NUM() variant"); \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_PREVIEW: \
		case PCB_HATT_PICTURE: \
		case PCB_HATT_PICBUTTON: \
			assert(0); \
		case PCB_HATT_BEGIN_COMPOUND: \
		case PCB_HATT_END: \
			{ \
				pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)table[table ## _len - 1].enumerations; \
				if ((cmp != NULL) && (cmp->set_field_num != NULL)) \
					cmp->set_field_num(&table[table ## _len - 1], #field, (long)(val), (double)(val), (pcb_coord_t)(val)); \
				else \
					assert(0); \
			} \
			break; \
	} \
} while(0)

#define PCB_DAD_SET_ATTR_FIELD_PTR(table, field, val) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			assert(0); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_BEGIN_TABBED: \
		case PCB_HATT_COORD: \
		case PCB_HATT_REAL: \
		case PCB_HATT_PROGRESS: \
		case PCB_HATT_BEGIN_HPANE: \
		case PCB_HATT_BEGIN_VPANE: \
			assert(!"please use the _NUM() variant instead of the _PTR() variant"); \
			break; \
		case PCB_HATT_UNIT: \
			{ \
				int __n__, __v__ = pcb_get_n_units(); \
				if (val != NULL) { \
					for(__n__ = 0; __n__ < __v__; __n__++) { \
						if (&pcb_units[__n__] == (pcb_unit_t *)(val)) { \
							table[table ## _len - 1].field.int_value = __n__; \
							break; \
						} \
					} \
				} \
			} \
			break; \
		case PCB_HATT_STRING: \
		case PCB_HATT_TEXT: \
		case PCB_HATT_BUTTON: \
		case PCB_HATT_TREE: \
			table[table ## _len - 1].field.str_value = (char *)val; \
			break; \
		case PCB_HATT_COLOR: \
			table[table ## _len - 1].field.clr_value = *((pcb_color_t *)val); \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_PREVIEW: \
		case PCB_HATT_PICTURE: \
		case PCB_HATT_PICBUTTON: \
			assert(0); \
		case PCB_HATT_BEGIN_COMPOUND: \
		case PCB_HATT_END: \
			{ \
				pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)table[table ## _len - 1].enumerations; \
				if ((cmp != NULL) && (cmp->set_field_ptr != NULL)) \
					cmp->set_field_ptr(&table[table ## _len - 1], #field, (void *)(val)); \
				else \
					assert(0); \
			} \
			break; \
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
		case PCB_HATT_BUTTON: \
		case PCB_HATT_PICTURE: \
		case PCB_HATT_PICBUTTON: \
		case PCB_HATT_COLOR: \
			break; \
		case PCB_HATT_TREE: \
			pcb_dad_tree_free(&table[field]); \
			break; \
		case PCB_HATT_PREVIEW: \
			{ \
				pcb_hid_preview_t *prv = (pcb_hid_preview_t *)table[field].enumerations; \
				if (prv->user_free_cb != NULL) \
					prv->user_free_cb(&table[field], prv->user_ctx, prv->hid_wdata); \
				if (prv->hid_free_cb != NULL) \
					prv->hid_free_cb(&table[field], prv->hid_wdata); \
				free(prv); \
			} \
			break; \
		case PCB_HATT_TEXT: \
			{ \
				pcb_hid_text_t *txt = (pcb_hid_text_t *)table[field].enumerations; \
				if (txt->user_free_cb != NULL) \
					txt->user_free_cb(&table[field], txt->user_ctx, txt->hid_wdata); \
				if (txt->hid_free_cb != NULL) \
					txt->hid_free_cb(&table[field], txt->hid_wdata); \
				free(txt); \
			} \
			break; \
		case PCB_HATT_BEGIN_COMPOUND: \
		case PCB_HATT_END: \
			{ \
				pcb_hid_compound_t *cmp = (pcb_hid_compound_t *)table[field].enumerations; \
				if ((cmp != NULL) && (cmp->free != NULL)) \
					cmp->free(&table[field]); \
			} \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_HPANE: \
		case PCB_HATT_BEGIN_VPANE: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_BEGIN_TABBED: \
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
	int dont_free;
	int valid;
	int value;
} pcb_dad_retovr_t;

typedef struct {
	const char *label;
	int retval;
} pcb_hid_dad_buttons_t;

void pcb_hid_dad_close(void *hid_ctx, pcb_dad_retovr_t *retovr, int retval);
void pcb_hid_dad_close_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr);
int pcb_hid_dad_run(void *hid_ctx, pcb_dad_retovr_t *retovr);
void pcb_hid_iterate(pcb_hid_t *hid);

/* sub-dialogs e.g. for the file selector dialog */
struct pcb_hid_dad_subdialog_s {
	/* filled in by the sub-dialog's creator */
	PCB_DAD_DECL_NOINIT(dlg)

	/* filled in by the parent dialog's code, the subdialog's code should
	   call this to query/change properties of the parent dialog. cmd and
	   argc/argv are all specific to the given dialog. Returns 0 on success,
	   return payload may be placed in res (if it is not NULL). Parent poke:
	     close()                   - cancel/close the dialog */
	int (*parent_poke)(pcb_hid_dad_subdialog_t *sub, const char *cmd, pcb_event_arg_t *res, int argc, pcb_event_arg_t *argv);

	void *parent_ctx; /* used by the parent dialog code */
	void *sub_ctx;    /* used by the sub-dialog's creator */

	gdl_elem_t link;  /* list of subdialogs: e.g. dock */
};

#endif
