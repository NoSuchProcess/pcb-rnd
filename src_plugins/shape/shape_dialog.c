#include "hid_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_any_obj_t *obj;

	/* tab management */
	int but_regpoly, but_roundrect, but_circle;
	int tab_regpoly, tab_roundrect, tab_circle;
	int tab;

	/* regpoly */
	int prx, pry, corners, pcx, pcy, prot, pell;

	/* roundrect */
	int w, h, rrect, rcx, rcy, rx, ry, rrot, rell;

	/* circle */
	int dia, ccx, ccy;

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

static void del_obj(ctx_t *shp)
{
	if (shp->obj != NULL) {
/*		pcb_remove_object(pcb_obj_type2oldtype(shp->obj->type), shp->layer, shp->obj, shp->obj);*/
		shp->obj = NULL;
	}
}

static void shp_chg_regpoly(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;

	/* elliptical logics */
	if (!shp->dlg[shp->pell].default_val.int_value) {
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->pry, pcb_false);
		PCB_DAD_SET_VALUE(hid_ctx, shp->pry, coord_value, shp->dlg[shp->prx].default_val.coord_value);
	}
	else
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->pry, pcb_true);

	del_obj(shp);
	shp->obj = (pcb_any_obj_t *)regpoly_place(
		shp->data, shp->layer, shp->dlg[shp->corners].default_val.int_value,
		shp->dlg[shp->prx].default_val.coord_value, shp->dlg[shp->pry].default_val.coord_value,
		shp->dlg[shp->prot].default_val.real_value,
		shp->dlg[shp->pcx].default_val.coord_value, shp->dlg[shp->pcy].default_val.coord_value);
}

static void shp_chg_roundrect(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;

	/* elliptical logics */
	if (!shp->dlg[shp->rell].default_val.int_value) {
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->ry, pcb_false);
		PCB_DAD_SET_VALUE(hid_ctx, shp->ry, coord_value, shp->dlg[shp->rx].default_val.coord_value);
	}
	else
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->ry, pcb_true);

	/* rectangular logics */
	if (!shp->dlg[shp->rrect].default_val.int_value) {
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->h, pcb_false);
		PCB_DAD_SET_VALUE(hid_ctx, shp->h, coord_value, shp->dlg[shp->w].default_val.coord_value);
	}
	else
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->w, pcb_true);

	del_obj(shp);
	shp->obj = (pcb_any_obj_t *)roundrect_place(
		shp->data, shp->layer,
		shp->dlg[shp->w].default_val.coord_value, shp->dlg[shp->h].default_val.coord_value,
		shp->dlg[shp->rx].default_val.coord_value, shp->dlg[shp->ry].default_val.coord_value,
		shp->dlg[shp->rrot].default_val.real_value,
		shp->dlg[shp->rcx].default_val.coord_value, shp->dlg[shp->rcy].default_val.coord_value);
}

static void shp_chg_circle(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;

	shp->obj = (pcb_any_obj_t *)circle_place(
		shp->data, shp->layer,
		shp->dlg[shp->dia].default_val.coord_value,
		shp->dlg[shp->ccx].default_val.coord_value, shp->dlg[shp->ccy].default_val.coord_value);
}

