/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor Palinkas
 *  Copyright (C) 2017 Alain Vigne
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */
#ifndef PCB_GTK_COMPAT_H
#define PCB_GTK_COMPAT_H

#ifdef PCB_GTK3
/* Need GTK3 >= 3.20 due to GdkSeat. */

#define gtkc_widget_get_window(w) gtk_widget_get_window(w)
#define gtkc_widget_get_allocation(w, a) gtk_widget_get_allocation(w, a)
#define gtkc_dialog_get_content_area(d)  gtk_widget_get_content_area(d)
#define gtk_combo_box_entry_new_text()   gtk_combo_box_text_new_with_entry()

typedef GdkRGBA pcb_gtk_color_t;

static inline GtkWidget *gtkc_hbox_new(gboolean homogenous, gint spacing)
{
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
	gtk_box_set_homogeneous(GTK_BOX(box), homogenous);
	return box;
}

static inline GtkWidget *gtkc_vbox_new(gboolean homogenous, gint spacing)
{
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
	gtk_box_set_homogeneous(GTK_BOX(box), homogenous);
	return box;
}

static inline GtkWidget *gtkc_hpaned_new()
{
	return gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
}

static inline GtkWidget *gtkc_vpaned_new()
{
	return gtk_paned_new(GTK_ORIENTATION_VERTICAL);
}

/* color button */

static inline GtkWidget *gtkc_color_button_new_with_color(pcb_gtk_color_t *color)
{
	return gtk_color_button_new_with_rgba(color);
}

static inline void gtkc_color_button_set_color(GtkWidget *button, pcb_gtk_color_t *color)
{
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button), color);
}

static inline void gtkc_color_button_get_color(GtkWidget *button, pcb_gtk_color_t *color)
{
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), color);
}

/* combo box text API, GTK3, GTK2.24 compatible. */

static inline GtkWidget *gtkc_combo_box_text_new(void)
{
	return gtk_combo_box_text_new();
}

static inline void gtkc_combo_box_text_append_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), text);
}

static inline void gtkc_combo_box_text_prepend_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(combo), text);
}

static inline void gtkc_combo_box_text_remove(GtkWidget *combo, gint position)
{
	gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(combo), position);
}

static inline gchar *gtkc_combo_box_text_get_active_text(GtkWidget *combo)
{
	return gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
}

void *gtk_trunctext_new(const gchar *str);
static inline GtkWidget *gtkc_trunctext_new(const gchar *str)
{
	return gtk_trunctext_new(str);
}

#define PCB_GTK_EXPOSE_EVENT_SET(obj, val) obj->draw = (gboolean (*)(GtkWidget *, cairo_t *))val
typedef cairo_t pcb_gtk_expose_t;

static inline void gtkc_scrolled_window_add_with_viewport(GtkWidget *scrolled, GtkWidget *child)
{
	gtk_container_add(GTK_CONTAINER(scrolled), child);
}

static inline GdkWindow * gdkc_window_get_pointer(GtkWidget *w, gint *x, gint *y, GdkModifierType *mask)
{
	GdkWindow *window = gtkc_widget_get_window(w);
	GdkSeat *seat = gdk_display_get_default_seat(gdk_window_get_display(window));
	GdkDevice *device = gdk_seat_get_pointer(seat);

	return gdk_window_get_device_position(window, device, x, y, mask);
}

/* Retrieves a widget's initial minimum and natural height. */
static inline void gtkc_widget_get_preferred_height(GtkWidget *w, gint *min_size, gint *natural_size)
{
	gtk_widget_get_preferred_height(w, min_size, natural_size);
}

/* Adds a CSS class to the widget, applying CSS properties from css_descr */
static inline void gtkc_widget_add_class_style(GtkWidget *w, const char *css_class, char *css_descr)
{
	GtkStyleContext *style_ctxt;
	GtkCssProvider *provider;

	style_ctxt = gtk_widget_get_style_context(w);
	gtk_style_context_add_class(style_ctxt, css_class);
	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), css_descr, -1, NULL);
	gtk_style_context_add_provider(style_ctxt, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);
}

static inline void pcb_gtk_set_selected(GtkWidget *widget, int set)
{
	GtkStyleContext *sc = gtk_widget_get_style_context(widget);
	GtkStateFlags sf = gtk_widget_get_state_flags(widget);

	if (set)
		gtk_style_context_set_state(sc, sf | GTK_STATE_FLAG_SELECTED);
	else
		gtk_style_context_set_state(sc, sf & (~GTK_STATE_FLAG_NORMAL));
  gtk_widget_queue_draw(widget);
}

/* Make a widget selectable (via recoloring) */
#define gtkc_widget_selectable(widget, name_space) \
	do { \
		GtkStyleContext *context; \
		GtkCssProvider *provider; \
\
		context = gtk_widget_get_style_context(widget); \
		provider = gtk_css_provider_new(); \
		gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), \
																		"*." name_space ":selected {\n" \
																		"   background-color: @theme_selected_bg_color;\n" \
																		"   color: @theme_selected_fg_color;\n" "}\n", -1, NULL); \
		gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION); \
		gtk_style_context_add_class(context, name_space); \
		g_object_unref(provider); \
	} while(0)

