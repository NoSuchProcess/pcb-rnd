#ifndef PCB_GTK_DLG_COMMAND_H
#define PCB_GTK_DLG_COMMAND_H

#include <gtk/gtk.h>
#include <glib.h>
#include "pcb_bool.h"
#include "glue.h"

typedef struct pcb_gtk_command_s {
	GtkWidget *command_combo_box;
	GtkEntry *command_entry;
	gboolean command_entry_status_line_active;

	void (*pack_in_status_line)(void); /* embed the command combo box in top window's status line */
	void (*post_entry)(void);
	void (*pre_entry)(void);

	pcb_gtk_common_t *com;
} pcb_gtk_command_t;

void ghid_handle_user_command(pcb_gtk_command_t *ctx, pcb_bool raise);
void ghid_command_window_show(pcb_gtk_command_t *ctx, pcb_bool raise);
char *ghid_command_entry_get(pcb_gtk_command_t *ctx, const char *prompt, const char *command);
void command_window_close_cb(pcb_gtk_command_t *ctx);


#endif
