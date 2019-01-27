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

#include <genvector/vtp0.h>
#include <X11/Xlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <Xm/ScrollBar.h>
#include "xm_tree_table_priv.h"

#define TABLE_COLS_GAP_PIXELS 2

#define TTBL_MIN(a,b) ((a) < (b)? (a) : (b))
#define TTBL_MAX(a,b) ((a) > (b)? (a) : (b))
#define TTBL_CLAMP(value, lower, higher) TTBL_MIN(TTBL_MAX((value), (lower)), (higher))

void xm_init_render_target(struct render_target_s *target)
{
	memset(target, 0x00, sizeof(struct render_target_s));
}

void xm_clear_render_target(struct render_target_s *target)
{
	if (target->column_dimensions_vector)
		free(target->column_dimensions_vector);

	if (target->visible_items_vector)
		free(target->visible_items_vector);
	memset(target, 0x00, sizeof(struct render_target_s));
}

static int translate_scroll_horizontal(XmTreeTableWidget w)
{
	XmTreeTablePart *tt = &(w->tree_table);
	xm_tt_scrollbar *hsb = &(tt->w_horiz_sbar);
	float pos = tt->virtual_canvas_size.width * (hsb->cur - hsb->lo);
	pos /= (float)(hsb->hi - hsb->lo);
	return (int)pos;
}

/* renders only table cells for the current row(entry).*/
static void draw_row_cells(GC gc, int cur_x, int cur_y, tt_entry_t *entry, XmTreeTableWidget w, struct render_target_s *s)
{
	XmTreeTablePart *tt = &(w->tree_table);
	unsigned col = 0;
	int x_start = (tt->b_show_tree ? entry->level : 0) * s->horizontal_stride + cur_x;
	XCharStruct chrs = { 0 };
	int i_ret[3] = { 0 };

	/* render 1-st column */
	const char *str = tt_get_cell(entry, 0)[0];
	int slen = str ? strlen(str) : 0;
	int pixmap_shift = tt->n_grid_x_gap_pixels + tt->n_max_pixmap_height;
	/* column by index 0 (can bee tree-like) */
	if (str) {
		XTextExtents(tt->font, str, slen, i_ret, i_ret + 1, i_ret + 2, &chrs);
		if ((x_start + chrs.width) > s->geom.x && x_start < (s->geom.x + s->geom.width)) {
			XDrawString(XtDisplay(w), XtWindow(w), gc, x_start + pixmap_shift, cur_y - chrs.descent, str, slen);
		}
	}
	x_start = s->column_dimensions_vector[0] + pixmap_shift + cur_x;

	/* render the rest, they take into account the width of previous column. */
	for(col = 1; col < entry->n_cells; x_start += s->column_dimensions_vector[col], ++col)
	{
		/* column shift */
		str = tt_get_cell(entry, col)[0];
		if (!str) {
			continue;
		}
		slen = strlen(str);
		XTextExtents(tt->font, str, slen, i_ret, i_ret + 1, i_ret + 2, &chrs);

		if (str && (x_start + chrs.width) > s->geom.x && x_start < (s->geom.x + s->geom.width)) {
			XDrawString(XtDisplay(w), XtWindow(w), gc, x_start, cur_y - chrs.descent, str, slen);
		}
	}
}

