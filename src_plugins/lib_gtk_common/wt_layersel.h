#ifndef PCB_GTK_LAYERSEL_H
#define PCB_GTK_LAYERSEL_H

#include <gtk/gtk.h>
#include "layer.h"
#include "layer_grp.h"
#include "glue.h"

typedef struct pcb_gtk_layersel_s pcb_gtk_layersel_t;
typedef struct pcb_gtk_ls_grp_s pcb_gtk_ls_grp_t;
typedef struct pcb_gtk_ls_lyr_s pcb_gtk_ls_lyr_t;

struct pcb_gtk_ls_lyr_s {
	GtkWidget *box, *vis_on, *vis_off, *name_box;

	int (*ev_vis)(pcb_gtk_ls_lyr_t *lsl, int toggle, int *is_on); /* called first in click handler with toggle=1, if returns non-zero, normal visibility update is supressed; also called from layer draw, with toggle=0 */
	int (*ev_selected)(pcb_gtk_ls_lyr_t *lsl, int do_select);     /* called first in click handler with do_select=1, if returns -1, layer can not be selected; when called with do_select=0, return current selection */

	pcb_layer_id_t lid;
	const pcb_color_t *force_color;

	int vis_ui_idx, virt_idx;

	/* for callbacks */
	pcb_gtk_ls_grp_t *lsg;  /* points to parent */
};

struct pcb_gtk_ls_grp_s {
	GtkWidget *grp_row, *grp_closed, *grp_open, *layers, *vis_on, *vis_off;

	pcb_layergrp_t *grp;
	pcb_gtk_ls_lyr_t *layer;

	/* for callbacks */
	pcb_gtk_layersel_t *ls;  /* points to parent */
};


struct pcb_gtk_layersel_s {
	pcb_gtk_common_t *com;
	GtkWidget *grp_box, *grp_box_outer;
	pcb_gtk_ls_grp_t grp[PCB_MAX_LAYERGRP];
	pcb_gtk_ls_grp_t lsg_virt, lsg_ui;
	pcb_layergrp_t grp_virt, grp_ui;
	unsigned running:1;
	unsigned no_copper_sel:1;
};

GtkWidget *pcb_gtk_layersel_build(pcb_gtk_common_t *com, pcb_gtk_layersel_t *ls);
void pcb_gtk_layersel_update(pcb_gtk_common_t *com, pcb_gtk_layersel_t *ls);
void pcb_gtk_layersel_vis_update(pcb_gtk_layersel_t *ls);

#endif
