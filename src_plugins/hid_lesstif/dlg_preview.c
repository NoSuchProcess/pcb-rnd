/* Directly included by main.c for now */

static int widget_depth(Widget w) {
	Arg args[1];
	int depth;

	XtSetArg(args[0], XtNdepth, &depth);
	XtGetValues(w, args, 1);
	return depth;
}

#define SHOW_SAVES \
	int save_vx, save_vy, save_vw, save_vh; \
	int save_fx, save_fy; \
	double save_vz; \
	Pixmap save_px, save_main_px, save_mask_px, save_mask_bm

/* saving flip setting in this macro is required to properly calculate coordinates of an event (Px, Py functions) */
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
	save_main_px = main_pixmap; \
	save_mask_px = mask_pixmap; \
	save_mask_bm = mask_bitmap; \
	main_pixmap = XCreatePixmap(XtDisplay(da), XtWindow(da), pd->v_width, pd->v_height, widget_depth(da)); \
	mask_pixmap = XCreatePixmap(XtDisplay(da), XtWindow(da), pd->v_width, pd->v_height, widget_depth(da)); \
	mask_bitmap = XCreatePixmap(XtDisplay(da), XtWindow(da), pd->v_width, pd->v_height, 1); \
	pixmap = main_pixmap; \
	view_left_x = pd->x; \
	view_top_y = pd->y; \
	view_zoom = pd->zoom; \
	view_width = pd->v_width; \
	view_height = pd->v_height; \
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
	XFreePixmap(lesstif_display, main_pixmap); \
	XFreePixmap(lesstif_display, mask_pixmap); \
	XFreePixmap(lesstif_display, mask_bitmap); \
	main_pixmap = save_main_px; \
	mask_pixmap = save_mask_px; \
	mask_bitmap = save_mask_bm; \
	pixmap = save_px; \
	conf_force_set_bool(conf_core.editor.view.flip_x, save_fx); \
	conf_force_set_bool(conf_core.editor.view.flip_y, save_fy); \
} while(0)

#define SHOW_DRAW \
do { \
	XGCValues gcv; \
	GC gc; \
	memset(&gcv, 0, sizeof(gcv)); \
	gcv.graphics_exposures = 0; \
	gc = XtGetGC(da, GCGraphicsExposures, &gcv); \
	XSetForeground(display, bg_gc, bgcolor); \
	XFillRectangle(display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height); \
	pcb_hid_expose_layer(&lesstif_hid, &pd->ctx); \
	XCopyArea(lesstif_display, pixmap, XtWindow(da), gc, 0, 0, pd->v_width, pd->v_height, 0, 0); \
	XtReleaseGC(da, gc); \
} while(0)

static void show_layer_callback(Widget da, PreviewData * pd, XmDrawingAreaCallbackStruct * cbs)
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

static void show_layer_inp_callback(Widget da, PreviewData * pd, XmDrawingAreaCallbackStruct * cbs)
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
		case PCB_MB_RIGHT:
			if (pd->mouse_ev != NULL)
				if (pd->mouse_ev(da, PCB_HID_MOUSE_POPUP, cx, cy))
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
	Widget da = w;
	PreviewData *pd = pd_;
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
		SHOW_DRAW;
	}

	SHOW_LEAVE;
}


static void show_layer_unmap(Widget w, PreviewData * pd, void *v)
{
	if (pd->pre_close != NULL)
		pd->pre_close(pd);
	XtDestroyWidget(XtParent(pd->form));
	free(pd);
}


static PreviewData *lesstif_show_layer(pcb_layer_id_t layer, const char *title, int modal)
{
	double scale;
	Widget da;
	PreviewData *pd;

	if (!mainwind)
		return NULL;

	pd = (PreviewData *) calloc(1, sizeof(PreviewData));

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

	if (modal)
		XtVaSetValues(pd->form, XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL, NULL);

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

#include "stub_draw.h"

static PreviewData *layergrp_edit = NULL;

static void layergrp_pre_close(struct PreviewData *pd)
{
	if (pd == layergrp_edit)
		layergrp_edit = NULL;
}

void lesstif_show_layergrp_edit(void)
{
	pcb_layer_id_t lid;
	if (layergrp_edit != NULL)
		return;
	if (pcb_layer_listp(PCB, PCB_LYT_VIRTUAL, &lid, 1, F_csect, NULL) > 0) {
		layergrp_edit = lesstif_show_layer(lid, "Layer groups", 0);
		layergrp_edit->mouse_ev = pcb_stub_draw_csect_mouse_ev;
		layergrp_edit->pre_close = layergrp_pre_close;
	}
}

/***************** instance for font selection **********************/

#include "stub_draw.h"

static PreviewData *fontsel_glob = NULL;

static void fontsel_pre_close_glob(struct PreviewData *pd)
{
	if (pd == fontsel_glob)
		fontsel_glob = NULL;
}

static void lesstif_show_fontsel_global()
{
	pcb_layer_id_t lid;
	if (fontsel_glob != NULL)
		return;
	if (pcb_layer_list(PCB, PCB_LYT_DIALOG, &lid, 1) > 0) {
		fontsel_glob = lesstif_show_layer(lid, "Pen font selection", 0);
		fontsel_glob->ctx.dialog_draw = pcb_stub_draw_fontsel;
		fontsel_glob->mouse_ev = pcb_stub_draw_fontsel_mouse_ev;
		fontsel_glob->overlay_draw = NULL;
		fontsel_glob->pre_close = fontsel_pre_close_glob;
	}
}


static PreviewData *fontsel_loc = NULL;


static void fontsel_pre_close_loc(struct PreviewData *pd)
{
	if (pd == fontsel_loc)
		fontsel_loc = NULL;
}

static void lesstif_show_fontsel_local(pcb_layer_t *txtly, pcb_text_t *txt, int type)
{
	pcb_layer_id_t lid;
	pcb_text_t *old_txt;
	pcb_layer_t *old_layer;
	int old_type;

	if (pcb_layer_list(PCB, PCB_LYT_DIALOG, &lid, 1) <= 0)
		return;

	old_txt = *pcb_stub_draw_fontsel_text_obj;
	old_layer = *pcb_stub_draw_fontsel_layer_obj;
	old_type = *pcb_stub_draw_fontsel_text_type;

	*pcb_stub_draw_fontsel_text_obj = txt;
	*pcb_stub_draw_fontsel_layer_obj = txtly;
	*pcb_stub_draw_fontsel_text_type = type;

	fontsel_loc = lesstif_show_layer(lid, "Change font of text object", 1);
	fontsel_loc->mouse_ev = pcb_stub_draw_fontsel_mouse_ev;
	fontsel_loc->overlay_draw = NULL;
	fontsel_loc->pre_close = fontsel_pre_close_loc;


	while (fontsel_loc != NULL) {
		XEvent e;
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}

	*pcb_stub_draw_fontsel_text_obj = old_txt;
	*pcb_stub_draw_fontsel_layer_obj = old_layer;
	*pcb_stub_draw_fontsel_text_type = old_type;
}

void lesstif_show_fontsel_edit(pcb_layer_t *txtly, pcb_text_t *txt, int type)
{
	if (txt == NULL)
		lesstif_show_fontsel_global();
	else
		lesstif_show_fontsel_local(txtly, txt, type);
}