/* renders pixmap & table cells for the current row(entry).*/
static void draw_row(tt_entry_t *entry, int x, int y, GC gc, XmTreeTableWidget w, struct render_target_s *s)
{
	Pixinfo pxinfo = w->tree_table.pix_leaf;
	int px = x + entry->level * s->horizontal_stride;
	if (entry->flags.is_branch) {
		pxinfo = entry->flags.is_unfolded ? w->tree_table.pix_branch_open : w->tree_table.pix_branch_closed;
	}
	/* render tree entry label, a bit shifted after the pixmap on X axis */
	if (entry->flags.is_selected) {
		XRectangle clip;
		clip.x = s->geom.x + 1;
		clip.y = y - s->vertical_stride;
		clip.width = s->geom.width - 1;
		clip.height = s->vertical_stride;
		XFillRectangle(XtDisplay(w), XtWindow(w), w->tree_table.gc_highlight, clip.x, clip.y, clip.width, clip.height);
		XFillRectangle(XtDisplay(w), XtWindow(w), w->tree_table.gc_inverted_color, clip.x, clip.y, clip.width, clip.height);
		draw_row_cells(w->tree_table.gc_highlight, x, y, entry, w, s);
	}
	else
		draw_row_cells(gc, x, y, entry, w, s);

	/* render node pixmap. */
	if (w->tree_table.b_show_tree && (px >= (s->geom.x - pxinfo.width)) && px < (s->geom.x + s->geom.width + pxinfo.width)) {
		XCopyArea(XtDisplay(w), pxinfo.pix, XtWindow(w), gc, 0, 0, pxinfo.width, pxinfo.height, px, y - pxinfo.height);
	}
}

static void load_visible_items(XmTreeTablePart *tt, const item_desc_t *start)
{
	struct render_target_s *s = &(tt->render_attr);
	tt_entry_t *et = start->item ? start->item : (tt_entry_t *)gdl_first(tt->table);
	unsigned showing_cnt = 0;
	long virtual_shift = start->virtual_region.y + start->virtual_region.height;
	long display_shift = tt->p_header ? s->vertical_stride : 0;
	if (s->visible_items_vector && s->visible_items_capacity > 0)
		memset(s->visible_items_vector, 0x00, sizeof(item_desc_t) * s->visible_items_capacity);

	for(; et && display_shift < (s->geom.height + s->vertical_stride); et = (tt_entry_t *)gdl_next(tt->table, (void *)et)) {
		if (!et->flags.is_hidden) {
			item_desc_t *desc;
			if (s->visible_items_capacity <= showing_cnt) {
				s->visible_items_capacity = 2 * (showing_cnt + 16);
				s->visible_items_vector = (item_desc_t *)realloc(s->visible_items_vector, sizeof(item_desc_t) * s->visible_items_capacity);
				memset(s->visible_items_vector + showing_cnt, 0x00, sizeof(item_desc_t) * (s->visible_items_capacity - showing_cnt));
			}
			desc = s->visible_items_vector + showing_cnt;
			memset(desc, 0x00, sizeof(item_desc_t));
			desc->item = et;
			desc->virtual_region.height = s->vertical_stride * TTBL_MIN(1, et->n_text_lines);
			desc->virtual_region.y = virtual_shift;

			desc->display_region.y += display_shift;
			desc->display_region.height = desc->virtual_region.height;

			et->flags.is_being_rendered = 1;
			virtual_shift += desc->virtual_region.height;
			display_shift += desc->virtual_region.height;
			++showing_cnt;
		}
	}
	s->visible_items_count = showing_cnt;
}

