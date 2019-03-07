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

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"

#include "board.h"
#include "hid.h"
#include "compat_misc.h"

#include "dlg_file_chooser.h"
#include "compat.h"

/* ---------------------------------------------- */
/* Caller must g_free() the returned filename.*/
gchar *ghid_dialog_file_select_open(GtkWidget *top_window, const gchar *title, gchar **path, const gchar *shortcuts)
{
	GtkWidget *dialog;
	gchar *result = NULL, *folder, *seed;
	GtkFileFilter *no_filter, *any_filter;

	dialog = gtk_file_chooser_dialog_new(title,
																			 GTK_WINDOW(top_window),
																			 GTK_FILE_CHOOSER_ACTION_OPEN,
																			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	/* add a default filter for not filtering files */
	no_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(no_filter, "all");
	gtk_file_filter_add_pattern(no_filter, "*.*");
	gtk_file_filter_add_pattern(no_filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), no_filter);

	any_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(any_filter, "any known format");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), any_filter);


	/* in case we have a dialog for loading a footprint file */
	if (strcmp(title, "Load element to buffer") == 0) {
		/* add a filter for footprint files */
		GtkFileFilter *fp_filter;
		fp_filter = gtk_file_filter_new();
		gtk_file_filter_set_name(fp_filter, "fp");
		gtk_file_filter_add_mime_type(fp_filter, "application/x-pcb-footprint");
		gtk_file_filter_add_pattern(fp_filter, "*.fp");
		gtk_file_filter_add_pattern(fp_filter, "*.FP");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fp_filter);
	}

	/* in case we have a dialog for loading a layout file */
	if ((strcmp(title, "Load layout file") == 0)
			|| (strcmp(title, "Load layout file to buffer") == 0)) {
		/* add a filter for layout files */
		pcb_io_formats_t fmts;
		int n, num_fmts = pcb_io_list(&fmts, PCB_IOT_PCB, 0, 0, PCB_IOL_EXT_BOARD);
		for (n = 0; n < num_fmts; n++) {
			int i;
			char *ext;
			GtkFileFilter *filter;

			/* register each short name only once - slow O(N^2), but we don't have too many formats anyway */
			for (i = 0; i < n; i++)
				if (strcmp(fmts.plug[n]->default_fmt, fmts.plug[i]->default_fmt) == 0)
					goto next_fmt;

			filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter, fmts.plug[n]->default_fmt);
			if (fmts.plug[n]->mime_type != NULL) {
				gtk_file_filter_add_mime_type(filter, fmts.plug[n]->mime_type);
				gtk_file_filter_add_mime_type(any_filter, fmts.plug[n]->mime_type);
			}
			if (fmts.plug[n]->default_extension != NULL) {
				char *s;
				ext = pcb_concat("*", fmts.plug[n]->default_extension, NULL);
				gtk_file_filter_add_pattern(filter, ext);
				gtk_file_filter_add_pattern(any_filter, ext);
				for (s = ext; *s != '\0'; s++)
					*s = toupper(*s);
				gtk_file_filter_add_pattern(filter, ext);
				gtk_file_filter_add_pattern(any_filter, ext);
				free(ext);
			}
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		next_fmt:;
		}
		pcb_io_list_free(&fmts);
	}

	/* in case we have a dialog for loading a netlist file */
	if (strcmp(title, "Load netlist file") == 0) {
		/* add a filter for netlist files */
		GtkFileFilter *net_filter;
		net_filter = gtk_file_filter_new();
		gtk_file_filter_set_name(net_filter, "netlist");
		gtk_file_filter_add_mime_type(net_filter, "application/x-pcb-netlist");
		gtk_file_filter_add_pattern(net_filter, "*.net");
		gtk_file_filter_add_pattern(net_filter, "*.NET");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), net_filter);
	}

	if (path && *path)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), *path);

	if (shortcuts && *shortcuts) {
		folder = g_strdup(shortcuts);
		seed = folder;
		while ((folder = strtok(seed, ":")) != NULL) {
			gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), folder, NULL);
			seed = NULL;
		}
		g_free(folder);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		if (folder && path) {
			pcb_gtk_g_strdup(path, folder);
			g_free(folder);
		}
	}
	gtk_widget_destroy(dialog);


	return result;
}


