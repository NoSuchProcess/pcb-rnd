/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port and
   got heavily reworked for pcb-rnd */

/* provides an interface for getting user input for executing a command. */


#include "config.h"
#include "bu_command.h"
#include "conf_core.h"

#include <gdk/gdkkeysyms.h>

#include "board.h"
#include "crosshair.h"
#include "actions.h"

#include "bu_text_view.h"
#include "bu_status_line.h"
#include "hid_gtk_conf.h"
#include "../src_plugins/lib_hid_common/cli_history.h"


#include "compat.h"

/* Put an allocated string on the history list and combo text list
   if it is not a duplicate.  The combo box is just a shadow of the
   real, common history, shared with other HIDs. The combo box strings
   take "const gchar *", which are allocated within the common history. */

static void ghid_chist_append(void *ctx_, const char *cmd)
{
	pcb_gtk_command_t *ctx = (pcb_gtk_command_t *)ctx_;
	gtkc_combo_box_text_append_text(ctx->command_combo_box, cmd);
}

static void ghid_chist_remove(void *ctx_, int idx)
{
	pcb_gtk_command_t *ctx = (pcb_gtk_command_t *)ctx_;
	gtkc_combo_box_text_remove(ctx->command_combo_box, idx);
}

static void command_history_add(pcb_gtk_command_t *ctx, gchar *cmd)
{
	pcb_clihist_append(cmd, ctx, ghid_chist_append, ghid_chist_remove);
}


/* Called when user hits "Enter" key in command entry.  The action to take
   depends on where the combo box is.  If it's in the command window, we can
   immediately execute the command and carry on.  If it's in the status
   line hbox, then we need stop the command entry g_main_loop from running
   and save the allocated string so it can be returned from
   ghid_command_entry_get() */
static void command_entry_activate_cb(GtkWidget * widget, gpointer data)
{
	pcb_gtk_command_t *ctx = data;
	gchar *command;
	const gchar *cmd = gtk_entry_get_text(GTK_ENTRY(ctx->command_entry));

	if (cmd != NULL) {
		while ((*cmd == ' ') || (*cmd == '\t'))
			cmd++;
		command = g_strdup(cmd);
	}
	gtk_entry_set_text(ctx->command_entry, "");

	if (*command)
		command_history_add(ctx, command);

	if (ctx->ghid_entry_loop && g_main_loop_is_running(ctx->ghid_entry_loop)) /* should always be */
		g_main_loop_quit(ctx->ghid_entry_loop);
	ctx->command_entered = command; /* Caller will free it */
}

/* Create the command_combo_box.  Called once, by
   ghid_command_entry_get().  The command_combo_box
   lives in the status_line_hbox either shown or hidden.
   Since it's never destroyed, the combo history strings never need
   rebuilding. */
static void command_combo_box_entry_create(pcb_gtk_command_t *ctx)
{
	ctx->command_combo_box = gtk_combo_box_text_new_with_entry();
	ctx->command_entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ctx->command_combo_box)));

	gtk_entry_set_width_chars(ctx->command_entry, 40);
	gtk_entry_set_activates_default(ctx->command_entry, TRUE);

	g_signal_connect(G_OBJECT(ctx->command_entry), "activate", G_CALLBACK(command_entry_activate_cb), ctx);

	g_object_ref(G_OBJECT(ctx->command_combo_box)); /* so can move it */
	pcb_clihist_init();
	pcb_clihist_sync(ctx, ghid_chist_append);
}

void ghid_cmd_close(pcb_gtk_command_t *ctx)
{
	if (!ctx->com->command_entry_is_active())
		return;

	if (ctx->ghid_entry_loop && g_main_loop_is_running(ctx->ghid_entry_loop)) /* should always be (the entry is active) */
		g_main_loop_quit(ctx->ghid_entry_loop);

	ctx->command_entered = NULL; /* We are aborting */

	/* Hide the host widget in full screen - not enough if only the entry is gone */
	if (conf_core.editor.fullscreen)
		gtk_widget_hide(gtk_widget_get_parent(ctx->command_combo_box));
}

static pcb_bool command_keypress_cb(GtkWidget * widget, GdkEventKey * kev, pcb_gtk_command_t *ctx)
{
	gint ksym = kev->keyval;

	if (ksym == GDK_KEY_Tab) {
		pcb_cli_tab();
		return TRUE;
	}

	/* escape key handling */
	if (ksym == GDK_KEY_Escape) {
		ghid_cmd_close(ctx);
		return TRUE;
	}

	return FALSE;
}

