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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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

#define PCB_DAD_DECL_NOINIT(table) \
	pcb_hid_attribute_t *table; \
	pcb_hid_attr_val_t *table ## _result; \
	int table ## _len; \
	int table ## _alloced; \
	void *table ## _hid_ctx; \

/* Free all resources allocated by DAD macros for table */
#define PCB_DAD_FREE(table) \
do { \
	int __n__; \
	if (table ## _hid_ctx != NULL) \
		pcb_gui->attr_dlg_free(table ## _hid_ctx); \
	for(__n__ = 0; __n__ < table ## _len; __n__++) { \
		PCB_DAD_FREE_FIELD(table, __n__); \
	} \
	free(table); \
	free(table ## _result); \
} while(0)

#define PCB_DAD_NEW(table, title, descr, caller_data, modal, ev_cb) \
do { \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	table ## _hid_ctx = pcb_gui->attr_dlg_new(table, table ## _len, table ## _result, title, descr, caller_data, modal, ev_cb); \
} while(0)

#define PCB_DAD_RUN(table) pcb_gui->attr_dlg_run(table ## _hid_ctx)

#define PCB_DAD_AUTORUN(table, title, descr, caller_data) \
do { \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	pcb_attribute_dialog(table, table ## _len, table ## _result, title, descr, caller_data); \
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
#define PCB_DAD_COMPFLAG(table, val)   PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_flags, val)

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

#define PCB_DAD_TREE(table, cols, first_col_is_tree) \
do { \
	pcb_hid_tree_t *tree = calloc(sizeof(pcb_hid_tree_t), 1); \
	htsp_init(&tree->paths, strhash, strkeyeq); \
	tree->attrib = &table[table ## _len-1]; \
	PCB_DAD_ALLOC(table, PCB_HATT_TREE); \
	PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_table_cols, cols); \
	PCB_DAD_SET_ATTR_FIELD(table, pcb_hatt_flags, first_col_is_tree ? PCB_HATF_TREE_COL : 0); \
	PCB_DAD_SET_ATTR_FIELD(table, enumerations, (const char **)tree); \
} while(0)

#define PCB_DAD_TREE_APPEND(table, hid_ctx, row_after, cells) \
	pcb_dad_tree_append(&table[table ## _len-1], hid_ctx, row_after, cells)

#define PCB_DAD_TREE_INSERT(table, hid_ctx, row_before, cells) \
	pcb_dad_tree_insert(&table[table ## _len-1], hid_ctx, row_before, cells)

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
#define PCB_DAD_ALLOC(table, item_type) \
	do { \
		if (table ## _len >= table ## _alloced) { \
			table ## _alloced += 16; \
			table = realloc(table, sizeof(table[0]) * table ## _alloced); \
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
		case PCB_HATT_COORD: \
		case PCB_HATT_UNIT: \
		case PCB_HATT_BEGIN_TABBED: \
			table[table ## _len - 1].field.int_value = (int)val; \
			break; \
		case PCB_HATT_REAL: \
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
			assert(0); \
	} \
} while(0)

#define PCB_DAD_FREE_FIELD(table, field) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			free((char *)table[table ## _len - 1].name); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_COORD: \
		case PCB_HATT_UNIT: \
		case PCB_HATT_REAL: \
		case PCB_HATT_STRING: \
		case PCB_HATT_PATH: \
		case PCB_HATT_BUTTON: \
			break; \
		case PCB_HATT_TREE: \
			pcb_dad_tree_free(&table[table ## _len - 1]); \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
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

#endif
