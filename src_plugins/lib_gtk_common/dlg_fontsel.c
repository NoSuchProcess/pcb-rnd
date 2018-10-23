/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *
 */

#include "config.h"

#include "dlg_fontsel.h"
#include "compat.h"
#include "bu_box.h"
#include "layer.h"
#include "wt_preview.h"
#include "stub_draw.h"

typedef struct {
	GtkDialog *dialog;
	pcb_text_t *txt, *old_txt;
	pcb_layer_t *layer, *old_layer;
	int old_type;
} pcb_gtk_dlg_fontsel_t;

static int dlg_fontsel_global_latch = 0;

static void fontsel_close_cb(gpointer ctx_)
{
	pcb_gtk_dlg_fontsel_t *ctx = ctx_;
	gtk_widget_destroy(GTK_WIDGET(ctx->dialog));
	if (ctx->txt == NULL)
		dlg_fontsel_global_latch = 0;
	*pcb_stub_draw_fontsel_text_obj = ctx->old_txt;
	*pcb_stub_draw_fontsel_layer_obj = ctx->old_layer;
	*pcb_stub_draw_fontsel_text_type = ctx->old_type;
	free(ctx);
}


void pcb_gtk_dlg_fontsel(pcb_gtk_common_t *com, pcb_layer_t *txtly, pcb_text_t *txt, int type, int modal)
{
	GtkWidget *w;
	GtkDialog *dialog;
	GtkWidget *content_area, *prv;
	GtkWidget *vbox;
	pcb_gtk_dlg_fontsel_t *ctx;

	if (txt == NULL) {
		if (dlg_fontsel_global_latch) /* do not open the global font selector twice */
			return;
		dlg_fontsel_global_latch = 1;
	}
	else {
		if (type != PCB_OBJ_TEXT)
			return;
		if (!modal) {
			pcb_message(PCB_MSG_ERROR, "text-targeted fontsel dialogs must be modal because of the global-var API on the txt object.\n");
			return;
		}
	}


	ctx = malloc(sizeof(pcb_gtk_dlg_fontsel_t));
	ctx->txt = txt;

	ctx->old_txt = *pcb_stub_draw_fontsel_text_obj;
	ctx->old_layer = *pcb_stub_draw_fontsel_layer_obj;
	ctx->old_type = *pcb_stub_draw_fontsel_text_type;
	*pcb_stub_draw_fontsel_text_obj = txt;
	*pcb_stub_draw_fontsel_layer_obj = txtly;
	*pcb_stub_draw_fontsel_text_type = type;

	w = gtk_dialog_new_with_buttons("PCB - font selector",
																	GTK_WINDOW(com->top_window),
																	GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);
	ctx->dialog = dialog = GTK_DIALOG(w);
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_CLOSE);

	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);

	g_signal_connect_swapped(G_OBJECT(dialog), "response", G_CALLBACK(fontsel_close_cb), ctx);
	gtk_window_set_role(GTK_WINDOW(w), "PCB_Dialog");

	content_area = gtk_dialog_get_content_area(dialog);

	vbox = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_area), vbox, TRUE, TRUE, 0);

	/* create the preview render */
	{
		pcb_gtk_preview_t *p;
		pcb_box_t b;
		prv = pcb_gtk_preview_dialog_new(com, com->init_drawing_widget, com->preview_expose, pcb_stub_draw_fontsel);
		gtk_box_pack_start(GTK_BOX(vbox), prv, TRUE, TRUE, 0);
		p = (pcb_gtk_preview_t *) prv;
		b.X1 = b.Y1 = 0;
		b.X2 = PCB_MM_TO_COORD(50);
		b.Y2 = PCB_MM_TO_COORD(50);

		pcb_gtk_preview_zoomto(p, &b);

		p->mouse_cb = pcb_stub_draw_fontsel_mouse_ev;
/*		p->overlay_draw_cb = pcb_stub_draw_csect_overlay;*/
		gtk_widget_set_size_request(prv, 200, 200);
	}


	gtk_widget_show_all(w);
	gtk_window_set_modal(GTK_WINDOW(dialog), modal);
}
