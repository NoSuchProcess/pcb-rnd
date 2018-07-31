/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"
#include "dlg_command.h"
#include "conf_core.h"

#include <gdk/gdkkeysyms.h>

#include "board.h"
#include "crosshair.h"
#include "actions.h"
#include "compat_nls.h"

#include "bu_text_view.h"
#include "bu_status_line.h"
#include "util_str.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#include "compat.h"


static GtkWidget *command_window;
static GtkWidget *combo_vbox;
static GList *history_list;
static gchar *command_entered;
GMainLoop *ghid_entry_loop;


/* gui-command-window.c provides two interfaces for getting user input
|  for executing a command.
|
|  As the Xt PCB was ported to Gtk, the traditional user entry in the
|  status line window presented some focus problems which require that
|  there can be no menu key shortcuts that might be a key the user would
|  type in.  It also requires a coordinating flag so the drawing area
|  won't grab focus while the command entry is up.
|
|  I thought the interface should be cleaner, so I made an alternate
|  command window interface which works better I think as a GUI interface.
|  The user must focus onto the command window, but since it's a separate
|  window, there's no confusion.  It has the restriction that objects to
|  be operated on must be selected, but that actually seems a better user
|  interface than one where typing into one location requires the user to
|  be careful about which object might be under the cursor somewhere else.
|
|  In any event, both interfaces are here to work with.
*/


	/* When using a command window for command entry, provide a quick and
	   |  abbreviated reference to available commands.
	   |  This is currently just a start and can be expanded if it proves useful.
	 */
static const gchar *command_ref_text[] = {
	N_("Common commands easily accessible via the gui may not be included here.\n"),
	"\n",
	N_("In user commands below, 'size' values may be absolute or relative\n"
		 "if preceded by a '+' or '-'.  Where 'units' are indicated, use \n"
		 "'mil' or 'mm' otherwise PCB internal units will be used.\n"),
	"\n",
	"<b>changesize(target, size, units)\n",
	"\ttarget = {selectedlines | selectedpins | selectedvias | selectedpads \n"
		"\t\t\t| selectedtexts | selectednames | selectedelements | selected}\n",
	"\n",
	"<b>changedrillsize(target, size, units)\n",
	"\ttarget = {selectedpins | selectedvias | selectedobjects | selected}\n",
	"\n",
	"<b>changeclearsize(target, size, units)\n",
	"\ttarget = {selectedpins | selectedpads | selectedvias | selectedlines\n"
		"\t\t\t| selectedarcs | selectedobjects | selected}\n",
	N_("\tChanges the clearance of objects.\n"),
	"\n",
	"<b>setvalue(target, size, units)\n",
	"\ttarget = {grid | zoom | line | textscale | viadrillinghole\n" "\t\t\t| viadrillinghole | via}\n",
	N_("\tChanges values.  Omit 'units' for 'grid' and 'zoom'.\n"),
	"\n",
	"<b>changejoin(target)\n",
	"\ttarget = {object | selectedlines | selectedarcs | selected}\n",
	N_("\tChanges the join (clearance through polygons) of objects.\n"),
	"\n",
	"<b>changesquare(target)\n",
	"<b>setsquare(target)\n",
	"<b>clearsquare(target)\n",
	"\ttarget = {object | selectedelements | selectedpins | selected}\n",
	N_("\tToggles, sets, or clears the square flag of objects.\n"),
	"\n",
	"<b>changeoctagon(target)\n",
	"<b>setoctagon(target)\n",
	"<b>clearoctagon(target)\n",
	"\ttarget = {object | selectedelements | selectedpins selectedvias | selected}\n",
	N_("\tToggles, sets, or clears the octagon flag of objects.\n"),
	"\n",
	"<b>changehole(target)\n",
	"\ttarget = {object | selectedvias | selected}\n",
	N_("\tChanges the hole flag of objects.\n"),
	"\n",
	"<b>flip(target)\n",
	"\ttarget = {object | selectedelements | selected}\n",
	N_("\tFlip elements to the opposite side of the board.\n"),
	"\n",
	"<b>togglethermal(target)\n",
	"<b>setthermal(target)\n",
	"<b>clearthermal(target)\n",
	"\ttarget = {object | selectedpins | selectedvias | selected}\n",
	N_("\tToggle, set or clear a thermal (on the current layer) to pins or vias.\n"),
	"\n",
	"<b>loadvendor(target)\n",
	"\ttarget = [filename]\n",
	N_("\tLoad a vendor file.  If 'filename' omitted, pop up file select dialog.\n"),
};


	/* Put an allocated string on the history list and combo text list
	   |  if it is not a duplicate.  The history_list is just a shadow of the
	   |  combo list, but I think is needed because I don't see an API for reading
	   |  the combo strings.  The combo box strings take "const gchar *", so the
	   |  same allocated string can go in both the history list and the combo list.
	   |  If removed from both lists, a string can be freed.
	 */
