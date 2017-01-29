#include <gtk/gtk.h>

#include "wt_coord_entry.h"

/** TODO: Rename these API ?  */
void ghid_coord_entry(GtkWidget * box, GtkWidget ** coord_entry, pcb_coord_t value,
											pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size,
											const pcb_unit_t * u, gint width,
											void (*cb_func) (pcb_gtk_coord_entry_t *, gpointer),
											gpointer data, const gchar * string_pre, const gchar * string_post);

void ghid_table_coord_entry(GtkWidget * table, gint row, gint column,
														GtkWidget ** coord_entry, pcb_coord_t value,
														pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size,
														gint width, void (*cb_func) (pcb_gtk_coord_entry_t *, gpointer),
														gpointer data, gboolean right_align, const gchar * string);
