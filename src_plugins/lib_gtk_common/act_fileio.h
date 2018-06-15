#include <gtk/gtk.h>
#include "unit.h"

int pcb_gtk_act_load(GtkWidget *top_window, int argc, const char **argv);

extern const char pcb_gtk_acts_save[];
extern const char pcb_gtk_acth_save[];
int pcb_gtk_act_save(GtkWidget *top_window, int argc, const char **argv);

extern const char pcb_gtk_acts_importgui[];
extern const char pcb_gtk_acth_importgui[];
int pcb_gtk_act_importgui(GtkWidget *top_window, int argc, const char **argv);
