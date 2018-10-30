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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "dlg_progress.h"

#include "pcb_bool.h"
#include "compat_nls.h"
#include "compat.h"

struct progress_dialog {
	GtkWidget *dialog;
	GtkWidget *message;
	GtkWidget *progress;
	gint response_id;
	GMainLoop *loop;
	gboolean destroyed;
	gboolean started;
	GTimer *timer;

	gulong response_handler;
	gulong destroy_handler;
	gulong delete_handler;
};

static void run_response_handler(GtkDialog * dialog, gint response_id, gpointer data)
{
	struct progress_dialog *pd = data;

	pd->response_id = response_id;
}

static gint run_delete_handler(GtkDialog * dialog, GdkEventAny * event, gpointer data)
{
	struct progress_dialog *pd = data;

	pd->response_id = GTK_RESPONSE_DELETE_EVENT;

	return TRUE;									/* Do not destroy */
}

static void run_destroy_handler(GtkDialog * dialog, gpointer data)
{
	struct progress_dialog *pd = data;

	pd->destroyed = TRUE;
}

static struct progress_dialog *make_progress_dialog(GtkWidget *top_window)
{
	struct progress_dialog *pd;
	GtkWidget *content_area;
	GtkWidget *alignment;
	GtkWidget *vbox;

	pd = g_new0(struct progress_dialog, 1);
	pd->response_id = GTK_RESPONSE_NONE;

	pd->dialog = gtk_dialog_new_with_buttons(_("Progress"), GTK_WINDOW(top_window),
																					 /* Modal so nothing else can get events whilst
																					    the main mainloop isn't running */
																					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
																					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	gtk_window_set_deletable(GTK_WINDOW(pd->dialog), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(pd->dialog), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(pd->dialog), TRUE);
	gtk_widget_set_size_request(pd->dialog, 300, -1);

	pd->message = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(pd->message), 0., 0.);

	pd->progress = gtk_progress_bar_new();
	gtk_widget_set_size_request(pd->progress, -1, 26);

	vbox = gtkc_vbox_new(pcb_false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), pd->message, pcb_true, pcb_true, 8);
	gtk_box_pack_start(GTK_BOX(vbox), pd->progress, pcb_false, pcb_true, 8);

	alignment = gtk_alignment_new(0., 0., 1., 1.);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 8, 8, 8, 8);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(pd->dialog));
	gtk_box_pack_start(GTK_BOX(content_area), alignment, pcb_true, pcb_true, 0);

	gtk_widget_show_all(alignment);

	g_object_ref(pd->dialog);
	gtk_window_present(GTK_WINDOW(pd->dialog));

	pd->response_handler = g_signal_connect(pd->dialog, "response", G_CALLBACK(run_response_handler), pd);
	pd->delete_handler = g_signal_connect(pd->dialog, "delete-event", G_CALLBACK(run_delete_handler), pd);
	pd->destroy_handler = g_signal_connect(pd->dialog, "destroy", G_CALLBACK(run_destroy_handler), pd);

	pd->loop = g_main_loop_new(NULL, FALSE);
	pd->timer = g_timer_new();

	return pd;
}

static void destroy_progress_dialog(struct progress_dialog *pd)
{
	if (pd == NULL)
		return;

	if (!pd->destroyed) {
		g_signal_handler_disconnect(pd->dialog, pd->response_handler);
		g_signal_handler_disconnect(pd->dialog, pd->delete_handler);
		g_signal_handler_disconnect(pd->dialog, pd->destroy_handler);
	}

	g_timer_destroy(pd->timer);
	g_object_unref(pd->dialog);
	g_main_loop_unref(pd->loop);

	gtk_widget_destroy(pd->dialog);

	pd->loop = NULL;
	g_free(pd);
}

static void handle_progress_dialog_events(struct progress_dialog *pd)
{
	GMainContext *context = g_main_loop_get_context(pd->loop);

	/* Process events */
	while (g_main_context_pending(context)) {
		g_main_context_iteration(context, FALSE);
	}
}

#define MIN_TIME_SEPARATION (50./1000.)	/* 50ms */
int pcb_gtk_dlg_progress(GtkWidget *top_window, int so_far, int total, const char *message)
{
	static struct progress_dialog *pd = NULL;

	/* If we are finished, destroy any dialog */
	if (so_far == 0 && total == 0 && message == NULL) {
		destroy_progress_dialog(pd);
		pd = NULL;
		return 0;
	}

	if (pd == NULL)
		pd = make_progress_dialog(top_window);

	/* We don't want to keep the underlying process too busy whilst we
	 * process events. If we get called quickly after the last progress
	 * update, wait a little bit before we respond - perhaps the next
	 * time progress is reported.

	 * The exception here is that we always want to process the first
	 * batch of events after having shown the dialog for the first time
	 */
	if (pd->started && g_timer_elapsed(pd->timer, NULL) < MIN_TIME_SEPARATION)
		return 0;

	gtk_label_set_text(GTK_LABEL(pd->message), message);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pd->progress), (double) so_far / (double) total);

	handle_progress_dialog_events(pd);
	g_timer_start(pd->timer);

	pd->started = TRUE;

	return (pd->response_id == GTK_RESPONSE_CANCEL || pd->response_id == GTK_RESPONSE_DELETE_EVENT) ? 1 : 0;
}