static pcb_bool command_keyrelease_cb(GtkWidget *widget, GdkEventKey *kev, pcb_gtk_command_t *ctx)
{
	if (ctx->com->command_entry_is_active())
		pcb_cli_edit();
	return TRUE;
}

void ghid_command_update_prompt(pcb_gtk_command_t *ctx)
{
	if (ctx->prompt_label != NULL)
		gtk_label_set_text(GTK_LABEL(ctx->prompt_label), pcb_cli_prompt(":"));
}


/* This is the command entry function called from Action Command().
   The command_combo_box is packed into the status line label hbox. */
char *ghid_command_entry_get(pcb_gtk_command_t *ctx, const char *prompt, const char *command)
{
	gchar *s;
	gint escape_sig_id, escape_sig2_id;

	/* If this is the first user command entry, we have to create the
	   command_combo_box and pack it into the status_line_hbox. */
	if (!ctx->command_combo_box) {
		command_combo_box_entry_create(ctx);
		g_signal_connect(G_OBJECT(ctx->command_entry), "key_press_event", G_CALLBACK(command_keypress_cb), ctx);
		g_signal_connect(G_OBJECT(ctx->command_entry), "key_release_event", G_CALLBACK(command_keyrelease_cb), ctx);
		ctx->pack_in_status_line();
	}

	/* Make the prompt bold and set the label before showing the combo to
	   avoid window resizing wider. */
	s = g_strdup_printf("<b>%s</b>", prompt ? prompt : "");
	ctx->com->status_line_set_text(s);
	g_free(s);

	/* Flag so output drawing area won't try to get focus away from us and
	   so resetting the status line label can be blocked when resize
	   callbacks are invokded from the resize caused by showing the combo box. */
	ctx->command_entry_status_line_active = TRUE;

	gtk_entry_set_text(ctx->command_entry, command ? command : "");
	gtk_widget_show_all(gtk_widget_get_parent(ctx->command_combo_box));

	/* Remove the top window accel group so keys intended for the entry
	   don't get intercepted by the menu system.  Set the interface
	   insensitive so all the user can do is enter a command, grab focus
	   and connect a handler to look for the escape key. */
	ctx->pre_entry();

	gtk_widget_grab_focus(GTK_WIDGET(ctx->command_entry));
	escape_sig_id = g_signal_connect(G_OBJECT(ctx->command_entry), "key_press_event", G_CALLBACK(command_keypress_cb), ctx);
	escape_sig2_id = g_signal_connect(G_OBJECT(ctx->command_entry), "key_release_event", G_CALLBACK(command_keyrelease_cb), ctx);

	ctx->ghid_entry_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ctx->ghid_entry_loop);

	g_main_loop_unref(ctx->ghid_entry_loop);
	ctx->ghid_entry_loop = NULL;

	ctx->command_entry_status_line_active = FALSE;

	/* Restore the damage we did before entering the loop. */
	g_signal_handler_disconnect(ctx->command_entry, escape_sig_id);
	g_signal_handler_disconnect(ctx->command_entry, escape_sig2_id);

	/* Hide the widgets */
	if (conf_core.editor.fullscreen) {
		gtk_widget_hide(gtk_widget_get_parent(ctx->command_combo_box));
	}

	/* Restore the status line label and give focus back to the drawing area */
	gtk_widget_hide(ctx->command_combo_box);
	ctx->post_entry();

	return ctx->command_entered;
}


void ghid_handle_user_command(pcb_gtk_command_t *ctx, pcb_bool raise)
{
	char *command;
	static char *previous = NULL;

	command = ghid_command_entry_get(ctx, pcb_cli_prompt(":"), (conf_core.editor.save_last_command && previous) ? previous : (gchar *)"");
	if (command != NULL) {
		/* copy new command line to save buffer */
		g_free(previous);
		previous = g_strdup(command);
		pcb_parse_command(command, pcb_false);
		g_free(command);
	}
	ctx->com->window_set_name_label(PCB->Name);
	ctx->com->set_status_line_label();
}

const char *pcb_gtk_cmd_command_entry(pcb_gtk_command_t *ctx, const char *ovr, int *cursor)
{
	if (!ctx->com->command_entry_is_active()) {
		if (cursor != NULL)
			*cursor = -1;
		return NULL;
	}
	if (ovr != NULL) {
		gtk_entry_set_text(ctx->command_entry, ovr);
		if (cursor != NULL)
			gtk_editable_set_position(GTK_EDITABLE(ctx->command_entry), *cursor);
	}
	if (cursor != NULL)
		*cursor = gtk_editable_get_position(GTK_EDITABLE(ctx->command_entry));
	return gtk_entry_get_text(ctx->command_entry);
}

