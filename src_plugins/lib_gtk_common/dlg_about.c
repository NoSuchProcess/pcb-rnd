/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas, Alain Vigne
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

#include "config.h"

#include "dlg_about.h"
#include "compat_nls.h"
#include "build_run.h"

void pcb_gtk_dlg_about(GtkWidget * top_window)
{
	GtkWidget *w = gtk_about_dialog_new();
	GtkAboutDialog *about = GTK_ABOUT_DIALOG(w);
	char *s_info = pcb_get_infostr();
	const gchar *authors[] = { "Igor2", "Alain", NULL };

	gtk_about_dialog_set_program_name(about, "pcb-rnd");
	gtk_about_dialog_set_version(about, VERSION);
	/*FIXME: pcb_author()  not good here because gchar ** expected : */
	/*gtk_about_dialog_set_authors (about, pcb_author()); */
	gtk_about_dialog_set_authors(about, authors);
	gtk_about_dialog_set_copyright (about, "FIXME : text of the license ...");
	/* in GTK3:
  gtk_about_dialog_set_license_type(about, GTK_LICENSE_GPL_2_0);*/
	/*FIXME: Refactor the string w.r.t. the dialog */
	gtk_about_dialog_set_comments(about, s_info);
	gtk_about_dialog_set_website(about, "http://repo.hu/projects/pcb-rnd/");
	gtk_about_dialog_set_documenters(about, authors);
	gtk_about_dialog_set_translator_credits(about, _("translator-credits"));

	gtk_dialog_run(GTK_DIALOG(w));
	gtk_widget_destroy(w);
}