void xm_render_ttwidget_contents(Widget aw, enum e_what_changed what)
{
	Display *dsp = XtDisplay(aw);
	XmTreeTableWidget w = (XmTreeTableWidget)aw;
	XmTreeTablePart *tt = &(w->tree_table);

	struct render_target_s *s = &(w->tree_table.render_attr);
	XRectangle clip = s->geom;
	int y_shift = 0;
	tt_entry_t *begin_entry = NULL;
	if (!tt->table)
		return;

	if (s->visible_items_vector)
		begin_entry = s->visible_items_vector[0].item;

	{
		unsigned cnt = 0;
		for(; cnt < s->visible_items_count; ++cnt)
			if (s->visible_items_vector[cnt].item)
				s->visible_items_vector[cnt].item->flags.is_being_rendered = 0;
	}
	/* TODO: reserve some space for tree's leafs' labels. */
	s->horizontal_stride = TTBL_MAX(w->tree_table.n_max_pixmap_height, s->horizontal_stride);
	s->vertical_stride = TTBL_MAX(w->tree_table.n_max_pixmap_height, GET_FONT_HEIGHT(tt->font));

	begin_entry = (tt_entry_t *)gdl_first(tt->table);
	if (begin_entry) {
		long occupied_height = 0;
		tt_entry_t *et = begin_entry;
		long diff = tt->w_vert_sbar.cur - tt->w_vert_sbar.lo;
		long ratio = tt->virtual_canvas_size.height / s->geom.height;
		diff *= ratio;

		for(; et && occupied_height < diff; et = (tt_entry_t *)gdl_next(tt->table, (void *)et)) {
			unsigned char showing = !et->flags.is_hidden;
			occupied_height += showing * TTBL_MAX(1, et->n_text_lines) * s->vertical_stride;
			begin_entry = et;
		}
	}
	else {
		return;
	}

	{
		item_desc_t desc;
		memset(&desc, 0x00, sizeof(item_desc_t));
		desc.item = begin_entry;
		desc.display_region.width = s->geom.width;
		desc.display_region.height = desc.item->n_text_lines * s->vertical_stride;
		desc.virtual_region.height = desc.display_region.height;
		load_visible_items(tt, &desc);
	}

	xm_fit_scrollbars_to_geometry(w, s);
	xm_clip_rectangle((Widget)w, clip);

	if (tt->p_header) {
		/* render the header, if provided. */
		XCharStruct chrs = { 0 };
		int i_ret[3] = { 0 };
		unsigned x;
		clip.height = s->vertical_stride;
		for(x = 0; x < tt->p_header->n_cells; ++x) {
			const char *str = tt_get_cell(tt->p_header, x)[0];
			if (str) {
				XTextExtents(tt->font, str, strlen(str), i_ret, i_ret + 1, i_ret + 2, &chrs);
				clip.height = TTBL_MAX(s->vertical_stride, TTBL_MAX(chrs.ascent - chrs.descent, clip.height));
			}
		}
		s->vertical_stride = TTBL_MAX(clip.height, s->vertical_stride);
		y_shift = 2 * s->vertical_stride;
		clip.y = s->geom.y;
		xm_clip_rectangle((Widget)w, clip);
		XFillRectangle(dsp, XtWindow(w), w->tree_table.gc_highlight, clip.x, clip.y, clip.width, clip.height);

		XFillRectangle(dsp, XtWindow(w), w->tree_table.gc_inverted_color, clip.x, clip.y, clip.width, clip.height);

		draw_row_cells(w->tree_table.gc_highlight, s->geom.x - translate_scroll_horizontal(w), s->geom.y + s->vertical_stride, tt->p_header, w, s);
		clip.y += clip.height;
	}

	clip.x = s->geom.x;
	clip.width = s->geom.width;
	clip.height = s->geom.height;

	xm_clip_rectangle((Widget)w, clip);
	XFillRectangle(dsp, XtWindow(w), w->tree_table.gc_highlight, clip.x, clip.y, clip.width, clip.height);

	if (s->visible_items_vector && s->visible_items_capacity > 0) {
		int cur_x = -translate_scroll_horizontal(w);
		tt_entry_t *entry = s->visible_items_vector[0].item;
		unsigned position_idx = 0;
		tt_cb_draw_t *ddata = &(((XmTreeTableWidget)w)->tree_table.draw_event_data);
		ddata->user_data = tt->user_data;
		ddata->visible_first = entry ? entry->row_index : -1;
		ddata->visible_last = -1;

		for(; entry && position_idx < s->visible_items_capacity; entry = s->visible_items_vector[++position_idx].item) {
			/* A tree shift added to the Y-position after scrollbar/whole extent relation computation. */
			int cur_y = position_idx * s->vertical_stride + y_shift;

			if (entry->flags.is_hidden)
				continue;
			draw_row(entry, cur_x, cur_y, tt->gc_draw, w, s);
			ddata->visible_last = entry->row_index;
		}
	}
}

