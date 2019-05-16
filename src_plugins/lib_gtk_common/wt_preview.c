/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
 *  pcb-rnd Copyright (C) 2017..2019 Tibor 'Igor2' Palinkas
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
 *
 */

/* This file copied and modified by Peter Clifton, starting from
 * gui-pinout-window.c, written by Bill Wilson for the PCB Gtk port
 * then got a major refactoring by Tibor 'Igor2' Palinkas and Alain in pcb-rnd
 */

#include "config.h"
#include "hidlib_conf.h"

#include "in_mouse.h"
#include "compat.h"
#include "wt_preview.h"

#include "board.h"
#include "data.h"
#include "draw.h"
#include "compat_misc.h"

static void get_ptr(pcb_gtk_preview_t *preview, pcb_coord_t *cx, pcb_coord_t *cy, gint *xp, gint *yp);

static void perview_update_offs(pcb_gtk_preview_t *preview)
{
	double xf, yf;

	xf = (double)preview->view.width / preview->view.canvas_width;
	yf = (double)preview->view.height / preview->view.canvas_height;
	preview->view.coord_per_px = (xf > yf ? xf : yf);

	preview->xoffs = (pcb_coord_t)(preview->view.width / 2 - preview->view.canvas_width * preview->view.coord_per_px / 2);
	preview->yoffs = (pcb_coord_t)(preview->view.height / 2 - preview->view.canvas_height * preview->view.coord_per_px / 2);
}

static void pcb_gtk_preview_update_x0y0(pcb_gtk_preview_t *preview)
{
	preview->x_min = preview->view.x0;
	preview->y_min = preview->view.y0;
	preview->x_max = preview->view.x0 + preview->view.width;
	preview->y_max = preview->view.y0 + preview->view.height;
	preview->w_pixels = preview->view.canvas_width;
	preview->h_pixels = preview->view.canvas_height;

	perview_update_offs(preview);
}

void pcb_gtk_preview_zoomto(pcb_gtk_preview_t *preview, const pcb_box_t *data_view)
{
	void (*orig)(void) = preview->com->pan_common;
	preview->com->pan_common = NULL; /* avoid pan logic for the main window */

	preview->view.width = data_view->X2 - data_view->X1;
	preview->view.height = data_view->Y2 - data_view->Y1;

	if (preview->view.width > preview->view.max_width)
		preview->view.max_width = preview->view.width;
	if (preview->view.height > preview->view.max_height)
		preview->view.max_height = preview->view.height;

	pcb_gtk_zoom_view_win(&preview->view, data_view->X1, data_view->Y1, data_view->X2, data_view->Y2);
	pcb_gtk_preview_update_x0y0(preview);
	preview->com->pan_common = orig;
}

/* modify the zoom level to coord_per_px (clamped), keeping window cursor
   position wx;wy at perview pcb_coord_t position cx;cy */
void pcb_gtk_preview_zoom_cursor(pcb_gtk_preview_t *preview, pcb_coord_t cx, pcb_coord_t cy, int wx, int wy, double coord_per_px)
{
	void (*orig)(void);

	coord_per_px = pcb_gtk_clamp_zoom(&preview->view, coord_per_px);

	if (coord_per_px == preview->view.coord_per_px)
		return;

	orig = preview->com->pan_common;
	preview->com->pan_common = NULL; /* avoid pan logic for the main window */
	preview->view.coord_per_px = coord_per_px;

	preview->view.width = preview->view.canvas_width * preview->view.coord_per_px;
	preview->view.height = preview->view.canvas_height * preview->view.coord_per_px;

	if (preview->view.width > preview->view.max_width)
		preview->view.max_width = preview->view.width;
	if (preview->view.height > preview->view.max_height)
		preview->view.max_height = preview->view.height;

	preview->view.x0 = cx - wx * preview->view.coord_per_px;
	preview->view.y0 = cy - wy * preview->view.coord_per_px;

	pcb_gtk_preview_update_x0y0(preview);
	preview->com->pan_common = orig;
}

