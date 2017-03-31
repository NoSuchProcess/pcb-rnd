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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include "dlg_about.h"
#include "compat_nls.h"
#include "build_run.h"

#include "dlg_report.h"

static void display_options_dialog(GtkWidget * button, gpointer data)
{
	const gchar *text = pcb_get_info_compile_options();
	GtkWidget *about = (GtkWidget *) data;

	pcb_gtk_dlg_report(about, "Compile time Options", text, TRUE);
}

void pcb_gtk_dlg_about(GtkWidget * top_window)
{
	GtkWidget *button, *action_area;
	GtkWidget *w = gtk_about_dialog_new();
	GtkAboutDialog *about = GTK_ABOUT_DIALOG(w);

	/* Add the compile options button */
	button = gtk_button_new_from_stock(_("_Options"));
	/*gtk_widget_set_can_default(button, TRUE);*/
	gtk_widget_show(button);
	action_area = gtk_dialog_get_action_area(GTK_DIALOG(about));
	gtk_box_pack_end(GTK_BOX(action_area), button, FALSE, TRUE, 0);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(action_area), button, TRUE);
	g_signal_connect(button, "clicked", G_CALLBACK(display_options_dialog), about);

	/* We don't want to maintain a list of authors... So, this is the minimum info */
	const gchar *authors[] = { "For authors, see the (C) in the main dialog",
		"GTK HID:", "  Bill Wilson", NULL
	};

	gtk_about_dialog_set_program_name(about, "pcb-rnd");
	gtk_about_dialog_set_version(about, PCB_VERSION);
	gtk_about_dialog_set_authors(about, authors);

	gtk_about_dialog_set_copyright(about, pcb_get_info_copyright());

	/*TODO: Find a proper way to include the COPYING file and/or display this info :
	   gds_append_str(&info, "It is licensed under the terms of the GNU\n");
	   gds_append_str(&info, "General Public License version 2\n");
	   gds_append_str(&info, "See the LICENSE file for more information\n\n");
	 */

	gtk_about_dialog_set_license(about,
															 "It is licensed under the terms of the GNU\n" "General Public License version 2\n"
															 "See the COPYING file for more information\n\n");
	/* in GTK3:
	   gtk_about_dialog_set_license_type(about, GTK_LICENSE_GPL_2_0); */
	/*FIXME: Refactor the string w.r.t. the dialog */

	gtk_about_dialog_set_comments(about, pcb_get_info_comments());

	gtk_about_dialog_set_website(about, "http://repo.hu/projects/pcb-rnd/");
	gtk_about_dialog_set_website_label(about, "Visit the PCB-rnd website");
	gtk_about_dialog_set_documenters(about, NULL);
	gtk_about_dialog_set_translator_credits(about, _("translator-credits"));

	gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(w);
}