void xm_render_ttwidget(Widget w)
{
	XtWidgetGeometry geom;
	XmTreeTablePart *tp = &(((XmTreeTableWidget)w)->tree_table);
	struct render_target_s *s = &(tp->render_attr);
	XtGeometryResult geom_result = XtQueryGeometry((Widget) (w), (XtWidgetGeometry *)NULL, &geom);

	if (0 == ((XtGeometryYes | XtGeometryDone) & geom_result))
		return;
	if (tp->table_access_padlock) {
		tp->table_access_padlock->lock(tp->table, tp->table_access_padlock->p_user_data);
	}

	if (s->geom.width != geom.width || s->geom.height != geom.height
			|| s->geom.x != geom.x || s->geom.y != geom.y) {
		xm_extent_prediction((XmTreeTableWidget)w);
	}
	s->geom.x = geom.x;
	s->geom.y = geom.y;
	s->geom.width = geom.width;
	s->geom.height = geom.height;

	xm_render_ttwidget_contents(w, e_what_window);

	if (tp->table_access_padlock) {
		tp->table_access_padlock->unlock(tp->table, tp->table_access_padlock->p_user_data);
	}

	if (tp->p_draw_handler)
		tp->p_draw_handler(&tp->draw_event_data);
}

int xm_find_row_pointed_by_mouse(Widget w, int y)
{
	XmTreeTablePart *tt = &(((XmTreeTableWidget)w)->tree_table);
	item_desc_t *et = NULL, *found = NULL;
	unsigned idx = 0;
	struct render_target_s *s = &tt->render_attr;
	if (tt->p_header) {
		if (y <= s->vertical_stride)
			return -1;
	}

	if (!s->visible_items_vector || !s->visible_items_vector[0].item)
		goto lb_row_fond;

	for(et = s->visible_items_vector + idx; et && idx < s->visible_items_count; et = s->visible_items_vector + (++idx)) {
		if (et->display_region.y <= y && y < (et->display_region.y + et->display_region.height)) {
			found = et;
			break;
		}
	}

lb_row_fond:
	return found ? (unsigned)(found->item->row_index) : -1;
}

void xm_clip_rectangle(Widget w, XRectangle clip)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)w;
	XSetClipRectangles(XtDisplay(w), tw->tree_table.gc_draw, 0, 0, &clip, 1, Unsorted);
	XSetClipRectangles(XtDisplay(w), tw->tree_table.gc_inverted_color, 0, 0, &clip, 1, Unsorted);
	XSetClipRectangles(XtDisplay(w), tw->tree_table.gc_highlight, 0, 0, &clip, 1, Unsorted);
}

/*** scrollbar ***/
void xm_horizontal_scroll_cb(Widget scroll_widget, XtPointer client_data, XtPointer call_data)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)client_data;
	XmTreeTablePart *tp = &(tw->tree_table);
	xm_tt_scrollbar *tt_bar = &tp->w_horiz_sbar;

	XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;
	(void)scroll_widget;
	if (tp->table_access_padlock) {
		tp->table_access_padlock->lock(tp->table, tp->table_access_padlock->p_user_data);
	}

	tt_bar->prev = tt_bar->cur;
	tt_bar->cur = cbs->value;
	xm_render_ttwidget_contents((Widget)tw, e_what_horizontal_scroll);

	if (tp->table_access_padlock) {
		tp->table_access_padlock->unlock(tp->table, tp->table_access_padlock->p_user_data);
	}

	tp->draw_event_data.user_data = tp->user_data;
	tp->draw_event_data.type = ett_scroll_horizontal;
	if (tp->p_draw_handler)
		tp->p_draw_handler(&tp->draw_event_data);

}

void xm_vertical_scroll_cb(Widget scroll_widget, XtPointer client_data, XtPointer call_data)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)client_data;
	XmTreeTablePart *tp = &(tw->tree_table);
	xm_tt_scrollbar *tt_bar = &tp->w_vert_sbar;

	XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;
	(void)scroll_widget;

	if (tp->table_access_padlock) {
		tp->table_access_padlock->lock(tp->table, tp->table_access_padlock->p_user_data);
	}

	tt_bar->prev = tt_bar->cur;
	tt_bar->cur = cbs->value;
	xm_render_ttwidget_contents((Widget)tw, e_what_vertical_scroll);

	if (tp->table_access_padlock) {
		tp->table_access_padlock->unlock(tp->table, tp->table_access_padlock->p_user_data);
	}

	tp->draw_event_data.user_data = tp->user_data;
	tp->draw_event_data.type = ett_scroll_vertical;
	if (tp->p_draw_handler)
		tp->p_draw_handler(&tp->draw_event_data);
}

