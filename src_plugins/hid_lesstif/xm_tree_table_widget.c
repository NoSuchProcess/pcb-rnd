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

#include "config.h"

#include "xm_tree_table_priv.h"
#include <string.h>
#include <assert.h>
#include <X11/IntrinsicP.h>
#include <Xm/ScrolledW.h>
#include <Xm/XmP.h>

#define TTBL_MIN(a,b) ((a) < (b)? (a) : (b))
#define TTBL_MAX(a,b) ((a) > (b)? (a) : (b))
#define TTBL_CLAMP(value, lower, higher) TTBL_MIN(TTBL_MAX((value), (lower)), (higher))

/*** model ***/
typedef char *payload_item_t;

const char **tt_get_cell(tt_entry_t *object, unsigned index)
{
	assert(object);
	return index < object->n_cells ? (const char **)(&(object->payload) + sizeof(payload_item_t) * index) : NULL;
}

tt_entry_t *tt_entry_alloc(unsigned n_cells)
{
	tt_entry_t *object = calloc(sizeof(tt_entry_t) + n_cells * sizeof(payload_item_t) - 1, 1);
	object->flags.is_unfolded = 1;
	object->n_cells = n_cells;
	return object;
}

void tt_entry_link(gdl_list_t *list, tt_entry_t *entry)
{
	assert(list && entry);
	gdl_append(list, entry, gdl_linkfield);
}

tt_entry_t *new_tt_entry(gdl_list_t *list, unsigned n_cells)
{
	tt_entry_t *object = tt_entry_alloc(n_cells);
	assert(list);
	gdl_append(list, object, gdl_linkfield);
	return object;
}

void delete_tt_entry(gdl_list_t *list, tt_entry_t *item)
{
	/* detach & free current item from parent's list, if any. */
	assert(list && item);
	gdl_remove(list, item, gdl_linkfield);
	free(item);
}


/*** widget ***/

typedef struct xm_tree_table_rendering_s
{
	void *dummy;
} xm_tree_table_rendering_t;

/* widget methods - CamelCase naming used for Xm-related callbacks & structures.*/
static void Initialize(Widget request, Widget tnew, ArgList args, Cardinal *num);
static void Destroy(XmTreeTableWidget w);
static void Redisplay(Widget aw, XExposeEvent *event, Region region);
static void Resize(XmTreeTableWidget w);
static Boolean SetValues(Widget current, Widget request, Widget reply,
                         ArgList args, Cardinal *nargs);
static void Realize(Widget aw, XtValueMask *value_mask, XSetWindowAttributes *attributes);
static XtGeometryResult QueryGeometry(XmTreeTableWidget w, XtWidgetGeometry *proposed, XtWidgetGeometry *answer);

/*helper methods: return 0 on success.*/
int init_pixmaps(XmTreeTableWidget w);
int make_pixmap_data(XmTreeTableWidget w, Pixinfo *pix);
int free_pixmap_data(XmTreeTableWidget w, Pixinfo *pix);

/*actions*/
static void lmb_drag(Widget aw, XEvent *event, String *params, Cardinal *num_params);
static void mmb_drag(Widget aw, XEvent *event, String *params, Cardinal *num_params);
static void rmb_drag(Widget aw, XEvent *event, String *params, Cardinal *num_params);
static void keypress(Widget aw, XEvent *event, String *, Cardinal *);

static char defaultTranslations[] = "\
<Btn1Motion>:lmb_drag()\n\
<Btn2Motion>:mmb_drag()\n\
<Btn3Motion>:rmb_drag()\n\
<Btn1Down>:keypress()\n\
<Btn2Down>:keypress()\n\
<Btn3Down>:keypress()\n\
<Key>:keypress()\n\
<Key>Escape:keypress()\n\
<Key>Return:keypress()\n\
<Key>space:keypress()\n\
<KeyUp>:keypress()\n\
<KeyDown>:keypress()\n\
<Key>osfLeft:keypress()\n\
<Key>osfRight:keypress()\n\
<Key>Tab:keypress()\n\
<Key>osfBeginLine:keypress()\n\
<Key>osfEndLine:keypress()\n\
<Key>osfInsert:keypress()\n\
<Key>osfDelete:keypress()\n\
<Key>osfPageUp:keypress()\n\
<Key>osfPageDown:keypress()\n\
";

