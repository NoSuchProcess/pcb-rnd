#ifndef PCB_GTK_BU_CURSOR_POS_H
#define PCB_GTK_BU_CURSOR_POS_H

#include <gtk/gtk.h>

typedef struct pcb_gtk_cursor_pos_s {
	GtkWidget *cursor_position_relative_label;
	GtkWidget *cursor_position_absolute_label;
	GtkWidget *grid_units_label;
	GtkWidget *grid_units_button;
} pcb_gtk_cursor_pos_t;

/* Create cps's widgets in the hbox */
void make_cursor_position_labels(GtkWidget *hbox, pcb_gtk_cursor_pos_t *cps);

/* update the content of the cursor pos labels */
void ghid_set_cursor_position_labels(pcb_gtk_cursor_pos_t *cps, int compact);

#endif
