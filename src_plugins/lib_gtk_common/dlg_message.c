/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include "dlg_message.h"
#include "compat_nls.h"

gint pcb_gtk_dlg_message(const char *message, GtkWindow * parent)
{
	GtkWidget *dialog;
	gint rv;

	dialog = gtk_message_dialog_new(parent,
																	(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
																	GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, NULL);
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), message);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
												 _("Close _without saving"), GTK_RESPONSE_NO,
												 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_YES, NULL);

	/* Set the alternative button order (ok, cancel, help) for other systems */
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_YES, GTK_RESPONSE_NO, GTK_RESPONSE_CANCEL, -1);

	rv = gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
	return rv;
}