static XtActionsRec actions[] =
{
	{"keypress", keypress},
	{"lmb_drag", lmb_drag},
	{"mmb_drag", mmb_drag},
	{"rmb_drag", rmb_drag}
};

#define offset(field) XtOffsetOf(XmTreeTableRec, tree_table.field)

static XtResource resources[] =
{
	{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), offset(foreground_pixel), XtRString, XtDefaultForeground},
	{XtNhorizontalSpacing, XtCMargin, XtRDimension, sizeof(Dimension), offset(h_spacing), XtRImmediate, (XtPointer) 2},
	{XtNverticalSpacing, XtCMargin, XtRDimension, sizeof(Dimension), offset(v_spacing), XtRImmediate, (XtPointer) 0},
	{XtNlineWidth, XtCMargin, XtRDimension, sizeof(Dimension), offset(line_width), XtRImmediate, (XtPointer) 1},
	{XtNpixWidth, XtCMargin, XtRDimension, sizeof(int), offset(pix_width), XtRInt, (XtPointer) 0},
	{XtNbranchPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap), offset(pix_branch_closed.bitmap), XtRImmediate, (XtPointer) XtUnspecifiedPixmap},
	{XtNbranchOpenPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap), offset(pix_branch_open.bitmap), XtRImmediate, (XtPointer) XtUnspecifiedPixmap},
	{XtNleafPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap), offset(pix_leaf.bitmap), XtRImmediate, (XtPointer) XtUnspecifiedPixmap},
	{XtNleafOpenPixmap, XtCPixmap, XtRBitmap, sizeof(Pixmap), offset(pix_leaf_open.bitmap), XtRImmediate, (XtPointer) XtUnspecifiedPixmap},
};

XmTreeTableClassRec xmTreeTableClassRec =
{
	{
		/* core_class fields     */
		/* MOTIF superclass      */ (WidgetClass) &xmPrimitiveClassRec,
		/* class_name            */ "xmTreeTable",
		/* widget_size           */ sizeof(XmTreeTableRec),
		/* class_initialize      */ NULL,
		/* class_part_initialize */ NULL,
		/* class_inited          */ False,
		/* initialize            */ Initialize,
		/* initialize_hook       */ NULL,
		/* realize               */ Realize,
		/* actions               */ actions,
		/* num_actions           */ XtNumber(actions),
		/* resources             */ resources,
		/* num_resources         */ XtNumber(resources),
		/* xrm_class             */ NULLQUARK,
		/* compress_motion       */ True,
		/* compress_exposure     */ XtExposeCompressMultiple,
		/* compress_enterleave   */ True,
		/* visible_interest      */ False,
		/* destroy               */ (XtWidgetProc)Destroy,
		/* resize                */ (XtWidgetProc)Resize,
		/* expose                */ (XtExposeProc)Redisplay,
		/* set_values            */ SetValues,
		/* set_values_hook       */ NULL,
		/* set_values_almost     */ XtInheritSetValuesAlmost,
		/* get_values_hook       */ NULL,
		/* accept_focus          */ NULL,
		/* version               */ XtVersion,
		/* callback_private      */ NULL,
		/* tm_table              */ defaultTranslations,
		/* query_geometry        */ (XtGeometryHandler)QueryGeometry,
		/* display_accelerator   */ XtInheritDisplayAccelerator,
		/* extension             */ NULL,
	},
		/* Primitive Class part */
	{
		/* border_highlight      */ XmInheritBorderHighlight,
		/* border_unhighlight    */ XmInheritBorderUnhighlight,
		/* translations          */ XtInheritTranslations,
		/* arm_and_activate_proc */ XmInheritArmAndActivate,
		/* synthetic resources   */ NULL,
		/* num syn res           */ 0,
		/* extension             */ NULL,
	},
	{
		/* some stupid compilers barf on empty structures */ 0
	}
};

WidgetClass xmTreeTableWidgetClass = (WidgetClass) &xmTreeTableClassRec;

