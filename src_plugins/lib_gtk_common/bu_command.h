#ifndef PCB_GTK_DLG_COMMAND_H
#define PCB_GTK_DLG_COMMAND_H

#include <gtk/gtk.h>
#include <glib.h>
#include "pcb_bool.h"

typedef struct pcb_gtk_command_s {
	GtkWidget *command_combo_box, *prompt_label;
	GtkEntry *command_entry;
	gboolean command_entry_status_line_active;

	void (*post_entry)(void);
	void (*pre_entry)(void);

	struct pcb_gtk_impl_s *com;

	/* internal */
	GMainLoop *ghid_entry_loop;
	gchar *command_entered;
	void (*hide_status)(void*,int); /* called with status_ctx when the status line needs to be hidden */
	void *status_ctx;
} pcb_gtk_command_t;

void ghid_handle_user_command(pcb_gtk_command_t *ctx, pcb_bool raise);
char *ghid_command_entry_get(pcb_gtk_command_t *ctx, const char *prompt, const char *command);

/* Update the prompt text before the command entry - call it when any of conf_core.rc.cli_* change */
void ghid_command_update_prompt(pcb_gtk_command_t *ctx);

const char *pcb_gtk_cmd_command_entry(pcb_gtk_command_t *ctx, const char *ovr, int *cursor);

/* cancel editing and quit the main loop - NOP if not active*/
void ghid_cmd_close(pcb_gtk_command_t *ctx);

void ghid_command_combo_box_entry_create(pcb_gtk_command_t *ctx, void (*hide_status)(void*,int), void *status_ctx);

#endif
