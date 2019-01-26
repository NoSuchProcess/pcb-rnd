/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018, 2019 Bohdan Maslovskyi
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

#ifndef XM_TREE_TABLE_WIDGET_H
#define XM_TREE_TABLE_WIDGET_H

/*** model part ***/

#include <genlist/gendlist.h>

/* Member of a gdl_list_t, the payload consists of (n_cells) of user managed text */
typedef struct tt_entry_s {
	/* tree indent/level count. */
	unsigned short level;

	struct {
		/* user selects. */
		unsigned is_hidden:1; /* 0 by default */
		unsigned is_unfolded:1; /* 1 by default, just specifies the type of pixmap, if it's leaf. */
		unsigned is_branch:1; /* 0 by default, just specifies the type of pixmap to be rendered: branch or leaf. */
		unsigned is_selected:1; /* 0 by default */
		unsigned is_being_rendered:1; /* 0 by default, updated by the view whenever the item fits on the screen. */
	} flags;

	/* filled in by the widget; -1 stands for unmanaged/detached item. */
	long row_index;

	/* N  cells of data */
	unsigned n_cells;

	/* to be set after first list traversal */
	unsigned n_text_lines;

	/* custom, opaque user data */
	void *user_data;

	gdl_elem_t gdl_linkfield;

	/* the payload - columns' text. Use tt_get_cell() to get an element. */
	char payload[1];

} tt_entry_t;

/* create an item -> returns new object capable storing (n_cells) strings.

disposal: if the entry hasn't been added to the list, just free() it later,
otherwise call delete_tt_entry(lest, item).
*/
tt_entry_t *tt_entry_alloc(unsigned n_cells);

/* put a non-NULL entry into the list.*/
void tt_entry_link(gdl_list_t *list, tt_entry_t *entry);

/* create an item -> returns new object capable storing (n_cells) strings, the object is added to the list. */
tt_entry_t *new_tt_entry(gdl_list_t *list, unsigned n_cells);

/* Remove item from parent's list and free it's memory. */
void delete_tt_entry(gdl_list_t *list, tt_entry_t *item);

/* On an object created by new_tt_entry():
object - non-NULL, must be obtained from new_tt_entry()

returns NULL, if the cells hasn't been allocated or the index is wrong. */
const char **tt_get_cell(tt_entry_t *object, unsigned index);

/*** widget part ***/

#include <Xm/Xm.h>

typedef enum ett_x11_event {
	ett_none, ett_mouse_btn_down, ett_mouse_btn_up, ett_mouse_btn_drag, ett_key
} ett_x11_event_t;

/* ett_render_finished - one should process to mangle the data structures,
this ensures the widget has finished rendering iteration and functions could be invoked:
xm_attach_tree_table_header();
xm_tt_set_x11_font_attr();
*/
typedef enum ett_draw_event {
	ett_draw_none, ett_scroll_vertical, ett_scroll_horizontal, ett_render_finished
} ett_draw_event_t;

/* Mouse & keyboard events callback data, provides navigation of the cursor in relation to the data. */
typedef struct tt_table_event_data {
	/* equals ett_render_finished on each drawing iteration finished, can also indicate scrollbar event. */
	ett_x11_event_t type;

	/* these are always non-NULL in context of tt_table_mouse_kbd_handler(),
	   if the widget has been created with non-NULL table data instance pointer. */
	gdl_list_t *root_entry;

	/* most recently selected row index at the list in (root_entry). -1 stands for invalid/unknown row index. */
	int current_row;
	unsigned current_cell;
	union {
		/* whenever event != ett_key
		 * {.x, .y} is the mouse position relatively to the widget's top-left (X,Y).
		 * {.width, .height} is current widget dimensions. */
		XRectangle mouse;
	} position;

	/* just pointer to the instance of tree_table widget, source of the event, never NULL> */
	Widget current_widget;
	/* contains valid non-NULL (XEvent*) for mouse/key event cases,.
	 * Please, refer to (event->xbutton.button) to find out which button triggered: Button1, Button2, Button3 or none (0). */
	XEvent *event;

	/* these are not used directly, just propagated from Xt event function */
	String *strings;
	Cardinal *strings_number;

} tt_table_event_data_t;