static void xt_gc_init(XmTreeTableWidget w)
{
	XGCValues values;
	XtGCMask mask;

	assert(w->tree_table.font);
	values.line_style = LineSolid;
	values.line_width = w->tree_table.line_width;
	values.fill_style = FillSolid;
	values.font = w->tree_table.font->fid;
	values.background = w->core.background_pixel;
	values.foreground = w->tree_table.foreground_pixel;

	mask = GCLineStyle | GCLineWidth | GCFillStyle | GCForeground | GCBackground | GCFont;
	w->tree_table.gc_draw = XtGetGC((Widget) w, mask, &values);

	values.function = GXinvert;
	mask = GCLineStyle | GCLineWidth | GCFillStyle | GCForeground | GCBackground | GCFont | GCFunction;
	w->tree_table.gc_inverted_color = XtGetGC((Widget) w, mask, &values);

	values.background = w->tree_table.foreground_pixel;
	values.foreground = w->core.background_pixel;
	mask = GCLineStyle | GCLineWidth | GCFillStyle | GCForeground | GCBackground | GCFont;
	w->tree_table.gc_highlight = XtGetGC((Widget) w, mask, &values);
}

static void init_xm_view_variables(XmTreeTableWidget w)
{
	if (XtHeight(w) < 10)
	{
		XtWidth(w) = 240;
		XtHeight(w) = 240;
	}
}


static void Initialize(Widget request, Widget tnew, ArgList args, Cardinal *num)
{
	XmTreeTableWidget w = (XmTreeTableWidget) tnew;
	XmTreeTablePart *tt = &(w->tree_table);
	tt->table = NULL;
	memset(&(tt->render_attr), 0x00, sizeof(tt->render_attr));
	tt->table_access_padlock = NULL;

	xm_init_render_target(&(w->tree_table.render_attr));
	tt->font = NULL;
	XmeRenderTableGetDefaultFont(XmeGetDefaultRenderTable(tnew, XmTEXT_RENDER_TABLE), &tt->font);

	/* init Xt GC structures for drawing routines. */
	xt_gc_init(w);
	/* allocate pixmaps with branch/leaf depicted. */
	init_pixmaps(w);

	/* 0-fill "virtual" extent rectangles.*/
	tt->n_minimum_cell_width = 60;
	tt->n_grid_line_thickness = 1;
	tt->n_grid_x_gap_pixels = 5;
	tt->n_grid_y_gap_pixels = 0;
	tt->b_show_tree = 1;
	memset(&tt->event_data, 0x00, sizeof(tt_table_event_data_t));
	tt->p_mouse_kbd_handler = NULL;
	/* ---- public API-related vars */
	tt->b_table_grid_visible = True;
	init_xm_view_variables(w);
	xm_init_scrollbars(w);

	tt->p_header = NULL;
}
/*--end of initialize-related------------------------------------------------*/
static void Destroy(XmTreeTableWidget w)
{
	XmTreeTablePart *tt = &(w->tree_table);
	xm_clear_render_target(&(tt->render_attr));
	if (tt->p_header)
		free(tt->p_header);
	free_pixmap_data(w, &tt->pix_branch_closed);
	free_pixmap_data(w, &tt->pix_branch_open);
	free_pixmap_data(w, &tt->pix_leaf);
	free_pixmap_data(w, &tt->pix_leaf_open);

	XtReleaseGC((Widget)w, tt->gc_draw);
	XtReleaseGC((Widget)w, tt->gc_highlight);
	XtReleaseGC((Widget)w, tt->gc_inverted_color);
}
/*---------------------------------------------------------------------------*/
static void Redisplay(Widget aw, XExposeEvent *event, Region region)
{
	XmTreeTableWidget w = (XmTreeTableWidget)aw;
	if (!XtIsRealized((Widget) aw))
		return;
	{
		XRectangle clip;
		clip.x = 0;
		clip.y = 0;
		clip.width = XtWidth(w);
		clip.height = XtHeight(w);
		xm_clip_rectangle(aw, clip);
	}
	xm_render_ttwidget(aw);
}
/*---------------------------------------------------------------------------*/
static void Resize(XmTreeTableWidget w)
{
	Redisplay((Widget)w, NULL, NULL);
}
static Boolean SetValues(Widget current, Widget request, Widget reply, ArgList args, Cardinal *nargs)
{
	return False;
}
static void Realize(Widget aw, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
#define	superclass	(&xmPrimitiveClassRec)
  (*superclass->core_class.realize) (aw, value_mask, attributes);
#undef	superclass
	Redisplay((Widget)aw, NULL, NULL);
}

