/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"
#include "dlg_confirm.h"
#include "compat_nls.h"
#include "board.h"
#include "actions.h"

#include "compat.h"

#include "dlg_message.h"

int pcb_gtk_dlg_confirm_open(GtkWidget *top_window, const char *msg, va_list ap)
{
	int rv = 0;
	const char *cancelmsg = NULL, *okmsg = NULL;
	static gint x = -1, y = -1;
	GtkWidget *dialog;

	cancelmsg = va_arg(ap, char *);
	okmsg = va_arg(ap, char *);

	if (!cancelmsg) {
		cancelmsg = _("_Cancel");
		okmsg = _("_OK");
	}

	dialog = gtk_message_dialog_new(GTK_WINDOW(top_window),
																	(GtkDialogFlags) (GTK_DIALOG_MODAL |
																										GTK_DIALOG_DESTROY_WITH_PARENT),
																	GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", msg);
	gtk_dialog_add_button(GTK_DIALOG(dialog), cancelmsg, GTK_RESPONSE_CANCEL);
	if (okmsg) {
		gtk_dialog_add_button(GTK_DIALOG(dialog), okmsg, GTK_RESPONSE_OK);
	}

	if (x != -1) {
		gtk_window_move(GTK_WINDOW(dialog), x, y);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
		rv = 1;

	gtk_window_get_position(GTK_WINDOW(dialog), &x, &y);

	gtk_widget_destroy(dialog);
	return rv;
}

int pcb_gtk_dlg_confirm_close(GtkWidget *top_window)
{
	gchar *bold_text;
	gchar *str;
	const char *msg;

	if (PCB->Filename == NULL) {
		bold_text = g_strdup_printf(_("Save the changes to layout before closing?"));
	}
	else {
		bold_text = g_strdup_printf(_("Save the changes to layout \"%s\" before closing?"), PCB->Filename);
	}
	str = g_strconcat("<big><b>", bold_text, "</b></big>", NULL);
	g_free(bold_text);
	msg = _("If you don't save, all your changes will be permanently lost.");
	str = g_strconcat(str, "\n\n", msg, NULL);

	switch (pcb_gtk_dlg_message(str, GTK_WINDOW(top_window))) {
		case GTK_RESPONSE_YES:
		{
			if (pcb_hid_actionl("Save", NULL)) {	/* Save failed */
				return 0;								/* Cancel */
			}
			else {
				return 1;								/* Close */
			}
		}
		case GTK_RESPONSE_NO:
		{
			return 1;									/* Close */
		}
		case GTK_RESPONSE_CANCEL:
		default:
		{
			return 0;									/* Cancel */
		}
	}
	g_free(str);
}
