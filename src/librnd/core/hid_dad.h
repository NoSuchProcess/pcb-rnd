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
#include <librnd/core/compat_misc.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/global_typedefs.h>

#include <genlist/gendlist.h>

typedef enum {
	PCB_HID_TEXT_INSERT,           /* insert at cursor or replace selection */
	PCB_HID_TEXT_REPLACE,          /* replace the entire text */
	PCB_HID_TEXT_APPEND,           /* append to the end of the text */

	/* modifiers (bitfield) */
	PCB_HID_TEXT_MARKUP = 0x0010   /* interpret minimal html-like markup - some HIDs may ignore these */
} pcb_hid_text_set_t;

typedef struct {
	/* cursor manipulation callbacks */
	void (*hid_get_xy)(pcb_hid_attribute_t *attrib, void *hid_ctx, long *x, long *y); /* can be very slow */
	long (*hid_get_offs)(pcb_hid_attribute_t *attrib, void *hid_ctx);
	void (*hid_set_xy)(pcb_hid_attribute_t *attrib, void *hid_ctx, long x, long y); /* can be very slow */
	void (*hid_set_offs)(pcb_hid_attribute_t *attrib, void *hid_ctx, long offs);
	void (*hid_scroll_to_bottom)(pcb_hid_attribute_t *attrib, void *hid_ctx);
	void (*hid_set_text)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_text_set_t how, const char *txt);
	char *(*hid_get_text)(pcb_hid_attribute_t *attrib, void *hid_ctx); /* caller needs to free the result */
	void (*hid_set_readonly)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_bool readonly); /* by default text views are not read-only */

	/* optional callbacks the user set after widget creation */
	void *user_ctx;
	void (*user_free_cb)(pcb_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx);

	/* optional callbacks HIDs may set after widget creation */
	void *hid_wdata;
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
} pcb_hid_text_t;


typedef struct {
	int cols;        /* number of columns used by this node (allocation size) */
	void *hid_data;  /* the hid running the widget can use this field to store a custom pointer per row */
	gdl_list_t children;
	gdl_elem_t link;
	char *path;      /* full path of the node; allocated/free'd by DAD (/ is the root, every entry is specified from the root, but the leading / is omitted; in non-tree case, this only points to the first col data) */
	unsigned hide:1; /* if non-zero, the row is not visible (e.g. filtered out) */

	/* caller/user data */
	void *user_data;
	union {
		void *ptr;
		long lng;
		double dbl;
	} user_data2;
	char *cell[1];   /* each cell is a char *; the true length of the array is the value of the len field; the array is allocated together with the struct */
} pcb_hid_row_t;

