/* This is the modified GtkSpinbox used for entering Coords.
 * Hopefully it can be used as a template whenever we migrate the
 * rest of the Gtk HID to use GObjects and GtkWidget subclassing. */
#ifndef GHID_COORD_ENTRY_H__
#define GHID_COORD_ENTRY_H__

#include <gtk/gtk.h>

#include "unit.h"

#define GHID_COORD_ENTRY_TYPE            (pcb_gtk_coord_entry_get_type ())
#define GHID_COORD_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GHID_COORD_ENTRY_TYPE, pcb_gtk_coord_entry_t))
#define GHID_COORD_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GHID_COORD_ENTRY_TYPE, pcb_gtk_coord_entry_class_t))
#define IS_GHID_COORD_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GHID_COORD_ENTRY_TYPE))
#define IS_GHID_COORD_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GHID_COORD_ENTRY_TYPE))
typedef struct pcb_gtk_coord_entry_s pcb_gtk_coord_entry_t;
typedef struct pcb_gtk_coord_entry_class_s pcb_gtk_coord_entry_class_t;

enum ce_step_size { CE_TINY, CE_SMALL, CE_MEDIUM, CE_LARGE };

GType pcb_gtk_coord_entry_get_type(void);

GtkWidget *pcb_gtk_coord_entry_new(pcb_coord_t min_val, pcb_coord_t max_val, pcb_coord_t default_value, const pcb_unit_t *default_unit, enum ce_step_size default_step_size);
pcb_coord_t pcb_gtk_coord_entry_get_value(pcb_gtk_coord_entry_t * ce);
int pcb_gtk_coord_entry_get_value_str(pcb_gtk_coord_entry_t * ce, char *out, int out_len);
void pcb_gtk_coord_entry_set_value(pcb_gtk_coord_entry_t * ce, pcb_coord_t val);

/* Change the unit only if it differs from what's set currently; returns whether changed */
int pcb_gtk_coord_entry_set_unit(pcb_gtk_coord_entry_t *ce, const pcb_unit_t *unit);

#endif
