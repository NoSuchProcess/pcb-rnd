#ifndef PCB_GTK_BU_INFO_BAR_H
#define PCB_GTK_BU_INFO_BAR_H

#include <gtk/gtk.h>

typedef struct {
	GtkWidget *info_bar;
} pcb_gtk_info_bar_t;

void pcb_gtk_close_info_bar(pcb_gtk_info_bar_t *ibar);

void pcb_gtk_info_bar_file_extmod_prompt(pcb_gtk_info_bar_t *ibar, GtkWidget *vbox_middle);

#endif
