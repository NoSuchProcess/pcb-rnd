#ifndef PCB_GTK_LAYERSEL_H
#define PCB_GTK_LAYERSEL_H

#include <gtk/gtk.h>

typedef struct pcb_gtk_layersel_s pcb_gtk_layersel_t;
typedef struct pcb_gtk_ls_grp_s pcb_gtk_ls_grp_t;
typedef struct pcb_gtk_ls_lyr_s pcb_gtk_ls_lyr_t;

struct pcb_gtk_ls_lyr_s {
	GtkWidget *box, *vis_on, *vis_off, *name_box;

	int (*ev_toggle_vis)(pcb_gtk_ls_lyr_t *lsl); /* called first in click handler, if returns non-zero, normal visibility update is supressed */
	int (*ev_selected)(pcb_gtk_ls_lyr_t *lsl);   /* called first in click handler, if returns non-zero, layer can not be selected */

	unsigned on:1; /* TODO: temporary hack: should be extracted from the layer struct */

	/* for callbacks */
	pcb_gtk_ls_grp_t *lsg;  /* points to parent */
};

struct pcb_gtk_ls_grp_s {
	GtkWidget *grp_row, *grp_closed, *grp_open, *layers, *vis_on, *vis_off;

	pcb_gtk_ls_lyr_t layer[4];

	/* for callbacks */
	pcb_gtk_layersel_t *ls;  /* points to parent */

	/* TODO: temporary hack: will come from the core */
	unsigned on:1;    /* central visibility (toggles all layers) */
	unsigned open:1;  /* whether group is expanded, layers are visible */
};


struct pcb_gtk_layersel_s {
	GtkWidget *grp_box;
	pcb_gtk_ls_grp_t grp[4];
};

GtkWidget *pcb_gtk_layersel_build(pcb_gtk_layersel_t *ls);

#endif