static XtGeometryResult QueryGeometry(XmTreeTableWidget w, XtWidgetGeometry *proposed, XtWidgetGeometry *answer)
{
	return XtGeometryNo;
}

static void reset_event_data_row_ptr(XmTreeTablePart *tp)
{
	tp->event_data.root_entry = tp->table;
	tp->event_data.user_data = tp->user_data;
	tp->event_data.current_row = 0;
	tp->event_data.current_cell = 0;
	tp->event_data.event = NULL;
	tp->event_data.type = ett_none;
}

static void on_mouse_action(ett_x11_event_t ett_event, Widget aw, XEvent *event, String *params, Cardinal *num_params)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)aw;
	XmTreeTablePart *tp = &(tw->tree_table);

	tp->event_data.type = ett_event;
	tp->event_data.user_data = tp->user_data;
	tp->event_data.current_widget = aw;
	tp->event_data.event = event;
	tp->event_data.strings = params;
	tp->event_data.strings_number = num_params;

	tp->event_data.position.mouse.x = event->xbutton.x - tp->render_attr.geom.x;
	tp->event_data.position.mouse.y = event->xbutton.y - tp->render_attr.geom.y;
	tp->event_data.position.mouse.width = tp->render_attr.geom.width;
	tp->event_data.position.mouse.height = tp->render_attr.geom.height;

	tp->event_data.root_entry = tp->table;
	if (tp->table_access_padlock) {
		tp->table_access_padlock->lock(tp->table, tp->table_access_padlock->p_user_data);
	}

	tp->event_data.current_row = xm_find_row_pointed_by_mouse(aw, tp->event_data.position.mouse.y);
	tp->event_data.current_cell = 0;
	if (0 <= tp->event_data.current_row) {
		unsigned idx = 0;
		long coord_x = SCROLL_TR(0, tp->virtual_canvas_size.width, tp->w_horiz_sbar.cur, tp->w_horiz_sbar.lo, tp->w_horiz_sbar.hi);
		for (; idx < tp->render_attr.column_vector_len && coord_x < tp->event_data.position.mouse.x; ++idx) {
			coord_x += tp->render_attr.column_dimensions_vector[idx];
		}
		tp->event_data.current_cell = idx ? (--idx) : 0;
	}

	if (tp->table_access_padlock) {
		tp->table_access_padlock->unlock(tp->table, tp->table_access_padlock->p_user_data);
	}
	if (tp->p_mouse_kbd_handler)
		tp->p_mouse_kbd_handler(&tp->event_data);
}

#define XM_TT_LOCKED_CODE(WIDGET_PARAM, XMTTWIDGET_NAME, XMTTPART_NAME, CODE) \
{\
{\
	XmTreeTableWidget (XMTTWIDGET_NAME) = (XmTreeTableWidget)(WIDGET_PARAM);\
	XmTreeTablePart *(XMTTPART_NAME) = &((XMTTWIDGET_NAME)->tree_table); \
	tt_table_access_cb_t *plock__ = (XMTTPART_NAME)->table_access_padlock;\
	if (plock__)\
		{ plock__->lock((XMTTPART_NAME)->table, plock__->p_user_data); }\
\
	do {CODE} while (0);\
\
	if (plock__)\
		{ plock__->unlock((XMTTPART_NAME)->table, plock__->p_user_data); }\
}\
}

static void lmb_drag(Widget aw, XEvent *event, String *params, Cardinal *num_params)
{
	event->xbutton.button = Button1;
	on_mouse_action(ett_mouse_btn_drag, aw, event, params, num_params);
}

static void mmb_drag(Widget aw, XEvent *event, String *params, Cardinal *num_params)
{
	event->xbutton.button = Button2;
	on_mouse_action(ett_mouse_btn_drag, aw, event, params, num_params);
}

static void rmb_drag(Widget aw, XEvent *event, String *params, Cardinal *num_params)
{
	event->xbutton.button = Button3;
	on_mouse_action(ett_mouse_btn_drag, aw, event, params, num_params);
}