void pcb_gtk_preview_zoom_cursor_rel(pcb_gtk_preview_t *preview, pcb_coord_t cx, pcb_coord_t cy, int wx, int wy, double factor)
{
	pcb_gtk_preview_zoom_cursor(preview, cx, cy, wx, wy, preview->view.coord_per_px * factor);
}

static void preview_set_view(pcb_gtk_preview_t *preview)
{
	pcb_box_t view;

	view.X1 = preview->obj->BoundingBox.X1;
	view.Y1 = preview->obj->BoundingBox.Y1;
	view.X2 = preview->obj->BoundingBox.X2;
	view.Y2 = preview->obj->BoundingBox.Y2;
	pcb_gtk_preview_zoomto(preview, &view);
}

static void preview_set_data(pcb_gtk_preview_t *preview, pcb_any_obj_t *obj)
{
	preview->obj = obj;
	if (obj == NULL) {
		preview->w_pixels = 0;
		preview->h_pixels = 0;
		return;
	}

	preview_set_view(preview);
}

enum {
	PROP_GPORT = 2,
	PROP_INIT_WIDGET = 3,
	PROP_EXPOSE = 4,
	PROP_KIND = 5,
	PROP_COM = 7,
	PROP_DIALOG_DRAW = 8, /* for PCB_LYT_DIALOG */
	PROP_DRAW_DATA = 9,
	PROP_CONFIG = 10
};

static GObjectClass *ghid_preview_parent_class = NULL;

/* Initialises the preview object once it is constructed. Chains up in case
   the parent class wants to do anything too. object is the preview object. */
static void ghid_preview_constructed(GObject *object)
{
	if (G_OBJECT_CLASS(ghid_preview_parent_class)->constructed != NULL)
		G_OBJECT_CLASS(ghid_preview_parent_class)->constructed(object);
}

/* Just before the pcb_gtk_preview_t GObject is finalized, free our
   allocated data, and then chain up to the parent's finalize handler. */
static void ghid_preview_finalize(GObject *object)
{
	pcb_gtk_preview_t *preview = PCB_GTK_PREVIEW(object);

	/* Passing NULL for subcircuit data will clear the preview */
	preview_set_data(preview, NULL);

	G_OBJECT_CLASS(ghid_preview_parent_class)->finalize(object);
}