void pcb_shape_dialog(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_t *layer, pcb_bool modal)
{
	ctx_t *shp = calloc(sizeof(ctx_t), 1);
	pcb_coord_t mm2 = PCB_MM_TO_COORD(2);
	pcb_coord_t maxr = PCB_MM_TO_COORD(1000);
	shp->pcb = pcb;
	shp->data = data;
	shp->layer = layer;

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
			PCB_DAD_BEGIN_TABLE(shp->dlg, 2);
				PCB_DAD_LABEL(shp->dlg, "Number of corners");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_INTEGER(shp->dlg, "");
						shp->corners = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINVAL(shp->dlg, 3);
						PCB_DAD_MAXVAL(shp->dlg, 64);
						PCB_DAD_DEFAULT(shp->dlg, 8);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "Shape radius");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->prx = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_DEFAULT(shp->dlg, mm2);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
					PCB_DAD_LABEL(shp->dlg, "x (horizontal)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->pry = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_DEFAULT(shp->dlg, mm2);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
					PCB_DAD_LABEL(shp->dlg, "y (vertical)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_BOOL(shp->dlg, "");
						shp->pell = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
					PCB_DAD_LABEL(shp->dlg, "elliptical");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "Rotation angle:");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_REAL(shp->dlg, "");
						shp->prot = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINVAL(shp->dlg, -360);
						PCB_DAD_MAXVAL(shp->dlg, 360);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
					PCB_DAD_LABEL(shp->dlg, "deg");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "Center offset");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->pcx = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
					PCB_DAD_LABEL(shp->dlg, "x (horizontal)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->pcy = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_regpoly);
					PCB_DAD_LABEL(shp->dlg, "y (vertical)");
				PCB_DAD_END(shp->dlg);
			PCB_DAD_END(shp->dlg);

		PCB_DAD_END(shp->dlg);

		/* roundrect tab */
		PCB_DAD_BEGIN_VBOX(shp->dlg);
			shp->tab_roundrect = PCB_DAD_CURRENT(shp->dlg);
			PCB_DAD_LABEL(shp->dlg, "Generate rectange with rounded corners");
			PCB_DAD_BEGIN_TABLE(shp->dlg, 2);

				PCB_DAD_LABEL(shp->dlg, "Rectangle size");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->w = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_DEFAULT(shp->dlg, mm2);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "width (horizontal)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->h = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_DEFAULT(shp->dlg, mm2);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "height (vertical)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_BOOL(shp->dlg, "");
						shp->rrect = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "rectangular");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "Rounding radius");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->rx = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_DEFAULT(shp->dlg, mm2);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "x (horizontal)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->ry = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_DEFAULT(shp->dlg, mm2);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "y (vertical)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_BOOL(shp->dlg, "");
						shp->rell = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "elliptical");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "Rotation angle:");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_REAL(shp->dlg, "");
						shp->rrot = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINVAL(shp->dlg, -360);
						PCB_DAD_MAXVAL(shp->dlg, 360);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "deg");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "Center offset");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->rcx = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "x (horizontal)");
				PCB_DAD_END(shp->dlg);

				PCB_DAD_LABEL(shp->dlg, "");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_COORD(shp->dlg, "");
						shp->rcy = PCB_DAD_CURRENT(shp->dlg);
						PCB_DAD_MINMAX(shp->dlg, 0, maxr);
						PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_LABEL(shp->dlg, "y (vertical)");
				PCB_DAD_END(shp->dlg);
			PCB_DAD_END(shp->dlg);
		PCB_DAD_END(shp->dlg);

		/* circle tab */
		PCB_DAD_BEGIN_VBOX(shp->dlg);
			shp->tab_circle = PCB_DAD_CURRENT(shp->dlg);
			PCB_DAD_LABEL(shp->dlg, "Generate filled circle");
			PCB_DAD_BEGIN_HBOX(shp->dlg);
				PCB_DAD_LABEL(shp->dlg, "Diameter:");
				PCB_DAD_COORD(shp->dlg, "");
					shp->dia = PCB_DAD_CURRENT(shp->dlg);
					PCB_DAD_MINMAX(shp->dlg, 0, maxr);
					PCB_DAD_DEFAULT(shp->dlg, mm2);
					PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_circle);
			PCB_DAD_END(shp->dlg);

			PCB_DAD_LABEL(shp->dlg, "Center offset");
			PCB_DAD_BEGIN_HBOX(shp->dlg);
				PCB_DAD_COORD(shp->dlg, "");
					shp->ccx = PCB_DAD_CURRENT(shp->dlg);
					PCB_DAD_MINMAX(shp->dlg, 0, maxr);
					PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_circle);
				PCB_DAD_LABEL(shp->dlg, "x (horizontal)");
			PCB_DAD_END(shp->dlg);

			PCB_DAD_LABEL(shp->dlg, "");
			PCB_DAD_BEGIN_HBOX(shp->dlg);
				PCB_DAD_COORD(shp->dlg, "");
					shp->ccy = PCB_DAD_CURRENT(shp->dlg);
					PCB_DAD_MINMAX(shp->dlg, 0, maxr);
					PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_circle);
				PCB_DAD_LABEL(shp->dlg, "y (vertical)");
			PCB_DAD_END(shp->dlg);
		PCB_DAD_END(shp->dlg);
	PCB_DAD_END(shp->dlg);

	PCB_DAD_NEW(shp->dlg, "dlg_shape", "Generate shapes", shp, modal, shp_button_cb);
	shp_tab_update(shp->dlg_hid_ctx, shp);
	shp_chg_circle(shp->dlg_hid_ctx, shp, NULL);
	shp_chg_roundrect(shp->dlg_hid_ctx, shp, NULL);
	shp_chg_regpoly(shp->dlg_hid_ctx, shp, NULL); /* has to be the last */
	if (modal) {
		PCB_DAD_RUN(shp->dlg);
		PCB_DAD_FREE(shp->dlg);
	}
}


static const char pcb_acts_shape[] = "shape()";
static const char pcb_acth_shape[] = "Interactive shape generator.";
int pcb_act_shape(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_shape_dialog(PCB, PCB_PASTEBUFFER->Data, CURRENT, pcb_false);
	return 0;
}

