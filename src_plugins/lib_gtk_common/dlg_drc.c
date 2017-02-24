/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
#include "dlg_drc.h"
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

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

/** \file   dlg_drc.c
    \brief  Implementation of \ref FIXME dialog.
 */

#define VIOLATION_PIXMAP_PIXEL_SIZE   100
#define VIOLATION_PIXMAP_PIXEL_BORDER 5
#define VIOLATION_PIXMAP_PCB_SIZE     PCB_MIL_TO_COORD (100)

static GtkWidget *drc_window, *drc_list;
static GtkListStore *drc_list_model = NULL;
static int num_violations = 0;

/* Remember user window resizes. */
static gint drc_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	wplc_config_event(widget, &hid_gtk_wgeo.drc_x, &hid_gtk_wgeo.drc_y, &hid_gtk_wgeo.drc_width, &hid_gtk_wgeo.drc_height);
	return FALSE;
}

static void drc_close_cb(gpointer data)
{
	gtk_widget_destroy(drc_window);
	drc_window = NULL;
}

static void drc_refresh_cb(gpointer data)
{
	pcb_hid_actionl("DRC", NULL);
}

static void drc_destroy_cb(GtkWidget * widget, gpointer data)
{
	drc_window = NULL;
}

enum {
	DRC_VIOLATION_NUM_COL = 0,
	DRC_VIOLATION_OBJ_COL,
	NUM_DRC_COLUMNS
};


static void unset_found_flags(int AndDraw)
{
	int flag = PCB_FLAG_FOUND;
	int change = 0;

	PCB_VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, via)) {
			pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, via, via, via);
			PCB_FLAG_CLEAR(flag, via);
			DrawVia(via);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(flag, pin)) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, element, pin, pin);
				PCB_FLAG_CLEAR(flag, pin);
				DrawPin(pin);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (PCB_FLAG_TEST(flag, pad)) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_PAD, element, pad, pad);
				PCB_FLAG_CLEAR(flag, pad);
				DrawPad(pad);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_RAT_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			pcb_undo_add_obj_to_flag(PCB_TYPE_RATLINE, line, line, line);
			PCB_FLAG_CLEAR(flag, line);
			DrawRat(line);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_LINE_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			pcb_undo_add_obj_to_flag(PCB_TYPE_LINE, layer, line, line);
			PCB_FLAG_CLEAR(flag, line);
			DrawLine(layer, line);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, arc)) {
			pcb_undo_add_obj_to_flag(PCB_TYPE_ARC, layer, arc, arc);
			PCB_FLAG_CLEAR(flag, arc);
			DrawArc(layer, arc);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, polygon)) {
			pcb_undo_add_obj_to_flag(PCB_TYPE_POLYGON, layer, polygon, polygon);
			PCB_FLAG_CLEAR(flag, polygon);
			DrawPolygon(layer, polygon);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	if (change) {
		pcb_board_set_changed_flag(pcb_true);
		if (AndDraw) {
			pcb_undo_inc_serial();
			pcb_draw();
		}
	}
}

static void selection_changed_cb(GtkTreeSelection * selection, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GhidDrcViolation *violation;
	int i;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
		unset_found_flags(pcb_true);
		return;
	}

	/* Check the selected node has children, if so; return. */
	if (gtk_tree_model_iter_has_child(model, &iter))
		return;

	gtk_tree_model_get(model, &iter, DRC_VIOLATION_OBJ_COL, &violation, -1);

	unset_found_flags(pcb_false);

	if (violation == NULL)
		return;

	/* Flag the objects listed against this DRC violation */
	for (i = 0; i < violation->object_count; i++) {
		int object_id = violation->object_id_list[i];
		int object_type = violation->object_type_list[i];
		int found_type;
		void *ptr1, *ptr2, *ptr3;

		found_type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, object_id, object_type);
		if (found_type == PCB_TYPE_NONE) {
			pcb_message(PCB_MSG_WARNING, _("Object ID %i identified during DRC was not found. Stale DRC window?\n"), object_id);
			continue;
		}
		pcb_undo_add_obj_to_flag(object_type, ptr1, ptr2, ptr3);
		PCB_FLAG_SET(PCB_FLAG_FOUND, (pcb_any_obj_t *) ptr2);
		switch (violation->object_type_list[i]) {
		case PCB_TYPE_LINE:
		case PCB_TYPE_ARC:
		case PCB_TYPE_POLYGON:
			pcb_layervis_change_group_vis(pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1), pcb_true, pcb_true);
		}
		pcb_draw_obj(object_type, ptr1, ptr2);
	}
	pcb_board_set_changed_flag(pcb_true);
	pcb_undo_inc_serial();
	pcb_draw();
}

