/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "error.h"
#include "search.h"
#include "draw.h"
#include "layer.h"
#include "pcb-printf.h"
#include "undo.h"
#include "dlg_drc_cr.h"
#include "hid_actions.h"
#include "compat_nls.h"
#include "obj_all.h"
#include "obj_pinvia_draw.h"
#include "obj_pad_draw.h"
#include "obj_rat_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_poly_draw.h"
#include "layer_vis.h"

#include "util_str.h"
#include "win_place.h"

/** TODO temporary */
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

/** \file   dlg_drc_cr.c
    \brief  Implementation of \ref FIXME cell text renderer.
 */

#define VIOLATION_PIXMAP_PIXEL_SIZE   100
#define VIOLATION_PIXMAP_PIXEL_BORDER 5
#define VIOLATION_PIXMAP_PCB_SIZE     PCB_MIL_TO_COORD (100)

enum {
	PROP_VIOLATION = 1,
};

static GObjectClass *ghid_violation_renderer_parent_class = NULL;

/*! \brief GObject finalise handler
 *
 *  \par Function Description
 *  Just before the GhidViolationRenderer GObject is finalized, free our
 *  allocated data, and then chain up to the parent's finalize handler.
 *
 *  \param [in] widget  The GObject being finalized.
 */
static void ghid_violation_renderer_finalize(GObject * object)
{
	GhidViolationRenderer *renderer = GHID_VIOLATION_RENDERER(object);

	if (renderer->violation != NULL)
		g_object_unref(renderer->violation);

	G_OBJECT_CLASS(ghid_violation_renderer_parent_class)->finalize(object);
}

/*! \brief GObject property setter function
 *
 *  \par Function Description
 *  Setter function for GhidViolationRenderer's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are setting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [in]  value        The GValue the property is being set from
 *  \param [in]  pspec        A GParamSpec describing the property being set
 */
static void ghid_violation_renderer_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	GhidViolationRenderer *renderer = GHID_VIOLATION_RENDERER(object);
	char *markup;

	switch (property_id) {
	case PROP_VIOLATION:
		if (renderer->violation != NULL)
			g_object_unref(renderer->violation);
		renderer->violation = (GhidDrcViolation *) g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		return;
	}

	if (renderer->violation == NULL)
		return;

	if (renderer->violation->have_measured) {
		markup = pcb_strdup_printf("%m+<b>%s (%$mS)</b>\n"
																 "<span size='1024'> </span>\n"
																 "<small>"
																 "<i>%s</i>\n"
																 "<span size='5120'> </span>\n"
																 "Required: %$mS"
																 "</small>",
																 conf_core.editor.grid_unit->allow,
																 renderer->violation->title,
																 renderer->violation->measured_value,
																 renderer->violation->explanation, renderer->violation->required_value);
	}
	else {
		markup = pcb_strdup_printf("%m+<b>%s</b>\n"
																 "<span size='1024'> </span>\n"
																 "<small>"
																 "<i>%s</i>\n"
																 "<span size='5120'> </span>\n"
																 "Required: %$mS"
																 "</small>",
																 conf_core.editor.grid_unit->allow,
																 renderer->violation->title,
																 renderer->violation->explanation, renderer->violation->required_value);
	}

	g_object_set(object, "markup", markup, NULL);
	free(markup);
}

/*! \brief GObject property getter function
 *
 *  \par Function Description
 *  Getter function for GhidViolationRenderer's GObject properties.
 *
 *  \param [in]  object       The GObject whose properties we are getting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [out] value        The GValue in which to return the value of the property
 *  \param [in]  pspec        A GParamSpec describing the property being got
 */
static void ghid_violation_renderer_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

}

static void
ghid_violation_renderer_get_size(GtkCellRenderer * cell,
																 GtkWidget * widget,
																 GdkRectangle * cell_area, gint * x_offset, gint * y_offset, gint * width, gint * height)
{
	GTK_CELL_RENDERER_CLASS(ghid_violation_renderer_parent_class)->get_size(cell,
																																					widget, cell_area, x_offset, y_offset, width, height);
	if (width != NULL)
		*width += VIOLATION_PIXMAP_PIXEL_SIZE;
	if (height != NULL)
		*height = MAX(*height, VIOLATION_PIXMAP_PIXEL_SIZE);
}

