/* Directly included by main.c for now */

#define SHOW_SAVES \
	int save_vx, save_vy, save_vw, save_vh; \
	int save_fx, save_fy; \
	double save_vz; \
	Pixmap save_px

#define SHOW_ENTER \
do { \
	pinout = 0; \
	save_vx = view_left_x; \
	save_vy = view_top_y; \
	save_vz = view_zoom; \
	save_vw = view_width; \
	save_vh = view_height; \
	save_fx = conf_core.editor.view.flip_x; \
	save_fy = conf_core.editor.view.flip_y; \
	save_px = pixmap; \
	pixmap = pd->window; \
	view_left_x = pd->x; \
	view_top_y = pd->y; \
	view_zoom = pd->zoom; \
	view_width = pd->v_width; \
	view_height = pd->v_height; \
	use_mask = 0; \
	conf_force_set_bool(conf_core.editor.view.flip_x, 0); \
	conf_force_set_bool(conf_core.editor.view.flip_y, 0); \
} while(0)

#define SHOW_LEAVE \
do { \
	view_left_x = save_vx; \
	view_top_y = save_vy; \
	view_zoom = save_vz; \
	view_width = save_vw; \
	view_height = save_vh; \
	pixmap = save_px; \
	conf_force_set_bool(conf_core.editor.view.flip_x, save_fx); \
	conf_force_set_bool(conf_core.editor.view.flip_y, save_fy); \
} while(0)

#define SHOW_DRAW \
do { \
	XFillRectangle(display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height); \
	pcb_hid_expose_layer(&lesstif_hid, &pd->ctx); \
} while(0)

static void show_layer_callback(Widget da, PinoutData * pd, XmDrawingAreaCallbackStruct * cbs)
{
	SHOW_SAVES;
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

	SHOW_ENTER;
	SHOW_DRAW;

	SHOW_LEAVE;

}

static void show_layer_inp_callback(Widget da, PinoutData * pd, XmDrawingAreaCallbackStruct * cbs)
{
	SHOW_SAVES;
	pcb_coord_t cx, cy;
	pcb_hid_cfg_mod_t btn;
	pcb_hid_mouse_ev_t kind;

	SHOW_ENTER;

	btn = lesstif_mb2cfg(cbs->event->xbutton.button);

	cx = Px(cbs->event->xbutton.x);
	cy = Py(cbs->event->xbutton.y);

/*	pcb_printf("ev %d;%d %$mm;%$mm %x\n", cbs->event->xbutton.x, cbs->event->xbutton.y, cx, cy, btn);*/

	kind = -1;
	if (cbs->event->type == ButtonPress) kind = PCB_HID_MOUSE_PRESS;
	if (cbs->event->type == ButtonRelease) kind = PCB_HID_MOUSE_RELEASE;

	switch(btn) {
		case PCB_MB_LEFT:
			if (pd->mouse_ev != NULL)
				if (pd->mouse_ev(da, kind, cx, cy))
					SHOW_DRAW;
			break;
		case PCB_MB_MIDDLE:
			if (kind == PCB_HID_MOUSE_PRESS) {
				pd->pan = 1;
				pd->pan_ox = cbs->event->xbutton.x;
				pd->pan_oy = cbs->event->xbutton.y;
				pd->pan_opx = view_left_x;
				pd->pan_opy = view_top_y;
			}
			else if (kind == PCB_HID_MOUSE_RELEASE)
				pd->pan = 0;
			break;
		case PCB_MB_SCROLL_DOWN:
			pd->zoom *= 1.25;
			view_zoom = pd->zoom;
			SHOW_DRAW;
			break;
		case PCB_MB_SCROLL_UP:
			pd->zoom *= 0.8;
			view_zoom = pd->zoom;
			SHOW_DRAW;
			break;
		default:;
	}

	SHOW_LEAVE;
}

static void show_layer_mot_callback(Widget w, XtPointer pd_, XEvent * e, Boolean * ctd)
{
	PinoutData *pd = pd_;
	SHOW_SAVES;
	Window root, child;
	unsigned int keys_buttons;
	int root_x, root_y, pos_x, pos_y;
	pcb_coord_t cx, cy;

	while (XCheckMaskEvent(display, PointerMotionMask, e));
	XQueryPointer(display, e->xmotion.window, &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);

	SHOW_ENTER;

	cx = Px(pos_x);
	cy = Py(pos_y);

/*	pcb_printf("mo %d;%d %$mm;%$mm\n", pos_x, pos_y, cx, cy);*/
	if (pd->pan) {
/*		pcb_coord_t ol = pd->x, ot = pd->y;*/
		pd->x = pd->pan_opx - (pos_x - pd->pan_ox) * view_zoom;
		pd->y = pd->pan_opy - (pos_y - pd->pan_oy) * view_zoom;
/*		pd->view_right += ol - pd->view_left_x;
		pd->view_bottom += ot - pd->view_top_y;*/
		SHOW_DRAW;
	}
	else if (pd->mouse_ev != NULL) {
		pd->mouse_ev(w, PCB_HID_MOUSE_MOTION, cx, cy);
		if (pd->overlay_draw != NULL)
			pd->overlay_draw(pcb_gui, &pd->ctx);
	}

	SHOW_LEAVE;
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
	PinoutData *pd;

	if (!mainwind)
		return NULL;

	pd = (PinoutData *) calloc(1, sizeof(PinoutData));

	pd->ctx.content.layer_id = layer;

	pd->ctx.view.X1 = pd->left = 0;
	pd->ctx.view.X2 = pd->right = PCB_MM_TO_COORD(130);
	pd->ctx.view.Y1 = pd->top = 0;
	pd->ctx.view.Y2 = pd->bottom = PCB_MM_TO_COORD(130);
	pd->ctx.force = 1;

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
	XtAddCallback(da, XmNinputCallback, (XtCallbackProc) show_layer_inp_callback, (XtPointer) pd);

	XtAddEventHandler(da,
		PointerMotionMask | PointerMotionHintMask | EnterWindowMask | LeaveWindowMask,
		0, show_layer_mot_callback, pd);

	XtManageChild(pd->form);

	return pd;
}

/***************** instance for layer group edit **********************/

#include "stub_draw_csect.h"

static PinoutData *layergrp_edit = NULL;
void lesstif_show_layergrp_edit(void)
{
	pcb_layer_id_t lid;
	if (pcb_layer_list(PCB_LYT_CSECT, &lid, 1) > 0) {
		layergrp_edit = lesstif_show_layer(lid, "Layer groups");
		layergrp_edit->mouse_ev = pcb_stub_draw_csect_mouse_ev;
		layergrp_edit->overlay_draw = pcb_stub_draw_csect_overlay;
	}
}