static void row_activated_cb(GtkTreeView * view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer user_data)
{
	GtkTreeModel *model = gtk_tree_view_get_model(view);
	GtkTreeIter iter;
	GhidDrcViolation *violation;

	gtk_tree_model_get_iter(model, &iter, path);

	gtk_tree_model_get(model, &iter, DRC_VIOLATION_OBJ_COL, &violation, -1);

	if (violation == NULL)
		return;

	pcb_center_display(violation->x_coord, violation->y_coord);
}


enum {
	PROP_TITLE = 1,
	PROP_EXPLANATION,
	PROP_X_COORD,
	PROP_Y_COORD,
	PROP_ANGLE,
	PROP_HAVE_MEASURED,
	PROP_MEASURED_VALUE,
	PROP_REQUIRED_VALUE,
	PROP_OBJECT_LIST,
	PROP_PIXMAP
};


static GObjectClass *ghid_drc_violation_parent_class = NULL;


/*! \brief GObject finalise handler
 *
 *  \par Function Description
 *  Just before the GhidDrcViolation GObject is finalized, free our
 *  allocated data, and then chain up to the parent's finalize handler.
 *
 *  \param [in] widget  The GObject being finalized.
 */
static void ghid_drc_violation_finalize(GObject * object)
{
	GhidDrcViolation *violation = GHID_DRC_VIOLATION(object);

	g_free(violation->title);
	g_free(violation->explanation);
	g_free(violation->object_id_list);
	g_free(violation->object_type_list);
	if (violation->pixmap != NULL)
		g_object_unref(violation->pixmap);

	G_OBJECT_CLASS(ghid_drc_violation_parent_class)->finalize(object);
}

typedef struct object_list {
	int count;
	long int *id_list;
	int *type_list;
} object_list;

/*! \brief GObject property setter function
 *
 *  \par Function Description
 *  Setter function for GhidDrcViolation's GObject properties,
 *  "settings-name" and "toplevel"
 *
 *  \param [in]  object       The GObject whose properties we are setting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [in]  value        The GValue the property is being set from
 *  \param [in]  pspec        A GParamSpec describing the property being set
 */
static void ghid_drc_violation_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	GhidDrcViolation *violation = GHID_DRC_VIOLATION(object);
	object_list *obj_list;

	switch (property_id) {
	case PROP_TITLE:
		g_free(violation->title);
		violation->title = g_value_dup_string(value);
		break;
	case PROP_EXPLANATION:
		g_free(violation->explanation);
		violation->explanation = g_value_dup_string(value);
		break;
	case PROP_X_COORD:
		violation->x_coord = g_value_get_int(value);
		break;
	case PROP_Y_COORD:
		violation->y_coord = g_value_get_int(value);
		break;
	case PROP_ANGLE:
		violation->angle = g_value_get_double(value);
		break;
	case PROP_HAVE_MEASURED:
		violation->have_measured = g_value_get_boolean(value);
		break;
	case PROP_MEASURED_VALUE:
		violation->measured_value = g_value_get_int(value);
		break;
	case PROP_REQUIRED_VALUE:
		violation->required_value = g_value_get_int(value);
		break;
	case PROP_OBJECT_LIST:
		/* Copy the passed data to make new lists */
		g_free(violation->object_id_list);
		g_free(violation->object_type_list);
		obj_list = (object_list *) g_value_get_pointer(value);
		violation->object_count = obj_list->count;
		violation->object_id_list = g_new(long int, obj_list->count);
		violation->object_type_list = g_new(int, obj_list->count);
		memcpy(violation->object_id_list, obj_list->id_list, sizeof(long int) * obj_list->count);
		memcpy(violation->object_type_list, obj_list->type_list, sizeof(int) * obj_list->count);
		break;
	case PROP_PIXMAP:
		if (violation->pixmap)
			g_object_unref(violation->pixmap);	/* Frees our old reference */
		violation->pixmap = (GdkDrawable *) g_value_dup_object(value);	/* Takes a new reference */
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		return;
	}
}


