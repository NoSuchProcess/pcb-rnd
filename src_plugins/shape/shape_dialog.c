#include "hid_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	pcb_data_t *data;

	/* tab management */
	int but_regpoly, but_roundrect, but_circle;
	int tab_regpoly, tab_roundrect, tab_circle;
	int tab;

	/* common */
	int but_gen;
} ctx_t;

static void shp_tab_update(void *hid_ctx, ctx_t *shp)
{
	int but, tab;
	pcb_gui->attr_dlg_widget_hide(hid_ctx,  shp->tab_regpoly,   pcb_true);
	pcb_gui->attr_dlg_widget_hide(hid_ctx,  shp->tab_roundrect, pcb_true);
	pcb_gui->attr_dlg_widget_hide(hid_ctx,  shp->tab_circle,    pcb_true);
	pcb_gui->attr_dlg_widget_state(hid_ctx, shp->but_regpoly,   pcb_true);
	pcb_gui->attr_dlg_widget_state(hid_ctx, shp->but_roundrect, pcb_true);
	pcb_gui->attr_dlg_widget_state(hid_ctx, shp->but_circle,    pcb_true);

	switch(shp->tab) {
		case 0: but = shp->but_regpoly;   tab = shp->tab_regpoly;   break;
		case 1: but = shp->but_roundrect; tab = shp->tab_roundrect; break;
		case 2: but = shp->but_circle;    tab = shp->tab_circle;    break;
	}

	pcb_gui->attr_dlg_widget_hide(hid_ctx,  tab, pcb_false);
	pcb_gui->attr_dlg_widget_state(hid_ctx, but, pcb_false);
}

static void shp_tab0(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;
	shp->tab = 0;
	shp_tab_update(hid_ctx, shp);
}

static void shp_tab1(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;
	shp->tab = 1;
	shp_tab_update(hid_ctx, shp);
}

static void shp_tab2(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;
	shp->tab = 2;
	shp_tab_update(hid_ctx, shp);
}

static void shp_button_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	ctx_t *shp = caller_data;

	PCB_DAD_FREE(shp->dlg);
	free(shp);
}

void pcb_shape_dialog(pcb_board_t *pcb, pcb_data_t *data, pcb_bool modal)
{
	ctx_t *shp = calloc(sizeof(ctx_t), 1);
	shp->pcb = pcb;
	shp->data = data;

	PCB_DAD_BEGIN_VBOX(shp->dlg);
		/* tabs */
		PCB_DAD_BEGIN_HBOX(shp->dlg);
			PCB_DAD_BUTTON(shp->dlg, "Regular polygon");
				shp->but_regpoly = PCB_DAD_CURRENT(shp->dlg);
				PCB_DAD_CHANGE_CB(shp->dlg, shp_tab0);
			PCB_DAD_BUTTON(shp->dlg, "Round rectangle");
				shp->but_roundrect = PCB_DAD_CURRENT(shp->dlg);
				PCB_DAD_CHANGE_CB(shp->dlg, shp_tab1);
			PCB_DAD_BUTTON(shp->dlg, "Filled circle");
				shp->but_circle = PCB_DAD_CURRENT(shp->dlg);
				PCB_DAD_CHANGE_CB(shp->dlg, shp_tab2);
		PCB_DAD_END(shp->dlg);

		/* regpoly tab */
		PCB_DAD_BEGIN_VBOX(shp->dlg);
			shp->tab_regpoly = PCB_DAD_CURRENT(shp->dlg);
			PCB_DAD_LABEL(shp->dlg, "Generate regular polygon");
		PCB_DAD_END(shp->dlg);

		/* roundrect tab */
		PCB_DAD_BEGIN_VBOX(shp->dlg);
			shp->tab_roundrect = PCB_DAD_CURRENT(shp->dlg);
			PCB_DAD_LABEL(shp->dlg, "Generate rectange with rounded corners");
		PCB_DAD_END(shp->dlg);

		/* circle tab */
		PCB_DAD_BEGIN_VBOX(shp->dlg);
			shp->tab_circle = PCB_DAD_CURRENT(shp->dlg);
			PCB_DAD_LABEL(shp->dlg, "Generate filled circle");
		PCB_DAD_END(shp->dlg);

		PCB_DAD_BUTTON(shp->dlg, "Generate!");
			shp->but_gen = PCB_DAD_CURRENT(shp->dlg);
	PCB_DAD_END(shp->dlg);

	PCB_DAD_NEW(shp->dlg, "dlg_shape", "Generate shapes", shp, modal, shp_button_cb);
/*	shp_tab_update(shp->dlg_hid_ctx, &shp);*/

	if (modal) {
		PCB_DAD_RUN(shp->dlg);
		PCB_DAD_FREE(shp->dlg);
	}
}


static const char pcb_acts_shape[] = "shape()";
static const char pcb_acth_shape[] = "Interactive shape generator.";
int pcb_act_shape(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_shape_dialog(PCB, PCB_PASTEBUFFER->Data, pcb_false);
	return 0;
}

