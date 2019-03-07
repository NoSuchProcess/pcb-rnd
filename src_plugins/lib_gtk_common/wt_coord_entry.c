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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/*  This widget is a modified spinbox for the user to enter
 *  pcb coords. It is assigned a default unit (for display),
 *  but this can be changed by the user by typing a new one
 *  or right-clicking on the box.
 *
 *  Internally, it keeps track of its value in pcb coords.
 *  From the user's perspective, it uses natural human units.
 */

#include "config.h"

#include "wt_coord_entry.h"

#include "pcb-printf.h"

enum {
	UNIT_CHANGE_SIGNAL,
	LAST_SIGNAL
};

static guint ghid_coord_entry_signals[LAST_SIGNAL] = { 0 };

struct pcb_gtk_coord_entry_s {
	GtkSpinButton parent;

	GtkAdjustment *adj;

	pcb_coord_t min_value;
	pcb_coord_t max_value;
	pcb_coord_t value;

	enum ce_step_size step_size;
	const pcb_unit_t *unit;
};

struct pcb_gtk_coord_entry_class_s {
	GtkSpinButtonClass parent_class;

	void (*change_unit) (pcb_gtk_coord_entry_t *, const pcb_unit_t *);
};

/* SIGNAL HANDLERS */
/* Callback for "Change Unit" menu click */
static void menu_item_activate_cb(GtkMenuItem * item, pcb_gtk_coord_entry_t * ce)
{
	const char *text = gtk_menu_item_get_label(item);
	const pcb_unit_t *unit = get_unit_struct(text);

	g_signal_emit(ce, ghid_coord_entry_signals[UNIT_CHANGE_SIGNAL], 0, unit);
}

/* Callback for context menu creation */
static void ghid_coord_entry_popup_cb(pcb_gtk_coord_entry_t * ce, GtkMenu * menu, gpointer data)
{
#if 0
/* this should work again in the DAD rewrite, but not from popup */
	int i, n;
	const pcb_unit_t *unit_list;
	GtkWidget *menu_item, *submenu;

	/* Build submenu */
	n = pcb_get_n_units();
	unit_list = pcb_units;

	submenu = gtk_menu_new();
	for (i = 0; i < n; ++i) {
		menu_item = gtk_menu_item_new_with_label(unit_list[i].suffix);
		g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(menu_item_activate_cb), ce);
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
		gtk_widget_show(menu_item);
	}

	/* Add submenu to menu */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_show(menu_item);

	menu_item = gtk_menu_item_new_with_label("Change Units");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), submenu);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);
	gtk_widget_show(menu_item);
#endif
}

/* Callback for user output */
static gboolean ghid_coord_entry_output_cb(pcb_gtk_coord_entry_t * ce, gpointer data)
{
	GtkAdjustment *adj = ce->adj;
	double ovdbl, value = gtk_adjustment_get_value(adj);
	const char *orig;
	char text[128], *suffix = NULL;
	const pcb_unit_t *orig_unit;
	pcb_coord_t orig_val;

	orig = gtk_entry_get_text(GTK_ENTRY(ce));
	ovdbl = strtod(orig, &suffix);
	orig_unit = get_unit_struct(suffix);

	if ((suffix != NULL) && (*suffix != '\0') && (orig_unit != NULL) && (ce->unit != orig_unit))
		pcb_gtk_coord_entry_set_unit(ce, orig_unit);

	if (value != ovdbl) {
		pcb_snprintf(text, sizeof(text), "%.*f %s", ce->unit->default_prec, value, ce->unit->suffix);
		gtk_entry_set_text(GTK_ENTRY(ce), text);
	}

	return TRUE;
}

/* Callback for user input */
static gboolean ghid_coord_text_changed_cb(pcb_gtk_coord_entry_t * entry, gpointer data)
{
	const char *text;
	char *suffix;
	const pcb_unit_t *new_unit;
	double value;

	/* Check if units have changed */
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	value = strtod(text, &suffix);
	new_unit = get_unit_struct(suffix);
	if (new_unit && new_unit != entry->unit) {
		entry->value = pcb_unit_to_coord(new_unit, value);
		g_signal_emit(entry, ghid_coord_entry_signals[UNIT_CHANGE_SIGNAL], 0, new_unit);
	}

	return FALSE;
}

/* Callback for change in value (input or ^v clicks) */
static gboolean ghid_coord_value_changed_cb(pcb_gtk_coord_entry_t * ce, gpointer data)
{
	GtkAdjustment *adj = ce->adj;

	/* Re-calculate internal value */
	double value = gtk_adjustment_get_value(adj);
	ce->value = pcb_unit_to_coord(ce->unit, value);
	/* Handle potential unit changes */
	ghid_coord_text_changed_cb(ce, data);

	return FALSE;
}

static void ghid_coord_entry_change_unit(pcb_gtk_coord_entry_t * ce, const pcb_unit_t * new_unit)
{
	double climb_rate = 0.0;
	GtkAdjustment *adj = ce->adj;

	ce->unit = new_unit;
	/* Re-calculate min/max values for spinbox */
	gtk_adjustment_configure(adj, pcb_coord_to_unit(new_unit, ce->value),
													 pcb_coord_to_unit(new_unit, ce->min_value),
													 pcb_coord_to_unit(new_unit, ce->max_value), ce->unit->step_small, ce->unit->step_medium, 0.0);

	switch (ce->step_size) {
	case CE_TINY:
		climb_rate = new_unit->step_tiny;
		break;
	case CE_SMALL:
		climb_rate = new_unit->step_small;
		break;
	case CE_MEDIUM:
		climb_rate = new_unit->step_medium;
		break;
	case CE_LARGE:
		climb_rate = new_unit->step_large;
		break;
	}
	gtk_spin_button_configure(GTK_SPIN_BUTTON(ce), adj, climb_rate, new_unit->default_prec + strlen(new_unit->suffix));
}