typedef struct {
	gdl_list_t rows; /* ordered list of first level rows (tree root) */
	htsp_t paths;    /* translate first column paths iinto (pcb_hid_row_t *) */
	pcb_hid_attribute_t *attrib;
	const char **hdr; /* optional column headers (NULL means disable header) */

	/* optional callbacks the user set after widget creation */
	void *user_ctx;
	void (*user_free_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);
	void (*user_selected_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row);
	int (*user_browse_activate_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row); /* returns non-zero if the row should auto-activate while browsing (e.g. stepping with arrow keys) */
	const char *(*user_copy_to_clip_cb)(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row); /* returns the string to copy to clipboard for the given row (if unset, first column text is used) */

	/* optional callbacks HIDs may set after widget creation */
	void *hid_wdata;
	void (*hid_insert_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *new_row);
	void (*hid_modify_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int col); /* if col is negative, all columns have changed */
	void (*hid_remove_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row);
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row);
	pcb_hid_row_t *(*hid_get_selected_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
	void (*hid_jumpto_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row); /* row = NULL means deselect all */
	void (*hid_expcoll_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, pcb_hid_row_t *row, int expanded); /* sets whether a row is expanded or collapsed */
	void (*hid_update_hide_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
} pcb_hid_tree_t;

typedef struct pcb_hid_preview_s pcb_hid_preview_t;
struct pcb_hid_preview_s {
	pcb_hid_attribute_t *attrib;

	pcb_box_t initial_view;
	unsigned initial_view_valid:1;

	int min_sizex_px, min_sizey_px; /* hint: widget minimum size in pixels */

	/* optional callbacks the user set after widget creation */
	void *user_ctx;
	void (*user_free_cb)(pcb_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx);
	void (*user_expose_cb)(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);
	pcb_bool (*user_mouse_cb)(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y); /* returns true if redraw is needed */

	/* optional callbacks HIDs may set after widget creation */
	void *hid_wdata;
	void (*hid_zoomto_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata, const pcb_box_t *view);
	void (*hid_free_cb)(pcb_hid_attribute_t *attrib, void *hid_wdata);
};

typedef struct {
	int wbegin, wend; /* widget index to the correspoding PCB_HATT_BEGIN_COMPOUND and PCB_HATT_END */

	/* compound implementation callbacks */
	int (*widget_state)(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool enabled);
	int (*widget_hide)(pcb_hid_attribute_t *end, void *hid_ctx, int idx, pcb_bool hide);
	int (*set_value)(pcb_hid_attribute_t *end, void *hid_ctx, int idx, const pcb_hid_attr_val_t *val); /* set value runtime */
	void (*set_val_num)(pcb_hid_attribute_t *attr, long l, double d, pcb_coord_t c); /* set value during creation; attr is the END */
	void (*set_val_ptr)(pcb_hid_attribute_t *attr, void *ptr); /* set value during creation; attr is the END */
	void (*set_help)(pcb_hid_attribute_t *attr, const char *text); /* set the tooltip help; attr is the END */
	void (*set_field_num)(pcb_hid_attribute_t *attr, const char *fieldname, long l, double d, pcb_coord_t c); /* set value during creation; attr is the END */
	void (*set_field_ptr)(pcb_hid_attribute_t *attr, const char *fieldname, void *ptr); /* set value during creation; attr is the END */
	void (*free)(pcb_hid_attribute_t *attrib); /* called by DAD on free'ing the PCB_HATT_BEGIN_COMPOUND and PCB_HATT_END_COMPOUND widget */
} pcb_hid_compound_t;

#include <librnd/core/hid_dad_spin.h>

/*** Helpers for building dynamic attribute dialogs (DAD) ***/
#define PCB_DAD_DECL(table) \
	pcb_hid_attribute_t *table = NULL; \
	int table ## _append_lock = 0; \
	int table ## _len = 0; \
	int table ## _alloced = 0; \
	void *table ## _hid_ctx = NULL; \
	int table ## _defx = 0, table ## _defy = 0; \
	int table ## _minx = 0, table ## _miny = 0; \
	pcb_dad_retovr_t *table ## _ret_override;

#define PCB_DAD_DECL_NOINIT(table) \
	pcb_hid_attribute_t *table; \
	int table ## _append_lock; \
	int table ## _len; \
	int table ## _alloced; \
	void *table ## _hid_ctx; \
	int table ## _defx, table ## _defy; \
	int table ## _minx, table ## _miny; \
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
	table = NULL; \
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
	table ## _ret_override = calloc(sizeof(pcb_dad_retovr_t), 1); \
	table ## _append_lock = 1; \
	table ## _hid_ctx = pcb_gui->attr_dlg_new(pcb_gui, id, table, table ## _len, title, caller_data, modal, ev_cb, table ## _defx, table ## _defy, table ## _minx, table ## _miny); \
} while(0)

/* Sets the default window size (that is only a hint) - NOTE: must be called
   before PCB_DAD_NEW() */
#define PCB_DAD_DEFSIZE(table, width, height) \
do { \
	table ## _defx = width; \
	table ## _defy = height; \
} while(0)

/* Sets the minimum window size (that is only a hint) - NOTE: must be called
   before PCB_DAD_NEW() */
#define PCB_DAD_MINSIZE(table, width, height) \
do { \
	table ## _minx = width; \
	table ## _miny = height; \
} while(0)

#define PCB_DAD_RUN(table) pcb_hid_dad_run(table ## _hid_ctx, table ## _ret_override)

/* failed is zero on success and -1 on error (e.g. cancel) or an arbitrary
   value set by ret_override (e.g. on close buttons) */
