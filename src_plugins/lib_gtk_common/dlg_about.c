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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include "dlg_about.h"
#include "compat_nls.h"
#include "build_run.h"

#include "compat.h"
#include "dlg_report.h"

static void display_options_dialog(GtkWidget * button, gpointer data)
{
	const gchar *text = pcb_get_info_compile_options();
	GtkWidget *about = (GtkWidget *) data;

	pcb_gtk_dlg_report(about, "Compile time Options", text, TRUE);
}

void pcb_gtk_dlg_about(GtkWidget * top_window)
{
	GtkWidget *button;
	GtkWidget *w = gtk_about_dialog_new();
	GtkAboutDialog *about = GTK_ABOUT_DIALOG(w);
	const char *url;

	/* Add the compile options button */
	button = gtk_button_new_with_mnemonic(_("_Options"));
	/*gtk_widget_set_can_default(button, TRUE);*/
	gtk_widget_show(button);
	pcb_gtk_dlg_about_add_button(GTK_DIALOG(about), button);
	g_signal_connect(button, "clicked", G_CALLBACK(display_options_dialog), about);

	/* We don't want to maintain a list of authors... So, this is the minimum info */
	const gchar *authors[] = { "For authors, see the (C) in the main dialog", NULL };

	gtk_about_dialog_set_program_name(about, "pcb-rnd");
	gtk_about_dialog_set_version(about, PCB_VERSION);
	gtk_about_dialog_set_authors(about, authors);

	gtk_about_dialog_set_copyright(about, pcb_get_info_copyright());

	gtk_about_dialog_set_license(about, pcb_get_infostr());

	/* in GTK3:
	   gtk_about_dialog_set_license_type(about, GTK_LICENSE_GPL_2_0); */
	/*FIXME: Refactor the string w.r.t. the dialog */

	gtk_about_dialog_set_comments(about, pcb_get_info_comments());

	pcb_get_info_websites(&url);
	gtk_about_dialog_set_website(about, url);
	gtk_about_dialog_set_website_label(about, "Visit the pcb-rnd website");
	gtk_about_dialog_set_documenters(about, NULL);
	gtk_about_dialog_set_translator_credits(about, NULL);

	gtk_window_set_transient_for(GTK_WINDOW(w), GTK_WINDOW(top_window));
	gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(w);
}