static void ghid_preview_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	pcb_gtk_preview_t *preview = PCB_GTK_PREVIEW(object);
	GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(preview));

	switch (property_id) {
	case PROP_GPORT:
		preview->gport = (void *) g_value_get_pointer(value);
		break;
	case PROP_COM:
		preview->com = (void *) g_value_get_pointer(value);
		break;
	case PROP_INIT_WIDGET:
		preview->init_drawing_widget = (void *) g_value_get_pointer(value);
		break;
	case PROP_EXPOSE:
		preview->expose = (void *) g_value_get_pointer(value);
		break;
	case PROP_DRAW_DATA:
		preview->expose_data.draw_data = g_value_get_pointer(value);
		if (window != NULL)
			gdk_window_invalidate_rect(window, NULL, FALSE);
		break;
	case PROP_DIALOG_DRAW:
		preview->expose_data.expose_cb = (void *) g_value_get_pointer(value);
		break;
	case PROP_CONFIG:
		preview->config_cb = (void *) g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void ghid_preview_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

/* Converter: set up a pinout expose and use the generic preview expose call */
static gboolean ghid_preview_expose(GtkWidget *widget, pcb_gtk_expose_t *ev)
{
	pcb_gtk_preview_t *preview = PCB_GTK_PREVIEW(widget);
	gboolean res;
	int save_fx, save_fy;

	preview->expose_data.view.X1 = preview->x_min;
	preview->expose_data.view.Y1 = preview->y_min;
	preview->expose_data.view.X2 = preview->x_max;
	preview->expose_data.view.Y2 = preview->y_max;
	save_fx = pcbhl_conf.editor.view.flip_x;
	save_fy = pcbhl_conf.editor.view.flip_y;
	conf_force_set_bool(pcbhl_conf.editor.view.flip_x, 0);
	conf_force_set_bool(pcbhl_conf.editor.view.flip_y, 0);

	res = preview->expose(widget, ev, pcbhl_expose_preview, &preview->expose_data);

	conf_force_set_bool(pcbhl_conf.editor.view.flip_x, save_fx);
	conf_force_set_bool(pcbhl_conf.editor.view.flip_y, save_fy);

	return res;
}

/* Override parent virtual class methods as needed and register GObject properties. */
static void ghid_preview_class_init(pcb_gtk_preview_class_t *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *gtk_widget_class = GTK_WIDGET_CLASS(klass);

	gobject_class->finalize = ghid_preview_finalize;
	gobject_class->set_property = ghid_preview_set_property;
	gobject_class->get_property = ghid_preview_get_property;
	gobject_class->constructed = ghid_preview_constructed;

	PCB_GTK_EXPOSE_EVENT_SET(gtk_widget_class, ghid_preview_expose);

	ghid_preview_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);

	g_object_class_install_property(gobject_class, PROP_GPORT, g_param_spec_pointer("gport", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_COM, g_param_spec_pointer("com", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_INIT_WIDGET,
																	g_param_spec_pointer("init-widget", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_EXPOSE, g_param_spec_pointer("expose", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_DIALOG_DRAW, g_param_spec_pointer("dialog_draw", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_DRAW_DATA,
																	g_param_spec_pointer("draw_data", "", "", G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_CONFIG,
																	g_param_spec_pointer("config", "", "", G_PARAM_WRITABLE));
}

static void update_expose_data(pcb_gtk_preview_t *prv)
{
	pcb_gtk_zoom_post(&prv->view);
	prv->expose_data.view.X1 = prv->view.x0;
	prv->expose_data.view.Y1 = prv->view.y0;
	prv->expose_data.view.X2 = prv->view.x0 + prv->view.width;
	prv->expose_data.view.Y2 = prv->view.y0 + prv->view.height;

/*	pcb_printf("EXPOSE_DATA: %$mm %$mm - %$mm %$mm (%f %$mm)\n",
		prv->expose_data.view.X1, prv->expose_data.view.Y1,
		prv->expose_data.view.X2, prv->expose_data.view.Y2,
		prv->view.coord_per_px, prv->view.x0);*/
}


static gboolean preview_configure_event_cb(GtkWidget *w, GdkEventConfigure *ev, void *tmp)
{
	int need_rezoom;
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	preview->win_w = ev->width;
	preview->win_h = ev->height;

	need_rezoom = (preview->view.canvas_width == 0) || (preview->view.canvas_height == 0);

	preview->view.canvas_width = ev->width;
	preview->view.canvas_height = ev->height;

	if (need_rezoom) {
		pcb_box_t b;
		b.X1 = b.Y1 = 0;
		b.X2 = preview->view.width;
		b.Y2 = preview->view.height;
		pcb_gtk_preview_zoomto(preview, &b);
	}
	perview_update_offs(preview);

	if (preview->config_cb != NULL)
		preview->config_cb(preview, w);

/*	update_expose_data(preview);*/
	return TRUE;
}


static gboolean button_press(GtkWidget *w, pcb_hid_cfg_mod_t btn)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	pcb_coord_t cx, cy;
	gint wx, wy;
	get_ptr(preview, &cx, &cy, &wx, &wy);
	void *draw_data = NULL;

	draw_data = preview->expose_data.draw_data;

	switch (btn) {
	case PCB_MB_LEFT:
		if (preview->mouse_cb != NULL) {
/*				pcb_printf("bp %mm %mm\n", cx, cy); */
			if (preview->mouse_cb(w, draw_data, PCB_HID_MOUSE_PRESS, cx, cy))
				gtk_widget_queue_draw(w);
		}
		break;
	case PCB_MB_MIDDLE:
		preview->view.panning = 1;
		preview->grabx = cx;
		preview->graby = cy;
		preview->grabt = time(NULL);
		preview->grabmot = 0;
		break;
	case PCB_MB_SCROLL_UP:
		pcb_gtk_preview_zoom_cursor_rel(preview, cx, cy, wx, wy, 0.8);
		goto do_zoom;
	case PCB_MB_SCROLL_DOWN:
		pcb_gtk_preview_zoom_cursor_rel(preview, cx, cy, wx, wy, 1.25);
		goto do_zoom;
	default:
		return FALSE;
	}
	return FALSE;

do_zoom:;
	update_expose_data(preview);
	gtk_widget_queue_draw(w);

	return FALSE;
}

static gboolean preview_button_press_cb(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
	return button_press(w, ghid_mouse_button(ev->button));
}

static gboolean preview_scroll_cb(GtkWidget *w, GdkEventScroll *ev, gpointer data)
{
	switch (ev->direction) {
	case GDK_SCROLL_UP:
		return button_press(w, PCB_MB_SCROLL_UP);
	case GDK_SCROLL_DOWN:
		return button_press(w, PCB_MB_SCROLL_DOWN);
	default:;
	}
	return FALSE;
}

static gboolean preview_button_release_cb(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	gint wx, wy;
	pcb_coord_t cx, cy;
	void *draw_data = NULL;

	draw_data = preview->expose_data.draw_data;

	get_ptr(preview, &cx, &cy, &wx, &wy);

	switch (ghid_mouse_button(ev->button)) {
	case PCB_MB_MIDDLE:
		preview->view.panning = 0;
		break;
	case PCB_MB_RIGHT:
		if ((preview->mouse_cb != NULL) && (preview->mouse_cb(w, draw_data, PCB_HID_MOUSE_POPUP, cx, cy)))
			gtk_widget_queue_draw(w);
		break;
	case PCB_MB_LEFT:
		if (preview->mouse_cb != NULL) {
/*				pcb_printf("br %mm %mm\n", cx, cy); */
			if (preview->mouse_cb(w, draw_data, PCB_HID_MOUSE_RELEASE, cx, cy))
				gtk_widget_queue_draw(w);
		}
		break;
	default:;
	}
	return FALSE;
}

static gboolean preview_motion_cb(GtkWidget *w, GdkEventMotion *ev, gpointer data)
{
	pcb_gtk_preview_t *preview = (pcb_gtk_preview_t *) w;
	pcb_coord_t cx, cy;
	gint wx, wy;
	void *draw_data = NULL;

	draw_data = preview->expose_data.draw_data;

	get_ptr(preview, &cx, &cy, &wx, &wy);
	if (preview->view.panning) {
		preview->grabmot++;
		preview->view.x0 = preview->grabx - wx * preview->view.coord_per_px;
		preview->view.y0 = preview->graby - wy * preview->view.coord_per_px;
		pcb_gtk_preview_update_x0y0(preview);
		update_expose_data(preview);
		gtk_widget_queue_draw(w);
		return FALSE;
	}

	if (preview->mouse_cb != NULL) {
		if (preview->mouse_cb(w, draw_data, PCB_HID_MOUSE_MOTION, cx, cy))
			gtk_widget_queue_draw(w);
	}
	return FALSE;
}

/*
static gboolean preview_key_press_cb(GtkWidget *preview, GdkEventKey *kev, gpointer data)
{
	printf("kp\n");
}

static gboolean preview_key_release_cb(GtkWidget *preview, GdkEventKey *kev, gpointer data)
{
	printf("kr\n");
}
*/

/* API */

GType pcb_gtk_preview_get_type()
{
	static GType ghid_preview_type = 0;

	if (!ghid_preview_type) {
		static const GTypeInfo ghid_preview_info = {
			sizeof(pcb_gtk_preview_class_t),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_preview_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(pcb_gtk_preview_t),
			0,												/* n_preallocs */
			NULL,											/* instance_init */
		};

		ghid_preview_type =
			g_type_register_static(GTK_TYPE_DRAWING_AREA, "pcb_gtk_preview_t", &ghid_preview_info, (GTypeFlags) 0);
	}

	return ghid_preview_type;
}

GtkWidget *pcb_gtk_preview_new(pcb_gtk_common_t *com, pcb_gtk_init_drawing_widget_t init_widget,
																			pcb_gtk_preview_expose_t expose, pcb_hid_expose_t dialog_draw, pcb_gtk_preview_config_t config, void *draw_data)
{
	pcb_gtk_preview_t *prv = (pcb_gtk_preview_t *)g_object_new(
		PCB_GTK_TYPE_PREVIEW,
		"com", com, "gport", com->gport, "init-widget", init_widget,
		"expose", expose, "dialog_draw", dialog_draw,
		"config", config, "draw_data", draw_data,
		"width-request", 50, "height-request", 50,
		NULL);
	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);


TODO(": maybe expose these through the object API so the caller can set it up?")
	memset(&prv->view, 0, sizeof(prv->view));
	prv->view.width = PCB_MM_TO_COORD(110);
	prv->view.height = PCB_MM_TO_COORD(110);
	prv->view.use_max_pcb = 0;
	prv->view.max_width = PCB_MM_TO_COORD(110);
	prv->view.max_height = PCB_MM_TO_COORD(110);
	prv->view.coord_per_px = PCB_MM_TO_COORD(0.25);
	prv->view.com = com;

	update_expose_data(prv);

	prv->init_drawing_widget(GTK_WIDGET(prv), prv->gport);

	gtk_widget_add_events(GTK_WIDGET(prv), GDK_EXPOSURE_MASK | GDK_SCROLL_MASK
												| GDK_LEAVE_NOTIFY_MASK | GDK_ENTER_NOTIFY_MASK | GDK_BUTTON_RELEASE_MASK
												| GDK_BUTTON_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_KEY_PRESS_MASK
												| GDK_FOCUS_CHANGE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);


	g_signal_connect(G_OBJECT(prv), "button_press_event", G_CALLBACK(preview_button_press_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "button_release_event", G_CALLBACK(preview_button_release_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "scroll_event", G_CALLBACK(preview_scroll_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "configure_event", G_CALLBACK(preview_configure_event_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "motion_notify_event", G_CALLBACK(preview_motion_cb), NULL);

/*
	g_signal_connect(G_OBJECT(prv), "key_press_event", G_CALLBACK(preview_key_press_cb), NULL);
	g_signal_connect(G_OBJECT(prv), "key_release_event", G_CALLBACK(preview_key_release_cb), NULL);
*/

	return GTK_WIDGET(prv);
}

void pcb_gtk_preview_get_natsize(pcb_gtk_preview_t *preview, int *width, int *height)
{
	*width = preview->w_pixels;
	*height = preview->h_pixels;
}

/* Has to be at the end since it undef's SIDE_X */
static void get_ptr(pcb_gtk_preview_t *preview, pcb_coord_t *cx, pcb_coord_t *cy, gint *xp, gint *yp)
{
	gdkc_window_get_pointer(GTK_WIDGET(preview), xp, yp, NULL);
#undef SIDE_X
#undef SIDE_Y
#define SIDE_X(x) x
#define SIDE_Y(y) y
	*cx = EVENT_TO_PCB_X(&preview->view, *xp) + preview->xoffs;
	*cy = EVENT_TO_PCB_Y(&preview->view, *yp) + preview->yoffs;
#undef SIDE_X
#undef SIDE_Y
}