void xm_init_scrollbars(XmTreeTableWidget w)
{
	Widget w_scrolled = XtParent(w);
	char name_buf[128] = { '\0' };

	const char *widget_name = XtName((Widget)w);
	int name_len = TTBL_MIN(120, strlen(widget_name));
	Widget vbar = NULL, hbar = NULL;
	char *cb_strings[6] = { XmNdecrementCallback, XmNdragCallback, XmNincrementCallback,
		XmNpageDecrementCallback, XmNpageIncrementCallback, XmNvalueChangedCallback
	};
	strcpy(name_buf, widget_name);
	strcpy(name_buf + name_len, "_v_scroll");

	vbar = XtVaCreateManagedWidget(name_buf, xmScrollBarWidgetClass, w_scrolled, XmNorientation, XmVERTICAL, NULL);
	hbar = XtVaCreateManagedWidget(name_buf, xmScrollBarWidgetClass, w_scrolled, XmNorientation, XmHORIZONTAL, NULL);

	{
		unsigned i = 0;
		for(; i < 6; ++i) {
			XtAddCallback(vbar, cb_strings[i], xm_vertical_scroll_cb, (XtPointer)w);
			XtAddCallback(hbar, cb_strings[i], xm_horizontal_scroll_cb, (XtPointer)w);
		}
	}

	XtAddCallback(vbar, XmNtoBottomCallback, xm_vertical_scroll_cb, (XtPointer)w);
	XtAddCallback(vbar, XmNtoTopCallback, xm_vertical_scroll_cb, (XtPointer)w);

	/* some minimal defaults, while the window hasn't got re-sized. */
	XtVaSetValues(vbar, XmNvalue, 0, XmNsliderSize, 1, XmNpageIncrement, 1, XmNminimum, 0, XmNmaximum, 1, NULL);
	XtVaSetValues(hbar, XmNvalue, 0, XmNsliderSize, 1, XmNpageIncrement, 1, XmNminimum, 0, XmNmaximum, 1, NULL);

	{
		xm_tt_scrollbar *tt_bar = &(w->tree_table.w_vert_sbar);
		memset(tt_bar, 0x00, sizeof(xm_tt_scrollbar));
		tt_bar->sbar = vbar;
		tt_bar->lo = 0;
		tt_bar->hi = 1;
		tt_bar->incr = 1;
		/* horizontal */
		strcpy(name_buf + name_len, "_h_scroll");

		tt_bar = &(w->tree_table.w_horiz_sbar);
		memset(tt_bar, 0x00, sizeof(xm_tt_scrollbar));
		tt_bar->sbar = hbar;
		tt_bar->lo = 0;
		tt_bar->hi = 1;
		tt_bar->incr = 1;
	}
	XtVaSetValues(w_scrolled, XmNscrollBarDisplayPolicy, XmSTATIC, XmNscrollingPolicy, XmAPPLICATION_DEFINED, XmNvisualPolicy, XmVARIABLE,
								/* Instead of call to XmScrolledWindowSetAreas() */
								XmNworkWindow, w, XmNhorizontalScrollBar, hbar, XmNverticalScrollBar, vbar, NULL);


}

