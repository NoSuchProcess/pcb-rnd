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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port */

#include "config.h"

#include <ctype.h>
#include <stdarg.h>
#include <gdk/gdkkeysyms.h>

#include "dlg_log.h"

#include "conf_core.h"
#include "conf_hid.h"

#include "pcb-printf.h"
#include "actions.h"
#include "compat_nls.h"

#include "win_place.h"
#include "bu_text_view.h"
#include "compat.h"
#include "event.h"

#include "hid_gtk_conf.h"

static GtkWidget *log_window, *log_text;
static pcb_bool log_show_on_append = FALSE;

typedef struct log_pending_s log_pending_t;
struct log_pending_s {
	log_pending_t *next;
	enum pcb_message_level level;
	char msg[1];
};

log_pending_t *log_pending_first = NULL, *log_pending_last = NULL;

static gint log_window_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, gpointer data)
{
	pcb_event(PCB_EVENT_DAD_NEW_GEO, "psiiii", NULL, "log",
		(int)ev->x, (int)ev->y, (int)ev->width, (int)ev->height);
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

static gboolean log_key_release_cb(GtkWidget *preview, GdkEventKey *kev, gpointer data)
{
	if (kev->keyval == GDK_KEY_Escape)
		log_close_cb(data);
	return FALSE;
}

static void ghid_log_window_create()
{
	GtkWidget *vbox, *hbox, *button;

	if (log_window)
		return;

	log_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	pcb_gtk_winplace(log_window, "log");
	g_signal_connect(G_OBJECT(log_window), "destroy", G_CALLBACK(log_destroy_cb), NULL);
	g_signal_connect(G_OBJECT(log_window), "configure_event", G_CALLBACK(log_window_configure_event_cb), NULL);
	gtk_window_set_title(GTK_WINDOW(log_window), _("pcb-rnd Log"));
	gtk_window_set_role(GTK_WINDOW(log_window), "PCB_Log");

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add(GTK_CONTAINER(log_window), vbox);

	log_text = ghid_scrolled_text_view(vbox, NULL, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(log_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(log_window), "key_release_event", G_CALLBACK(log_key_release_cb), NULL);

	gtk_widget_realize(log_window);
}

static void ghid_log_append_string_(enum pcb_message_level level, gchar * msg)
{
	const char *tag;
	int popup;

	conf_loglevel_props(level, &tag, &popup);
	if (tag != NULL)
		ghid_text_view_append(log_text, (gchar *) tag);

	ghid_text_view_append(log_text, msg);

	if (popup)
		pcb_actionl("DoWindows", "Log", "false", NULL);
}

static void ghid_log_append_string(enum pcb_message_level level, gchar *s, int hid_active)
{
	log_pending_t *m, *next;

	if (!hid_active) {
		/* GUI not initialized yet - save these messages for later
		   NOTE: no need to free this at quit: the GUI will be inited and then it'll be freed */
		m = malloc(sizeof(log_pending_t) + strlen(s));
		strcpy(m->msg, s);
		m->level = level;
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
		for (m = log_pending_first; m != NULL; m = next) {
			next = m->next;
			ghid_log_append_string_(m->level, m->msg);
			free(m);
		}
		log_pending_last = log_pending_first = NULL;
	}
	else
		pcb_gtk_dlg_log_show(FALSE);

	ghid_log_append_string_(level, s);
}

const char pcb_gtk_acts_logshowonappend[] = "LogShowOnAppend(true|false)";
const char pcb_gtk_acth_logshowonappend[] = "If true, the log window will be shown whenever something is appended to it. If false, the log will still be updated, but the window won't be shown.";
fgw_error_t pcb_gtk_act_logshowonappend(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a = "";
	const char *pcb_acts_logshowonappend = pcb_gtk_acts_logshowonappend;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, logshowonappend, a = argv[1].val.str);

	if (tolower(*a) == 't')
		log_show_on_append = TRUE;
	else if (tolower(*a) == 'f')
		log_show_on_append = FALSE;

	PCB_ACT_IRES(0);
	return 0;
}

void pcb_gtk_dlg_log_show(pcb_bool raise)
{
	ghid_log_window_create();
	gtk_widget_show_all(log_window);
	if (raise)
		gtk_window_present(GTK_WINDOW(log_window));
}

void pcb_gtk_logv(int hid_active, enum pcb_message_level level, const char *fmt, va_list args)
{
	char *msg = pcb_strdup_vprintf(fmt, args);
	ghid_log_append_string(level, msg, hid_active);
	free(msg);
}
