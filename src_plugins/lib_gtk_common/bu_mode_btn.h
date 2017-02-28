#ifndef PCB_GTK_BU_MODE_BTN_H
#define PCB_GTK_BU_MODE_BTN_H

#include <gtk/gtk.h>
#include "glue.h"

typedef struct {
	int settings_mode;
	GtkWidget *mode_buttons_frame;
	GtkWidget *mode_toolbar;
	GtkWidget *mode_toolbar_vbox;

	pcb_gtk_common_t *com;
} pcb_gtk_mode_btn_t;

void pcb_gtk_make_mode_buttons_and_toolbar(pcb_gtk_common_t *com, pcb_gtk_mode_btn_t *mb);
void ghid_mode_buttons_update(void);
void pcb_gtk_pack_mode_buttons(pcb_gtk_mode_btn_t *mb);


#endif