static void command_history_add(pcb_gtk_command_t *ctx, gchar *cmd)
{
	GList *list;
	gchar *s;
	gint i;

	if (!cmd || !*cmd)
		return;

	/* Check for a duplicate command.  If found, move it to the
	   |  top of the list and similarly modify the combo box strings.
	 */
	for (i = 0, list = history_list; list; list = list->next, ++i) {
		s = (gchar *) list->data;
		if (!strcmp(cmd, s)) {
			history_list = g_list_remove(history_list, s);
			history_list = g_list_append(history_list, s);
			gtkc_combo_box_text_remove(ctx->command_combo_box, i);
			gtkc_combo_box_text_append_text(ctx->command_combo_box, s);
			return;
		}
	}

	/* Not a duplicate, so put first in history list and combo box text list.
	 */
	s = g_strdup(cmd);
	history_list = g_list_append(history_list, s);
	gtkc_combo_box_text_append_text(ctx->command_combo_box, s);

	/* And keep the lists trimmed!
	 */
	if (g_list_length(history_list) > conf_hid_gtk.plugins.hid_gtk.history_size) {
		s = (gchar *) g_list_nth_data(history_list, 0);
		history_list = g_list_remove(history_list, s);
		gtkc_combo_box_text_remove(ctx->command_combo_box, 0);
		g_free(s);
	}
}


	/* Called when user hits "Enter" key in command entry.  The action to take
	   |  depends on where the combo box is.  If it's in the command window, we can
	   |  immediately execute the command and carry on.  If it's in the status
	   |  line hbox, then we need stop the command entry g_main_loop from running
	   |  and save the allocated string so it can be returned from
	   |  ghid_command_entry_get()
	 */
static void command_entry_activate_cb(GtkWidget * widget, gpointer data)
{
	pcb_gtk_command_t *ctx = data;
	gchar *command;

	/*command = g_strdup(ghid_entry_get_text(GTK_WIDGET(ctx->command_entry))); */
	command = g_strdup(pcb_str_strip_left(gtk_entry_get_text(GTK_ENTRY(ctx->command_entry))));
	gtk_entry_set_text(ctx->command_entry, "");

	if (*command)
		command_history_add(ctx, command);

	if (conf_hid_gtk.plugins.hid_gtk.use_command_window) {
		pcb_parse_command(command, pcb_false);
		g_free(command);
	}
	else {
		if (ghid_entry_loop && g_main_loop_is_running(ghid_entry_loop))	/* should always be */
			g_main_loop_quit(ghid_entry_loop);
		command_entered = command;	/* Caller will free it */
	}
}

	/* Create the command_combo_box.  Called once, either by
	   |  ghid_command_window_show() or ghid_command_entry_get().  Then as long as
	   |  conf_hid_gtk.plugins.hid_gtk.use_command_window is TRUE, the command_combo_box will live
	   |  in a command window vbox or float if the command window is not up.
	   |  But if conf_hid_gtk.plugins.hid_gtk.use_command_window is FALSE, the command_combo_box
	   |  will live in the status_line_hbox either shown or hidden.
	   |  Since it's never destroyed, the combo history strings never need
	   |  rebuilding and history is maintained if the combo box location is moved.
	 */