#else
/* GTK2 */

#define gtkc_widget_get_window(w) (GDK_WINDOW(GTK_WIDGET(w)->window))

#define gtkc_widget_get_allocation(w, a) \
do { \
	*(a) = (GTK_WIDGET(w)->allocation); \
} while(0) \

#define gtkc_dialog_get_content_area(d)  ((d)->vbox)
#define gtkc_combo_box_entry_new_text()  gtk_combo_box_entry_new_text()

typedef GdkColor pcb_gtk_color_t;

static inline GtkWidget *gtkc_hbox_new(gboolean homogenous, gint spacing)
{
	return gtk_hbox_new(homogenous, spacing);
}

static inline GtkWidget *gtkc_vbox_new(gboolean homogenous, gint spacing)
{
	return gtk_vbox_new(homogenous, spacing);
}

static inline GtkWidget *gtkc_hpaned_new()
{
	return gtk_hpaned_new();
}

static inline GtkWidget *gtkc_vpaned_new()
{
	return gtk_vpaned_new();
}

/* color button */

static inline GtkWidget *gtkc_color_button_new_with_color(pcb_gtk_color_t *color)
{
	return gtk_color_button_new_with_color(color);
}

static inline void gtkc_color_button_set_color(GtkWidget *button, pcb_gtk_color_t *color)
{
	gtk_color_button_set_color(GTK_COLOR_BUTTON(button), color);
}

static inline void gtkc_color_button_get_color(GtkWidget *button, pcb_gtk_color_t *color)
{
	gtk_color_button_get_color(GTK_COLOR_BUTTON(button), color);
}

/* combo box text API, GTK2.4 compatible, GTK3 incompatible. */

static inline GtkWidget *gtkc_combo_box_text_new(void)
{
	return gtk_combo_box_new_text();
}

static inline void gtkc_combo_box_text_append_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
}

static inline void gtkc_combo_box_text_prepend_text(GtkWidget *combo, const gchar *text)
{
	gtk_combo_box_prepend_text(GTK_COMBO_BOX(combo), text);
}

static inline void gtkc_combo_box_text_remove(GtkWidget *combo, gint position)
{
	gtk_combo_box_remove_text(GTK_COMBO_BOX(combo), position);
}

static inline gchar *gtkc_combo_box_text_get_active_text(GtkWidget *combo)
{
	return gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
}

static inline GtkWidget *gtkc_trunctext_new(const gchar *str)
{
	GtkWidget *w = gtk_label_new(str);
	gtk_widget_set_size_request(w, 1, 1);

	return w;
}

#define PCB_GTK_EXPOSE_EVENT_SET(obj, val) obj->expose_event = (gboolean (*)(GtkWidget *, GdkEventExpose *))val
typedef GdkEventExpose pcb_gtk_expose_t;

static inline void gtkc_scrolled_window_add_with_viewport(GtkWidget *scrolled, GtkWidget *child)
{
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), child);
}

static inline GdkWindow * gdkc_window_get_pointer(GtkWidget *w, gint *x, gint *y, GdkModifierType *mask)
{
	return gdk_window_get_pointer(gtkc_widget_get_window(w), x, y, mask);
}

static inline void gtkc_widget_get_preferred_height(GtkWidget *w, gint *min_size, gint *natural_size)
{
}

static inline void gtkc_widget_add_class_style(GtkWidget *w, const char *css_class, char *css_descr)
{
}

static inline void pcb_gtk_set_selected(GtkWidget *widget, int set)
{
	GtkStateType st = gtk_widget_get_state(widget);
	/* race condition... */
	if (set)
		gtk_widget_set_state(widget, st | GTK_STATE_SELECTED);
	else
		gtk_widget_set_state(widget, st & (~GTK_STATE_SELECTED));
}

#define gtkc_widget_selectable(widget, name_space)

#endif

/*** common for now ***/

/* gtk deprecated gtk_widget_hide_all() for some reason; this naive
   implementation seems to work. */
static inline void pcb_gtk_widget_hide_all(GtkWidget *widget)
{
	if(GTK_IS_CONTAINER(widget)) {
		GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
		while ((children = g_list_next(children)) != NULL)
			pcb_gtk_widget_hide_all(GTK_WIDGET(children->data));
	}
	gtk_widget_hide(widget);
}

/* gtk_table() is depreaceted in gtk3 for gtk_grid, but there's no grid between
   3.0 and 3.2 so we should stay with table as long as it is not actually
   removed */

/* create a table with known size (all rows and cols created empty) */
static inline GtkWidget *gtkc_table_static(int rows, int cols, gboolean homog)
{
	return gtk_table_new(rows, cols, homog);
}


/* Attach child in a single cell of the table */
static inline void gtkc_table_attach1(GtkWidget *table, GtkWidget *child, int row, int col)
{
	gtk_table_attach(GTK_TABLE(table), child, col, col+1, row, row+1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 4);
}

#endif  /* PCB_GTK_COMPAT_H */