/* Callback to change the file name in the "save as" dialog according to
   format selection */
typedef struct {
	GtkWidget *dialog;
	const char **formats, **extensions;
} ghid_save_ctx_t;

static void fmt_changed_cb(GtkWidget * combo_box, ghid_save_ctx_t * ctx)
{
	char *fn, *s, *bn;
	const char *ext;
	gint active;

	if (ctx->extensions == NULL)
		return;

	active = gtk_combo_box_get_active(GTK_COMBO_BOX(combo_box));
	if (active < 0)
		return;

	fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ctx->dialog));
	if (fn == NULL)
		return;

	/* find and truncate extension */
	for (s = fn + strlen(fn) - 1; *s != '.'; s--) {
		if ((s <= fn) || (*s == '/') || (*s == '\\')) {
			g_free(fn);
			return;
		}
	}
	*s = '\0';

	/* calculate basename in bn */
	bn = strrchr(fn, '/');
	if (bn == NULL) {
		bn = strrchr(fn, '\\');
		if (bn == NULL)
			bn = fn;
		else
			bn++;
	}
	else
		bn++;

	/* fetch the desired extension */
	ext = ctx->extensions[active];
	if (ext == NULL)
		ext = ".";

	/* build a new file name with the right extension */
	s = pcb_concat(bn, ext, NULL);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(ctx->dialog), s);

	free(s);
	g_free(fn);
}

/* ---------------------------------------------- */
/* Caller must g_free() the returned filename. */
gchar *ghid_dialog_file_select_save(GtkWidget *top_window, const gchar *title, gchar **path, const gchar *file, const gchar *shortcuts, const char **formats, const char **extensions, int *format)
{
	GtkWidget *fmt, *tmp, *fmt_combo;
	gchar *result = NULL, *folder, *seed;
	ghid_save_ctx_t ctx;

	ctx.formats = formats;
	ctx.extensions = extensions;
	ctx.dialog = gtk_file_chooser_dialog_new(title,
																					 GTK_WINDOW(top_window),
																					 GTK_FILE_CHOOSER_ACTION_SAVE,
																					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(ctx.dialog), TRUE);
	gtk_dialog_set_default_response(GTK_DIALOG(ctx.dialog), GTK_RESPONSE_OK);

	/* Create and add the file format widget */
	if (format != NULL) {
		const char **s;
		fmt = gtkc_hbox_new(FALSE, 0);

		tmp = gtkc_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(fmt), tmp, TRUE, TRUE, 0);

		tmp = gtk_label_new("File format: ");
		gtk_box_pack_start(GTK_BOX(fmt), tmp, FALSE, FALSE, 0);

		fmt_combo = gtkc_combo_box_text_new();
		gtk_box_pack_start(GTK_BOX(fmt), fmt_combo, FALSE, FALSE, 0);

		for (s = formats; *s != NULL; s++)
			gtkc_combo_box_text_append_text(fmt_combo, *s);

		gtk_combo_box_set_active(GTK_COMBO_BOX(fmt_combo), *format);
		g_signal_connect(G_OBJECT(fmt_combo), "changed", G_CALLBACK(fmt_changed_cb), &ctx);

		gtk_widget_show_all(fmt);
		gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(ctx.dialog), fmt);
	}

	if (path && *path && **path)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ctx.dialog), *path);

	if (file && *file) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(ctx.dialog), g_path_get_basename(file));
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ctx.dialog), g_path_get_dirname(file));
	}

	if (shortcuts && *shortcuts) {
		folder = g_strdup(shortcuts);
		seed = folder;
		while ((folder = strtok(seed, ":")) != NULL) {
			gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(ctx.dialog), folder, NULL);
			seed = NULL;
		}
		g_free(folder);
	}
	if (gtk_dialog_run(GTK_DIALOG(ctx.dialog)) == GTK_RESPONSE_OK) {
		result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ctx.dialog));
		folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(ctx.dialog));
		if (folder && path) {
			pcb_gtk_g_strdup(path, folder);
			g_free(folder);
		}
	}

	if (format != NULL)
		*format = gtk_combo_box_get_active(GTK_COMBO_BOX(fmt_combo));

	gtk_widget_destroy(ctx.dialog);

	return result;
}