/* ARGSUSED */
static void keypress(Widget aw, XEvent *event, String *strings, Cardinal *number)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)aw;
	XmTreeTablePart *tp = &(tw->tree_table);
	reset_event_data_row_ptr(tp);
	tp->event_data.user_data = tp->user_data;
	tp->event_data.current_widget = aw;
	tp->event_data.event = event;
	tp->event_data.type = ett_none;
	while (Button1 == event->xbutton.button || Button2 == event->xbutton.button || Button3 == event->xbutton.button)
	{
		ett_x11_event_t te = ett_none;
		if (ButtonPress & event->type)
			te = ett_mouse_btn_down;
		else if (ButtonReleaseMask & event->type)
			te = ett_mouse_btn_up;
		else
		{
			break;
		}
		tp->event_data.type = te;
		on_mouse_action(te, aw, event, strings, number);
		return;
	}
	tp->event_data.type = ett_key;
	if (tp->p_mouse_kbd_handler)
		tp->p_mouse_kbd_handler(&tp->event_data);
}

Widget xm_create_tree_table_widget(Widget parent, gdl_list_t *table_root,
	void *user_data,
	tt_table_mouse_kbd_handler mouse_kbd_handler,
	tt_table_draw_handler draw_status_handler)
{
	return xm_create_tree_table_widget_cb(parent, table_root,
		user_data, mouse_kbd_handler, draw_status_handler, NULL);
}

Widget xm_create_tree_table_widget_cb(Widget parent, gdl_list_t *table_root,
	void *user_data,
	tt_table_mouse_kbd_handler mouse_kbd_handler,
	tt_table_draw_handler draw_status_handler,
	tt_table_access_cb_t *access_padlock)
{
	Widget scroll_w = NULL;
	Widget table_widget = NULL;
	Arg args[4];
	const char *widget_name = "tree_table_widget";

	XtSetArg(args[0], XmNscrollingPolicy, XmAPPLICATION_DEFINED);
	XtSetArg(args[1], XmNvisualPolicy, XmVARIABLE);
	XtSetArg(args[2], XmNscrollBarDisplayPolicy, XmSTATIC);
	XtSetArg(args[3], XmNshadowThickness, 0);

	scroll_w = XtCreateManagedWidget(widget_name, xmScrolledWindowWidgetClass, parent, args, 4);

	table_widget = XtCreateWidget(widget_name, xmTreeTableWidgetClass, scroll_w, NULL, 0);
	{
		XmTreeTablePart *tp = &((XmTreeTableWidget)table_widget)->tree_table;
		tp->user_data = user_data;
		tp->table = table_root;
		tp->table_access_padlock = access_padlock;
		tp->event_data.user_data = user_data;
		tp->p_mouse_kbd_handler = mouse_kbd_handler;
		tp->p_draw_handler = draw_status_handler;
	}

	xm_extent_prediction((XmTreeTableWidget)table_widget);
	return table_widget;
}

void xm_draw_tree_table_widget(Widget w)
{
	XmTreeTablePart *tpart = &(((XmTreeTableWidget)w)->tree_table);
	XM_TT_LOCKED_CODE(w, tw, tp, xm_render_ttwidget_contents(w, e_what_window);)
	if (tpart->p_draw_handler)
		tpart->p_draw_handler(&tpart->draw_event_data);
}

void xm_set_tree_table_pointer(Widget w, gdl_list_t *new_table_root, tt_table_access_cb_t *access_padlock)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)w;
	XmTreeTablePart *tp = &(tw->tree_table);
	gdl_list_t *old_ptr = tp->table;
	tt_table_access_cb_t *old_plock = tp->table_access_padlock;
	if (old_plock && old_ptr)
	{
		old_plock->lock(old_ptr, old_plock->p_user_data);
	}

	tp->table = new_table_root;
	xm_clear_render_target(&tp->render_attr);
	reset_event_data_row_ptr(tp);

	tp->table_access_padlock = access_padlock;
	xm_extent_prediction(tw);
	if (old_plock && old_ptr)
	{
		old_plock->unlock(old_ptr, old_plock->p_user_data);
	}
}