typedef void (*tt_table_mouse_kbd_handler) (const tt_table_event_data_t *data);

/* Re-draw/scroll events callback data, provides items visibility hints. */
typedef struct tt_table_draw_event_data {
	ett_draw_event_t type;
	Widget tree_table_widget;
	XRectangle widget_geometry;

	/* non-NULL, if the widget has been provided non-NULL table data pointer. */
	gdl_list_t *root;

	/* at the list in (root) it's the index of the rows that are currently visible in the widget.
	   value == -1 means invalid/unavailable position. */
	int visible_first, visible_last;

} tt_cb_draw_t;

typedef void (*tt_table_draw_handler) (const tt_cb_draw_t *data);


/* Create & return Xm Widget instance. It'll re-draw on window resize, scrollbars interaction.
One can call xm_draw_tree_table_widget() to re-draw the contents on demand.

(Widget)parent - valid instance of parent widget (top-level or below).
table_root - can be NULL, later use xm_set_tree_table_pointer() & xm_draw_tree_table_widget().

mouse_kbd_handler - can be NULL, if set, it'll be invoked on each mouse/keyboard action,
the handler will receive event type/mouse coords in the tt_table_event_data_t
and the original XEvent* pointer (volatile, not stored).

access_padlock - can be NULL, if set, it'll be invoked on each model data access during re-drawing:
begin(lock), end(unlock).

 */
Widget xm_create_tree_table_widget(Widget parent, gdl_list_t *table_root, tt_table_mouse_kbd_handler mouse_kbd_handler, tt_table_draw_handler draw_status_handler);

/* Re-render widget's content on demand. Should be invoked only inside the rendering loop.
In the end it will invoke tt_table_draw_handler() with (ett_render_finished) event type.

w - must be an instance returned by xm_create_tree_table_widget(). */
void xm_draw_tree_table_widget(Widget w);

/* Set font attributes like "-bitstream-courier 10 pitch-medium-r-*-*-*-*-*-*-*-*-*-1".
Affects whole widget (header, table cells), takes effect on next re-draw event:
when resized of manual invocation of xm_draw_tree_table_widget().
Function does NOT re-draw, use xm_draw_tree_table_widget() for that.

Returns 0 on success, -1 otherwise. */
int xm_tt_set_x11_font_attr(Widget xm_tree_table, const char *attributes);

/* can be optionally used to lock the access on rendering refresh*/
typedef struct tt_table_access_cb_s {
	/* optional things. */
	void *p_user_data;

	/* on each table modification/redraw action, lock/unlock will be triggered, if provided. */
	/* called when the widget enters list traversal. (begin) */
	void (*lock) (gdl_list_t *entry, void *user_data);

	/* called when the widget quits list traversal. (end) */
	void (*unlock) (gdl_list_t *entry, void *user_data);
} tt_table_access_cb_t;

/* same as xm_create_tree_table_widget(), but also passes a content locking callback.

(Widget)parent - valid instance of parent widget (top-level or below).
table_root - can be NULL, later use xm_set_tree_table_pointer() & xm_draw_tree_table_widget().

mouse_kbd_handler - can be NULL, if set, it'll be invoked on each mouse/keyboard action,
the handler will receive event type/mouse coords in the tt_table_event_data_t
and the original XEvent* pointer (volatile, not stored).

access_padlock - can be NULL, if set, it'll be invoked on each model data access during re-drawing:
begin(lock), end(unlock).

 */
Widget xm_create_tree_table_widget_cb(Widget parent, gdl_list_t *table_root, tt_table_mouse_kbd_handler mouse_kbd_handler, tt_table_draw_handler draw_status_handler, tt_table_access_cb_t *access_padlock);

