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

#ifndef XM_TREE_TABLE_DRAW_ROUTINES_H
#define XM_TREE_TABLE_DRAW_ROUTINES_H

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include "xm_tree_table_widget.h"

/* Font convenience macros from XmGrace. */
#define GET_FONT_HEIGHT(xfont)  (int)(xfont->max_bounds.ascent + xfont->max_bounds.descent)

struct xm_long_rectangle_s {
	long x, y, width, height;
};

typedef struct xm_long_rectangle_s xm_long_rectangle;
typedef struct {
	tt_entry_t *item;
	xm_long_rectangle virtual_region;
	XRectangle display_region;
} item_desc_t;

struct render_target_s {
	XRectangle geom;
	Dimension horizontal_stride, vertical_stride;

	long *column_dimensions_vector;
	unsigned column_vector_len;

	item_desc_t *visible_items_vector;
	unsigned visible_items_capacity;
	unsigned visible_items_count;
};
void xm_init_render_target(struct render_target_s *target);
void xm_clear_render_target(struct render_target_s *target);

/* main drawing function: on window resize*/
void xm_render_ttwidget(Widget w);

/* Returns row index. -1 for not found.*/
int xm_find_row_pointed_by_mouse(Widget w, int y);

enum e_what_changed {
	/* on full re-draw */
	e_what_window,
	e_what_vertical_scroll,
	/* these are more performant, using cached results. */
	e_what_horizontal_scroll,
	e_what_content_selection
};
/* secondary drawing function: on scroll event. */
void xm_render_ttwidget_contents(Widget aw, enum e_what_changed what);
void xm_clip_rectangle(Widget w, XRectangle clip);

/*** scrollbar ***/
void xm_init_scrollbars(XmTreeTableWidget w);
void xm_horizontal_scroll_cb(Widget w, XtPointer client_data, XtPointer call_data);
void xm_vertical_scroll_cb(Widget w, XtPointer client_data, XtPointer call_data);
void xm_fit_scrollbars_to_geometry(XmTreeTableWidget w, struct render_target_s *s);

/*** text extent ***/
void xm_extent_prediction(XmTreeTableWidget w);

/*** primitive ***/
#include <Xm/Primitive.h>
#include <Xm/XmP.h>
#include <X11/Core.h>
#include <Xm/PrimitiveP.h>

#include "xm_tree_table_widget.h"
#include "xm_tree_table_priv.h"

/*definition of the widget class part*/
typedef GC(*XmTreeTableSelectGCProc) (Widget);
typedef void (*XmTreeTableReconfigureProc) (WidgetClass, Widget, Widget);
extern WidgetClass xmTreeTableWidgetClass;

typedef struct {
	Pixmap bitmap;
	Pixmap pix;
	int width, height;
	int xoff;
} Pixinfo;

typedef struct {
	XtWidgetProc draw_visual;
	XtWidgetProc draw_shadow;
	XtWidgetProc create_gc;
	XtWidgetProc destroy_gc;
	XmTreeTableSelectGCProc select_gc;
	XtWidgetProc calc_visual_size;
	XtWidgetProc calc_widget_size;
	XmTreeTableReconfigureProc reconfigure;
	XtPointer extension;
} XmTreeTableClassPart;

/*declaration of the full class record*/
typedef struct _XmTreeTableClassRec {
	CoreClassPart core_class;
	XmPrimitiveClassPart primitive_class;
	XmTreeTableClassPart tree_table_class;
} XmTreeTableClassRec;

externalref XmTreeTableClassRec xmTreeTableClassRec;

typedef struct {
	/* a pointer to external table object. */
	gdl_list_t *table;

	struct render_target_s render_attr;
	tt_table_access_cb_t *table_access_padlock;
	/*---drawing context stuff----*/
	XFontStruct *font;
	/*---spacing variables--------*/
	long foreground_pixel;
	Dimension h_spacing;
	Dimension v_spacing;
	Dimension line_width;
	/*---pixmap resources---------*/
	Dimension pix_width;
	Pixinfo pix_branch_open;
	Pixinfo pix_branch_closed;
	Pixinfo pix_leaf;
	Pixinfo pix_leaf_open;
	/*----view state variables------*/
	Dimension n_max_pixmap_height;
	Dimension n_grid_line_thickness;
	unsigned char n_grid_x_gap_pixels;
	unsigned char n_grid_y_gap_pixels;

	tt_entry_t *p_header;
	/* predicted rectangular extent of the tree part and the table part.
	 * It will be compared to the widget's geometry to compute the scrollbars position. */
	xm_long_rectangle virtual_canvas_size;
	Dimension n_minimum_cell_width;
	unsigned char b_show_tree;
	/* scrollbars */
	xm_tt_scrollbar w_vert_sbar, w_horiz_sbar;
	tt_table_mouse_kbd_handler p_mouse_kbd_handler;
	tt_table_event_data_t event_data;

	tt_table_draw_handler p_draw_handler;
	tt_cb_draw_t draw_event_data;

	/*----drawing GC structures-----*/
	GC gc_draw;
	GC gc_inverted_color;
	GC gc_highlight;

	/* ==== public API-related variables ==== */
	Bool b_table_grid_visible;
	void *user_data;
} XmTreeTablePart;

typedef struct _XmTreeTableRec {
	CorePart core;
	XmPrimitivePart primitive;
	XmTreeTablePart tree_table;
} XmTreeTableRec;

#define XtNmargin "margin"
#define XtNindent "indent"
#define XtNspacing "spacing"
#define XtNhorizontalSpacing "horizontalSpacing"
#define XtNverticalSpacing "verticalSpacing"
#define XtNlineWidth "lineWidth"
#define XtNpixWidth "pixWidth"
#define XtNhighlightPath "highlightPath"
#define XtNclickPixmapToOpen "clickPixmapToOpen"
#define XtNdoIncrementalHighlightCallback "incCallback"
#define XtNbranchPixmap "branchPixmap"
#define XtNbranchOpenPixmap "branchOpenPixmap"
#define XtNleafPixmap "leafPixmap"
#define XtNleafOpenPixmap "leafOpenPixmap"

#endif /* XM_TREE_TABLE_DRAW_ROUTINES_H */
