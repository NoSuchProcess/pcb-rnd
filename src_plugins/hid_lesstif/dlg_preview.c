/* Directly included by main.c for now */

static void show_layer_callback(Widget da, PinoutData * pd, XmDrawingAreaCallbackStruct * cbs)
{
	int save_vx, save_vy, save_vw, save_vh;
	int save_fx, save_fy;
	double save_vz;
	Pixmap save_px;
	int reason = cbs ? cbs->reason : 0;

	if (pd->window == 0 && reason == XmCR_RESIZE)
		return;
	if (pd->window == 0 || reason == XmCR_RESIZE) {
		Dimension w, h;
		double z;

		stdarg_n = 0;
		stdarg(XmNwidth, &w);
		stdarg(XmNheight, &h);
		XtGetValues(da, stdarg_args, stdarg_n);

		pd->window = XtWindow(da);
		pd->v_width = w;
		pd->v_height = h;
		pd->zoom = (pd->right - pd->left + 1) / (double) w;
		z = (pd->bottom - pd->top + 1) / (double) h;
		if (pd->zoom < z)
			pd->zoom = z;

		pd->x = (pd->left + pd->right) / 2 - pd->v_width * pd->zoom / 2;
		pd->y = (pd->top + pd->bottom) / 2 - pd->v_height * pd->zoom / 2;
	}

	save_vx = view_left_x;
	save_vy = view_top_y;
	save_vz = view_zoom;
	save_vw = view_width;
	save_vh = view_height;
	save_fx = conf_core.editor.view.flip_x;
	save_fy = conf_core.editor.view.flip_y;
	save_px = pixmap;
	pinout = pd;
	pixmap = pd->window;
	view_left_x = pd->x;
	view_top_y = pd->y;
	view_zoom = pd->zoom;
	view_width = pd->v_width;
	view_height = pd->v_height;
	use_mask = 0;
	conf_force_set_bool(conf_core.editor.view.flip_x, 0);
	conf_force_set_bool(conf_core.editor.view.flip_y, 0);

	XFillRectangle(display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height);
	pcb_hid_expose_layer(&lesstif_hid, &pd->ctx);

	pinout = 0;
	view_left_x = save_vx;
	view_top_y = save_vy;
	view_zoom = save_vz;
	view_width = save_vw;
	view_height = save_vh;
	pixmap = save_px;
	conf_force_set_bool(conf_core.editor.view.flip_x, save_fx);
	conf_force_set_bool(conf_core.editor.view.flip_y, save_fy);
}

static void show_layer_unmap(Widget w, PinoutData * pd, void *v)
{
	XtDestroyWidget(XtParent(pd->form));
	free(pd);
}

static PinoutData *lesstif_show_layer(pcb_layer_id_t layer, const char *title)
{
	double scale;
	Widget da;
	pcb_box_t *extents;
	PinoutData *pd;

	if (!mainwind)
		return;

	pd = (PinoutData *) calloc(1, sizeof(PinoutData));

	pd->ctx.content.layer_id = layer;

	pd->left = 0;
	pd->right = 0;
	pd->top = PCB_MM_TO_COORD(130);
	pd->bottom = PCB_MM_TO_COORD(130);

	pd->prev = 0;
	pd->zoom = 0;

	stdarg_n = 0;
	pd->form = XmCreateFormDialog(mainwind, XmStrCast(title), stdarg_args, stdarg_n);
	pd->window = 0;
	XtAddCallback(pd->form, XmNunmapCallback, (XtCallbackProc) show_layer_unmap, (XtPointer) pd);

	scale = sqrt(200.0 * 200.0 / ((pd->right - pd->left + 1.0) * (pd->bottom - pd->top + 1.0)));

	stdarg_n = 0;
	stdarg(XmNwidth, (int) (scale * (pd->right - pd->left + 1)));
	stdarg(XmNheight, (int) (scale * (pd->bottom - pd->top + 1)));
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	da = XmCreateDrawingArea(pd->form, XmStrCast(title), stdarg_args, stdarg_n);
	XtManageChild(da);

	XtAddCallback(da, XmNexposeCallback, (XtCallbackProc) show_layer_callback, (XtPointer) pd);
	XtAddCallback(da, XmNresizeCallback, (XtCallbackProc) show_layer_callback, (XtPointer) pd);

	XtManageChild(pd->form);

	return pd;
}

static PinoutData *layergrp_edit = NULL;
void lesstif_show_layergrp_edit(void)
{
	pcb_layer_id_t lid;
	if (pcb_layer_list(PCB_LYT_CSECT, &lid, 1) > 0)
		layergrp_edit = lesstif_show_layer(lid, "Layer groups");
}
