/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"

#include "dlg_message.h"

/** :
 *  Display a new GtkMessageDialog FIXME.
 */

/* ---------------------------------------------- */
gint ghid_dialog_close_confirm()
{
	GtkWidget *dialog;
	gint rv;
	GHidPort *out = &ghid_port;
	gchar *tmp;
	gchar *str;
	const char *msg;

	if (PCB->Filename == NULL) {
		tmp = g_strdup_printf(_("Save the changes to layout before closing?"));
	}
	else {
		tmp = g_strdup_printf(_("Save the changes to layout \"%s\" before closing?"), PCB->Filename);
	}
	str = g_strconcat("<big><b>", tmp, "</b></big>", NULL);
	g_free(tmp);
	msg = _("If you don't save, all your changes will be permanently lost.");
	str = g_strconcat(str, "\n\n", msg, NULL);

	dialog = gtk_message_dialog_new(GTK_WINDOW(out->top_window),
																	(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
																	GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, NULL);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), str);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
												 _("Close _without saving"), GTK_RESPONSE_NO,
												 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_YES, NULL);

	/* Set the alternative button order (ok, cancel, help) for other systems */
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_YES, GTK_RESPONSE_NO, GTK_RESPONSE_CANCEL, -1);

	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_NO:
		{
			rv = GUI_DIALOG_CLOSE_CONFIRM_NOSAVE;
			break;
		}
	case GTK_RESPONSE_YES:
		{
			rv = GUI_DIALOG_CLOSE_CONFIRM_SAVE;
			break;
		}
	case GTK_RESPONSE_CANCEL:
	default:
		{
			rv = GUI_DIALOG_CLOSE_CONFIRM_CANCEL;
			break;
		}
	}
	gtk_widget_destroy(dialog);
	return rv;
}