void xm_fit_scrollbars_to_geometry(XmTreeTableWidget w, struct render_target_s *s)
{
	XmTreeTablePart *tt = &(w->tree_table);

	/* update scroll bar variables */
	{
		xm_tt_scrollbar *vsb = &(tt->w_vert_sbar);
		vsb->lo = 0;
		vsb->size = TTBL_MAX(1, s->geom.height * s->geom.height / tt->virtual_canvas_size.height);
		vsb->hi = s->geom.height + vsb->size;
		vsb->incr = s->vertical_stride;
		if (vsb->incr * s->geom.height < tt->virtual_canvas_size.height)
			vsb->incr = TTBL_MIN(s->geom.height, tt->virtual_canvas_size.height / s->geom.height);
		vsb->cur = TTBL_CLAMP(vsb->cur, vsb->lo, vsb->hi - vsb->size);
		vsb->prev = TTBL_CLAMP(vsb->prev, vsb->lo, vsb->hi - vsb->size);
		XtVaSetValues(vsb->sbar, XmNvalue, vsb->cur, XmNsliderSize, vsb->size, XmNpageIncrement, 1, XmNminimum, vsb->lo, XmNmaximum, vsb->hi, NULL);
	}

	{
		xm_tt_scrollbar *hsb = &(tt->w_horiz_sbar);
		hsb->lo = 0;
		hsb->size = TTBL_MAX(1, s->geom.width * s->geom.width / tt->virtual_canvas_size.width);
		hsb->hi = s->geom.width + hsb->size;

		hsb->incr = tt->n_max_pixmap_height;
		if (hsb->incr * s->geom.width < tt->virtual_canvas_size.width)
			hsb->incr = TTBL_MIN(s->geom.width, tt->virtual_canvas_size.width / s->geom.width);

		hsb->cur = TTBL_CLAMP(hsb->cur, hsb->lo, hsb->hi - hsb->size);
		hsb->prev = TTBL_CLAMP(hsb->prev, hsb->lo, hsb->hi - hsb->hi);
		XtVaSetValues(hsb->sbar, XmNvalue, hsb->cur, XmNsliderSize, hsb->size, XmNpageIncrement, 1, XmNminimum, hsb->lo, XmNmaximum, hsb->hi, NULL);
	}
}

/*** Text extent compute ***/
static unsigned count_new_lines(const char *str, unsigned slen)
{
	unsigned idx = 0, cnt = 0;
	for(; idx < slen; ++idx) {
		cnt += (unsigned)('\n' == str[idx]);
	}
	return cnt;
}

static void xm_extent_prediction_item(tt_entry_t *entry, XmTreeTableWidget w, struct render_target_s *s)
{
	XmTreeTablePart *tt = &(w->tree_table);
	XCharStruct chrs = { 0 };
	int i_ret[3] = { 0 };
	unsigned col = 0;
	long len_diff = 0;
	const char *str = NULL;
	if (!(entry) || (entry->flags.is_hidden) || !entry->n_cells)
		return;

	len_diff = (long)entry->n_cells - (long)s->column_vector_len;
	if (0 <= len_diff) {
		if (!s->column_dimensions_vector) {
			s->column_dimensions_vector = (long *)calloc(entry->n_cells, sizeof(long));
		}
		else {
			s->column_dimensions_vector = (long *)realloc(s->column_dimensions_vector, sizeof(long) * entry->n_cells);
			memset(s->column_dimensions_vector + s->column_vector_len, 0x00, sizeof(int) * (entry->n_cells - s->column_vector_len));
		}
		s->column_vector_len = entry->n_cells;
	}

	str = tt_get_cell(entry, 0)[0];
	tt->virtual_canvas_size.x = s->geom.x;
	tt->virtual_canvas_size.height += s->vertical_stride + tt->n_grid_y_gap_pixels;
	if (str) {
		int slen = strlen(str);
		int level_n_pixmap_shift = entry->level * s->horizontal_stride;
		entry->n_text_lines = TTBL_MAX(1, count_new_lines(str, slen));
		XTextExtents(tt->font, str, slen, i_ret, i_ret + 1, i_ret + 2, &chrs);

		/* compute the string extent of the tree branches' names, taking into account the indent. */
		tt->virtual_canvas_size.width = TTBL_MAX(tt->virtual_canvas_size.width, level_n_pixmap_shift + chrs.width);

		s->column_dimensions_vector[0] = TTBL_MAX(chrs.width + level_n_pixmap_shift, s->column_dimensions_vector[0]);
	}
	else {
		tt->virtual_canvas_size.width = TTBL_MAX(tt->virtual_canvas_size.width, (1 + entry->level) * s->horizontal_stride);
		s->column_dimensions_vector[0] = TTBL_MAX(tt->n_minimum_cell_width, s->column_dimensions_vector[0]);
	}

	/* table horizontal extent */

	{
		for(col = 1; col < entry->n_cells; ++col) {
			const char *str = tt_get_cell(entry, (unsigned)col)[0];
			if (!str) {
				s->column_dimensions_vector[col] = TTBL_MAX(s->column_dimensions_vector[col], tt->n_minimum_cell_width);
			}
			else {
				int slen = strlen(str);
				entry->n_text_lines = TTBL_MAX(1, count_new_lines(str, slen));
				XTextExtents(tt->font, str, slen, i_ret, i_ret + 1, i_ret + 2, &chrs);
				s->column_dimensions_vector[col] = TTBL_MAX(s->column_dimensions_vector[col], chrs.width);
			}
		}
		tt->virtual_canvas_size.width = 0;
	}

}

