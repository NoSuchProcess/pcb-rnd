/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port
*/

#include "config.h"
#include "conf_core.h"

#include "gui.h"
#include "win_place.h"
#include "pcb-printf.h"

static GtkWidget *log_window, *log_text;
static gboolean log_show_on_append = FALSE;

/* Remember user window resizes. */
static gint log_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	wplc_config_event(widget, &hid_gtk_wgeo.log_x, &hid_gtk_wgeo.log_y, &hid_gtk_wgeo.log_width, &hid_gtk_wgeo.log_height);
	return FALSE;
}

static void log_close_cb(gpointer data)
{
	gtk_widget_destroy(log_window);
	log_window = NULL;
}

static void log_destroy_cb(GtkWidget * widget, gpointer data)
{
	log_window = NULL;
}

void ghid_log_window_create()
{
	GtkWidget *vbox, *hbox, *button;
	extern int gtkhid_active;

	if ((log_window) || (ghidgui == NULL) || (!gtkhid_active))
		return;

	log_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(log_window), "destroy", G_CALLBACK(log_destroy_cb), NULL);
	g_signal_connect(G_OBJECT(log_window), "configure_event", G_CALLBACK(log_window_configure_event_cb), NULL);
	gtk_window_set_title(GTK_WINDOW(log_window), _("pcb-rnd Log"));
	gtk_window_set_wmclass(GTK_WINDOW(log_window), "PCB_Log", "PCB");
	gtk_window_set_default_size(GTK_WINDOW(log_window), hid_gtk_wgeo.log_width, hid_gtk_wgeo.log_height);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(log_window), vbox);

	log_text = ghid_scrolled_text_view(vbox, NULL, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(log_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	wplc_place(WPLC_LOG, log_window);

	gtk_widget_realize(log_window);
}

void ghid_log_window_show(gboolean raise)
{
	ghid_log_window_create();
	gtk_widget_show_all(log_window);
	if (raise)
		gtk_window_present(GTK_WINDOW(log_window));
}

typedef struct log_pending_s log_pending_t;
struct log_pending_s {
	log_pending_t *next;
	char msg[1];
};

log_pending_t *log_pending_first = NULL, *log_pending_last = NULL;



static void ghid_log_append_string(gchar * s)
{
	extern int gtkhid_active;
	log_pending_t *m, *next;

	if (!gtkhid_active) {
		/* GUI not initialized yet - save these messages for later
		   NOTE: no need to free this at quit: the GUI will be inited and then it'll be freed */
		m = malloc(sizeof(log_pending_t) + strlen(s));
		strcpy(m->msg, s);
		m->next = NULL;
		if (log_pending_last != NULL)
			log_pending_last->next = m;
		log_pending_last = m;
		if (log_pending_first == NULL)
			log_pending_first = m;
		return;
	}

	if (!log_show_on_append) {
		ghid_log_window_create();
		/* display and free pending messages */
		for(m = log_pending_first; m != NULL; m = next) {
			next = m->next;
			ghid_text_view_append(log_text, m->msg);
			free(m);
		}
		log_pending_last = log_pending_first = NULL;
	}
	else
		ghid_log_window_show(FALSE);

	ghid_text_view_append(log_text, s);
}

void ghid_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	ghid_logv(fmt, ap);
	va_end(ap);
}

void ghid_logv(const char *fmt, va_list args)
{
	char *msg = pcb_strdup_vprintf(fmt, args);
	ghid_log_append_string(msg);
	free(msg);
}

static const char logshowonappend_syntax[] = "LogShowOnAppend(true|false)";

static const char logshowonappend_help[] = "If true, the log window will be shown whenever something is appended \
to it.  If false, the log will still be updated, but the window won't \
be shown.";

static gint GhidLogShowOnAppend(int argc, char **argv, Coord x, Coord y)
{
	char *a = argc == 1 ? argv[0] : (char *) "";

	if (strncasecmp(a, "t", 1) == 0) {
		log_show_on_append = TRUE;
	}
	else if (strncasecmp(a, "f", 1) == 0) {
		log_show_on_append = FALSE;
	}
	return 0;
}

HID_Action ghid_log_action_list[] = {
	{"LogShowOnAppend", 0, GhidLogShowOnAppend,
	 logshowonappend_help, logshowonappend_syntax}
	,
};

REGISTER_ACTIONS(ghid_log_action_list, ghid_cookie)
