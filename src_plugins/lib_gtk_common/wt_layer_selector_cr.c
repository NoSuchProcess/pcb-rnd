/* COPYRIGHT: missing in the original file inherited from gEDA/PCB:
   must be the same as in other gtk-related sources. */
/*! \file <gtk-pcb-cell-render-visibility.c>
 *  \brief Implementation of GtkCellRenderer for layer visibility toggler
 *  \par More Information
 *  For details on the functions implemented here, see the Gtk
 *  documentation for the GtkCellRenderer object, which defines
 *  the interface we are implementing.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "config.h"
#include "compat_nls.h"
#include "error.h"

#include "wt_layer_selector_cr.h"

#include "util_str.h"

/* how much non-group layers should be indented to the right, in pixel */
#define IND_SIZE 10

enum {
	TOGGLED,
	LAST_SIGNAL
};
static guint toggle_cell_signals[LAST_SIGNAL] = { 0 };

enum {
	PROP_ACTIVE = 1,
	PROP_GROUP,
	PROP_COLOR
};

struct _GHidCellRendererVisibility {
	GtkCellRendererPixbufClass parent;

	gboolean active, group;
	gchar *color;
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;
};

struct _GHidCellRendererVisibilityClass {
	GtkCellRendererPixbufClass parent_class;

	void (*toggled) (GHidCellRendererVisibility * cell, const gchar * path);
};