/* CONSTRUCTOR */
static void ghid_coord_entry_init(pcb_gtk_coord_entry_t * ce)
{
	/* Hookup signal handlers */
	g_signal_connect(G_OBJECT(ce), "focus_out_event", G_CALLBACK(ghid_coord_text_changed_cb), NULL);
	g_signal_connect(G_OBJECT(ce), "value_changed", G_CALLBACK(ghid_coord_value_changed_cb), NULL);
	g_signal_connect(G_OBJECT(ce), "populate_popup", G_CALLBACK(ghid_coord_entry_popup_cb), NULL);
	g_signal_connect(G_OBJECT(ce), "output", G_CALLBACK(ghid_coord_entry_output_cb), NULL);
}

static void ghid_coord_entry_class_init(pcb_gtk_coord_entry_class_t * klass)
{
	klass->change_unit = ghid_coord_entry_change_unit;

	/* GtkAutoComplete *ce : the object acted on */
	/* const pcb_unit_t *new_unit: the new unit that was set */
	ghid_coord_entry_signals[UNIT_CHANGE_SIGNAL] =
		g_signal_new("change-unit",
								 G_TYPE_FROM_CLASS(klass),
								 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
								 G_STRUCT_OFFSET(pcb_gtk_coord_entry_class_t, change_unit),
								 NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

}

/* PUBLIC FUNCTIONS */
GType pcb_gtk_coord_entry_get_type(void)
{
	static GType ce_type = 0;

	if (!ce_type) {
		const GTypeInfo ce_info = {
			sizeof(pcb_gtk_coord_entry_class_t),
			NULL,											/* base_init */
			NULL,											/* base_finalize */
			(GClassInitFunc) ghid_coord_entry_class_init,
			NULL,											/* class_finalize */
			NULL,											/* class_data */
			sizeof(pcb_gtk_coord_entry_t),
			0,												/* n_preallocs */
			(GInstanceInitFunc) ghid_coord_entry_init,
		};

		ce_type = g_type_register_static(GTK_TYPE_SPIN_BUTTON, "pcb_gtk_coord_entry_t", &ce_info, 0);
	}

	return ce_type;
}

static void coord_cfg(pcb_gtk_coord_entry_t *ce)
{
	/* Setup spinbox min/max values */
	double small_step, big_step;
	
	switch (ce->step_size) {
	case CE_TINY:
		small_step = ce->unit->step_tiny;
		big_step = ce->unit->step_small;
		break;
	case CE_SMALL:
		small_step = ce->unit->step_small;
		big_step = ce->unit->step_medium;
		break;
	case CE_MEDIUM:
		small_step = ce->unit->step_medium;
		big_step = ce->unit->step_large;
		break;
	case CE_LARGE:
		small_step = ce->unit->step_large;
		big_step = ce->unit->step_huge;
		break;
	default:
		small_step = big_step = 0;
		break;
	}

	if (ce->adj == NULL) {
		ce->adj = GTK_ADJUSTMENT(gtk_adjustment_new(pcb_coord_to_unit(ce->unit, ce->value),
																					pcb_coord_to_unit(ce->unit, ce->min_value),
																					pcb_coord_to_unit(ce->unit, ce->max_value), small_step, big_step, 0.0));
		gtk_spin_button_configure(GTK_SPIN_BUTTON(ce), ce->adj, small_step, ce->unit->default_prec + strlen(ce->unit->suffix));
	}
	else {
		gtk_adjustment_configure(ce->adj, pcb_coord_to_unit(ce->unit, ce->value),
																					pcb_coord_to_unit(ce->unit, ce->min_value),
																					pcb_coord_to_unit(ce->unit, ce->max_value), small_step, big_step, 0.0);
	}
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ce), FALSE);
}

GtkWidget *pcb_gtk_coord_entry_new(pcb_coord_t min_val, pcb_coord_t max_val, pcb_coord_t value, const pcb_unit_t * unit,
																	 enum ce_step_size step_size)
{
	pcb_gtk_coord_entry_t *ce = g_object_new(GHID_COORD_ENTRY_TYPE, NULL);

	ce->unit = unit;
	ce->min_value = min_val;
	ce->max_value = max_val;
	ce->value = value;
	ce->step_size = step_size;
	ce->adj = NULL;

	coord_cfg(ce);

	return GTK_WIDGET(ce);
}

pcb_coord_t pcb_gtk_coord_entry_get_value(pcb_gtk_coord_entry_t * ce)
{
	return ce->value;
}

int pcb_gtk_coord_entry_get_value_str(pcb_gtk_coord_entry_t * ce, char *out, int out_len)
{
	GtkAdjustment *adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ce));
	double value = gtk_adjustment_get_value(adj);
	return pcb_snprintf(out, out_len, "%.*f %s", ce->unit->default_prec, value, ce->unit->suffix);
}

void pcb_gtk_coord_entry_set_value(pcb_gtk_coord_entry_t * ce, pcb_coord_t val)
{
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(ce), pcb_coord_to_unit(ce->unit, val));
}

int pcb_gtk_coord_entry_set_unit(pcb_gtk_coord_entry_t *ce, const pcb_unit_t *unit)
{
	const char *text;
	char *suffix;

	if (ce->unit == unit)
		return 0;

	text = gtk_entry_get_text(GTK_ENTRY(ce));
	ce->value = pcb_unit_to_coord(unit, strtod(text, &suffix));
	ce->unit = unit;
	coord_cfg(ce);
	return 1;
}