/* Set new table pointer or re-parse the same list again on items addition/removal.
WHEN to call it? - any time, if the locking pointer tt_table_access_cb_t* was non-NULL,
(it'll lock/unlock), or otherwise only on (ett_render_finished) event of tt_table_draw_handler().

w - must be NOT NULL, a valid tree_table widget instance pointer.

new_table_root - can be NULL to untie previous instance pointer from the widget;

access_padlock - can be NULL, if set, it'll be invoked on each model data access during re-drawing:
begin(lock), end(unlock).
*/
void xm_set_tree_table_pointer(Widget w, gdl_list_t *new_table_root, tt_table_access_cb_t *access_padlock);

/* Display table header. WHEN to call it? - any time, if the locking pointer tt_table_access_cb_t* was non-NULL
(it'll lock/unlock), or otherwise only on (ett_render_finished) event of tt_table_draw_handler().

w - must be NOT NULL, a valid tree_table widget instance pointer.

n_strings - size of (char*) array.

strings - array of (char*) strings. Caller cares, the exist while the widget is displayed.
pass NULL to remove the header, but WHEN: one can do this any tt_table_draw_handler() is invoked,
that signal the user that the widget has finished the rendering iteration;
*/
void xm_attach_tree_table_header(Widget w, unsigned n_strings, const char **strings);

/* mode == 1 enables(default) tree-like rendering of the first column of the data structure.
mode == 0 disables it. Can be called any time.
*/
void xm_tree_table_tree_mode(Widget w, unsigned char mode);

/* The gaps between the columns(x) and the rows (y) to make the view less cluttered.
The default is 5pixels for X(columns), 0 for Y(rows).
WHEN to call it? - any time, if the locking pointer tt_table_access_cb_t* was non-NULL
(it'll lock/unlock), or otherwise only on (ett_render_finished) event of tt_table_draw_handler().

*/
void xm_tree_table_pixel_gaps(Widget w, unsigned char x, unsigned char y);

/* Scroll down to specific row index of the list.
 WHEN to call this? - any time, if the locking pointer tt_table_access_cb_t* was non-NULL
(it'll lock/unlock), or otherwise only on (ett_render_finished) event of tt_table_draw_handler().

Function does NOT re-draw, use xm_draw_tree_table_widget() for that.

return 0 on success, -1 on inaccessible row index.
*/
int xm_tree_table_focus_row(Widget w, int row_index);

typedef struct {
	int lo, hi; /* min, max values */
	int incr; /* increment - a minimal unit of scrollbar state change per minimal 1 pixel GUI step. */
	int size; /* scrollbar's size in pixels. */
	int cur, prev; /* libXm expects range [lo, hi - size] */

	Widget sbar; /* private: an internal instance of XmScrollbar */
} xm_tt_scrollbar;

/* WHEN to call these? - any time, if the locking pointer tt_table_access_cb_t* was non-NULL
(it'll lock/unlock), or otherwise only on (ett_render_finished) event of tt_table_draw_handler(). */

/* Read current scrollbar state, the output is different for different window size. */
xm_tt_scrollbar xm_tree_table_scrollbar_vertical_get(Widget table_widget);
xm_tt_scrollbar xm_tree_table_scrollbar_horizontal_get(Widget table_widget);

/* After figuring out the current state, the user can set a value in range [lo, hi - size].
Function does NOT re-draw, use xm_draw_tree_table_widget() for that. */
void xm_tree_table_scrollbar_vertical_set(Widget table_widget, int current_value);
void xm_tree_table_scrollbar_horizontal_set(Widget table_widget, int current_value);

typedef struct _XmTreeTableClassRec *XmTreeTableWidgetClass;
typedef struct _XmTreeTableRec *XmTreeTableWidget;


#endif /* XM_TREE_TABLE_WIDGET_H */
