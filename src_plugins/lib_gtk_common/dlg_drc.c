/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "actions.h"
#include "compat_nls.h"
#include "obj_pstk_draw.h"
#include "obj_rat_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_poly_draw.h"
#include "layer_vis.h"
#include "wt_preview.h"

#include "compat.h"
#include "util_str.h"
#include "win_place.h"

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#define VIOLATION_PIXMAP_PIXEL_SIZE   100
#define VIOLATION_PIXMAP_PIXEL_BORDER 5
#define VIOLATION_PIXMAP_PCB_SIZE     PCB_MIL_TO_COORD (100)

/* Remember user window resizes. */
static gint drc_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	wplc_config_event(widget, ev, &hid_gtk_wgeo.drc_x, &hid_gtk_wgeo.drc_y, &hid_gtk_wgeo.drc_width, &hid_gtk_wgeo.drc_height);
	return FALSE;
}

static void drc_close_cb(GtkButton *button, gpointer data)
{
	pcb_gtk_drcwin_t *p = data;

	gtk_widget_hide(p->drc_window);
}

static void destroy_widget(GtkWidget * widget, gpointer data)
{
	gtk_widget_destroy(widget);
}

static void drc_refresh_cb(GtkButton *button, gpointer data)
{
	pcb_gtk_drcwin_t *p = data;

	p->num_violations = 0;
	gtk_container_foreach(GTK_CONTAINER(p->drc_vbox), destroy_widget, NULL);
	pcb_actionl("DRC", NULL);
}

static void drc_destroy_cb(GtkWidget * widget, gpointer data)
{
	pcb_gtk_drcwin_t *p = data;

	p->drc_window = NULL;
}

static void unset_found_flags(int AndDraw)
{
	int flag = PCB_FLAG_FOUND;
	int change = 0;

#warning subc TODO: use data.h for this
#if 0
	PCB_VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, via)) {
			pcb_undo_add_obj_to_flag(via);
			PCB_FLAG_CLEAR(flag, via);
			pcb_via_invalidate_draw(via);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(flag, pin)) {
				pcb_undo_add_obj_to_flag(pin);
				PCB_FLAG_CLEAR(flag, pin);
				pcb_pin_invalidate_draw(pin);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (PCB_FLAG_TEST(flag, pad)) {
				pcb_undo_add_obj_to_flag(pad);
				PCB_FLAG_CLEAR(flag, pad);
				pcb_pad_invalidate_draw(pad);
				change = pcb_true;
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
#endif
	PCB_RAT_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_CLEAR(flag, line);
			pcb_rat_invalidate_draw(line);
			change = pcb_true;
		}
	}
	PCB_END_LOOP;
	PCB_LINE_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, line)) {
			pcb_undo_add_obj_to_flag(line);
			PCB_FLAG_CLEAR(flag, line);
			pcb_line_invalidate_draw(layer, line);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, arc)) {
			pcb_undo_add_obj_to_flag(arc);
			PCB_FLAG_CLEAR(flag, arc);
			pcb_arc_invalidate_draw(layer, arc);
			change = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_COPPER_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(flag, polygon)) {
			pcb_undo_add_obj_to_flag(polygon);
			PCB_FLAG_CLEAR(flag, polygon);
			pcb_poly_invalidate_draw(layer, polygon);
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

static void unselect_widget(GtkWidget * widget, gpointer data)
{
	gtk_widget_set_state(widget, GTK_STATE_NORMAL);
}

void row_clicked_cb(GtkWidget * widget, GdkEvent * event, GhidDrcViolation * violation)
{
	int i;
	GtkWidget *parent;

	if (violation == NULL)
		return;

	/* Marks DRC error violation as selected line. De-select previous line. */
	parent = gtk_widget_get_parent(widget);
	gtk_container_foreach(GTK_CONTAINER(parent), unselect_widget, NULL);
	gtk_widget_set_state(widget, GTK_STATE_SELECTED);
	gtk_widget_queue_draw(parent);

	if (event->type == GDK_2BUTTON_PRESS) {
		unset_found_flags(pcb_false);

		/* Flag the objects listed against this DRC violation */
		for (i = 0; i < violation->object_count; i++) {
			int object_id = violation->object_id_list[i];
			int object_type = violation->object_type_list[i];
			int found_type;
			void *ptr1, *ptr2, *ptr3;

			found_type = pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, object_id, object_type);
			if (found_type == PCB_OBJ_VOID) {
				pcb_message(PCB_MSG_WARNING, _("Object ID %i identified during DRC was not found. Stale DRC window?\n"), object_id);
				continue;
			}
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(PCB_FLAG_FOUND, (pcb_any_obj_t *) ptr2);
			switch (violation->object_type_list[i]) {
			case PCB_OBJ_LINE:
			case PCB_OBJ_ARC:
			case PCB_OBJ_POLY:
				pcb_layervis_change_group_vis(pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1), pcb_true, pcb_true);
			}
			pcb_draw_obj((pcb_any_obj_t *)ptr2);
		}
		pcb_board_set_changed_flag(pcb_true);
		pcb_undo_inc_serial();
		pcb_draw();

		pcb_center_display(violation->x_coord, violation->y_coord);
	}
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
};