static void
ghid_violation_renderer_render(GtkCellRenderer * cell,
															 GdkDrawable * window,
															 GtkWidget * widget,
															 GdkRectangle * background_area,
															 GdkRectangle * cell_area, GdkRectangle * expose_area, GtkCellRendererState flags)
{
	GdkDrawable *mydrawable;
	GtkStyle *style = gtk_widget_get_style(widget);
	GhidViolationRenderer *renderer = GHID_VIOLATION_RENDERER(cell);
	GhidDrcViolation *violation = renderer->violation;
	int pixmap_size = VIOLATION_PIXMAP_PIXEL_SIZE - 2 * VIOLATION_PIXMAP_PIXEL_BORDER;

	cell_area->width -= VIOLATION_PIXMAP_PIXEL_SIZE;
	GTK_CELL_RENDERER_CLASS(ghid_violation_renderer_parent_class)->render(cell,
																																				window,
																																				widget, background_area, cell_area, expose_area, flags);

	if (violation == NULL)
		return;

	if (violation->pixmap == NULL) {
		GdkPixmap *pixmap = ghid_render_pixmap(violation->x_coord,
																					 violation->y_coord,
																					 VIOLATION_PIXMAP_PCB_SIZE / pixmap_size,
																					 pixmap_size, pixmap_size,
																					 gdk_drawable_get_depth(window));
		g_object_set(violation, "pixmap", pixmap, NULL);
		g_object_unref(pixmap);
	}

	if (violation->pixmap == NULL)
		return;

	mydrawable = GDK_DRAWABLE(violation->pixmap);

	gdk_draw_drawable(window, style->fg_gc[gtk_widget_get_state(widget)],
										mydrawable, 0, 0,
										cell_area->x + cell_area->width + VIOLATION_PIXMAP_PIXEL_BORDER,
										cell_area->y + VIOLATION_PIXMAP_PIXEL_BORDER, -1, -1);
}

/*! \brief GType class initialiser for GhidViolationRenderer
 *
 *  \par Function Description
 *  GType class initialiser for GhidViolationRenderer. We override our parent
 *  virtual class methods as needed and register our GObject properties.
 *
 *  \param [in]  klass       The GhidViolationRendererClass we are initialising
 */
static void ghid_violation_renderer_class_init(GhidViolationRendererClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GtkCellRendererClass *cellrenderer_class = GTK_CELL_RENDERER_CLASS(klass);

	gobject_class->finalize = ghid_violation_renderer_finalize;
	gobject_class->set_property = ghid_violation_renderer_set_property;
	gobject_class->get_property = ghid_violation_renderer_get_property;

	cellrenderer_class->get_size = ghid_violation_renderer_get_size;
	cellrenderer_class->render = ghid_violation_renderer_render;

	ghid_violation_renderer_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);

	g_object_class_install_property(gobject_class, PROP_VIOLATION,
																	g_param_spec_object("violation", "", "", GHID_TYPE_DRC_VIOLATION, G_PARAM_WRITABLE));
}

/* API */

/*! \brief Function to retrieve GhidViolationRenderer's GType identifier.
 *
 *  \par Function Description
 *  Function to retrieve GhidViolationRenderer's GType identifier.
 *  Upon first call, this registers the GhidViolationRenderer in the GType system.
 *  Subsequently it returns the saved value from its first execution.
 *
 *  \return the GType identifier associated with GhidViolationRenderer.
 */
GType ghid_violation_renderer_get_type()
{
	static GType ghid_violation_renderer_type = 0;

	if (!ghid_violation_renderer_type) {
		static const GTypeInfo ghid_violation_renderer_info = {
			sizeof(GhidViolationRendererClass),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_violation_renderer_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(GhidViolationRenderer),
			0,												/* n_preallocs */
			NULL,											/* instance_init */
		};

		ghid_violation_renderer_type =
			g_type_register_static(GTK_TYPE_CELL_RENDERER_TEXT, "GhidViolationRenderer",
														 &ghid_violation_renderer_info, (GTypeFlags) 0);
	}

	return ghid_violation_renderer_type;
}

/*! \brief Convenience function to create a new violation renderer
 *
 *  \par Function Description
 *  Convenience function which creates a GhidViolationRenderer.
 *
 *  \return  The GhidViolationRenderer created.
 */
GtkCellRenderer *ghid_violation_renderer_new(void)
{
	GhidViolationRenderer *renderer;

	renderer = (GhidViolationRenderer *) g_object_new(GHID_TYPE_VIOLATION_RENDERER, "ypad", 6, NULL);

	return GTK_CELL_RENDERER(renderer);
}