static void command_combo_box_entry_create(pcb_gtk_command_t *ctx)
{
	ctx->command_combo_box = gtk_combo_box_text_new_with_entry();
	ctx->command_entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(ctx->command_combo_box)));

	gtk_entry_set_width_chars(ctx->command_entry, 40);
	gtk_entry_set_activates_default(ctx->command_entry, TRUE);

	g_signal_connect(G_OBJECT(ctx->command_entry), "activate", G_CALLBACK(command_entry_activate_cb), ctx);

	g_object_ref(G_OBJECT(ctx->command_combo_box)); /* so can move it */
}

/* CB function related to Command Window destruction */
static void command_destroy_cb(GtkWidget *dlg, pcb_gtk_command_t *ctx)
{
	if (command_window) {
		gtk_container_remove(GTK_CONTAINER(combo_vbox), /* Float it */
			ctx->command_combo_box);
		gtk_widget_hide(command_window);
	}
	ctx->prompt_label = NULL;
	combo_vbox = NULL;
	/* Command Window is hidden/destroyed, so expected future value for command_window is NULL. */
	command_window = NULL;
}

static pcb_bool command_keypress_cb(GtkWidget * widget, GdkEventKey * kev, pcb_gtk_command_t *ctx)
{
	gint ksym = kev->keyval;

	if (ksym == GDK_KEY_Tab) {
		pcb_cli_tab();
		return TRUE;
	}

	/* escape key handling */
	if (ksym != GDK_KEY_Escape)
		return FALSE;

	if (command_window) {
		command_window_close_cb(ctx);
		return TRUE;
	}

	if (ghid_entry_loop && g_main_loop_is_running(ghid_entry_loop))	/* should always be */
		g_main_loop_quit(ghid_entry_loop);
	command_entered = NULL;				/* We are aborting */
	/* Hidding the widgets */
	if (conf_core.editor.fullscreen) {
		gtk_widget_hide(gtk_widget_get_parent(ctx->command_combo_box));
	}

	return TRUE;
}


	/* If conf_hid_gtk.plugins.hid_gtk.use_command_window is TRUE this will get called from
	   |  Action Command() to show the command window.
	 */
void ghid_command_window_show(pcb_gtk_command_t *ctx, pcb_bool raise)
{
	GtkWidget *vbox, *vbox1, *hbox, *button, *expander, *text;
	gint i;

	if (command_window != NULL) {
		if (raise)
			gtk_window_present(GTK_WINDOW(command_window));
		return;
	}

	command_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(command_window), "destroy", G_CALLBACK(command_destroy_cb), ctx);
	gtk_window_set_title(GTK_WINDOW(command_window), _("pcb-rnd Command Entry"));
	gtk_window_set_role(GTK_WINDOW(command_window), "PCB_Command");
	gtk_window_set_resizable(GTK_WINDOW(command_window), FALSE);

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(command_window), vbox);

	ctx->prompt_label = gtk_label_new(pcb_cli_prompt(":"));
	gtk_box_pack_start(GTK_BOX(vbox), ctx->prompt_label, FALSE, FALSE, 0);

	if (!ctx->command_combo_box) {
		command_combo_box_entry_create(ctx);
		g_signal_connect(G_OBJECT(ctx->command_entry), "key_press_event", G_CALLBACK(command_keypress_cb), ctx);
	}

	gtk_box_pack_start(GTK_BOX(vbox), ctx->command_combo_box, FALSE, FALSE, 0);
	combo_vbox = vbox;

	/* Make the command reference scrolled text view.  Use high level
	   |  utility functions in gui-utils.c
	 */
	expander = gtk_expander_new(_("Command Reference"));
	gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE, TRUE, 2);
	vbox1 = gtkc_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox1);
	gtk_widget_set_size_request(vbox1, -1, 350);

	text = ghid_scrolled_text_view(vbox1, NULL, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	for (i = 0; i < sizeof(command_ref_text) / sizeof(gchar *); ++i)
		ghid_text_view_append(text, _(command_ref_text[i]));

	/* The command window close button.
	 */
	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(command_destroy_cb), ctx);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	gtk_widget_show_all(command_window);
}