static GObjectClass *ghid_drc_violation_parent_class = NULL;


/* Just before the GhidDrcViolation GObject is finalized, free our
   allocated data, and then chain up to the parent's finalize handler. */
static void ghid_drc_violation_finalize(GObject * object)
{
	GhidDrcViolation *violation = GHID_DRC_VIOLATION(object);

	g_free(violation->title);
	g_free(violation->explanation);
	g_free(violation->object_id_list);
	g_free(violation->object_type_list);

	G_OBJECT_CLASS(ghid_drc_violation_parent_class)->finalize(object);
}

typedef struct object_list {
	int count;
	long int *id_list;
	int *type_list;
} object_list;

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
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		return;
	}
}

static void ghid_drc_violation_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}

}

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
}

/* Upon first call, register the GhidViolationRenderer in the GType system
 * and cache. Subsequently return the cached value. */
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

GhidDrcViolation *ghid_drc_violation_new(pcb_drc_violation_t * violation)
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
																					 "object-list", &obj_list, NULL);
}

/* Returns the DRC text string using markup feature. Should be freed with free() */
static char *get_drc_violation_markup(GhidDrcViolation * violation)
{
	char *markup;

	if (violation->have_measured) {
		markup = pcb_strdup_printf("%m+<b>%s (%$mS)</b>\n"
															 "<span size='1024'> </span>\n"
															 "<small>"
															 "<i>%s</i>\n"
															 "<span size='5120'> </span>\n"
															 "Required: %$mS"
															 "</small>",
															 conf_core.editor.grid_unit->allow,
															 violation->title, violation->measured_value, violation->explanation, violation->required_value);
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
															 violation->title, violation->explanation, violation->required_value);
	}

	return markup;
}

void ghid_drc_window_show(pcb_gtk_drcwin_t *drcwin, gboolean raise)
{
	GtkWidget *vbox, *hbox, *button, *scrolled_window, *label;
	GtkWidget *drc_window;

	if (drcwin->drc_window) {
		if (raise)
			gtk_window_present(GTK_WINDOW(drcwin->drc_window));
		return;
	}

	drc_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	drcwin->drc_window = drc_window;
	g_signal_connect(G_OBJECT(drc_window), "destroy", G_CALLBACK(drc_destroy_cb), drcwin);
	g_signal_connect(G_OBJECT(drc_window), "configure_event", G_CALLBACK(drc_window_configure_event_cb), NULL);
	gtk_window_set_title(GTK_WINDOW(drc_window), _("pcb-rnd DRC"));
	gtk_window_set_role(GTK_WINDOW(drc_window), "PCB_DRC");
	gtk_window_set_default_size(GTK_WINDOW(drc_window), hid_gtk_wgeo.drc_width, hid_gtk_wgeo.drc_height);

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(drc_window), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_box_set_spacing(GTK_BOX(vbox), 6);

	/* V/Hbox mechanism */
	hbox = gtkc_hbox_new(FALSE, 0);
	label = gtk_label_new("No.   Violation details");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtkc_hbox_new(FALSE, 0), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE /* EXPAND */ , TRUE /* FILL */ , 0 /* PADDING */ );

	drcwin->drc_vbox = gtkc_vbox_new(FALSE, 0);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtkc_scrolled_window_add_with_viewport(scrolled_window, drcwin->drc_vbox);

	/* Dialog buttons */
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_box_set_spacing(GTK_BOX(hbox), 6);

	button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(drc_refresh_cb), drcwin);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(drc_close_cb), drcwin);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	wplc_place(WPLC_DRC, drc_window);

	gtk_widget_realize(drc_window);
	gtk_widget_show_all(drc_window);
}

