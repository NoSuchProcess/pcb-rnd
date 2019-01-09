#include "hid_dad.h"

typedef struct {
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	pcb_data_t *data;
	pcb_layer_t *layer;
	pcb_any_obj_t *obj;

	/* tab management */
	int tab;

	/* regpoly */
	int prx, pry, corners, pcx, pcy, prot, pell;

	/* roundrect */
	int w, h, rrect, rcx, rcy, rx, ry, rrot, rell, corner[4];

	/* circle */
	int dia, ccx, ccy;

} ctx_t;

/* Last open non-modal shape dialog */
static ctx_t *shape_active = NULL;

static void shp_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	ctx_t *shp = caller_data;

	if (shp == shape_active)
		shape_active = NULL;

	PCB_DAD_FREE(shp->dlg);
	free(shp);
}

static void del_obj(ctx_t *shp)
{
	if (shp->obj != NULL) {
/*		pcb_remove_object(shp->obj->type, shp->layer, shp->obj, shp->obj);*/
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
	pcb_shape_corner_t corner[4];
	int n;

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
		pcb_gui->attr_dlg_widget_state(hid_ctx, shp->h, pcb_true);

	for(n = 0; n < 4; n++)
		corner[n] = shp->dlg[shp->corner[n]].default_val.int_value;

	del_obj(shp);
	shp->obj = (pcb_any_obj_t *)roundrect_place(
		shp->data, shp->layer,
		shp->dlg[shp->w].default_val.coord_value, shp->dlg[shp->h].default_val.coord_value,
		shp->dlg[shp->rx].default_val.coord_value, shp->dlg[shp->ry].default_val.coord_value,
		shp->dlg[shp->rrot].default_val.real_value,
		shp->dlg[shp->rcx].default_val.coord_value, shp->dlg[shp->rcy].default_val.coord_value,
		corner);
}

static void shp_chg_circle(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ctx_t *shp = caller_data;
	pcb_coord_t dia = shp->dlg[shp->dia].default_val.coord_value;

	if ((dia < 1) || (dia > (PCB->MaxWidth + PCB->MaxHeight)/4)) {
		pcb_message(PCB_MSG_ERROR, "Invalid diameter.\n");
		return;
	}

	shp->obj = (pcb_any_obj_t *)circle_place(
		shp->data, shp->layer,
		dia,
		shp->dlg[shp->ccx].default_val.coord_value, shp->dlg[shp->ccy].default_val.coord_value);
}


static void shape_layer_chg(void *user_data, int argc, pcb_event_arg_t argv[])
{
	void *hid_ctx;
	int tab;

	if (shape_active == NULL)
		return;

	hid_ctx = shape_active->dlg_hid_ctx;
	tab = shape_active->dlg[shape_active->tab].default_val.int_value;
	switch(tab) {
		case 0: shp_chg_regpoly(hid_ctx, shape_active, NULL); break;
		case 1: shp_chg_roundrect(hid_ctx, shape_active, NULL); break;
		case 2: shp_chg_circle(hid_ctx, shape_active, NULL); break;
	}
}


void pcb_shape_dialog(pcb_board_t *pcb, pcb_data_t *data, pcb_layer_t *layer, pcb_bool modal)
{
	ctx_t *shp = calloc(sizeof(ctx_t), 1);
	pcb_coord_t mm2 = PCB_MM_TO_COORD(2);
	pcb_coord_t maxr = PCB_MM_TO_COORD(1000);
	const char *tabs[] = {"Regular polygon", "Round rectangle", "Filled circle", NULL};
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	shp->pcb = pcb;
	shp->data = data;
	shp->layer = layer;

	PCB_DAD_BEGIN_VBOX(shp->dlg);
		PCB_DAD_COMPFLAG(shp->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABBED(shp->dlg, tabs);
/*			shp->tab = PCB_DAD_CURRENT(shp->dlg);
			PCB_DAD_CHANGE_CB(shp->dlg, shp_tab);*/

			/* regpoly tab */
			PCB_DAD_BEGIN_VBOX(shp->dlg);
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

				PCB_DAD_LABEL(shp->dlg, "Corner style:");
				PCB_DAD_BEGIN_HBOX(shp->dlg);
					PCB_DAD_BEGIN_TABLE(shp->dlg, 2);
						PCB_DAD_ENUM(shp->dlg, pcb_shape_corner_name);
							shp->corner[0] = PCB_DAD_CURRENT(shp->dlg);
							PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
						PCB_DAD_ENUM(shp->dlg, pcb_shape_corner_name);
							shp->corner[1] = PCB_DAD_CURRENT(shp->dlg);
							PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
						PCB_DAD_ENUM(shp->dlg, pcb_shape_corner_name);
							shp->corner[2] = PCB_DAD_CURRENT(shp->dlg);
							PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
						PCB_DAD_ENUM(shp->dlg, pcb_shape_corner_name);
							shp->corner[3] = PCB_DAD_CURRENT(shp->dlg);
							PCB_DAD_CHANGE_CB(shp->dlg, shp_chg_roundrect);
					PCB_DAD_END(shp->dlg);
				PCB_DAD_END(shp->dlg);
			PCB_DAD_END(shp->dlg);

			/* circle tab */
			PCB_DAD_BEGIN_VBOX(shp->dlg);
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
		PCB_DAD_BUTTON_CLOSES(shp->dlg, clbtn);
	PCB_DAD_END(shp->dlg);

	PCB_DAD_NEW("shape", shp->dlg, "dlg_shape", shp, modal, shp_close_cb);
	PCB_DAD_SET_VALUE(shp->dlg_hid_ctx, shp->dia, coord_value, PCB_MM_TO_COORD(25.4)); /* suppress a runtime warning on invalid dia (zero) */
	shp_chg_circle(shp->dlg_hid_ctx, shp, NULL);
	shp_chg_roundrect(shp->dlg_hid_ctx, shp, NULL);
	shp_chg_regpoly(shp->dlg_hid_ctx, shp, NULL); /* has to be the last */
	if (modal) {
		PCB_DAD_RUN(shp->dlg);
		PCB_DAD_FREE(shp->dlg);
	}
	else
		shape_active = shp;
}


static const char pcb_acts_shape[] = "shape()";
static const char pcb_acth_shape[] = "Interactive shape generator.";
fgw_error_t pcb_act_shape(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_shape_dialog(PCB, PCB_PASTEBUFFER->Data, pcb_shape_current_layer, pcb_false);
	PCB_ACT_IRES(0);
	return 0;
}

