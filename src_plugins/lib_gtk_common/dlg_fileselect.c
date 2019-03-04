/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <gtk/gtk.h>
#include <genht/htsp.h>
#include <genht/hash.h>

#include "hid.h"
#include "hid_dad.h"
#include "compat_misc.h"
#include "compat.h"
#include "event.h"

#include "dlg_attribute.h"

static htsp_t history;
static int inited = 0;

#define MAX_HIST 8
typedef struct file_history_s {
	char *fn[MAX_HIST];
} file_history_t;

/* Move any items between from,to one slot down; [to] is the item that
   is overwritten (lost). [from] will be an empty slot (set to NULL)
   after the operation */
static void shift_history(file_history_t *hi, int from, int to)
{
	int n;

	free(hi->fn[to]);
	for(n = to; n > from; n--)
		hi->fn[n] = hi->fn[n-1];
	hi->fn[from] = NULL;
}

static void update_history(file_history_t *hi, const char *path)
{
	int n;

	/* if already on the list, move to top */
	for(n = 0; (n < MAX_HIST) && (hi->fn[n] != NULL); n++) {
		if (strcmp(hi->fn[n], path) == 0) {
			shift_history(hi, 0, n);
			hi->fn[0] = pcb_strdup(path);
			return;
		}
	}

	/* else move everything down and insert in front */
	shift_history(hi, 0, MAX_HIST);
	hi->fn[0] = pcb_strdup(path);
}

typedef struct {
	GtkWidget *dialog;
	int active;
	void *hid_ctx; /* DAD subdialog context */
} pcb_gtk_fsd_t;

static int pcb_gtk_fsd_poke(pcb_hid_dad_subdialog_t *sub, const char *cmd, pcb_event_arg_t *res, int argc, pcb_event_arg_t *argv)
{
	pcb_gtk_fsd_t *pctx = sub->parent_ctx;

	if (strcmp(cmd, "close") == 0) {
		if (pctx->active) {
			gtk_widget_destroy(pctx->dialog);
			pctx->active = 0;
		}
		return 0;
	}

	if (strcmp(cmd, "get_path") == 0) {
		gchar *gp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(pctx->dialog));
		res->type = PCB_EVARG_STR;
		res->d.s = pcb_strdup(gp);
		g_free(gp);
		return 0;
	}

	if ((strcmp(cmd, "set_file_name") == 0) && (argc == 1) && (argv[0].type == PCB_EVARG_STR)) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(pctx->dialog), argv[0].d.s);
		return 0;
	}

	return -1;
}

char *pcb_gtk_fileselect2(pcb_gtk_common_t *com, const char *title, const char *descr, const char *default_file, const char *default_ext, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub)
{
	GtkWidget *top_window = com->top_window;
	gchar *path = NULL, *base = NULL, *res = NULL;
	char *result;
	file_history_t *hi;
	int n;
	pcb_gtk_fsd_t pctx;

	if (!inited) {
		htsp_init(&history, strhash, strkeyeq);
		inited = 1;
	}

	if ((history_tag != NULL) && (*history_tag != '\0')) {
		hi = htsp_get(&history, history_tag);
		if (hi == NULL) {
			hi = calloc(sizeof(file_history_t), 1);
			htsp_set(&history, pcb_strdup(history_tag), hi);
		}
	}

	if ((default_file != NULL) && (*default_file != '\0')) {
		path = g_path_get_dirname(default_file);
		base = g_path_get_basename(default_file);
	}

	pctx.dialog = gtk_file_chooser_dialog_new(
		title, GTK_WINDOW(top_window),
		(flags & PCB_HID_FSD_READ) ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE,
		 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(pctx.dialog), GTK_RESPONSE_OK);

	if (sub != NULL) {
		GtkWidget *subbox;
		subbox = gtkc_hbox_new(FALSE, 0);

		sub->parent_ctx = &pctx;
		sub->parent_poke = pcb_gtk_fsd_poke;

		pctx.hid_ctx = ghid_attr_sub_new(com, subbox, sub->dlg, sub->dlg_len, sub);

		gtk_widget_show_all(subbox);
		gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(pctx.dialog), subbox);
	}

	if ((path != NULL) && (*path != '\0')) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(pctx.dialog), path);
		g_free(path);
	}

	if ((base != NULL) && (*base != '\0')) {
		/* default_file is useful only for write */
		if (!(flags & PCB_HID_FSD_READ))
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(pctx.dialog), base);
		g_free(base);
	}

	for(n = 0; (n < MAX_HIST) && (hi->fn[n] != NULL); n++)
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(pctx.dialog), hi->fn[n], NULL);

	pctx.active = 1;
	if (gtk_dialog_run(GTK_DIALOG(pctx.dialog)) == GTK_RESPONSE_OK) {
		res = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(pctx.dialog));
		if (res != NULL)
			path = g_path_get_dirname(res);
		else
			path = NULL;

		update_history(hi, path);
	}
	
	if (pctx.active) {
		gtk_widget_destroy(pctx.dialog);
		pctx.active = 0;
	}


	if (res == NULL)
		return NULL;

	/* can't return something that needs g_free, have to copy */
	result = pcb_strdup(res);
	g_free(res);
	return result;
}