void xm_tt_set_x11_font(Widget xm_tree_table, XFontStruct *new_font)
{
	if (!new_font)
		return;
	XM_TT_LOCKED_CODE(xm_tree_table, tw, tp,
		tp->font = new_font;
	    tp->render_attr.vertical_stride = TTBL_MAX(tp->n_max_pixmap_height, GET_FONT_HEIGHT(tp->font));
	    xm_extent_prediction(tw);
	)
}

void xm_attach_tree_table_header(Widget w, unsigned n_strings, const char **strings)
{
	XM_TT_LOCKED_CODE(w, tw, tp,
		unsigned x;
		if (tp->p_header)
			free(tp->p_header);
		tp->p_header = tt_entry_alloc(n_strings);
		for (x = 0; x < n_strings; ++x)
			tt_get_cell(tp->p_header, x)[0] = strings[x];
	)
}

void xm_tree_table_tree_mode(Widget w, unsigned char mode)
{
	XmTreeTableWidget tw = (XmTreeTableWidget)w;
	XmTreeTablePart *tp = &(tw->tree_table);
	tp->b_show_tree = mode;
}

void xm_tree_table_pixel_gaps(Widget w, unsigned char x, unsigned char y)
{
	XM_TT_LOCKED_CODE(w, tw, tp,
		tp->n_grid_x_gap_pixels = x;
		tp->n_grid_y_gap_pixels = y;
		xm_extent_prediction(tw); )
}

static int is_hidden(tt_entry_t *et)
{
	return et->flags.is_thidden || et->flags.is_uhidden;
}

int xm_tree_table_focus_row(Widget w, int row_index)
{
	int ret = -1;
	XM_TT_LOCKED_CODE(w, tw, tt,
	    struct render_target_s *s = &tt->render_attr;
	    int idx = 0;
	    if (row_index < 0 || !tt->table)
			break;
		/* check if it's already visible*/
		for(; idx < (int)s->len; ++idx)
		{
			item_desc_t* ds = s->visible_items_vector + idx;
			if (ds->item && ds->item->row_index == row_index)
				break;
		}
		if (idx == row_index) {
			ret = 0;
			break;/* it's already visible*/
		}
		else {
			long occupied_height = 0;
			tt_entry_t* et = (tt_entry_t *)gdl_first(tt->table);
			tt->w_vert_sbar.cur = tt->w_vert_sbar.lo;
			/* find the position */
			for(; et && et->row_index < row_index; et = (tt_entry_t *)gdl_next(tt->table, (void *)et)) {
				tt->w_vert_sbar.cur += !is_hidden(et);
			}
			/* rewind screen.height/2 back. */
			for(; et && occupied_height < s->geom.height / 2; et = (tt_entry_t *)gdl_prev(tt->table, (void *)et)) {
				occupied_height += (!is_hidden(et)) *
				    (et->n_text_lines * s->vertical_stride + tt->n_grid_y_gap_pixels);
				tt->w_vert_sbar.cur -= !is_hidden(et);
			}
			ret = et ? 0 : -1;
		}
	)
	return ret;
}

xm_tt_scrollbar xm_tree_table_scrollbar_vertical_get(Widget table_widget)
{
	xm_tt_scrollbar bar;
	XM_TT_LOCKED_CODE(table_widget, tw, tp, bar = tp->w_vert_sbar;)
	return bar;
}

xm_tt_scrollbar xm_tree_table_scrollbar_horizontal_get(Widget table_widget)
{
	xm_tt_scrollbar bar;
	XM_TT_LOCKED_CODE(table_widget, tw, tp, bar = tp->w_horiz_sbar;)
	return bar;
}

void xm_tree_table_scrollbar_vertical_set(Widget table_widget, int current_value)
{
	XM_TT_LOCKED_CODE(table_widget, tw, tp,
		xm_tt_scrollbar *bar = &tp->w_vert_sbar;
		bar->prev = bar->cur;
		bar->cur = TTBL_CLAMP(current_value, bar->lo, bar->hi - bar->size);)
}

void xm_tree_table_scrollbar_horizontal_set(Widget table_widget, int current_value)
{
	XM_TT_LOCKED_CODE(table_widget, tw, tp,
		xm_tt_scrollbar *bar = &tp->w_horiz_sbar;
		bar->prev = bar->cur;
		bar->cur = TTBL_CLAMP(current_value, bar->lo, bar->hi - bar->size);)
}