/* RENDERER FUNCTIONS */
/*! \brief Calculates the window area the renderer will use */
static void
ghid_cell_renderer_visibility_get_size(GtkCellRenderer * cell,
																			 GtkWidget * widget,
																			 GdkRectangle * cell_area, gint * x_offset, gint * y_offset, gint * width, gint * height)
{
	GtkStyle *style = gtk_widget_get_style(widget);
	gint w, h;
	gint xpad, ypad;
	gfloat xalign, yalign;

	gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
	gtk_cell_renderer_get_alignment(cell, &xalign, &yalign);

	w = VISIBILITY_TOGGLE_SIZE + 2 * (xpad + style->xthickness + IND_SIZE);
	h = VISIBILITY_TOGGLE_SIZE + 2 * (ypad + style->ythickness);

	if (width)
		*width = w;
	if (height)
		*height = h;

	if (cell_area) {
		if (x_offset) {
			if (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
				xalign = 1. - xalign;
			*x_offset = MAX(0, xalign * (cell_area->width - w));
		}
		if (y_offset)
			*y_offset = MAX(0, yalign * (cell_area->height - h));
	}
}

static void convert_alpha(guchar * dest_data, int dest_stride,
													guchar * src_data, int src_stride, int src_x, int src_y, int width, int height)
{
	int x, y;

	src_data += src_stride * src_y + src_x * 4;

	for (y = 0; y < height; y++) {
		guint32 *src = (guint32 *) src_data;

		for (x = 0; x < width; x++) {
			guint alpha = src[x] >> 24;

			if (alpha == 0) {
				dest_data[x * 4 + 0] = 0;
				dest_data[x * 4 + 1] = 0;
				dest_data[x * 4 + 2] = 0;
			}
			else {
				dest_data[x * 4 + 0] = (((src[x] & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
				dest_data[x * 4 + 1] = (((src[x] & 0x00ff00) >> 8) * 255 + alpha / 2) / alpha;
				dest_data[x * 4 + 2] = (((src[x] & 0x0000ff) >> 0) * 255 + alpha / 2) / alpha;
			}
			dest_data[x * 4 + 3] = alpha;
		}

		src_data += src_stride;
		dest_data += dest_stride;
	}
}

static void convert_no_alpha(guchar * dest_data, int dest_stride,
														 guchar * src_data, int src_stride, int src_x, int src_y, int width, int height)
{
	int x, y;

	src_data += src_stride * src_y + src_x * 4;

	for (y = 0; y < height; y++) {
		guint32 *src = (guint32 *) src_data;

		for (x = 0; x < width; x++) {
			dest_data[x * 3 + 0] = src[x] >> 16;
			dest_data[x * 3 + 1] = src[x] >> 8;
			dest_data[x * 3 + 2] = src[x];
		}

		src_data += src_stride;
		dest_data += dest_stride;
	}
}

/** Updates the content of surface into pixbuf */
void pcb_gtk_surface_draw_to_pixbuf(cairo_surface_t * surface, GdkPixbuf * pixbuf)
{
	gint width = gdk_pixbuf_get_width(pixbuf);
	gint height = gdk_pixbuf_get_height(pixbuf);

	cairo_surface_flush(surface);
	if (cairo_surface_status(surface))
		return;

	/* Performs the conversion */
	if (gdk_pixbuf_get_has_alpha(pixbuf))
		convert_alpha(gdk_pixbuf_get_pixels(pixbuf), gdk_pixbuf_get_rowstride(pixbuf),
									cairo_image_surface_get_data(surface), cairo_image_surface_get_stride(surface), 0, 0, width, height);
	else
		convert_no_alpha(gdk_pixbuf_get_pixels(pixbuf), gdk_pixbuf_get_rowstride(pixbuf),
										 cairo_image_surface_get_data(surface), cairo_image_surface_get_stride(surface), 0, 0, width, height);
}

/*! \brief Actually renders the swatch */
static void
ghid_cell_renderer_visibility_render(GtkCellRenderer * cell,
																		 GdkWindow * window,
																		 GtkWidget * widget,
																		 GdkRectangle * background_area,
																		 GdkRectangle * cell_area, GdkRectangle * expose_area, GtkCellRendererState flags)
{
	GHidCellRendererVisibility *pcb_cell;
	GdkRectangle toggle_rect;
	GdkRectangle draw_rect;
	gint xpad, ypad;

	pcb_cell = GHID_CELL_RENDERER_VISIBILITY(cell);
	ghid_cell_renderer_visibility_get_size(cell, widget, cell_area,
																				 &toggle_rect.x, &toggle_rect.y, &toggle_rect.width, &toggle_rect.height);
	gtk_cell_renderer_get_padding(cell, &xpad, &ypad);

	if (!pcb_cell->group)
		xpad += IND_SIZE;

	toggle_rect.x += cell_area->x + xpad;
	toggle_rect.y += cell_area->y + ypad;
	toggle_rect.width -= xpad * 2;
	toggle_rect.height -= ypad * 2;

	if (pcb_cell->group)
		toggle_rect.width -= IND_SIZE*2;

	if (toggle_rect.width <= 0 || toggle_rect.height <= 0)
		return;

	if (gdk_rectangle_intersect(expose_area, cell_area, &draw_rect)) {
		GdkColor color;
		cairo_t *cr = gdk_cairo_create(window);
		if (expose_area) {
			gdk_cairo_rectangle(cr, expose_area);
			cairo_clip(cr);
		}
		cairo_set_line_width(cr, 1);

		cairo_rectangle(cr, toggle_rect.x + 0.5, toggle_rect.y + 0.5, toggle_rect.width - 1, toggle_rect.height - 1);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_fill_preserve(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_stroke(cr);

		gdk_color_parse(pcb_cell->color, &color);
		if (flags & GTK_CELL_RENDERER_PRELIT) {
			color.red = (4 * color.red + 65535) / 5;
			color.green = (4 * color.green + 65535) / 5;
			color.blue = (4 * color.blue + 65535) / 5;
		}
		gdk_cairo_set_source_color(cr, &color);
		if (pcb_cell->active)
			cairo_rectangle(cr, toggle_rect.x + 0.5, toggle_rect.y + 0.5, toggle_rect.width - 1, toggle_rect.height - 1);
		else {
			cairo_move_to(cr, toggle_rect.x + 1, toggle_rect.y + 1);
			cairo_rel_line_to(cr, toggle_rect.width / 2, 0);
			cairo_rel_line_to(cr, -toggle_rect.width / 2, toggle_rect.width / 2);
			cairo_close_path(cr);
		}
		cairo_fill(cr);

		cairo_destroy(cr);
	}
}

/* Actually renders the swatch */
static void pcb_gtk_layer_visibility_render_pixbuf(GHidCellRendererVisibility * ls)
{
	cairo_t *cr;
	GdkColor color;
	GdkRectangle toggle_rect;

	cr = cairo_create(ls->surface);

	toggle_rect.x = 0;
	toggle_rect.y = 0;
	toggle_rect.width = gdk_pixbuf_get_width(ls->pixbuf);
	toggle_rect.height = gdk_pixbuf_get_height(ls->pixbuf);

	cairo_set_line_width(cr, 1);

	cairo_rectangle(cr, toggle_rect.x + 0.5, toggle_rect.y + 0.5, toggle_rect.width - 1, toggle_rect.height - 1);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);

	if (ls->color != NULL) {
		/* FIXME: better use pango_color_parse: not deprecated over GTKx */
		gdk_color_parse(ls->color, &color);
		gdk_cairo_set_source_color(cr, &color);
		if (ls->active)
			cairo_rectangle(cr, toggle_rect.x + 0.5, toggle_rect.y + 0.5, toggle_rect.width - 1, toggle_rect.height - 1);
		else {
			cairo_move_to(cr, toggle_rect.x + 1, toggle_rect.y + 1);
			cairo_rel_line_to(cr, toggle_rect.width / 2, 0);
			cairo_rel_line_to(cr, -toggle_rect.width / 2, toggle_rect.width / 2);
			cairo_close_path(cr);
		}
	}
	cairo_fill(cr);
	pcb_gtk_surface_draw_to_pixbuf(ls->surface, ls->pixbuf);
	cairo_destroy(cr);
}

/** Creates an image surface of same size than pixbuf. This surface can be used
    to draw with cairo and then, transfer surface pixels to pixbuf pixels.

    Returns a pointer to the newly created surface. The caller owns the surface and
      should call cairo_surface_destroy() when done with it.

      This function always returns a valid pointer, but it will return a pointer
      to a "nil" surface in the case of an error. See cairo_image_surface_create_for_data ()
 **/
cairo_surface_t *pcb_gtk_surface_init_for_pixbuf(GdkPixbuf * pixbuf)
{
	gint width = gdk_pixbuf_get_width(pixbuf);
	gint height = gdk_pixbuf_get_height(pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels(pixbuf);

	int cairo_stride;
	guchar *cairo_pixels;
	cairo_format_t format;
	cairo_surface_t *surface;

	if (n_channels == 3)
		format = CAIRO_FORMAT_RGB24;
	else
		format = CAIRO_FORMAT_ARGB32;

	cairo_stride = cairo_format_stride_for_width(format, width);
	cairo_pixels = g_malloc_n(height, cairo_stride);
	surface = cairo_image_surface_create_for_data((unsigned char *) cairo_pixels, format, width, height, cairo_stride);

	if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
		g_free(cairo_pixels);
		pcb_message(PCB_MSG_ERROR, "Surface creation problem in pcb_gtk_surface_init_for_pixbuf() !\n");
	}
	return surface;
}

/*! \brief Toggless the swatch */
static gint
ghid_cell_renderer_visibility_activate(GtkCellRenderer * cell,
																			 GdkEvent * event,
																			 GtkWidget * widget,
																			 const gchar * path,
																			 GdkRectangle * background_area, GdkRectangle * cell_area, GtkCellRendererState flags)
{
	g_signal_emit(cell, toggle_cell_signals[TOGGLED], 0, path);
	return TRUE;
}

/* Setter/Getter */
static void ghid_cell_renderer_visibility_get_property(GObject * object, guint param_id, GValue * value, GParamSpec * pspec)
{
	GHidCellRendererVisibility *pcb_cell = GHID_CELL_RENDERER_VISIBILITY(object);

	switch (param_id) {
	case PROP_ACTIVE:
		g_value_set_boolean(value, pcb_cell->active);
		break;
	case PROP_GROUP:
		g_value_set_boolean(value, pcb_cell->group);
		break;
	case PROP_COLOR:
		g_value_set_string(value, pcb_cell->color);
		break;
	}
}

static void
ghid_cell_renderer_visibility_set_property(GObject * object, guint param_id, const GValue * value, GParamSpec * pspec)
{
	GHidCellRendererVisibility *pcb_cell = GHID_CELL_RENDERER_VISIBILITY(object);

	switch (param_id) {
	case PROP_ACTIVE:
		pcb_cell->active = g_value_get_boolean(value);
		break;
	case PROP_GROUP:
		pcb_cell->group = g_value_get_boolean(value);
		break;
	case PROP_COLOR:
		g_free(pcb_cell->color);
		pcb_cell->color = g_value_dup_string(value);
		break;
	}
	/*pcb_gtk_layer_visibility_render_pixbuf(pcb_cell);*/
}


/* CONSTRUCTOR */
static void ghid_cell_renderer_visibility_init(GHidCellRendererVisibility * ls)
{
	g_object_set(ls, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
	/* Creates a new Pixbuf, used for rendering, initialized to "blue", associated with ls->surface */
	ls->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, VISIBILITY_TOGGLE_SIZE + 2, VISIBILITY_TOGGLE_SIZE + 2);
	gdk_pixbuf_fill(ls->pixbuf, 0x0000ffff);
	ls->surface = pcb_gtk_surface_init_for_pixbuf(ls->pixbuf);
	/*g_object_set(ls, "pixbuf", ls->pixbuf, NULL);*/
}

static void ghid_cell_renderer_visibility_class_init(GHidCellRendererVisibilityClass * klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);

	object_class->get_property = ghid_cell_renderer_visibility_get_property;
	object_class->set_property = ghid_cell_renderer_visibility_set_property;

	cell_class->get_size = ghid_cell_renderer_visibility_get_size;
	cell_class->render = ghid_cell_renderer_visibility_render;
	cell_class->activate = ghid_cell_renderer_visibility_activate;

	g_object_class_install_property(object_class, PROP_ACTIVE,
																	g_param_spec_boolean("active",
																											 _("Visibility state"),
																											 _("Visibility of the layer"), FALSE, G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_GROUP,
																	g_param_spec_boolean("group",
																											 _("Group"),
																											 _("Whether item controls a group"), FALSE, G_PARAM_READWRITE));
	g_object_class_install_property(object_class, PROP_COLOR,
																	g_param_spec_string("color", _("Layer color"), _("Layer color"), FALSE, G_PARAM_READWRITE));


 /**
  * GHidCellRendererVisibility::toggled:
  * @cell_renderer: the object which received the signal
  * @path: string representation of #GtkTreePath describing the 
  *        event location
  *
  * The ::toggled signal is emitted when the cell is toggled. 
  **/
	toggle_cell_signals[TOGGLED] =
		g_signal_new(_("toggled"),
								 G_OBJECT_CLASS_TYPE(object_class),
								 G_SIGNAL_RUN_LAST,
								 G_STRUCT_OFFSET(GHidCellRendererVisibilityClass, toggled),
								 NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

/* PUBLIC FUNCTIONS */
GType ghid_cell_renderer_visibility_get_type(void)
{
	static GType ls_type = 0;

	if (!ls_type) {
		const GTypeInfo ls_info = {
			sizeof(GHidCellRendererVisibilityClass),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_cell_renderer_visibility_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(GHidCellRendererVisibility),
			0,												/* n_preallocs */
			(GInstanceInitFunc) ghid_cell_renderer_visibility_init,
		};

		ls_type = g_type_register_static(GTK_TYPE_CELL_RENDERER_PIXBUF, "GHidCellRendererVisibility", &ls_info, 0);
	}

	return ls_type;
}

GtkCellRenderer *ghid_cell_renderer_visibility_new(void)
{
	GHidCellRendererVisibility *rv = g_object_new(GHID_CELL_RENDERER_VISIBILITY_TYPE, NULL);

	rv->active = FALSE;

	return GTK_CELL_RENDERER(rv);
}