#define PCB_DAD_AUTORUN(id, table, title, caller_data, failed) \
do { \
	int __ok__; \
	table ## _ret_override = calloc(sizeof(pcb_dad_retovr_t), 1); \
	table ## _ret_override->dont_free++; \
	table ## _ret_override->valid = 0; \
	table ## _append_lock = 1; \
	__ok__ = pcb_attribute_dialog_(id,table, table ## _len, title, caller_data, (void **)&(table ## _ret_override), table ## _defx, table ## _defy, table ## _minx, table ## _miny, &table ## _hid_ctx); \
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
	PCB_DAD_SET_ATTR_FIELD(table, wdata, tabs); \
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
	PCB_DAD_SET_ATTR_FIELD(table, wdata, choices); \
} while(0)

#define PCB_DAD_BOOL(table, label) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_BOOL); \
	PCB_DAD_SET_ATTR_FIELD(table, name, pcb_strdup(label)); \
} while(0)

#define PCB_DAD_INTEGER(table, label) \
do { \
	PCB_DAD_SPIN_INT(table); \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_REAL(table, label) \
do { \
	PCB_DAD_SPIN_DOUBLE(table); \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_COORD(table, label) \
do { \
	PCB_DAD_SPIN_COORD(table); \
	PCB_DAD_SET_ATTR_FIELD(table, name, label); \
} while(0)

#define PCB_DAD_STRING(table) \
	PCB_DAD_ALLOC(table, PCB_HATT_STRING); \

#define PCB_DAD_TEXT(table, user_ctx_) \
do { \
	pcb_hid_text_t *txt = calloc(sizeof(pcb_hid_text_t), 1); \
	txt->user_ctx = user_ctx_; \
	PCB_DAD_ALLOC(table, PCB_HATT_TEXT); \
	PCB_DAD_SET_ATTR_FIELD(table, wdata, txt); \
} while(0)

#define PCB_DAD_BUTTON(table, text) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_BUTTON); \
	table[table ## _len - 1].val.str = text; \
} while(0)

#define PCB_DAD_BUTTON_CLOSE(table, text, retval) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_BUTTON); \
	table[table ## _len - 1].val.str = text; \
	table[table ## _len - 1].val.lng = retval; \
	table[table ## _len - 1].wdata = (&table ## _ret_override); \
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
	PCB_DAD_SET_ATTR_FIELD(table, wdata, prv); \
} while(0)

#define pcb_dad_preview_zoomto(attr, view) \
do { \
	pcb_hid_preview_t *prv = ((attr)->wdata); \
	if (prv->hid_zoomto_cb != NULL) \
		prv->hid_zoomto_cb((attr), prv->hid_wdata, view); \
} while(0)


#define PCB_DAD_PICTURE(table, xpm) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_PICTURE); \
	table[table ## _len - 1].wdata = xpm; \
} while(0)

#define PCB_DAD_PICBUTTON(table, xpm) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_PICBUTTON); \
	table[table ## _len - 1].wdata = xpm; \
} while(0)


#define PCB_DAD_COLOR(table) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_COLOR); \
} while(0)

#define PCB_DAD_BEGIN_HPANE(table) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_HPANE); \
	table[table ## _len - 1].val.dbl = 0.5; \
} while(0)

#define PCB_DAD_BEGIN_VPANE(table) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_VPANE); \
	table[table ## _len - 1].val.dbl = 0.5; \
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
	PCB_DAD_SET_ATTR_FIELD(table, wdata, tree); \
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
	pcb_hid_tree_t *__tree__ = table[table ## _len-1].wdata; \
	__tree__->user_ ## name = func_or_data; \
} while(0)

#define PCB_DAD_DUP_ATTR(table, attr) \
do { \
	PCB_DAD_ALLOC(table, 0); \
	memcpy(&table[table ## _len-1], (attr), sizeof(pcb_hid_attribute_t)); \
	PCB_DAD_UPDATE_INTERNAL(table, table ## _len-1); \
} while(0)

#define PCB_DAD_DUP_EXPOPT(table, opt) \
do { \
	pcb_export_opt_t *__opt__ = (opt); \
	PCB_DAD_ALLOC(table, 0); \
	table[table ## _len-1].name = __opt__->name; \
	table[table ## _len-1].help_text = __opt__->help_text; \
	table[table ## _len-1].type = __opt__->type; \
	table[table ## _len-1].min_val = __opt__->min_val; \
	table[table ## _len-1].max_val = __opt__->max_val; \
	table[table ## _len-1].val = __opt__->default_val; \
	table[table ## _len-1].wdata = __opt__->enumerations; \
	PCB_DAD_UPDATE_INTERNAL(table, table ## _len-1); \
} while(0)


/* Set properties of the current item */
#define PCB_DAD_MINVAL(table, val)       PCB_DAD_SET_ATTR_FIELD(table, min_val, val)
#define PCB_DAD_MAXVAL(table, val)       PCB_DAD_SET_ATTR_FIELD(table, max_val, val)
#define PCB_DAD_MINMAX(table, min, max)  (PCB_DAD_SET_ATTR_FIELD(table, min_val, min),PCB_DAD_SET_ATTR_FIELD(table, max_val, max))
#define PCB_DAD_CHANGE_CB(table, cb)     PCB_DAD_SET_ATTR_FIELD(table, change_cb, cb)
#define PCB_DAD_RIGHT_CB(table, cb)      PCB_DAD_SET_ATTR_FIELD(table, right_cb, cb)
#define PCB_DAD_ENTER_CB(table, cb)      PCB_DAD_SET_ATTR_FIELD(table, enter_cb, cb)

#define PCB_DAD_HELP(table, val) \
	do { \
		switch(table[table ## _len - 1].type) { \
			case PCB_HATT_END: \
				{ \
					pcb_hid_compound_t *cmp = table[table ## _len - 1].wdata; \
					if ((cmp != NULL) && (cmp->set_help != NULL)) \
						cmp->set_help(&table[table ## _len - 1], (val)); \
				} \
				break; \
			default: \
				PCB_DAD_SET_ATTR_FIELD(table, help_text, val); \
		} \
	} while(0)

#define PCB_DAD_DEFAULT_PTR(table, val_) \
	do {\
		switch(table[table ## _len - 1].type) { \
			case PCB_HATT_BEGIN_COMPOUND: \
			case PCB_HATT_END: \
				{ \
					pcb_hid_compound_t *cmp = table[table ## _len - 1].wdata; \
					if ((cmp != NULL) && (cmp->set_val_ptr != NULL)) \
						cmp->set_val_ptr(&table[table ## _len - 1], (void *)(val_)); \
					else \
						assert(0); \
				} \
				break; \
			default: \
				PCB_DAD_SET_ATTR_FIELD_PTR(table, val, val_); \
		} \
	} while(0)

#define PCB_DAD_DEFAULT_NUM(table, val_) \
	do {\
		switch(table[table ## _len - 1].type) { \
			case PCB_HATT_BEGIN_COMPOUND: \
			case PCB_HATT_END: \
				{ \
					pcb_hid_compound_t *cmp = table[table ## _len - 1].wdata; \
					if ((cmp != NULL) && (cmp->set_val_num != NULL)) \
						cmp->set_val_num(&table[table ## _len - 1], (long)(val_), (double)(val_), (pcb_coord_t)(val_)); \
					else \
						assert(0); \
				} \
				break; \
			default: \
				PCB_DAD_SET_ATTR_FIELD_NUM(table, val, val_); \
		} \
	} while(0);

/* safe way to call gui->attr_dlg_set_value() - resets the unused fields */
#define PCB_DAD_SET_VALUE(hid_ctx, wid, field, val_) \
	do { \
		pcb_hid_attr_val_t __val__; \
		memset(&__val__, 0, sizeof(__val__)); \
		__val__.field = val_; \
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
				__prv__ = table[(widx)].wdata; \
				__prv__->attrib = &table[(widx)]; \
				break; \
			case PCB_HATT_TREE: \
				__tree__ = table[(widx)].wdata; \
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

#define PCB_DAD_OR_ATTR_FIELD(table, field, value) \
	table[table ## _len - 1].field |= (value)

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
			table[table ## _len - 1].field.lng = (int)val; \
			break; \
		case PCB_HATT_COORD: \
			table[table ## _len - 1].field.crd = (pcb_coord_t)val; \
			break; \
		case PCB_HATT_REAL: \
		case PCB_HATT_PROGRESS: \
		case PCB_HATT_BEGIN_HPANE: \
		case PCB_HATT_BEGIN_VPANE: \
			table[table ## _len - 1].field.dbl = (double)val; \
			break; \
		case PCB_HATT_STRING: \
		case PCB_HATT_TEXT: \
		case PCB_HATT_BUTTON: \
		case PCB_HATT_TREE: \
			table[table ## _len - 1].field.str = (char *)val; \
			break; \
		case PCB_HATT_COLOR: \
			table[table ## _len - 1].field.clr = *(pcb_color_t *)val; \
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

#define PCB_DAD_SET_ATTR_FIELD_NUM(table, field, val_) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			assert(0); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_BEGIN_TABBED: \
			table[table ## _len - 1].field.lng = (int)val_; \
			break; \
		case PCB_HATT_COORD: \
			table[table ## _len - 1].field.crd = (pcb_coord_t)val_; \
			break; \
		case PCB_HATT_REAL: \
		case PCB_HATT_PROGRESS: \
		case PCB_HATT_BEGIN_HPANE: \
		case PCB_HATT_BEGIN_VPANE: \
			table[table ## _len - 1].field.dbl = (double)val_; \
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
				pcb_hid_compound_t *cmp = table[table ## _len - 1].wdata; \
				if ((cmp != NULL) && (cmp->set_field_num != NULL)) \
					cmp->set_field_num(&table[table ## _len - 1], #field, (long)(val_), (double)(val_), (pcb_coord_t)(val_)); \
				else \
					assert(0); \
			} \
			break; \
	} \
} while(0)

#define PCB_DAD_SET_ATTR_FIELD_PTR(table, field, val_) \
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
				int __n__, __v__ = pcb_get_n_units(0); \
				if (val_ != NULL) { \
					for(__n__ = 0; __n__ < __v__; __n__++) { \
						if (&pcb_units[__n__] == (pcb_unit_t *)(val_)) { \
							table[table ## _len - 1].field.lng = __n__; \
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
			table[table ## _len - 1].field.str = (char *)val_; \
			break; \
		case PCB_HATT_COLOR: \
			table[table ## _len - 1].field.clr = *((pcb_color_t *)val_); \
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
				pcb_hid_compound_t *cmp = table[table ## _len - 1].wdata; \
				if ((cmp != NULL) && (cmp->set_field_ptr != NULL)) \
					cmp->set_field_ptr(&table[table ## _len - 1], #field, (void *)(val_)); \
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
				pcb_hid_preview_t *prv = table[field].wdata; \
				if (prv->user_free_cb != NULL) \
					prv->user_free_cb(&table[field], prv->user_ctx, prv->hid_wdata); \
				if (prv->hid_free_cb != NULL) \
					prv->hid_free_cb(&table[field], prv->hid_wdata); \
				free(prv); \
			} \
			break; \
		case PCB_HATT_TEXT: \
			{ \
				pcb_hid_text_t *txt = table[field].wdata; \
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
				pcb_hid_compound_t *cmp = table[field].wdata; \
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

#define PCB_DAD_WIDTH_CHR(table, width) \
do { \
	PCB_DAD_OR_ATTR_FIELD(table, hatt_flags, PCB_HATF_HEIGHT_CHR); \
	PCB_DAD_SET_ATTR_FIELD(table, geo_width, width); \
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

	/* OPTIONAL: filled in by the sub-dialog's creator: called by the
	   sub-dialog's parent while the parent dialog is being closed. If
	   ok is false, the dialog was cancelled */
	void (*on_close)(pcb_hid_dad_subdialog_t *sub, pcb_bool ok);

	void *parent_ctx; /* used by the parent dialog code */
	void *sub_ctx;    /* used by the sub-dialog's creator */

	gdl_elem_t link;  /* list of subdialogs: e.g. dock */
};

typedef struct pcb_hid_export_opt_func_dad_s {
	PCB_DAD_DECL_NOINIT(dlg)
} pcb_hid_export_opt_func_dad_t;

#endif