void ghid_drc_window_append_violation(pcb_gtk_drcwin_t *drcwin, pcb_gtk_common_t *common, pcb_drc_violation_t * violation)
{
	GhidDrcViolation *violation_obj;
	GtkWidget *hbox, *label;
	GtkWidget *event_box;
	char number[8];								/* if there is more than a million DRC errors ... change this ! */
	char *markup;
	GtkWidget *preview;
	int preview_size = VIOLATION_PIXMAP_PIXEL_SIZE - 2 * VIOLATION_PIXMAP_PIXEL_BORDER;

	/* Ensure the required structures are setup */
	ghid_drc_window_show(drcwin, FALSE);

	drcwin->num_violations += 1;

	violation_obj = ghid_drc_violation_new(violation);

	/* Pack texts and preview in an horizontal box (within an event box) */
	hbox = gtkc_hbox_new(FALSE, 0);
	event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box), hbox);
	g_signal_connect(event_box, "button-press-event", G_CALLBACK(row_clicked_cb), violation_obj);
	gtk_box_pack_start(GTK_BOX(drcwin->drc_vbox), event_box, TRUE, TRUE, VIOLATION_PIXMAP_PIXEL_BORDER);

	/*FIXME: Do we need to keep the DRC number for this violation ? What for ? */
	pcb_sprintf(number, " %d ", drcwin->num_violations);
	label = gtk_label_new(number);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	markup = get_drc_violation_markup(violation_obj);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	free(markup);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtkc_hbox_new(FALSE, 0), TRUE, TRUE, 0);

	preview = pcb_gtk_preview_board_new(common, common->init_drawing_widget, common->preview_expose);
	gtk_widget_set_size_request(preview, preview_size, preview_size);
	gtk_widget_set_events(preview, GDK_EXPOSURE_MASK);
	pcb_gtk_preview_board_zoomto(PCB_GTK_PREVIEW(preview),
															 violation_obj->x_coord - VIOLATION_PIXMAP_PCB_SIZE,
															 violation_obj->y_coord - VIOLATION_PIXMAP_PCB_SIZE,
															 violation_obj->x_coord + VIOLATION_PIXMAP_PCB_SIZE,
															 violation_obj->y_coord + VIOLATION_PIXMAP_PCB_SIZE, preview_size, preview_size);
	gtk_box_pack_start(GTK_BOX(hbox), preview, FALSE, FALSE, VIOLATION_PIXMAP_PIXEL_BORDER);

	gtk_widget_show_all(event_box);
}

void ghid_drc_window_reset_message(pcb_gtk_drcwin_t *drcwin)
{
	drcwin->num_violations = 0;
	gtk_container_foreach(GTK_CONTAINER(drcwin->drc_vbox), destroy_widget, NULL);
}

int ghid_drc_window_throw_dialog(pcb_gtk_drcwin_t *drcwin)
{
	ghid_drc_window_show(drcwin, TRUE);
	return 1;
}

void pcb_gtk_drcwin_init(pcb_gtk_drcwin_t *drcwin)
{
	drcwin->drc_window = NULL;
	drcwin->drc_vbox = NULL;
	drcwin->num_violations = 0;
}