/*! \brief GObject property getter function
 *
 *  \par Function Description
 *  Getter function for GhidDrcViolation's GObject properties,
 *  "settings-name" and "toplevel".
 *
 *  \param [in]  object       The GObject whose properties we are getting
 *  \param [in]  property_id  The numeric id. under which the property was
 *                            registered with g_object_class_install_property()
 *  \param [out] value        The GValue in which to return the value of the property
 *  \param [in]  pspec        A GParamSpec describing the property being got
 */
static void ghid_drc_violation_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

}


/*! \brief GType class initialiser for GhidDrcViolation
 *
 *  \par Function Description
 *  GType class initialiser for GhidDrcViolation. We override our parent
 *  virtual class methods as needed and register our GObject properties.
 *
 *  \param [in]  klass       The GhidDrcViolationClass we are initialising
 */
static void ghid_drc_violation_class_init(GhidViolationRendererClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->finalize = ghid_drc_violation_finalize;
	gobject_class->set_property = ghid_drc_violation_set_property;
	gobject_class->get_property = ghid_drc_violation_get_property;

	ghid_drc_violation_parent_class = (GObjectClass *) g_type_class_peek_parent(klass);

	g_object_class_install_property(gobject_class, PROP_TITLE, g_param_spec_string("title", "", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_EXPLANATION,
																	g_param_spec_string("explanation", "", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_X_COORD,
																	g_param_spec_int("x-coord", "", "", G_MININT, G_MAXINT, 0, G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_Y_COORD,
																	g_param_spec_int("y-coord", "", "", G_MININT, G_MAXINT, 0, G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_ANGLE,
																	g_param_spec_double("angle", "", "", G_MININT, G_MAXINT, 0, G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_HAVE_MEASURED,
																	g_param_spec_boolean("have-measured", "", "", 0, G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_MEASURED_VALUE,
																	g_param_spec_int("measured-value", "", "", G_MININT, G_MAXINT, 0., G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_REQUIRED_VALUE,
																	g_param_spec_int("required-value", "", "", G_MININT, G_MAXINT, 0., G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_OBJECT_LIST,
																	g_param_spec_pointer("object-list", "", "", G_PARAM_WRITABLE));
	g_object_class_install_property(gobject_class, PROP_PIXMAP,
																	g_param_spec_object("pixmap", "", "", GDK_TYPE_DRAWABLE, G_PARAM_WRITABLE));
}

/*! \brief Function to retrieve GhidViolationRenderer's GType identifier.
 *
 *  \par Function Description
 *  Function to retrieve GhidViolationRenderer's GType identifier.
 *  Upon first call, this registers the GhidViolationRenderer in the GType system.
 *  Subsequently it returns the saved value from its first execution.
 *
 *  \return the GType identifier associated with GhidViolationRenderer.
 */
GType ghid_drc_violation_get_type()
{
	static GType ghid_drc_violation_type = 0;

	if (!ghid_drc_violation_type) {
		static const GTypeInfo ghid_drc_violation_info = {
			sizeof(GhidDrcViolationClass),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_drc_violation_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(GhidDrcViolation),
			0,												/* n_preallocs */
			NULL,											/* instance_init */
		};

		ghid_drc_violation_type =
			g_type_register_static(G_TYPE_OBJECT, "GhidDrcViolation", &ghid_drc_violation_info, (GTypeFlags) 0);
	}

	return ghid_drc_violation_type;
}

/* API */

GhidDrcViolation *ghid_drc_violation_new(pcb_drc_violation_t * violation, GdkDrawable * pixmap)
{
	object_list obj_list;

	obj_list.count = violation->object_count;
	obj_list.id_list = violation->object_id_list;
	obj_list.type_list = violation->object_type_list;

	return (GhidDrcViolation *) g_object_new(GHID_TYPE_DRC_VIOLATION,
																					 "title", violation->title,
																					 "explanation", violation->explanation,
																					 "x-coord", violation->x,
																					 "y-coord", violation->y,
																					 "angle", violation->angle,
																					 "have-measured", violation->have_measured,
																					 "measured-value", violation->measured_value,
																					 "required-value", violation->required_value,
																					 "object-list", &obj_list, "pixmap", pixmap, NULL);
}

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
		GdkPixmap *pixmap = renderer->common->render_pixmap(violation->x_coord,
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
GtkCellRenderer *ghid_violation_renderer_new(pcb_gtk_common_t *common)
{
	GhidViolationRenderer *renderer;

	renderer = (GhidViolationRenderer *) g_object_new(GHID_TYPE_VIOLATION_RENDERER, "ypad", 6, NULL);
	renderer->common = common;

	return GTK_CELL_RENDERER(renderer);
}


void ghid_drc_window_show(pcb_gtk_common_t *common, gboolean raise)
{
	GtkWidget *vbox, *hbox, *button, *scrolled_window;
	GtkCellRenderer *violation_renderer;

	if (drc_window) {
		if (raise)
			gtk_window_present(GTK_WINDOW(drc_window));
		return;
	}

	drc_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(drc_window), "destroy", G_CALLBACK(drc_destroy_cb), NULL);
	g_signal_connect(G_OBJECT(drc_window), "configure_event", G_CALLBACK(drc_window_configure_event_cb), NULL);
	gtk_window_set_title(GTK_WINDOW(drc_window), _("pcb-rnd DRC"));
	gtk_window_set_wmclass(GTK_WINDOW(drc_window), "PCB_DRC", "PCB");
	gtk_window_set_default_size(GTK_WINDOW(drc_window), hid_gtk_wgeo.drc_width, hid_gtk_wgeo.drc_height);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(drc_window), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_box_set_spacing(GTK_BOX(vbox), 6);

	drc_list_model = gtk_list_store_new(NUM_DRC_COLUMNS, G_TYPE_INT,	/* DRC_VIOLATION_NUM_COL */
																			G_TYPE_OBJECT);	/* DRC_VIOLATION_OBJ_COL */

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE /* EXPAND */ , TRUE /* FILL */ , 0 /* PADDING */ );

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	drc_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(drc_list_model));
	gtk_container_add(GTK_CONTAINER(scrolled_window), drc_list);

	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(drc_list), TRUE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(drc_list)), "changed", G_CALLBACK(selection_changed_cb), NULL);
	g_signal_connect(drc_list, "row-activated", G_CALLBACK(row_activated_cb), NULL);

	violation_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(drc_list), -1,	/* APPEND */
																							_("No."),	/* TITLE */
																							violation_renderer, "text", DRC_VIOLATION_NUM_COL, NULL);

	violation_renderer = ghid_violation_renderer_new(common);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(drc_list), -1,	/* APPEND */
																							_("Violation details"),	/* TITLE */
																							violation_renderer, "violation", DRC_VIOLATION_OBJ_COL, NULL);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_box_set_spacing(GTK_BOX(hbox), 6);

	button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(drc_refresh_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(drc_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	wplc_place(WPLC_DRC, drc_window);

	gtk_widget_realize(drc_window);
	gtk_widget_show_all(drc_window);
}

void ghid_drc_window_append_violation(pcb_gtk_common_t *common, pcb_drc_violation_t * violation)
{
	GhidDrcViolation *violation_obj;
	GtkTreeIter iter;

	/* Ensure the required structures are setup */
	ghid_drc_window_show(common, FALSE);

	num_violations++;

	violation_obj = ghid_drc_violation_new(violation, /* pixmap */ NULL);

	gtk_list_store_append(drc_list_model, &iter);
	gtk_list_store_set(drc_list_model, &iter, DRC_VIOLATION_NUM_COL, num_violations, DRC_VIOLATION_OBJ_COL, violation_obj, -1);

	g_object_unref(violation_obj);	/* The list store takes its own reference */
}

void ghid_drc_window_reset_message(void)
{
	if (drc_list_model != NULL)
		gtk_list_store_clear(drc_list_model);
	num_violations = 0;
}

int ghid_drc_window_throw_dialog(pcb_gtk_common_t *common)
{
	ghid_drc_window_show(common, TRUE);
	return 1;
}
