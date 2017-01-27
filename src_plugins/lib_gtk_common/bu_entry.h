#include <gtk/gtk.h>

/*FIXME: I think this is bad, long term speaking */
#include "../src_plugins/hid_gtk/ghid-coord-entry.h"

/**   */
void ghid_coord_entry(GtkWidget * box, GtkWidget ** coord_entry, pcb_coord_t value,
											pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size,
											const pcb_unit_t * u, gint width,
											void (*cb_func) (GHidCoordEntry *, gpointer),
											gpointer data, const gchar * string_pre, const gchar * string_post);

void ghid_table_coord_entry(GtkWidget * table, gint row, gint column,
														GtkWidget ** coord_entry, pcb_coord_t value,
														pcb_coord_t low, pcb_coord_t high, enum ce_step_size step_size,
														gint width, void (*cb_func) (GHidCoordEntry *, gpointer),
														gpointer data, gboolean right_align, const gchar * string);