void xm_extent_prediction(XmTreeTableWidget w)
{
	XmTreeTablePart *tt = &(w->tree_table);
	struct render_target_s *s = &tt->render_attr;
	long row_index = -1;
	unsigned col = 0;
	{
		/* a stub for uninitialized window geometry, it'll be recomputed later. */
		if (0 == s->geom.width)
			s->geom.width = 100;
		if (0 == s->geom.height)
			s->geom.height = 100;
	}
	tt->virtual_canvas_size.x = s->geom.x;
	tt->virtual_canvas_size.y = s->geom.y;
	tt->draw_event_data.user_data = tt->user_data;
	tt->draw_event_data.tree_table_widget = (Widget)w;
	tt->draw_event_data.widget_geometry = s->geom;
	tt->draw_event_data.root = tt->table;
	tt->draw_event_data.type = ett_render_finished;
	s->vertical_stride = TTBL_MAX(s->vertical_stride, TTBL_MAX(w->tree_table.n_max_pixmap_height, GET_FONT_HEIGHT(tt->font)) + tt->n_grid_y_gap_pixels);
	s->horizontal_stride = TTBL_MAX(tt->n_max_pixmap_height, s->horizontal_stride);

	if (tt->table && gdl_first(tt->table)) {
		tt->draw_event_data.visible_last = tt->draw_event_data.visible_first = 0;
	}

	if (s->column_dimensions_vector) {
		memset(s->column_dimensions_vector, 0x00, s->column_vector_len * sizeof(long));
	}

	tt->virtual_canvas_size.height = tt->p_header ? s->vertical_stride : 0;
	{
		tt_entry_t *entry = (tt_entry_t *)gdl_first(tt->table);
		/* computes widths for both the tree & the table parts. */
		for(; entry; entry = (tt_entry_t *)gdl_next(tt->table, (void *)entry)) {
			entry->row_index = ++row_index;
			xm_extent_prediction_item(entry, w, s);
		}
	}
	/* the data list could be empty, but we need this vector to have at least one 0-filled entry. */
	if (!s->column_dimensions_vector) {
		s->column_dimensions_vector = (long*)malloc(sizeof(long));
		s->column_vector_len = 1;
	}

	tt->virtual_canvas_size.width = 0;
	for(col = 0; col < s->column_vector_len; ++col) {
		s->column_dimensions_vector[col] += tt->n_grid_x_gap_pixels;
		tt->virtual_canvas_size.width += s->column_dimensions_vector[col];
	}
	if (tt->p_header)
		s->column_dimensions_vector[0] += s->horizontal_stride;

	tt->virtual_canvas_size.height = TTBL_MAX(tt->virtual_canvas_size.height, s->geom.height);
	tt->virtual_canvas_size.width = TTBL_MAX(tt->virtual_canvas_size.width, s->geom.width);
	tt->virtual_canvas_size.width += tt->n_max_pixmap_height;
}