void ghid_command_update_prompt(pcb_gtk_command_t *ctx)
{
	if (ctx->prompt_label != NULL)
		gtk_label_set_text(GTK_LABEL(ctx->prompt_label), pcb_cli_prompt(":"));
}


	/* This is the command entry function called from Action Command() when
	   |  conf_hid_gtk.plugins.hid_gtk.use_command_window is FALSE.  The command_combo_box is already
	   |  packed into the status line label hbox in this case.
	 */
char *ghid_command_entry_get(pcb_gtk_command_t *ctx, const char *prompt, const char *command)
{
	gchar *s;
	gint escape_sig_id;

	/* If this is the first user command entry, we have to create the
	   |  command_combo_box and pack it into the status_line_hbox.
	 */
	if (!ctx->command_combo_box) {
		command_combo_box_entry_create(ctx);
		g_signal_connect(G_OBJECT(ctx->command_entry), "key_press_event", G_CALLBACK(command_keypress_cb), ctx);
		ctx->pack_in_status_line();
	}

	/* Make the prompt bold and set the label before showing the combo to
	   |  avoid window resizing wider.
	 */
	s = g_strdup_printf("<b>%s</b>", prompt ? prompt : "");
	ctx->com->status_line_set_text(s);
	g_free(s);

	/* Flag so output drawing area won't try to get focus away from us and
	   |  so resetting the status line label can be blocked when resize
	   |  callbacks are invokded from the resize caused by showing the combo box.
	 */
	ctx->command_entry_status_line_active = TRUE;

	gtk_entry_set_text(ctx->command_entry, command ? command : "");
	gtk_widget_show_all(gtk_widget_get_parent(ctx->command_combo_box));

	/* Remove the top window accel group so keys intended for the entry
	   |  don't get intercepted by the menu system.  Set the interface
	   |  insensitive so all the user can do is enter a command, grab focus
	   |  and connect a handler to look for the escape key.
	 */
	ctx->pre_entry();

	gtk_widget_grab_focus(GTK_WIDGET(ctx->command_entry));
	escape_sig_id = g_signal_connect(G_OBJECT(ctx->command_entry), "key_press_event", G_CALLBACK(command_keypress_cb), ctx);

	ghid_entry_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ghid_entry_loop);

	g_main_loop_unref(ghid_entry_loop);
	ghid_entry_loop = NULL;

	ctx->command_entry_status_line_active = FALSE;

	/* Restore the damage we did before entering the loop.
	 */
	g_signal_handler_disconnect(ctx->command_entry, escape_sig_id);

	/* Hide the widgets */
	if (conf_core.editor.fullscreen) {
		gtk_widget_hide(gtk_widget_get_parent(ctx->command_combo_box));
	}

	/* Restore the status line label and give focus back to the drawing area
	 */
	gtk_widget_hide(ctx->command_combo_box);
	ctx->post_entry();

	return command_entered;
}


void ghid_handle_user_command(pcb_gtk_command_t *ctx, pcb_bool raise)
{
	char *command;
	static char *previous = NULL;

	if (conf_hid_gtk.plugins.hid_gtk.use_command_window)
		ghid_command_window_show(ctx, raise);
	else {
		command = ghid_command_entry_get(ctx, pcb_cli_prompt(":"), (conf_core.editor.save_last_command && previous) ? previous : (gchar *)"");
		if (command != NULL) {
			/* copy new command line to save buffer */
			g_free(previous);
			previous = g_strdup(command);
			pcb_parse_command(command, pcb_false);
			g_free(command);
		}
	}
	ctx->com->window_set_name_label(PCB->Name);
	ctx->com->set_status_line_label();
}

void command_window_close_cb(pcb_gtk_command_t *ctx)
{
	if (command_window)
		gtk_widget_destroy(command_window);
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
