#include "xincludes.h"

#include "config.h"
#include <librnd/core/math_helper.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/pixmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <setjmp.h>

#include "data.h"
#include "draw.h"
#include <librnd/core/color.h>
#include <librnd/core/color_cache.h>
#include "crosshair.h"
#include <librnd/core/conf_hid.h>
#include "layer.h"
#include <librnd/core/pcb-printf.h>
#include "event.h"
#include <librnd/core/error.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "funchash_core.h"

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/hid_cfg.h>
#include "lesstif.h"
#include <librnd/core/hid_cfg_input.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_dad.h>
#include <librnd/core/actions.h>
#include "ltf_stdarg.h"
#include <librnd/core/grid.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include "layer_vis.h"
#include <librnd/core/tool.h>

#include "wt_preview.h"
#include "dlg_fileselect.h"

#include "FillBox.h"

#include "../src_plugins/lib_hid_common/clip.h"
#include "../src_plugins/lib_hid_pcbui/util.h"
#include "../src_plugins/lib_hid_common/cli_history.h"

#include <sys/poll.h>

const char *lesstif_cookie = "lesstif HID";

pcb_hidlib_t *ltf_hidlib;

pcb_hid_cfg_mouse_t lesstif_mouse;
pcb_hid_cfg_keys_t lesstif_keymap;
int lesstif_active = 0;

static int idle_proc_set = 0;
static int need_redraw = 0;

static pcb_clrcache_t ltf_ccache;
static int ltf_ccache_inited;

#ifndef XtRDouble
#define XtRDouble "Double"
#endif

/* How big the viewport can be relative to the pcb size.  */
#define MAX_ZOOM_SCALE	10
#define UUNIT	pcbhl_conf.editor.grid_unit->allow

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;
	Pixel color;
	pcb_color_t pcolor;
	int width;
	pcb_cap_style_t cap;
	char xor_set;
	char erase;
} hid_gc_s;

pcb_hid_t lesstif_hid;

#define CRASH(func) fprintf(stderr, "HID error: pcb called unimplemented GUI function %s\n", func), abort()

XtAppContext app_context;
Widget appwidget;
Display *display;
static Window window = 0;
static int old_cursor_mode = -1;
static int over_point = 0;

static Pixmap pixmap = 0;          /* Current pixmap we are drawing to (either main_pixmap in direct mode or mask_pixmap when drawing the sketch) */
static Pixmap main_pixmap = 0;     /* 'Base pixmap', the final output all sketches are composed onto (then drawn) */
static Pixmap mask_pixmap = 0;     /* 'Sketch pixmap' for compositing: color array */
static Pixmap mask_bitmap = 0;     /* 'Sketch transparency bitmap' for compositing: tells which pixels shall be copied and which one are transparent/erased */

static int use_xrender = 0;
#ifdef HAVE_XRENDER
static Picture main_picture;
static Picture mask_picture;
static Pixmap pale_pixmap;
static Picture pale_picture;
#endif /* HAVE_XRENDER */

static int pixmap_w = 0, pixmap_h = 0;
Screen *screen_s;
int screen;
Colormap lesstif_colormap;
static GC my_gc = 0, bg_gc, clip_gc = 0, pxm_clip_gc = 0, bset_gc = 0, bclear_gc = 0, mask_gc = 0;
static Pixel bgcolor, offlimit_color, grid_color;
static int bgred, bggreen, bgblue;

static pcb_coord_t crosshair_x = 0, crosshair_y = 0;
static int in_move_event = 0, crosshair_in_window = 1;

Widget mainwind;
Widget work_area, messages, command, hscroll, vscroll;
Widget m_click;

/* This is the size, in pixels, of the viewport.  */
static int view_width, view_height;
/* This is the PCB location represented by the upper left corner of
   the viewport.  Note that PCB coordinates put 0,0 in the upper left,
   much like X does.  */
static pcb_coord_t view_left_x = 0, view_top_y = 0;
/* Denotes PCB units per screen pixel.  Larger numbers mean zooming
   out - the largest value means you are looking at the whole
   board.  */
static double view_zoom = PCB_MIL_TO_COORD(10);
static pcb_bool autofade = 0;
static pcb_bool crosshair_on = pcb_true;

static void lesstif_reg_attrs(void);
static void lesstif_begin(void);
static void lesstif_end(void);

static Widget ltf_dockbox[PCB_HID_DOCK_max];
static gdl_list_t ltf_dock[PCB_HID_DOCK_max];

typedef struct {
	void *hid_ctx;
	Widget frame;
	pcb_hid_dock_t where;
} docked_t;

static Widget ltf_create_dockbox(Widget parent, pcb_hid_dock_t where, int vert)
{
	stdarg(PxmNfillBoxVertical, vert);
	stdarg(XmNmarginWidth, 0);
	stdarg(XmNmarginHeight, 0);
	ltf_dockbox[where] = PxmCreateFillBox(parent, "dockbox", stdarg_args, stdarg_n);

	{
		Widget w;
		stdarg_n = 0;
		stdarg(XmNlabelString, XmStringCreatePCB("TODO#12"));
		w = XmCreateLabel(ltf_dockbox[where], XmStrCast("dock1"), stdarg_args, stdarg_n);
		XtManageChild(w);
	}


	return ltf_dockbox[where];
}

static int ltf_dock_poke(pcb_hid_dad_subdialog_t *sub, const char *cmd, pcb_event_arg_t *res, int argc, pcb_event_arg_t *argv)
{
	return -1;
}

static int ltf_dock_enter(pcb_hid_t *hid, pcb_hid_dad_subdialog_t *sub, pcb_hid_dock_t where, const char *id)
{
	docked_t *docked;
	Widget hvbox;

	if (ltf_dockbox[where] == NULL)
		return -1;

	docked = calloc(sizeof(docked_t), 1);
	docked->where = where;

	stdarg_n = 0;
	stdarg(PxmNfillBoxVertical, pcb_dock_is_vert[where]);
	stdarg(XmNmarginWidth, 0);
	stdarg(XmNmarginHeight, 0);
	hvbox = PxmCreateFillBox(ltf_dockbox[where], "dockbox", stdarg_args, stdarg_n);

TODO("hidlib: dock frame");
/*
	if (pcb_dock_has_frame[where]) {
		docked->frame = gtk_frame_new(id);
		gtk_container_add(GTK_CONTAINER(docked->frame), hvbox);
	}
	else*/
		docked->frame = hvbox;

	XtManageChild(docked->frame);

	sub->parent_poke = ltf_dock_poke;
	sub->dlg_hid_ctx = docked->hid_ctx = lesstif_attr_sub_new(hvbox, sub->dlg, sub->dlg_len, sub);
	sub->parent_ctx = docked;

	gdl_append(&ltf_dock[where], sub, link);

	return 0;
}


typedef struct {
	void *hid_ctx;
	Widget frame;
	pcb_hid_dock_t where;
} ltf_docked_t;

static void ShowCrosshair(pcb_bool show)
{
	if (crosshair_on == show)
		return;

	pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_false);
	if (pcb_marked.status)
		pcb_notify_mark_change(pcb_false);

	crosshair_on = show;

	pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_true);
	if (pcb_marked.status)
		pcb_notify_mark_change(pcb_true);
}

/* This is the size of the current PCB work area.  */
/* Use ltf_hidlib->size_x, ltf_hidlib->size_y.  */
/* static int pcb_width, pcb_height; */
static int use_private_colormap = 0;
static int stdin_listen = 0;
static char *background_image_file = 0;

pcb_export_opt_t lesstif_attribute_list[] = {
	{"install", "Install private colormap",
		PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, &use_private_colormap},
#define HA_colormap 0

/* %start-doc options "22 lesstif GUI Options"
@ftable @code
@item --listen
Listen for actions on stdin.
@end ftable
%end-doc
*/
	{"listen", "Listen on standard input for actions",
		PCB_HATT_BOOL, 0, 0, {0, 0, 0}, 0, &stdin_listen},
#define HA_listen 1

/* %start-doc options "22 lesstif GUI Options"
@ftable @code
@item --bg-image <string>
File name of an image to put into the background of the GUI canvas. The image must
be a color PPM image, in binary (not ASCII) format. It can be any size, and will be
automatically scaled to fit the canvas.
@end ftable
%end-doc
*/
	{"bg-image", "Background Image",
		PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, &background_image_file},
#define HA_bg_image 2
};

TODO("menu: pcb-menu should be generic and not depend on the HID")


static int lesstif_direct = 0;
static pcb_composite_op_t lesstif_drawing_mode = 0;
#define use_mask() ((!lesstif_direct) && ((lesstif_drawing_mode == PCB_HID_COMP_POSITIVE) || (lesstif_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (lesstif_drawing_mode == PCB_HID_COMP_NEGATIVE)))

static void zoom_max();
static void zoom_to(double factor, pcb_coord_t x, pcb_coord_t y);
static void zoom_win(pcb_hid_t *hid, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, int setch);
static void zoom_by(double factor, pcb_coord_t x, pcb_coord_t y);
static void Pan(int mode, pcb_coord_t x, pcb_coord_t y);

/* Px converts view->pcb, Vx converts pcb->view */

static inline int Vx(pcb_coord_t x)
{
	int rv = (x - view_left_x) / view_zoom + 0.5;
	if (pcbhl_conf.editor.view.flip_x)
		rv = view_width - rv;
	return rv;
}

static inline int Vy(pcb_coord_t y)
{
	int rv = (y - view_top_y) / view_zoom + 0.5;
	if (pcbhl_conf.editor.view.flip_y)
		rv = view_height - rv;
	return rv;
}

static inline int Vz(pcb_coord_t z)
{
	return z / view_zoom + 0.5;
}

static inline int Vw(pcb_coord_t w)
{
	return w < 0 ? -w : w / view_zoom + 0.5;
}

static inline pcb_coord_t Px(int x)
{
	if (pcbhl_conf.editor.view.flip_x)
		x = view_width - x;
	return x * view_zoom + view_left_x;
}

static inline pcb_coord_t Py(int y)
{
	if (pcbhl_conf.editor.view.flip_y)
		y = view_height - y;
	return y * view_zoom + view_top_y;
}

void lesstif_coords_to_pcb(int vx, int vy, pcb_coord_t * px, pcb_coord_t * py)
{
	*px = Px(vx);
	*py = Py(vy);
}

Pixel lesstif_parse_color(const pcb_color_t *value)
{
	XColor color;
	color.pixel = 0;
	color.red = (unsigned)value->r << 8;
	color.green = (unsigned)value->g << 8;
	color.blue = (unsigned)value->b << 8;
	color.flags = DoRed | DoBlue | DoGreen;
	if (XAllocColor(display, lesstif_colormap, &color))
		return color.pixel;
	return 0;
}

/* ------------------------------------------------------------ */

static const char *cur_clip()
{
	if (conf_core.editor.orthogonal_moves)
		return "+";
	if (conf_core.editor.all_direction_lines)
		return "*";
	if (conf_core.editor.line_refraction == 0)
		return "X";
	if (conf_core.editor.line_refraction == 1)
		return "_/";
	return "\\_";
}

/* Called from the core when it's busy doing something and we need to indicate that to the user.  */
static void ltf_busy(pcb_hid_t *hid, pcb_bool busy)
{
	static Cursor busy_cursor = 0;
	if (!lesstif_active)
		return;

	if (busy) {
		if (busy_cursor == 0)
			busy_cursor = XCreateFontCursor(display, XC_watch);
		XDefineCursor(display, window, busy_cursor);
		XFlush(display);
		old_cursor_mode = -1;
	}
	else
		need_idle_proc(); /* restores the cursor */
}


/* ---------------------------------------------------------------------- */

/* Local actions.  */

static void PointCursor(pcb_hid_t *hid, pcb_bool grabbed)
{
	if (grabbed > 0)
		over_point = 1;
	else
		over_point = 0;
	old_cursor_mode = -1;
}

extern void LesstifNetlistChanged(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
extern void LesstifLibraryChanged(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);


static void ltf_set_hidlib(pcb_hid_t *hid, pcb_hidlib_t *hidlib)
{
	ltf_hidlib = hidlib;
	if ((work_area == 0) || (hidlib == NULL))
		return;
	/*pcb_printf("PCB Changed! %$mD\n", ltf_hidlib->size_x, ltf_hidlib->size_y); */
	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNsliderSize, ltf_hidlib->size_x ? ltf_hidlib->size_x : 1);
	stdarg(XmNmaximum, ltf_hidlib->size_x ? ltf_hidlib->size_x : 1);
	XtSetValues(hscroll, stdarg_args, stdarg_n);
	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNsliderSize, ltf_hidlib->size_y ? ltf_hidlib->size_y : 1);
	stdarg(XmNmaximum, ltf_hidlib->size_y ? ltf_hidlib->size_y : 1);
	XtSetValues(vscroll, stdarg_args, stdarg_n);
	zoom_max();

	LesstifNetlistChanged(ltf_hidlib, NULL, 0, NULL);
	lesstif_update_layer_groups();
	return;
}

static Widget m_cmd = 0, m_cmd_label;
static int cmd_is_active = 0;

static void command_callback(Widget w, XtPointer uptr, XmTextVerifyCallbackStruct * cbs)
{
	char *s;
	switch (cbs->reason) {
	case XmCR_ACTIVATE:
		s = XmTextGetString(w);
		lesstif_show_crosshair(0);
		pcb_clihist_append(s, NULL, NULL, NULL);
		pcb_parse_command(ltf_hidlib, s, pcb_false);
		XtFree(s);
		XmTextSetString(w, XmStrCast(""));

		XtUnmanageChild(m_cmd);
		XtUnmanageChild(m_cmd_label);
		cmd_is_active = 0;
		break;
	}
}

static int panning = 0;
static int shift_pressed;
static int ctrl_pressed;
static int alt_pressed;

static void ltf_mod_key(XKeyEvent *e, int set, int mainloop)
{
	switch (XKeycodeToKeysym(display, e->keycode, 0)) {
	case XK_Shift_L:
	case XK_Shift_R:
		shift_pressed = set;
		break;
	case XK_Control_L:
	case XK_Control_R:
		ctrl_pressed = set;
		break;
#ifdef __APPLE__
	case XK_Mode_switch:
#else
	case XK_Alt_L:
	case XK_Alt_R:
#endif
		alt_pressed = set;
		break;
	default:
		/* to include the Apple keyboard left and right command keys use XK_Meta_L and XK_Meta_R respectivly. */
		if (mainloop)
			return;
	}

	if (!mainloop)
		return;

	in_move_event = 1;
	pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_false);
	if (panning)
		Pan(2, e->x, e->y);
	pcb_hidcore_crosshair_move_to(ltf_hidlib, Px(e->x), Py(e->y), 1);
	pcb_hidlib_adjust_attached_objects(ltf_hidlib);
	pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_true);
	in_move_event = 0;
}

static void command_event_handler(Widget w, XtPointer p, XEvent * e, Boolean * cont)
{
	const char *hist;
	char buf[10];
	KeySym sym;

	switch (e->type) {
		case KeyRelease:
			if (cmd_is_active)
				pcb_cli_edit(ltf_hidlib);
			break;
		case KeyPress:

			/* update mod keys */
			switch (e->type) {
				case KeyPress:   ltf_mod_key((XKeyEvent *)e, 1, 0);
				case KeyRelease: ltf_mod_key((XKeyEvent *)e, 0, 0);
			}

			XLookupString((XKeyEvent *) e, buf, sizeof(buf), &sym, NULL);
			switch (sym) {
				case XK_Up:
					hist = pcb_clihist_prev();
					if (hist != NULL)
						XmTextSetString(w, XmStrCast(hist));
					else
						XmTextSetString(w, XmStrCast(""));
					break;
				case XK_Down:
					hist = pcb_clihist_next();
					if (hist != NULL)
						XmTextSetString(w, XmStrCast(hist));
					else
						XmTextSetString(w, XmStrCast(""));
					break;
				case XK_Tab:
					pcb_cli_tab(ltf_hidlib);
					*cont = False;
					break;
				case XK_Escape:
					XtUnmanageChild(m_cmd);
					XtUnmanageChild(m_cmd_label);
					XmTextSetString(w, XmStrCast(""));
					cmd_is_active = 0;
					*cont = False;
					break;
			}
			break;
		}
}


static const char *lesstif_command_entry(pcb_hid_t *hid, const char *ovr, int *cursor)
{
	if (!cmd_is_active) {
		if (cursor != NULL)
			*cursor = -1;
		return NULL;
	}

	if (ovr != NULL) {
		XmTextSetString(m_cmd, XmStrCast(ovr));
		if (cursor != NULL)
			XtVaSetValues(m_cmd, XmNcursorPosition, *cursor, NULL);
	}

	if (cursor != NULL) {
		XmTextPosition pos;
		stdarg_n = 0;
		stdarg(XmNcursorPosition, &pos);
		XtGetValues(m_cmd, stdarg_args, stdarg_n);
		*cursor = pos;
	}

	return XmTextGetString(m_cmd);
}

static double ltf_benchmark(pcb_hid_t *hid)
{
	int i = 0;
	time_t start, end;
	pcb_hid_expose_ctx_t ctx;
	Drawable save_main;


	save_main = main_pixmap;
	main_pixmap = window;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = ltf_hidlib->size_x;
	ctx.view.Y2 = ltf_hidlib->size_y;

	pixmap = window;
	XSync(display, 0);
	time(&start);
	do {
		XFillRectangle(display, pixmap, bg_gc, 0, 0, view_width, view_height);
		pcbhl_expose_main(&lesstif_hid, &ctx, NULL);
		XSync(display, 0);
		time(&end);
		i++;
	}
	while (end - start < 10);

	main_pixmap = save_main;
	return i/10.0;
}

/* ----------------------------------------------------------------------
 * redraws the background image
 */

typedef struct {
	const pcb_pixmap_t *pxm;

	/* cache */
	int w_scaled, h_scaled;
	XImage *img_scaled;
	Pixmap pm_scaled;
	Pixmap mask_scaled;
	char *img_data;

#ifdef HAVE_XRENDER
	Picture p_img_scaled;
	Picture p_mask_scaled;
#endif
	GC mask_gc, img_gc;
	unsigned inited:1;
} pcb_ltf_pixmap_t;

static void pcb_ltf_draw_pixmap_(pcb_hidlib_t *hidlib, pcb_ltf_pixmap_t *lpm, pcb_coord_t ox, pcb_coord_t oy, pcb_coord_t dw, pcb_coord_t dh)
{
	int w, h, sx3, done = 0;

	w = dw / view_zoom;
	h = dh / view_zoom;

	if ((w != lpm->w_scaled) || (h != lpm->h_scaled)) {
		int x, y, nret;
		double xscale, yscale;
		static int vis_inited = 0;
		static XVisualInfo vinfot, *vinfo;
		static Visual *vis;
		static enum { CLRSPC_MISC, CLRSPC_RGB565, CLRSPC_RGB888 } color_space;


		if (!vis_inited) {
			vis = DefaultVisual(display, DefaultScreen(display));
			vinfot.visualid = XVisualIDFromVisual(vis);
			vinfo = XGetVisualInfo(display, VisualIDMask, &vinfot, &nret);
			vis_inited = 1;
			color_space = CLRSPC_MISC;

			if ((vinfo->class == TrueColor) && (vinfo->depth == 16) && (vinfo->red_mask == 0xf800) && (vinfo->green_mask == 0x07e0) && (vinfo->blue_mask == 0x001f))
				color_space = CLRSPC_RGB565;

			if ((vinfo->class == TrueColor) && (vinfo->depth == 24) && (vinfo->red_mask == 0xff0000) && (vinfo->green_mask == 0x00ff00) && (vinfo->blue_mask == 0x0000ff))
				color_space = CLRSPC_RGB888;
		}

		if (!lpm->inited) {
			lpm->img_gc = XCreateGC(display, window, 0, 0);
			if (lpm->pxm->has_transp)
				lpm->mask_gc = XCreateGC(display, window, 0, 0);
			lpm->inited = 1;
		}

		if (lpm->img_scaled != NULL)
			XDestroyImage(lpm->img_scaled);
		if (lpm->mask_scaled != 0)
			XFreePixmap(display, lpm->mask_scaled);
		if (lpm->pm_scaled != 0)
			XFreePixmap(display, lpm->pm_scaled);

		lpm->img_data = malloc(w*h*4);
		lpm->img_scaled = XCreateImage(display, vis, vinfo->depth, ZPixmap, 0, lpm->img_data, w, h, 32, 0);
		lpm->mask_scaled = XCreatePixmap(display, window, w, h, 1);
		lpm->pm_scaled = XCreatePixmap(display, window, w, h, 24);
		lpm->w_scaled = w;
		lpm->h_scaled = h;

		xscale = (double)lpm->pxm->sx / w;
		yscale = (double)lpm->pxm->sy / h;

		sx3 = lpm->pxm->sx * 3;
		for (y = 0; y < h; y++) {
			XColor pix;
			unsigned char *row;

			int ir = y * yscale;
			row = lpm->pxm->p + ir * sx3;

			pix.flags = DoRed | DoGreen | DoBlue;

			for (x = 0; x < w; x++) {
				unsigned long pp;
				int tr = 0, ic = x * xscale;
				unsigned int r, g, b;

				if ((ir < 0) || (ir >= lpm->pxm->sy) || (ic < 0) || (ic >= lpm->pxm->sx))
					tr = 1;
				else {
					ic = ic * 3;
					r = row[ic]; g = row[ic+1]; b = row[ic+2];
					if (lpm->pxm->has_transp && (r == lpm->pxm->tr) && (g == lpm->pxm->tg) && (b == lpm->pxm->tb))
						tr = 1;
					else {
						switch (color_space) {
							case CLRSPC_MISC:
								pix.red = r << 8;
								pix.green = g << 8;
								pix.blue = b << 8;
								XAllocColor(display, lesstif_colormap, &pix);
								pp = pix.pixel;
								break;
							case CLRSPC_RGB565:
								pp = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);
								break;
							case CLRSPC_RGB888:
								pp = (r << 16) | (g << 8) | (b);
								break;
						}
					}

					if (!tr) {
						XDrawPoint(display, lpm->mask_scaled, bset_gc, x, y);
						XPutPixel(lpm->img_scaled, x, y, pp);
					}
					else
						XDrawPoint(display, lpm->mask_scaled, bclear_gc, x, y);
				}
			}

			if (lpm->pxm->has_transp) {
				lpm->pm_scaled = XCreatePixmap(display, window, w, h, 24);
				XPutImage(display, lpm->pm_scaled, bg_gc, lpm->img_scaled, 0, 0, 0, 0, w, h);
			}
			else
				lpm->pm_scaled = 0;
		}

#ifdef HAVE_XRENDER
		if (use_xrender) {
			if (lpm->p_img_scaled != 0)
				XRenderFreePicture(display, lpm->p_img_scaled);
			if (lpm->p_mask_scaled != 0)
				XRenderFreePicture(display, lpm->p_mask_scaled);

			lpm->p_img_scaled = XRenderCreatePicture(display, lpm->img_scaled, XRenderFindVisualFormat(display, DefaultVisual(display, screen)), 0, 0);
			if (lpm->pxm->has_transp)
				lpm->p_mask_scaled = XRenderCreatePicture(display, lpm->mask_scaled, XRenderFindVisualFormat(display, DefaultVisual(display, screen)), 0, 0);
			else
				lpm->p_mask_scaled = 0;
		}
#endif
	}

#ifdef HAVE_XRENDER
	if (use_xrender) {
		fprintf(stderr, "clip xrender\n");
		XRenderPictureAttributes pa;
		pa.clip_mask = mask_bitmap;
		XRenderChangePicture(display, lpm->p_img_scaled, CPClipMask, &pa);
		XRenderComposite(display, PictOpOver, lpm->p_img_scaled, lpm->p_mask_scaled, main_picture, 0, 0, 0, 0, Vx(ox), Vy(ox), w, h);
		done = 1;
	}
#endif

	if (!done) {
		if (lpm->pxm->has_transp) {
			XSetClipMask(display, pxm_clip_gc, lpm->mask_scaled);
			XSetClipOrigin(display, pxm_clip_gc, Vx(ox), Vy(oy));
			XCopyArea(display, lpm->pm_scaled, main_pixmap, pxm_clip_gc, 0, 0, w, h, Vx(ox), Vy(oy));
		}
		else
			XPutImage(display, main_pixmap, bg_gc, lpm->img_scaled, 0, 0, Vx(ox), Vy(oy), w, h);
	}
}

static void pcb_ltf_draw_pixmap(pcb_hid_t *hid, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t sx, pcb_coord_t sy, pcb_pixmap_t *pixmap)
{
	if (pixmap->hid_data == NULL) {
		pcb_ltf_pixmap_t *lpm = calloc(sizeof(pcb_ltf_pixmap_t), 1);
		lpm->pxm = pixmap;
		pixmap->hid_data = lpm;
	}
	if (pixmap->hid_data != NULL)
		pcb_ltf_draw_pixmap_(ltf_hidlib, pixmap->hid_data, cx - sx/2, cy - sy/2, sx, sy);
}


static pcb_pixmap_t ltf_bg_img;
static void DrawBackgroundImage()
{
	static pcb_ltf_pixmap_t lpm;

	if (!window || (ltf_bg_img.p == NULL))
		return;

	if (lpm.pxm == NULL)
		lpm.pxm = &ltf_bg_img;
	pcb_ltf_draw_pixmap_(ltf_hidlib, &lpm, 0, 0, ltf_hidlib->size_x, ltf_hidlib->size_y);
}

static void LoadBackgroundImage(const char *fn)
{
	if (pcb_pixmap_load(ltf_hidlib, &ltf_bg_img, fn) != 0)
		pcb_message(PCB_MSG_ERROR, "Failed to load pixmap %s for background image\n", fn);
}

/* ---------------------------------------------------------------------- */

static pcb_export_opt_t *lesstif_get_export_options(pcb_hid_t *hid, int *n)
{
	if (n != NULL)
		*n = sizeof(lesstif_attribute_list) / sizeof(pcb_export_opt_t);
	return lesstif_attribute_list;
}

static void set_scroll(Widget s, int pos, int view, int pcb)
{
	unsigned int sz = view * view_zoom;
	if (sz > pcb)
		sz = pcb;
	if (pos > pcb - sz)
		pos = pcb - sz;
	if (pos < 0)
		pos = 0;
	stdarg_n = 0;
	stdarg(XmNvalue, pos);
	stdarg(XmNsliderSize, sz);
	stdarg(XmNincrement, view_zoom);
	stdarg(XmNpageIncrement, sz);
	stdarg(XmNmaximum, pcb);
	XtSetValues(s, stdarg_args, stdarg_n);
}

void lesstif_pan_fixup()
{
	if (ltf_hidlib == NULL)
		return;
	if (view_left_x > ltf_hidlib->size_x + (view_width * view_zoom))
		view_left_x = ltf_hidlib->size_x + (view_width * view_zoom);
	if (view_top_y > ltf_hidlib->size_y + (view_height * view_zoom))
		view_top_y = ltf_hidlib->size_y + (view_height * view_zoom);
	if (view_left_x < -(view_width * view_zoom))
		view_left_x = -(view_width * view_zoom);
	if (view_top_y < -(view_height * view_zoom))
		view_top_y = -(view_height * view_zoom);

	set_scroll(hscroll, view_left_x, view_width, ltf_hidlib->size_x);
	set_scroll(vscroll, view_top_y, view_height, ltf_hidlib->size_y);

	lesstif_invalidate_all(pcb_gui);
}

static void zoom_max()
{
	double new_zoom = ltf_hidlib->size_x / view_width;
	if (new_zoom < ltf_hidlib->size_y / view_height)
		new_zoom = ltf_hidlib->size_y / view_height;

	view_left_x = -(view_width * new_zoom - ltf_hidlib->size_x) / 2;
	view_top_y = -(view_height * new_zoom - ltf_hidlib->size_y) / 2;
	view_zoom = new_zoom;
	pcb_pixel_slop = view_zoom;
	lesstif_pan_fixup();
}


static void zoom_to(double new_zoom, pcb_coord_t x, pcb_coord_t y)
{
	double max_zoom, xfrac, yfrac;
	pcb_coord_t cx, cy;

	if (PCB == NULL)
		return;

	xfrac = (double) x / (double) view_width;
	yfrac = (double) y / (double) view_height;

	if (pcbhl_conf.editor.view.flip_x)
		xfrac = 1 - xfrac;
	if (pcbhl_conf.editor.view.flip_y)
		yfrac = 1 - yfrac;

	max_zoom = ltf_hidlib->size_x / view_width;
	if (max_zoom < ltf_hidlib->size_y / view_height)
		max_zoom = ltf_hidlib->size_y / view_height;

	max_zoom *= MAX_ZOOM_SCALE;

	if (new_zoom < 1)
		new_zoom = 1;
	if (new_zoom > max_zoom)
		new_zoom = max_zoom;

	cx = view_left_x + view_width * xfrac * view_zoom;
	cy = view_top_y + view_height * yfrac * view_zoom;

	if (view_zoom != new_zoom) {
		view_zoom = new_zoom;
		pcb_pixel_slop = view_zoom;

		view_left_x = cx - view_width * xfrac * view_zoom;
		view_top_y = cy - view_height * yfrac * view_zoom;
	}
	lesstif_pan_fixup();
}

static void zoom_win(pcb_hid_t *hid, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, int setch)
{
	pcb_coord_t w = x2 - x1, h = y2 - y1;
	double new_zoom = w / view_width;
	if (new_zoom < h / view_height)
		new_zoom = h / view_height;

	if (new_zoom < 1)
		new_zoom = 1;

	if (view_zoom != new_zoom) {
		view_zoom = new_zoom;
		pcb_pixel_slop = view_zoom;
	}

	view_left_x = x1;
	view_top_y = y1;

	lesstif_pan_fixup();
	if (setch)
		pcb_hidcore_crosshair_move_to(ltf_hidlib, (x1+x2)/2, (y1+y2)/2, 0);
}

void zoom_by(double factor, pcb_coord_t x, pcb_coord_t y)
{
	zoom_to(view_zoom * factor, x, y);
}

/* X and Y are in screen coordinates.  */
static void Pan(int mode, pcb_coord_t x, pcb_coord_t y)
{
	static pcb_coord_t ox, oy;
	static pcb_coord_t opx, opy;

	panning = mode;
	/* This is for ctrl-pan, where the viewport's position is directly
	   proportional to the cursor position in the window (like the Xaw
	   thumb panner) */
TODO("remove this if there is no bugreport for a long time");
#if 0
	if (pan_thumb_mode) {
		opx = x * ltf_hidlib->size_x / view_width;
		opy = y * ltf_hidlib->size_y / view_height;
		if (pcbhl_conf.editor.view.flip_x)
			opx = ltf_hidlib->size_x - opx;
		if (pcbhl_conf.editor.view.flip_y)
			opy = ltf_hidlib->size_y - opy;
		view_left_x = opx - view_width / 2 * view_zoom;
		view_top_y = opy - view_height / 2 * view_zoom;
		lesstif_pan_fixup();
	}
	else
#endif

	/* This is the start of a regular pan.  On the first click, we
	   remember the coordinates where we "grabbed" the screen.  */
	if (mode == 1) {
		ox = x;
		oy = y;
		opx = view_left_x;
		opy = view_top_y;
	}
	/* continued drag, we calculate how far we've moved the cursor and
	   set the position accordingly.  */
	else {
		if (pcbhl_conf.editor.view.flip_x)
			view_left_x = opx + (x - ox) * view_zoom;
		else
			view_left_x = opx - (x - ox) * view_zoom;
		if (pcbhl_conf.editor.view.flip_y)
			view_top_y = opy + (y - oy) * view_zoom;
		else
			view_top_y = opy - (y - oy) * view_zoom;
		lesstif_pan_fixup();
	}
}


static void mod_changed(XKeyEvent *e, int set)
{
	ltf_mod_key(e, set, 1);
}

static pcb_hid_cfg_mod_t lesstif_mb2cfg(int but)
{
	switch(but) {
		case 1: return PCB_MB_LEFT;
		case 2: return PCB_MB_MIDDLE;
		case 3: return PCB_MB_RIGHT;
		case 4: return PCB_MB_SCROLL_UP;
		case 5: return PCB_MB_SCROLL_DOWN;
	}
	return 0;
}

static void work_area_input(Widget w, XtPointer v, XEvent * e, Boolean * ctd)
{
	static int pressed_button = 0;

	show_crosshair(0);
	switch (e->type) {
	case KeyPress:
		mod_changed(&(e->xkey), 1);
		if (lesstif_key_event(&(e->xkey)))
			return;
		break;

	case KeyRelease:
		mod_changed(&(e->xkey), 0);
		break;

	case ButtonPress:
		{
			int mods;
			if (pressed_button)
				return;
			/*printf("click %d\n", e->xbutton.button); */
			if (lesstif_button_event(w, e))
				return;

			pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_false);
			pressed_button = e->xbutton.button;
			mods = ((e->xbutton.state & ShiftMask) ? PCB_M_Shift : 0)
				+ ((e->xbutton.state & ControlMask) ? PCB_M_Ctrl : 0)
#ifdef __APPLE__
				+ ((e->xbutton.state & (1 << 13)) ? PCB_M_Alt : 0);
#else
				+ ((e->xbutton.state & Mod1Mask) ? PCB_M_Alt : 0);
#endif
			hid_cfg_mouse_action(ltf_hidlib, &lesstif_mouse, lesstif_mb2cfg(e->xbutton.button) | mods, cmd_is_active);

			pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_true);
			break;
		}

	case ButtonRelease:
		{
			int mods;
			if (e->xbutton.button != pressed_button)
				return;
			lesstif_button_event(w, e);
			pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_false);
			pressed_button = 0;
			mods = ((e->xbutton.state & ShiftMask) ? PCB_M_Shift : 0)
				+ ((e->xbutton.state & ControlMask) ? PCB_M_Ctrl : 0)
#ifdef __APPLE__
				+ ((e->xbutton.state & (1 << 13)) ? PCB_M_Alt : 0)
#else
				+ ((e->xbutton.state & Mod1Mask) ? PCB_M_Alt : 0)
#endif
				+ PCB_M_Release;
			hid_cfg_mouse_action(ltf_hidlib, &lesstif_mouse, lesstif_mb2cfg(e->xbutton.button) | mods, cmd_is_active);
			pcb_hid_notify_crosshair_change(ltf_hidlib, pcb_true);
			break;
		}

	case MotionNotify:
		{
			Window root, child;
			unsigned int keys_buttons;
			int root_x, root_y, pos_x, pos_y;
			while (XCheckMaskEvent(display, PointerMotionMask, e));
			XQueryPointer(display, e->xmotion.window, &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);
			shift_pressed = (keys_buttons & ShiftMask);
			ctrl_pressed = (keys_buttons & ControlMask);
#ifdef __APPLE__
			alt_pressed = (keys_buttons & (1 << 13));
#else
			alt_pressed = (keys_buttons & Mod1Mask);
#endif
			/*pcb_printf("m %#mS %#mS\n", Px(e->xmotion.x), Py(e->xmotion.y)); */
			crosshair_in_window = 1;
			in_move_event = 1;
			if (panning)
				Pan(2, pos_x, pos_y);
			pcb_hidcore_crosshair_move_to(ltf_hidlib, Px(pos_x), Py(pos_y), 1);
			in_move_event = 0;
		}
		break;

	case LeaveNotify:
		crosshair_in_window = 0;
		if (crosshair_on)
			pcbhl_draw_attached(ltf_hidlib, 1);
		pcbhl_draw_marks(ltf_hidlib, 1);
		ShowCrosshair(pcb_false);
		need_idle_proc();
		break;

	case EnterNotify:
		crosshair_in_window = 1;
		in_move_event = 1;
		pcb_hidcore_crosshair_move_to(ltf_hidlib, Px(e->xcrossing.x), Py(e->xcrossing.y), 1);
		ShowCrosshair(pcb_true);
		in_move_event = 0;
		need_redraw = 1;
		need_idle_proc();
		break;

	default:
		printf("work_area: unknown event %d\n", e->type);
		break;
	}

	if (cmd_is_active)
		XmProcessTraversal(m_cmd, XmTRAVERSE_CURRENT);
}

static void draw_right_cross(GC xor_gc, int x, int y, int view_width, int view_height)
{
	XDrawLine(display, window, xor_gc, 0, y, view_width, y);
	XDrawLine(display, window, xor_gc, x, 0, x, view_height);
}

static void draw_slanted_cross(GC xor_gc, int x, int y, int view_width, int view_height)
{
	int x0, y0, x1, y1;

	x0 = x + (view_height - y);
	x0 = MAX(0, MIN(x0, view_width));
	x1 = x - y;
	x1 = MAX(0, MIN(x1, view_width));
	y0 = y + (view_width - x);
	y0 = MAX(0, MIN(y0, view_height));
	y1 = y - x;
	y1 = MAX(0, MIN(y1, view_height));
	XDrawLine(display, window, xor_gc, x0, y0, x1, y1);
	x0 = x - (view_height - y);
	x0 = MAX(0, MIN(x0, view_width));
	x1 = x + y;
	x1 = MAX(0, MIN(x1, view_width));
	y0 = y + x;
	y0 = MAX(0, MIN(y0, view_height));
	y1 = y - (view_width - x);
	y1 = MAX(0, MIN(y1, view_height));
	XDrawLine(display, window, xor_gc, x0, y0, x1, y1);
}

static void draw_dozen_cross(GC xor_gc, int x, int y, int view_width, int view_height)
{
	int x0, y0, x1, y1;
	double tan60 = sqrt(3);

	x0 = x + (view_height - y) / tan60;
	x0 = MAX(0, MIN(x0, view_width));
	x1 = x - y / tan60;
	x1 = MAX(0, MIN(x1, view_width));
	y0 = y + (view_width - x) * tan60;
	y0 = MAX(0, MIN(y0, view_height));
	y1 = y - x * tan60;
	y1 = MAX(0, MIN(y1, view_height));
	XDrawLine(display, window, xor_gc, x0, y0, x1, y1);

	x0 = x + (view_height - y) * tan60;
	x0 = MAX(0, MIN(x0, view_width));
	x1 = x - y * tan60;
	x1 = MAX(0, MIN(x1, view_width));
	y0 = y + (view_width - x) / tan60;
	y0 = MAX(0, MIN(y0, view_height));
	y1 = y - x / tan60;
	y1 = MAX(0, MIN(y1, view_height));
	XDrawLine(display, window, xor_gc, x0, y0, x1, y1);

	x0 = x - (view_height - y) / tan60;
	x0 = MAX(0, MIN(x0, view_width));
	x1 = x + y / tan60;
	x1 = MAX(0, MIN(x1, view_width));
	y0 = y + x * tan60;
	y0 = MAX(0, MIN(y0, view_height));
	y1 = y - (view_width - x) * tan60;
	y1 = MAX(0, MIN(y1, view_height));
	XDrawLine(display, window, xor_gc, x0, y0, x1, y1);

	x0 = x - (view_height - y) * tan60;
	x0 = MAX(0, MIN(x0, view_width));
	x1 = x + y * tan60;
	x1 = MAX(0, MIN(x1, view_width));
	y0 = y + x / tan60;
	y0 = MAX(0, MIN(y0, view_height));
	y1 = y - (view_width - x) / tan60;
	y1 = MAX(0, MIN(y1, view_height));
	XDrawLine(display, window, xor_gc, x0, y0, x1, y1);
}

static void draw_crosshair(GC xor_gc, int x, int y, int view_width, int view_height)
{
	draw_right_cross(xor_gc, x, y, view_width, view_height);
	if (pcbhl_conf.editor.crosshair_shape_idx == pcb_ch_shape_union_jack)
		draw_slanted_cross(xor_gc, x, y, view_width, view_height);
	if (pcbhl_conf.editor.crosshair_shape_idx == pcb_ch_shape_dozen)
		draw_dozen_cross(xor_gc, x, y, view_width, view_height);
}

void lesstif_show_crosshair(int show)
{
	static int showing = 0;
	static int sx, sy;
	static GC xor_gc = 0;
	Pixel crosshair_color;
	static unsigned long cross_color_packed;

	if (!crosshair_in_window || !window)
		return;
	if ((xor_gc == 0) || (cross_color_packed != pcbhl_conf.appearance.color.cross.packed)) {
		crosshair_color = lesstif_parse_color(&pcbhl_conf.appearance.color.cross);
		xor_gc = XCreateGC(display, window, 0, 0);
		XSetFunction(display, xor_gc, GXxor);
		XSetForeground(display, xor_gc, crosshair_color);
		cross_color_packed = pcbhl_conf.appearance.color.cross.packed;
	}
	if (show == showing)
		return;
	if (show) {
		sx = Vx(crosshair_x);
		sy = Vy(crosshair_y);
	}
	else
		need_idle_proc();
	draw_crosshair(xor_gc, sx, sy, view_width, view_height);
	showing = show;
}

static void work_area_expose(Widget work_area, void *me, XmDrawingAreaCallbackStruct * cbs)
{
	XExposeEvent *e;

	show_crosshair(0);
	e = &(cbs->event->xexpose);
	XSetFunction(display, my_gc, GXcopy);
	XCopyArea(display, main_pixmap, window, my_gc, e->x, e->y, e->width, e->height, e->x, e->y);
	show_crosshair(1);
}

static void scroll_callback(Widget scroll, int *view_dim, XmScrollBarCallbackStruct * cbs)
{
	*view_dim = cbs->value;
	lesstif_invalidate_all(pcb_gui);
}

static void work_area_make_pixmaps(Dimension width, Dimension height)
{
	if (main_pixmap)
		XFreePixmap(display, main_pixmap);
	main_pixmap = XCreatePixmap(display, window, width, height, XDefaultDepth(display, screen));

	if (mask_pixmap)
		XFreePixmap(display, mask_pixmap);
	mask_pixmap = XCreatePixmap(display, window, width, height, XDefaultDepth(display, screen));
#ifdef HAVE_XRENDER
	if (main_picture) {
		XRenderFreePicture(display, main_picture);
		main_picture = 0;
	}
	if (mask_picture) {
		XRenderFreePicture(display, mask_picture);
		mask_picture = 0;
	}
	if (use_xrender) {
		main_picture = XRenderCreatePicture(display, main_pixmap, XRenderFindVisualFormat(display, DefaultVisual(display, screen)), 0, 0);
		mask_picture = XRenderCreatePicture(display, mask_pixmap, XRenderFindVisualFormat(display, DefaultVisual(display, screen)), 0, 0);
		if (!main_picture || !mask_picture)
			use_xrender = 0;
	}
#endif /* HAVE_XRENDER */

	if (mask_bitmap)
		XFreePixmap(display, mask_bitmap);
	mask_bitmap = XCreatePixmap(display, window, width, height, 1);

	pixmap = use_mask() ? main_pixmap : mask_pixmap;
	pixmap_w = width;
	pixmap_h = height;
}

static void work_area_resize(Widget work_area, void *me, XmDrawingAreaCallbackStruct * cbs)
{
	XColor color;
	Dimension width, height;

	show_crosshair(0);

	stdarg_n = 0;
	stdarg(XtNwidth, &width);
	stdarg(XtNheight, &height);
	stdarg(XmNbackground, &bgcolor);
	XtGetValues(work_area, stdarg_args, stdarg_n);
	view_width = width;
	view_height = height;

	color.pixel = bgcolor;
	XQueryColor(display, lesstif_colormap, &color);
	bgred = color.red;
	bggreen = color.green;
	bgblue = color.blue;

	if (!window)
		return;

	work_area_make_pixmaps(view_width, view_height);

	zoom_by(1, 0, 0);
}

static void work_area_first_expose(Widget work_area, void *me, XmDrawingAreaCallbackStruct * cbs)
{
	Dimension width, height;

	window = XtWindow(work_area);
	my_gc = XCreateGC(display, window, 0, 0);

	stdarg_n = 0;
	stdarg(XtNwidth, &width);
	stdarg(XtNheight, &height);
	stdarg(XmNbackground, &bgcolor);
	XtGetValues(work_area, stdarg_args, stdarg_n);
	view_width = width;
	view_height = height;

	offlimit_color = lesstif_parse_color(&pcbhl_conf.appearance.color.off_limit);
	grid_color = lesstif_parse_color(&pcbhl_conf.appearance.color.grid);

	bg_gc = XCreateGC(display, window, 0, 0);
	XSetForeground(display, bg_gc, bgcolor);

	work_area_make_pixmaps(width, height);

#ifdef HAVE_XRENDER
	if (use_xrender) {
		double l_alpha = pcbhl_conf.appearance.layer_alpha;
		XRenderPictureAttributes pa;
		XRenderColor a = { 0, 0, 0,  0x8000};
		
		if (l_alpha < 0)
			l_alpha = 0;
		else if (l_alpha > 1)
			l_alpha = 1;
		a.alpha = (int)(l_alpha * (double)0xFFFF);

		pale_pixmap = XCreatePixmap(display, window, 1, 1, 8);
		pa.repeat = True;
		pale_picture = XRenderCreatePicture(display, pale_pixmap, XRenderFindStandardFormat(display, PictStandardA8), CPRepeat, &pa);
		if (pale_picture)
			XRenderFillRectangle(display, PictOpSrc, pale_picture, &a, 0, 0, 1, 1);
		else
			use_xrender = 0;
	}
#endif /* HAVE_XRENDER */

	clip_gc = XCreateGC(display, window, 0, 0);
	pxm_clip_gc = XCreateGC(display, window, 0, 0);
	bset_gc = XCreateGC(display, mask_bitmap, 0, 0);
	XSetForeground(display, bset_gc, 1);
	bclear_gc = XCreateGC(display, mask_bitmap, 0, 0);
	XSetForeground(display, bclear_gc, 0);

	XtRemoveCallback(work_area, XmNexposeCallback, (XtCallbackProc) work_area_first_expose, 0);
	XtAddCallback(work_area, XmNexposeCallback, (XtCallbackProc) work_area_expose, 0);
	lesstif_invalidate_all(pcb_gui);
}

static unsigned short int lesstif_translate_key(const char *desc, int len)
{
	KeySym key;

	if (pcb_strcasecmp(desc, "enter") == 0) desc = "Return";

	key = XStringToKeysym(desc);
	if (key == NoSymbol && len > 1) {
		pcb_message(PCB_MSG_INFO, "lesstif_translate_key: no symbol for %s\n", desc);
		return 0;
	}
	return key;
}

int lesstif_key_name(unsigned short int key_raw, char *out, int out_len)
{
TODO("TODO#3: do not ingore key_tr (either of them is 0)")
	char *name = XKeysymToString(key_raw);
	if (name == NULL)
		return -1;
	strncpy(out, name, out_len);
	out[out_len-1] = '\0';
	return 0;
}

void lesstif_uninit_menu(void);
void lesstif_init_menu(void);

extern Widget lesstif_menubar;
static int lesstif_hid_inited = 0;

#include "mouse.c"

static void lesstif_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	Dimension width, height;
	Widget menu;
	Widget work_area_frame;

	/* this only registers in core, safe to call before anything else */
	lesstif_init_menu();

	lesstif_begin();

	pcb_hid_cfg_keys_init(&lesstif_keymap);
	lesstif_keymap.translate_key = lesstif_translate_key;
	lesstif_keymap.key_name = lesstif_key_name;
	lesstif_keymap.auto_chr = 1;
	lesstif_keymap.auto_tr = hid_cfg_key_default_trans;

	stdarg_n = 0;
	stdarg(XtNwidth, &width);
	stdarg(XtNheight, &height);
	XtGetValues(appwidget, stdarg_args, stdarg_n);

	if (width < 1)
		width = 640;
	if (width > XDisplayWidth(display, screen))
		width = XDisplayWidth(display, screen);
	if (height < 1)
		height = 480;
	if (height > XDisplayHeight(display, screen))
		height = XDisplayHeight(display, screen);

	stdarg_n = 0;
	stdarg(XmNwidth, width);
	stdarg(XmNheight, height);
	XtSetValues(appwidget, stdarg_args, stdarg_n);

	stdarg(XmNspacing, 0);
	mainwind = XmCreateMainWindow(appwidget, XmStrCast("mainWind"), stdarg_args, stdarg_n);
	XtManageChild(mainwind);

	stdarg_n = 0;
	stdarg(XmNmarginWidth, 0);
	stdarg(XmNmarginHeight, 0);
	menu = lesstif_menu(mainwind, "menubar", stdarg_args, stdarg_n);
	XtManageChild(menu);

	stdarg_n = 0;
	stdarg(XmNshadowType, XmSHADOW_IN);
	work_area_frame = XmCreateFrame(mainwind, XmStrCast("work_area_frame"), stdarg_args, stdarg_n);
	XtManageChild(work_area_frame);

	stdarg_n = 0;
	stdarg_do_color(&pcbhl_conf.appearance.color.background, XmNbackground);
	work_area = XmCreateDrawingArea(work_area_frame, XmStrCast("work_area"), stdarg_args, stdarg_n);
	XtManageChild(work_area);
	XtAddCallback(work_area, XmNexposeCallback, (XtCallbackProc) work_area_first_expose, 0);
	XtAddCallback(work_area, XmNresizeCallback, (XtCallbackProc) work_area_resize, 0);
	/* A regular callback won't work here, because lesstif swallows any
	   Ctrl<Button>1 event.  */
	XtAddEventHandler(work_area,
										ButtonPressMask | ButtonReleaseMask
										| PointerMotionMask | PointerMotionHintMask
										| KeyPressMask | KeyReleaseMask | EnterWindowMask | LeaveWindowMask, 0, work_area_input, 0);

	stdarg_n = 0;
	stdarg(XmNorientation, XmVERTICAL);
	stdarg(XmNprocessingDirection, XmMAX_ON_BOTTOM);
	stdarg(XmNmaximum, ltf_hidlib->size_y ? ltf_hidlib->size_y : 1);
	vscroll = XmCreateScrollBar(mainwind, XmStrCast("vscroll"), stdarg_args, stdarg_n);
	XtAddCallback(vscroll, XmNvalueChangedCallback, (XtCallbackProc) scroll_callback, (XtPointer) & view_top_y);
	XtAddCallback(vscroll, XmNdragCallback, (XtCallbackProc) scroll_callback, (XtPointer) & view_top_y);
	XtManageChild(vscroll);

	stdarg_n = 0;
	stdarg(XmNorientation, XmHORIZONTAL);
	stdarg(XmNmaximum, ltf_hidlib->size_x ? ltf_hidlib->size_x : 1);
	hscroll = XmCreateScrollBar(mainwind, XmStrCast("hscroll"), stdarg_args, stdarg_n);
	XtAddCallback(hscroll, XmNvalueChangedCallback, (XtCallbackProc) scroll_callback, (XtPointer) & view_left_x);
	XtAddCallback(hscroll, XmNdragCallback, (XtCallbackProc) scroll_callback, (XtPointer) & view_left_x);
	XtManageChild(hscroll);

	stdarg_n = 0;
	stdarg(XmNresize, True);
	stdarg(XmNresizePolicy, XmRESIZE_ANY);
	messages = XmCreateForm(mainwind, XmStrCast("messages"), stdarg_args, stdarg_n);
	XtManageChild(messages);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNalignment, XmALIGNMENT_CENTER);
	stdarg(XmNshadowThickness, 2);
	m_click = XmCreateLabel(messages, XmStrCast("click"), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNlabelString, XmStringCreatePCB(pcb_cli_prompt(":")));
	m_cmd_label = XmCreateLabel(messages, XmStrCast("command"), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_WIDGET);
	stdarg(XmNleftWidget, m_cmd_label);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNshadowThickness, 1);
	stdarg(XmNhighlightThickness, 0);
	stdarg(XmNmarginWidth, 2);
	stdarg(XmNmarginHeight, 2);
	m_cmd = XmCreateTextField(messages, XmStrCast("command"), stdarg_args, stdarg_n);
	XtAddCallback(m_cmd, XmNactivateCallback, (XtCallbackProc) command_callback, 0);
	XtAddCallback(m_cmd, XmNlosingFocusCallback, (XtCallbackProc) command_callback, 0);
	XtAddEventHandler(m_cmd, KeyPressMask | KeyReleaseMask, 0, command_event_handler, 0);

	/* status dock */
	{
		Widget w;

		stdarg_n = 0;
		stdarg(XmNtopAttachment, XmATTACH_FORM);
		stdarg(XmNbottomAttachment, XmATTACH_FORM);
		stdarg(XmNleftAttachment, XmATTACH_FORM);
		stdarg(XmNrightAttachment, XmATTACH_FORM);
		w = ltf_create_dockbox(messages, PCB_HID_DOCK_BOTTOM, 0);
		XtManageChild(w);
	}

	stdarg_n = 0;
	stdarg(XmNmessageWindow, messages);
	XtSetValues(mainwind, stdarg_args, stdarg_n);

	if ((background_image_file != NULL) && (*background_image_file != '\0'))
		LoadBackgroundImage(background_image_file);

	XtRealizeWidget(appwidget);
	pcb_ltf_winplace(display, XtWindow(appwidget), "top", 640, 480);
	XtAddEventHandler(appwidget, StructureNotifyMask, False, pcb_ltf_wplc_config_cb, "top");

	while (!window) {
		XEvent e;
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}

	pcb_board_changed(0);

	lesstif_menubar = menu;
	pcb_event(&PCB->hidlib, PCB_EVENT_GUI_INIT, NULL);


	lesstif_hid_inited = 1;

	XtAppMainLoop(app_context);

	pcb_hid_cfg_keys_uninit(&lesstif_keymap);
	lesstif_end();
}

static void lesstif_do_exit(pcb_hid_t *hid)
{
	XtAppSetExitFlag(app_context);
}

static void lesstif_uninit(pcb_hid_t *hid)
{
	if (lesstif_hid_inited) {
		lesstif_uninit_menu();
		ltf_mouse_uninit();
		lesstif_hid_inited = 0;
	}
}

static void lesstif_iterate(pcb_hid_t *hid)
{
	while (XtAppPending(app_context))
		XtAppProcessEvent(app_context, XtIMAll);
}

typedef union {
	int i;
	double f;
	char *s;
	pcb_coord_t c;
} val_union;

static Boolean
pcb_cvt_string_to_double(Display * d, XrmValue * args, Cardinal * num_args, XrmValue * from, XrmValue * to, XtPointer * data)
{
	static double rv;
	rv = strtod((char *) from->addr, 0);
	if (to->addr)
		*(double *) to->addr = rv;
	else
		to->addr = (XPointer) & rv;
	to->size = sizeof(rv);
	return True;
}

static Boolean
pcb_cvt_string_to_coord(Display * d, XrmValue * args, Cardinal * num_args, XrmValue * from, XrmValue * to, XtPointer * data)
{
	static pcb_coord_t rv;
	rv = pcb_get_value((char *) from->addr, NULL, NULL, NULL);
	if (to->addr)
		*(pcb_coord_t *) to->addr = rv;
	else
		to->addr = (XPointer) & rv;
	to->size = sizeof(rv);
	return TRUE;
}

static void mainwind_delete_cb()
{
	pcb_action(ltf_hidlib, "Quit");
}

static void lesstif_listener_cb(XtPointer client_data, int *fid, XtInputId * id)
{
	char buf[BUFSIZ];
	int nbytes;

	if ((nbytes = read(*fid, buf, BUFSIZ)) == -1)
		perror("lesstif_listener_cb");

	if (nbytes) {
		buf[nbytes] = '\0';
		pcb_parse_actions(ltf_hidlib, buf);
	}
}

static jmp_buf lesstif_err_jmp;
static void lesstif_err_msg(String name, String type, String class, String dflt, String *params, Cardinal *num_params)
{
	char *par[8];
	int n;
	for(n = 0; n < 8; n++) par[n] = "";
	for(n = 0; n < *num_params; n++) par[n] = params[n];
	fprintf(stderr, "Lesstif/motif initializaion error:\n");
	fprintf(stderr, dflt, par[0], par[1], par[2], par[3], par[4], par[5], par[6], par[7]);
	fprintf(stderr, "\n");
	longjmp(lesstif_err_jmp, 1);
}

static int lesstif_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	Atom close_atom;
	pcb_hid_attr_node_t *ha;
	int acount = 0, amax;
	int rcount = 0, rmax;
	int i, err;
	XrmOptionDescRec *new_options;
	XtResource *new_resources;
	val_union *new_values;
	int render_event, render_error;

	XtSetTypeConverter(XtRString, XtRDouble, pcb_cvt_string_to_double, NULL, 0, XtCacheAll, NULL);
	XtSetTypeConverter(XtRString, XtRPCBCoord, pcb_cvt_string_to_coord, NULL, 0, XtCacheAll, NULL);

	lesstif_reg_attrs();

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			pcb_export_opt_t *a = ha->opts + i;
			switch (a->type) {
			case PCB_HATT_INTEGER:
			case PCB_HATT_COORD:
			case PCB_HATT_REAL:
			case PCB_HATT_STRING:
			case PCB_HATT_BOOL:
				acount++;
				rcount++;
				break;
			default:
				break;
			}
		}

#if 0
	amax = acount + XtNumber(lesstif_options);
#else
	amax = acount;
#endif

	new_options = (XrmOptionDescRec *) malloc((amax + 1) * sizeof(XrmOptionDescRec));

#if 0
	memcpy(new_options + acount, lesstif_options, sizeof(lesstif_options));
#endif
	acount = 0;

	rmax = rcount;

	new_resources = (XtResource *) malloc((rmax + 1) * sizeof(XtResource));
	new_values = (val_union *) malloc((rmax + 1) * sizeof(val_union));
	rcount = 0;

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			pcb_export_opt_t *a = ha->opts + i;
			XrmOptionDescRec *o = new_options + acount;
			char *tmpopt, *tmpres;
			XtResource *r = new_resources + rcount;

			tmpopt = (char *) malloc(strlen(a->name) + 3);
			tmpopt[0] = tmpopt[1] = '-';
			strcpy(tmpopt + 2, a->name);
			o->option = tmpopt;

			tmpres = (char *) malloc(strlen(a->name) + 2);
			tmpres[0] = '*';
			strcpy(tmpres + 1, a->name);
			o->specifier = tmpres;

			switch (a->type) {
			case PCB_HATT_INTEGER:
			case PCB_HATT_COORD:
			case PCB_HATT_REAL:
			case PCB_HATT_STRING:
				o->argKind = XrmoptionSepArg;
				o->value = NULL;
				acount++;
				break;
			case PCB_HATT_BOOL:
				o->argKind = XrmoptionNoArg;
				o->value = XmStrCast("True");
				acount++;
				break;
			default:
				break;
			}

			r->resource_name = XmStrCast(a->name);
			r->resource_class = XmStrCast(a->name);
			r->resource_offset = sizeof(val_union) * rcount;

			switch (a->type) {
			case PCB_HATT_INTEGER:
				r->resource_type = XtRInt;
				r->default_type = XtRInt;
				r->resource_size = sizeof(int);
				r->default_addr = &(a->default_val.lng);
				rcount++;
				break;
			case PCB_HATT_COORD:
				r->resource_type = XmStrCast(XtRPCBCoord);
				r->default_type = XmStrCast(XtRPCBCoord);
				r->resource_size = sizeof(pcb_coord_t);
				r->default_addr = &(a->default_val.crd);
				rcount++;
				break;
			case PCB_HATT_REAL:
				r->resource_type = XmStrCast(XtRDouble);
				r->default_type = XmStrCast(XtRDouble);
				r->resource_size = sizeof(double);
				r->default_addr = &(a->default_val.dbl);
				rcount++;
				break;
			case PCB_HATT_STRING:
				r->resource_type = XtRString;
				r->default_type = XtRString;
				r->resource_size = sizeof(char *);
				r->default_addr = (char *) a->default_val.str;
				rcount++;
				break;
			case PCB_HATT_BOOL:
				r->resource_type = XtRBoolean;
				r->default_type = XtRInt;
				r->resource_size = sizeof(int);
				r->default_addr = &(a->default_val.lng);
				rcount++;
				break;
			default:
				break;
			}
		}

	stdarg_n = 0;
	stdarg(XmNdeleteResponse, XmDO_NOTHING);

	XtSetErrorMsgHandler(lesstif_err_msg);
	err = setjmp(lesstif_err_jmp);
	if (err != 0)
		return err;
	appwidget = XtAppInitialize(&app_context, pcbhl_app_package, new_options, amax, argc, *argv, 0, stdarg_args, stdarg_n);
	if (appwidget == NULL)
		return 1;
	XtSetErrorMsgHandler(NULL); /* restore the default handler */

	display = XtDisplay(appwidget);
	screen_s = XtScreen(appwidget);
	screen = XScreenNumberOfScreen(screen_s);
	lesstif_colormap = XDefaultColormap(display, screen);

	close_atom = XmInternAtom(display, XmStrCast("WM_DELETE_WINDOW"), 0);
	XmAddWMProtocolCallback(appwidget, close_atom, (XtCallbackProc) mainwind_delete_cb, 0);

	/*  XSynchronize(display, True); */

	XtGetApplicationResources(appwidget, new_values, new_resources, rmax, 0, 0);

#ifdef HAVE_XRENDER
	use_xrender = XRenderQueryExtension(display, &render_event, &render_error) &&
		XRenderFindVisualFormat(display, DefaultVisual(display, screen));
#ifdef HAVE_XINERAMA
	/* Xinerama and XRender don't get along well */
	if (XineramaQueryExtension(display, &render_event, &render_error)
			&& XineramaIsActive(display))
		use_xrender = 0;
#endif /* HAVE_XINERAMA */
#endif /* HAVE_XRENDER */

	rcount = 0;
	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			pcb_export_opt_t *a = ha->opts + i;
			val_union *v = new_values + rcount;
			switch (a->type) {
			case PCB_HATT_INTEGER:
				if (a->value)
					*(int *) a->value = v->i;
				else
					a->default_val.lng = v->i;
				rcount++;
				break;
			case PCB_HATT_COORD:
				if (a->value)
					*(pcb_coord_t *) a->value = v->c;
				else
					a->default_val.crd = v->c;
				rcount++;
				break;
			case PCB_HATT_BOOL:
				if (a->value)
					*(char *) a->value = v->i;
				else
					a->default_val.lng = v->i;
				rcount++;
				break;
			case PCB_HATT_REAL:
				if (a->value)
					*(double *) a->value = v->f;
				else
					a->default_val.dbl = v->f;
				rcount++;
				break;
			case PCB_HATT_STRING:
				if (a->value)
					*(char **) a->value = v->s;
				else
					a->default_val.str = v->s;
				rcount++;
				break;
			default:
				break;
			}
		}

	pcb_hid_parse_command_line(argc, argv);

	/* redefine lesstif_colormap, if requested via "-install" */
	if (use_private_colormap) {
		lesstif_colormap = XCopyColormapAndFree(display, lesstif_colormap);
		XtVaSetValues(appwidget, XtNcolormap, lesstif_colormap, NULL);
	}

	/* listen on standard input for actions */
	if (stdin_listen) {
		XtAppAddInput(app_context, pcb_fileno(stdin), (XtPointer) XtInputReadMask, lesstif_listener_cb, NULL);
	}
	return 0;
}

static void draw_grid()
{
	static XPoint *points = 0;
	static int npoints = 0;
	pcb_coord_t x1, y1, x2, y2, prevx;
	pcb_coord_t x, y;
	int n;
	static GC grid_gc = 0;

	if (!pcbhl_conf.editor.draw_grid)
		return;
	if (Vz(ltf_hidlib->grid) < PCB_MIN_GRID_DISTANCE)
		return;
	if (!grid_gc) {
		grid_gc = XCreateGC(display, window, 0, 0);
		XSetFunction(display, grid_gc, GXxor);
		XSetForeground(display, grid_gc, grid_color);
	}
	if (pcbhl_conf.editor.view.flip_x) {
		x2 = pcb_grid_fit(Px(0), ltf_hidlib->grid, ltf_hidlib->grid_ox);
		x1 = pcb_grid_fit(Px(view_width), ltf_hidlib->grid, ltf_hidlib->grid_ox);
		if (Vx(x2) < 0)
			x2 -= ltf_hidlib->grid;
		if (Vx(x1) >= view_width)
			x1 += ltf_hidlib->grid;
	}
	else {
		x1 = pcb_grid_fit(Px(0), ltf_hidlib->grid, ltf_hidlib->grid_ox);
		x2 = pcb_grid_fit(Px(view_width), ltf_hidlib->grid, ltf_hidlib->grid_ox);
		if (Vx(x1) < 0)
			x1 += ltf_hidlib->grid;
		if (Vx(x2) >= view_width)
			x2 -= ltf_hidlib->grid;
	}
	if (pcbhl_conf.editor.view.flip_y) {
		y2 = pcb_grid_fit(Py(0), ltf_hidlib->grid, ltf_hidlib->grid_oy);
		y1 = pcb_grid_fit(Py(view_height), ltf_hidlib->grid, ltf_hidlib->grid_oy);
		if (Vy(y2) < 0)
			y2 -= ltf_hidlib->grid;
		if (Vy(y1) >= view_height)
			y1 += ltf_hidlib->grid;
	}
	else {
		y1 = pcb_grid_fit(Py(0), ltf_hidlib->grid, ltf_hidlib->grid_oy);
		y2 = pcb_grid_fit(Py(view_height), ltf_hidlib->grid, ltf_hidlib->grid_oy);
		if (Vy(y1) < 0)
			y1 += ltf_hidlib->grid;
		if (Vy(y2) >= view_height)
			y2 -= ltf_hidlib->grid;
	}
	n = (x2 - x1) / ltf_hidlib->grid + 1;
	if (n > npoints) {
		npoints = n + 10;
		points = (XPoint *) realloc(points, npoints * sizeof(XPoint));
	}
	n = 0;
	prevx = 0;
	for (x = x1; x <= x2; x += ltf_hidlib->grid) {
		int temp = Vx(x);
		points[n].x = temp;
		if (n) {
			points[n].x -= prevx;
			points[n].y = 0;
		}
		prevx = temp;
		n++;
	}
	for (y = y1; y <= y2; y += ltf_hidlib->grid) {
		int vy = Vy(y);
		points[0].y = vy;
		XDrawPoints(display, pixmap, grid_gc, points, n, CoordModePrevious);
	}
}

static Boolean idle_proc(XtPointer dummy)
{
	if (need_redraw) {
		int mx, my;
		pcb_hid_expose_ctx_t ctx;

		pixmap = main_pixmap;
		mx = view_width;
		my = view_height;
		ctx.view.X1 = Px(0);
		ctx.view.Y1 = Py(0);
		ctx.view.X2 = Px(view_width);
		ctx.view.Y2 = Py(view_height);
		if (pcbhl_conf.editor.view.flip_x) {
			pcb_coord_t tmp = ctx.view.X1;
			ctx.view.X1 = ctx.view.X2;
			ctx.view.X2 = tmp;
		}
		if (pcbhl_conf.editor.view.flip_y) {
			pcb_coord_t tmp = ctx.view.Y1;
			ctx.view.Y1 = ctx.view.Y2;
			ctx.view.Y2 = tmp;
		}
		XSetForeground(display, bg_gc, bgcolor);
		XFillRectangle(display, main_pixmap, bg_gc, 0, 0, mx, my);

		if (ctx.view.X1 < 0 || ctx.view.Y1 < 0 || ctx.view.X2 > ltf_hidlib->size_x || ctx.view.Y2 > ltf_hidlib->size_y) {
			int leftmost, rightmost, topmost, bottommost;

			leftmost = Vx(0);
			rightmost = Vx(ltf_hidlib->size_x);
			topmost = Vy(0);
			bottommost = Vy(ltf_hidlib->size_y);
			if (leftmost > rightmost) {
				int t = leftmost;
				leftmost = rightmost;
				rightmost = t;
			}
			if (topmost > bottommost) {
				int t = topmost;
				topmost = bottommost;
				bottommost = t;
			}
			if (leftmost < 0)
				leftmost = 0;
			if (topmost < 0)
				topmost = 0;
			if (rightmost > view_width)
				rightmost = view_width;
			if (bottommost > view_height)
				bottommost = view_height;

			XSetForeground(display, bg_gc, offlimit_color);

			/* L T R
			   L x R
			   L B R */

			if (leftmost > 0) {
				XFillRectangle(display, main_pixmap, bg_gc, 0, 0, leftmost, view_height);
			}
			if (rightmost < view_width) {
				XFillRectangle(display, main_pixmap, bg_gc, rightmost + 1, 0, view_width - rightmost + 1, view_height);
			}
			if (topmost > 0) {
				XFillRectangle(display, main_pixmap, bg_gc, leftmost, 0, rightmost - leftmost + 1, topmost);
			}
			if (bottommost < view_height) {
				XFillRectangle(display, main_pixmap, bg_gc, leftmost, bottommost + 1,
											 rightmost - leftmost + 1, view_height - bottommost + 1);
			}
		}
		DrawBackgroundImage();
		pcbhl_expose_main(&lesstif_hid, &ctx, NULL);
		lesstif_drawing_mode = PCB_HID_COMP_POSITIVE;
		draw_grid();
		show_crosshair(0);					/* To keep the drawn / not drawn info correct */
		XSetFunction(display, my_gc, GXcopy);
		XCopyArea(display, main_pixmap, window, my_gc, 0, 0, view_width, view_height, 0, 0);
		pixmap = window;
		need_redraw = 0;
		if (crosshair_on) {
			pcbhl_draw_attached(ltf_hidlib, 1);
			pcbhl_draw_marks(ltf_hidlib, 1);
		}

		pcb_ltf_preview_invalidate(NULL);
	}

	{
		static int c_x = -2, c_y = -2;
		static pcb_mark_t saved_mark;
		static const pcb_unit_t *old_grid_unit = NULL;
		if (crosshair_x != c_x || crosshair_y != c_y || pcbhl_conf.editor.grid_unit != old_grid_unit || memcmp(&saved_mark, &pcb_marked, sizeof(pcb_mark_t))) {
			c_x = crosshair_x;
			c_y = crosshair_y;

			memcpy(&saved_mark, &pcb_marked, sizeof(pcb_mark_t));

			if (old_grid_unit != pcbhl_conf.editor.grid_unit)
				old_grid_unit = pcbhl_conf.editor.grid_unit;
		}
	}

	{
		static const char *old_clip = NULL;
		static int old_tscale = -1;
		const char *new_clip = cur_clip();

		if (new_clip != old_clip || conf_core.design.text_scale != old_tscale) {
			old_clip = new_clip;
			old_tscale = conf_core.design.text_scale;
		}
	}

TODO(": remove this, update-on should handle all cases")
	lesstif_update_widget_flags(NULL, NULL);

	show_crosshair(1);
	idle_proc_set = 0;
	return True;
}

void lesstif_need_idle_proc()
{
	if (idle_proc_set || window == 0)
		return;
	XtAppAddWorkProc(app_context, idle_proc, 0);
	idle_proc_set = 1;
}

static void lesstif_invalidate_lr(pcb_hid_t *hid, pcb_coord_t l, pcb_coord_t r, pcb_coord_t t, pcb_coord_t b)
{
	if (!window)
		return;

	need_redraw = 1;
	need_idle_proc();
}

void lesstif_invalidate_all(pcb_hid_t *hid)
{
	if (ltf_hidlib != NULL)
		lesstif_invalidate_lr(hid, 0, ltf_hidlib->size_x, 0, ltf_hidlib->size_y);
}

static void lesstif_notify_crosshair_change(pcb_hid_t *hid, pcb_bool changes_complete)
{
	static int invalidate_depth = 0;
	Pixmap save_pixmap;

	if (!my_gc)
		return;

	if (changes_complete)
		invalidate_depth--;

	if (invalidate_depth < 0) {
		invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		 * is not expected to occur, but we will try to handle it gracefully.
		 * As we know the crosshair will have been shown already, we must
		 * repaint the entire view to be sure not to leave an artaefact.
		 */
		need_redraw = 1;
		need_idle_proc();
		return;
	}

	if (invalidate_depth == 0 && crosshair_on) {
		save_pixmap = pixmap;
		pixmap = window;
		pcbhl_draw_attached(ltf_hidlib, 1);
		pixmap = save_pixmap;
	}

	if (!changes_complete)
		invalidate_depth++;
}

static void lesstif_notify_mark_change(pcb_hid_t *hid, pcb_bool changes_complete)
{
	static int invalidate_depth = 0;
	Pixmap save_pixmap;

	if (changes_complete)
		invalidate_depth--;

	if (invalidate_depth < 0) {
		invalidate_depth = 0;
		/* A mismatch of changes_complete == pcb_false and == pcb_true notifications
		 * is not expected to occur, but we will try to handle it gracefully.
		 * As we know the mark will have been shown already, we must
		 * repaint the entire view to be sure not to leave an artaefact.
		 */
		need_idle_proc();
		return;
	}

	if (invalidate_depth == 0 && crosshair_on) {
		save_pixmap = pixmap;
		pixmap = window;
		pcbhl_draw_marks(ltf_hidlib, 1);
		pixmap = save_pixmap;
	}

	if (!changes_complete)
		invalidate_depth++;
}

static int lesstif_set_layer_group(pcb_hid_t *hid, pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	/* accept anything and draw */
	return 1;
}

static pcb_hid_gc_t lesstif_make_gc(pcb_hid_t *hid)
{
	pcb_hid_gc_t rv = (hid_gc_s *) malloc(sizeof(hid_gc_s));
	memset(rv, 0, sizeof(hid_gc_s));
	rv->me_pointer = &lesstif_hid;
	return rv;
}

static void lesstif_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void lesstif_render_burst(pcb_hid_t *hid, pcb_burst_op_t op, const pcb_box_t *screen)
{
	pcb_gui->coord_per_pix = view_zoom;
}

static void lesstif_set_drawing_mode(pcb_hid_t *hid, pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *drw_screen)
{
	lesstif_drawing_mode = op;

	lesstif_direct = direct;
	if (direct) {
		pixmap = main_pixmap;
		return;
	}

	switch(op) {
		case PCB_HID_COMP_RESET:
			if (mask_pixmap == 0) {
				mask_pixmap = XCreatePixmap(display, window, pixmap_w, pixmap_h, XDefaultDepth(display, screen));
				mask_bitmap = XCreatePixmap(display, window, pixmap_w, pixmap_h, 1);
			}
			pixmap = mask_pixmap;
			XSetForeground(display, my_gc, 0);
			XSetFunction(display, my_gc, GXcopy);
			XFillRectangle(display, mask_pixmap, my_gc, 0, 0, view_width, view_height);
			XFillRectangle(display, mask_bitmap, bclear_gc, 0, 0, view_width, view_height);
			mask_gc = bset_gc;
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_POSITIVE_XOR:
			mask_gc = bset_gc;
			break;

		case PCB_HID_COMP_NEGATIVE:
			mask_gc = bclear_gc;
			break;

		case PCB_HID_COMP_FLUSH:
			pixmap = main_pixmap;

			/* blit back the result */
#ifdef HAVE_XRENDER
			if (use_xrender) {
				XRenderPictureAttributes pa;

				pa.clip_mask = mask_bitmap;
				XRenderChangePicture(display, main_picture, CPClipMask, &pa);
				XRenderComposite(display, PictOpOver, mask_picture, pale_picture,
					main_picture, 0, 0, 0, 0, 0, 0, view_width, view_height);
			}
			else
#endif /* HAVE_XRENDER */
			{
				XSetClipMask(display, clip_gc, mask_bitmap);
				XCopyArea(display, mask_pixmap, main_pixmap, clip_gc, 0, 0, view_width, view_height, 0, 0);
			}
			break;
	}
}


typedef struct {
	Pixel pix;
} ltf_color_cache_t;

static void lesstif_set_color(pcb_hid_gc_t gc, const pcb_color_t *pcolor)
{
	ltf_color_cache_t *cc;

	if (!display)
		return;
	if ((pcolor == NULL) || (*pcolor->str == '\0'))
		pcolor = pcb_color_magenta;

	gc->pcolor = *pcolor;

	if (!ltf_ccache_inited) {
		pcb_clrcache_init(&ltf_ccache, sizeof(ltf_color_cache_t), NULL);
		ltf_ccache_inited = 1;
	}

	if (pcb_color_is_drill(pcolor)) {
		gc->color = offlimit_color;
		gc->erase = 0;
	}
	else if ((cc = pcb_clrcache_get(&ltf_ccache, pcolor, 0)) != NULL) {
		gc->color = cc->pix;
		gc->erase = 0;
	}
	else {
		cc = pcb_clrcache_get(&ltf_ccache, pcolor, 1);
		cc->pix = lesstif_parse_color(pcolor);
#if 0
		printf("lesstif_set_color `%s' %08x rgb/%d/%d/%d\n", name, color.pixel, color.red, color.green, color.blue);
#endif
		gc->color = cc->pix;
		gc->erase = 0;
	}
	if (autofade) {
		static int lastcolor = -1, lastfade = -1;
		if (gc->color == lastcolor)
			gc->color = lastfade;
		else {
			XColor color;
			lastcolor = gc->color;
			color.pixel = gc->color;

			XQueryColor(display, lesstif_colormap, &color);
			color.red = (bgred + color.red) / 2;
			color.green = (bggreen + color.green) / 2;
			color.blue = (bgblue + color.blue) / 2;
			XAllocColor(display, lesstif_colormap, &color);
			lastfade = gc->color = color.pixel;
		}
	}
}

static void set_gc(pcb_hid_gc_t gc)
{
	int cap, join, width;
	if (gc->me_pointer != &lesstif_hid) {
		fprintf(stderr, "Fatal: GC from another HID passed to lesstif HID\n");
		abort();
	}
#if 0
	pcb_printf("set_gc c%s %08lx w%#mS c%d x%d e%d\n", gc->colorname, gc->color, gc->width, gc->cap, gc->xor_set, gc->erase);
#endif
	switch (gc->cap) {
	case pcb_cap_square:
		cap = CapProjecting;
		join = JoinMiter;
		break;
	case pcb_cap_round:
		cap = CapRound;
		join = JoinRound;
		break;
	default:
		assert(!"unhandled cap");
		cap = CapRound;
		join = JoinRound;
	}
	if (gc->xor_set) {
		XSetFunction(display, my_gc, GXxor);
		XSetForeground(display, my_gc, gc->color ^ bgcolor);
	}
	else if (gc->erase) {
		XSetFunction(display, my_gc, GXcopy);
		XSetForeground(display, my_gc, offlimit_color);
	}
	else {
		XSetFunction(display, my_gc, GXcopy);
		XSetForeground(display, my_gc, gc->color);
	}
	width = Vw(gc->width);
	if (width < 0)
		width = 0;
	XSetLineAttributes(display, my_gc, width, LineSolid, cap, join);
	if (use_mask()) {
		XSetLineAttributes(display, mask_gc, width, LineSolid, cap, join);
	}
}

static void lesstif_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void lesstif_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}

static void lesstif_set_draw_xor(pcb_hid_gc_t gc, int xor_set)
{
	gc->xor_set = xor_set;
}

#define ISORT(a,b) if (a>b) { a^=b; b^=a; a^=b; }

static void lesstif_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double dx1, dy1, dx2, dy2;
	int vw = Vw(gc->width);
	if ((conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly) && gc->erase)
		return;
#if 0
	pcb_printf("draw_line %#mD-%#mD @%#mS", x1, y1, x2, y2, gc->width);
#endif
	dx1 = Vx(x1);
	dy1 = Vy(y1);
	dx2 = Vx(x2);
	dy2 = Vy(y2);
#if 0
	pcb_printf(" = %#mD-%#mD %s\n", x1, y1, x2, y2, gc->colorname);
#endif

#if 1
	if (!pcb_line_clip(0, 0, view_width, view_height, &dx1, &dy1, &dx2, &dy2, vw))
		return;
#endif

	x1 = dx1;
	y1 = dy1;
	x2 = dx2;
	y2 = dy2;

	set_gc(gc);
	if (gc->cap == pcb_cap_square && x1 == x2 && y1 == y2) {
		XFillRectangle(display, pixmap, my_gc, x1 - vw / 2, y1 - vw / 2, vw, vw);
		if (use_mask())
			XFillRectangle(display, mask_bitmap, mask_gc, x1 - vw / 2, y1 - vw / 2, vw, vw);
	}
	else {
		XDrawLine(display, pixmap, my_gc, x1, y1, x2, y2);
		if (use_mask())
			XDrawLine(display, mask_bitmap, mask_gc, x1, y1, x2, y2);
	}
}

static void lesstif_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	if (conf_core.editor.thin_draw && gc->erase)
		return;
#if 0
	pcb_printf("draw_arc %#mD %#mSx%#mS s %d d %d", cx, cy, width, height, start_angle, delta_angle);
#endif
	width = Vz(width);
	height = Vz(height);
	cx = Vx(cx) - width;
	cy = Vy(cy) - height;

	if ((delta_angle >= 360.0) || (delta_angle <= -360.0)) {
		start_angle = 0;
		delta_angle = 360;
	}

	if (pcbhl_conf.editor.view.flip_x) {
		start_angle = 180 - start_angle;
		delta_angle = -delta_angle;
	}
	if (pcbhl_conf.editor.view.flip_y) {
		start_angle = -start_angle;
		delta_angle = -delta_angle;
	}
	start_angle = pcb_normalize_angle(start_angle);
	if (start_angle >= 180)
		start_angle -= 360;
#if 0
	pcb_printf(" = %#mD %#mSx%#mS %d %s\n", cx, cy, width, height, gc->width, gc->colorname);
#endif
	set_gc(gc);
	XDrawArc(display, pixmap, my_gc, cx, cy, width * 2, height * 2, (start_angle + 180) * 64, delta_angle * 64);
	if (use_mask() && !conf_core.editor.thin_draw)
		XDrawArc(display, mask_bitmap, mask_gc, cx, cy, width * 2, height * 2, (start_angle + 180) * 64, delta_angle * 64);
}

static void lesstif_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int vw = Vw(gc->width);
	if (conf_core.editor.thin_draw && gc->erase)
		return;
	x1 = Vx(x1);
	y1 = Vy(y1);
	x2 = Vx(x2);
	y2 = Vy(y2);
	if (x1 < -vw && x2 < -vw)
		return;
	if (y1 < -vw && y2 < -vw)
		return;
	if (x1 > view_width + vw && x2 > view_width + vw)
		return;
	if (y1 > view_height + vw && y2 > view_height + vw)
		return;
	if (x1 > x2) {
		pcb_coord_t xt = x1;
		x1 = x2;
		x2 = xt;
	}
	if (y1 > y2) {
		int yt = y1;
		y1 = y2;
		y2 = yt;
	}
	set_gc(gc);
	XDrawRectangle(display, pixmap, my_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	if (use_mask())
		XDrawRectangle(display, mask_bitmap, mask_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

static void lesstif_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	if ((conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly) && gc->erase)
		return;
#if 0
	pcb_printf("fill_circle %#mD %#mS", cx, cy, radius);
#endif
	radius = Vz(radius);
	cx = Vx(cx) - radius;
	cy = Vy(cy) - radius;
	if (cx < -2 * radius || cx > view_width)
		return;
	if (cy < -2 * radius || cy > view_height)
		return;
#if 0
	pcb_printf(" = %#mD %#mS %lx %s\n", cx, cy, radius, gc->color, gc->colorname);
#endif
	set_gc(gc);
	XFillArc(display, pixmap, my_gc, cx, cy, radius * 2, radius * 2, 0, 360 * 64);
	if (use_mask())
		XFillArc(display, mask_bitmap, mask_gc, cx, cy, radius * 2, radius * 2, 0, 360 * 64);
}

/* Intentaional code duplication for performance */
static void lesstif_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	static XPoint *p = 0;
	static int maxp = 0;
	int i;

	if (maxp < n_coords) {
		maxp = n_coords + 10;
		if (p)
			p = (XPoint *) realloc(p, maxp * sizeof(XPoint));
		else
			p = (XPoint *) malloc(maxp * sizeof(XPoint));
	}

	for (i = 0; i < n_coords; i++) {
		p[i].x = Vx(x[i]);
		p[i].y = Vy(y[i]);
	}
#if 0
	printf("fill_polygon %d pts\n", n_coords);
#endif
	set_gc(gc);
	XFillPolygon(display, pixmap, my_gc, p, n_coords, Complex, CoordModeOrigin);
	if (use_mask())
		XFillPolygon(display, mask_bitmap, mask_gc, p, n_coords, Complex, CoordModeOrigin);
}

/* Intentaional code duplication for performance */
static void lesstif_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	static XPoint *p = 0;
	static int maxp = 0;
	int i;

	if (maxp < n_coords) {
		maxp = n_coords + 10;
		if (p)
			p = (XPoint *) realloc(p, maxp * sizeof(XPoint));
		else
			p = (XPoint *) malloc(maxp * sizeof(XPoint));
	}

	for (i = 0; i < n_coords; i++) {
		p[i].x = Vx(x[i] + dx);
		p[i].y = Vy(y[i] + dy);
	}

	set_gc(gc);
	XFillPolygon(display, pixmap, my_gc, p, n_coords, Complex, CoordModeOrigin);
	if (use_mask())
		XFillPolygon(display, mask_bitmap, mask_gc, p, n_coords, Complex, CoordModeOrigin);
}

static void lesstif_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int vw = Vw(gc->width);
	if (conf_core.editor.thin_draw && gc->erase)
		return;
	x1 = Vx(x1);
	y1 = Vy(y1);
	x2 = Vx(x2);
	y2 = Vy(y2);
	if (x1 < -vw && x2 < -vw)
		return;
	if (y1 < -vw && y2 < -vw)
		return;
	if (x1 > view_width + vw && x2 > view_width + vw)
		return;
	if (y1 > view_height + vw && y2 > view_height + vw)
		return;
	if (x1 > x2) {
		pcb_coord_t xt = x1;
		x1 = x2;
		x2 = xt;
	}
	if (y1 > y2) {
		int yt = y1;
		y1 = y2;
		y2 = yt;
	}
	set_gc(gc);
	XFillRectangle(display, pixmap, my_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	if (use_mask())
		XFillRectangle(display, mask_bitmap, mask_gc, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

static void lesstif_calibrate(pcb_hid_t *hid, double xval, double yval)
{
	CRASH("lesstif_calibrate");
}

static int lesstif_shift_is_pressed(pcb_hid_t *hid)
{
	return shift_pressed;
}

static int lesstif_control_is_pressed(pcb_hid_t *hid)
{
	return ctrl_pressed;
}

static int lesstif_mod1_is_pressed(pcb_hid_t *hid)
{
	return alt_pressed;
}

extern void lesstif_get_coords(pcb_hid_t *hid, const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force);

static void lesstif_set_crosshair(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int action)
{
	if (crosshair_x != x || crosshair_y != y) {
		lesstif_show_crosshair(0);
		crosshair_x = x;
		crosshair_y = y;
		need_idle_proc();

		if (mainwind
				&& !in_move_event
				&& (x < view_left_x
						|| x > view_left_x + view_width * view_zoom || y < view_top_y || y > view_top_y + view_height * view_zoom)) {
			view_left_x = x - (view_width * view_zoom) / 2;
			view_top_y = y - (view_height * view_zoom) / 2;
			lesstif_pan_fixup();
		}

	}

	if (action == HID_SC_PAN_VIEWPORT) {
		Window root, child;
		unsigned int keys_buttons;
		int pos_x, pos_y, root_x, root_y;
		XQueryPointer(display, window, &root, &child, &root_x, &root_y, &pos_x, &pos_y, &keys_buttons);
		if (pcbhl_conf.editor.view.flip_x)
			view_left_x = x - (view_width - pos_x) * view_zoom;
		else
			view_left_x = x - pos_x * view_zoom;
		if (pcbhl_conf.editor.view.flip_y)
			view_top_y = y - (view_height - pos_y) * view_zoom;
		else
			view_top_y = y - pos_y * view_zoom;
		lesstif_pan_fixup();
		action = HID_SC_WARP_POINTER;
	}
	if (action == HID_SC_WARP_POINTER) {
		in_move_event++;
		XWarpPointer(display, None, window, 0, 0, 0, 0, Vx(x), Vy(y));
		in_move_event--;
	}
}

typedef struct {
	void (*func) (pcb_hidval_t);
	pcb_hidval_t user_data;
	XtIntervalId id;
} TimerStruct;

static void lesstif_timer_cb(XtPointer * p, XtIntervalId * id)
{
	TimerStruct *ts = (TimerStruct *) p;
	ts->func(ts->user_data);
	free(ts);
}

static pcb_hidval_t lesstif_add_timer(pcb_hid_t *hid, void (*func)(pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
{
	TimerStruct *t;
	pcb_hidval_t rv;
	t = (TimerStruct *) malloc(sizeof(TimerStruct));
	rv.ptr = t;
	t->func = func;
	t->user_data = user_data;
	t->id = XtAppAddTimeOut(app_context, milliseconds, (XtTimerCallbackProc) lesstif_timer_cb, t);
	return rv;
}

static void lesstif_stop_timer(pcb_hid_t *hid, pcb_hidval_t hv)
{
	TimerStruct *ts = (TimerStruct *) hv.ptr;
	XtRemoveTimeOut(ts->id);
	free(ts);
}


typedef struct {
	pcb_bool (*func) (pcb_hidval_t, int, unsigned int, pcb_hidval_t);
	pcb_hidval_t user_data;
	int fd;
	XtInputId id;
} WatchStruct;

void lesstif_unwatch_file(pcb_hid_t *hid, pcb_hidval_t data)
{
	WatchStruct *watch = (WatchStruct *) data.ptr;
	XtRemoveInput(watch->id);
	free(watch);
}

/* We need a wrapper around the hid file watch because to pass the correct flags */
static void lesstif_watch_cb(XtPointer client_data, int *fid, XtInputId *id)
{
	unsigned int pcb_condition = 0;
	struct pollfd fds;
	short condition;
	pcb_hidval_t x;
	WatchStruct *watch = (WatchStruct *) client_data;

	fds.fd = watch->fd;
	fds.events = POLLIN | POLLOUT;
	poll(&fds, 1, 0);
	condition = fds.revents;

	/* Should we only include those we were asked to watch? */
	if (condition & POLLIN)
		pcb_condition |= PCB_WATCH_READABLE;
	if (condition & POLLOUT)
		pcb_condition |= PCB_WATCH_WRITABLE;
	if (condition & POLLERR)
		pcb_condition |= PCB_WATCH_ERROR;
	if (condition & POLLHUP)
		pcb_condition |= PCB_WATCH_HANGUP;

	x.ptr = (void *) watch;
	if (!watch->func(x, watch->fd, pcb_condition, watch->user_data))
		lesstif_unwatch_file(pcb_gui, x);
	return;
}

pcb_hidval_t lesstif_watch_file(pcb_hid_t *hid, int fd, unsigned int condition, pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data), pcb_hidval_t user_data)
{
	WatchStruct *watch = (WatchStruct *)malloc(sizeof(WatchStruct));
	pcb_hidval_t ret;
	unsigned int xt_condition = 0;

	if (condition & PCB_WATCH_READABLE)
		xt_condition |= XtInputReadMask;
	if (condition & PCB_WATCH_WRITABLE)
		xt_condition |= XtInputWriteMask;
	if (condition & PCB_WATCH_ERROR)
		xt_condition |= XtInputExceptMask;
	if (condition & PCB_WATCH_HANGUP)
		xt_condition |= XtInputExceptMask;

	watch->func = func;
	watch->user_data = user_data;
	watch->fd = fd;
	watch->id = XtAppAddInput(app_context, fd, (XtPointer) (size_t) xt_condition, lesstif_watch_cb, watch);

	ret.ptr = (void *) watch;
	return ret;
}

extern void *lesstif_attr_dlg_new(pcb_hid_t *hid, const char *id, pcb_hid_attribute_t *attrs_, int n_attrs_, const char *title_, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny);

extern int lesstif_attr_dlg_run(void *hid_ctx);
extern void lesstif_attr_dlg_raise(void *hid_ctx);
extern void lesstif_attr_dlg_close(void *hid_ctx);
extern void lesstif_attr_dlg_free(void *hid_ctx);
extern void lesstif_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val);
extern int lesstif_attr_dlg_widget_state(void *hid_ctx, int idx, int enabled);
extern int lesstif_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide);
extern int lesstif_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);
extern void lesstif_attr_dlg_set_help(void *hid_ctx, int idx, const char *val);


#include "wt_preview.c"

static void lesstif_beep(pcb_hid_t *hid)
{
	putchar(7);
	fflush(stdout);
}

static int lesstif_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nLesstif GUI command line arguments:\n\n");
	pcb_hid_usage(lesstif_attribute_list, sizeof(lesstif_attribute_list) / sizeof(lesstif_attribute_list[0]));
	fprintf(stderr, "\nInvocation: pcb-rnd --gui lesstif [options]\n");
	return 0;
}

static void lesstif_globconf_change_post(conf_native_t *cfg, int arr_idx)
{
	if (!lesstif_active)
		return;
	if (strncmp(cfg->hash_path, "appearance/color/", 17) == 0)
		lesstif_invalidate_all(pcb_gui);
	if (strncmp(cfg->hash_path, "rc/cli_", 7) == 0) {
		stdarg_n = 0;
		stdarg(XmNlabelString, XmStringCreatePCB(pcb_cli_prompt(":")));
		XtSetValues(m_cmd_label, stdarg_args, stdarg_n);
	}
}

static conf_hid_id_t lesstif_conf_id = -1;

TODO("decide if we'll ever need this")
#if 0
static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func)(conf_native_t *, int))
{
	conf_native_t *n = pcb_conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		pcb_conf_hid_set_cb(n, lesstif_conf_id, cbs);
	}
}

static void lesstif_conf_regs(const char *cookie)
{
	static conf_hid_callbacks_t cbs_grid_unit;
}
#endif

#include <Xm/CutPaste.h>

static int ltf_clip_set(pcb_hid_t *hid, pcb_hid_clipfmt_t format, const void *data, size_t len)
{
	static long cnt = 0;
	long item_id, data_id;
	XmString lab = XmStringCreateLocalized("pcb_rnd");
	
	
	if (XmClipboardStartCopy(display, window, lab, CurrentTime, 0, NULL, &item_id) != XmClipboardSuccess) {
		XmStringFree(lab);
		return -1;
	}
	XmStringFree(lab);
	if (XmClipboardCopy(display, window, item_id, "STRING", (void *)data, len, ++cnt, &data_id) != XmClipboardSuccess) {
		XmClipboardCancelCopy(display, window, item_id);
		return -1;
	}
	if (XmClipboardEndCopy(display, window, item_id) != XmClipboardSuccess) {
		XmClipboardCancelCopy(display, window, item_id);
		return -1;
	}
	return 0;
}

static int ltf_clip_get(pcb_hid_t *hid, pcb_hid_clipfmt_t *format, void **data, size_t *len)
{
	int res;
	gds_t tmp;
	char buff[65536];
	long unsigned bl = 0;
	long dummy;

	if (XmClipboardStartRetrieve(display, window, CurrentTime) != XmClipboardSuccess)
		return -1;

	gds_init(&tmp);


	res = XmClipboardRetrieve(display, window, "STRING", buff, sizeof(buff), &bl, &dummy);
	if (res == XmClipboardSuccess) {
		if (bl > 0)
			gds_append_len(&tmp, buff, bl);
	}

	XmClipboardEndRetrieve(display, window);
	if (tmp.used == 0) {
		gds_uninit(&tmp);
		return -1;
	}
	*data = tmp.array;
	*len = tmp.used;
	return 0;
}

static void ltf_clip_free(pcb_hid_t *hid, pcb_hid_clipfmt_t format, void *data, size_t len)
{
	free(data);
}

static void ltf_zoom_win(pcb_hid_t *hid, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_bool set_crosshair)
{
	zoom_win(hid, x1, y1, x2, y2, 1);
}

static void ltf_zoom(pcb_hid_t *hid, pcb_coord_t center_x, pcb_coord_t center_y, double factor, int relative)
{
	if (relative)
		zoom_by(factor, Vx(center_x), Vy(center_y));
	else
		zoom_to(factor, Vx(center_x), Vy(center_y));
}

static void ltf_pan(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int relative)
{
	if (relative) {
		view_left_x += x;
		view_top_y += y;
		lesstif_pan_fixup();

	}
	else {
		view_left_x = x - (view_width * view_zoom) / 2;
		view_top_y = y - (view_height * view_zoom) / 2;
		lesstif_pan_fixup();
		XWarpPointer(display, window, window, 0, 0, view_width, view_height, Vx(x), Vy(y));
	}
}

static void ltf_pan_mode(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, pcb_bool mode)
{
	Pan(mode, Vx(x), Vy(y));
}


static void ltf_view_get(pcb_hid_t *hid, pcb_box_t *viewbox)
{
	viewbox->X1 = view_left_x;
	viewbox->Y1 = view_top_y;
	viewbox->X2 = pcb_round(view_left_x + view_width * view_zoom);
	viewbox->Y2 = pcb_round(view_top_y + view_height * view_zoom);
}

static void ltf_open_command(pcb_hid_t *hid)
{
	pcb_clihist_init();
	pcb_clihist_reset();
	XtManageChild(m_cmd_label);
	XtManageChild(m_cmd);
	XmProcessTraversal(m_cmd, XmTRAVERSE_CURRENT);
	cmd_is_active = 1;
}

static void ltf_set_top_title(pcb_hid_t *hid, const char *title)
{
	stdarg_n = 0;
	stdarg(XmNtitle, title);
	XtSetValues(appwidget, stdarg_args, stdarg_n);
}

void lesstif_create_menu(pcb_hid_t *hid, const char *menu, const pcb_menu_prop_t *props);
int lesstif_remove_menu(pcb_hid_t *hid, const char *menu);
int lesstif_remove_menu_node(pcb_hid_t *hid, lht_node_t *node);
pcb_hid_cfg_t *lesstif_get_menu_cfg(pcb_hid_t *hid);
int ltf_open_popup(pcb_hid_t *hid, const char *menupath);

int pplg_check_ver_hid_lesstif(int version_we_need) { return 0; }

void pplg_uninit_hid_lesstif(void)
{
	pcb_export_remove_opts_by_cookie(lesstif_cookie);
	pcb_event_unbind_allcookie(lesstif_cookie);
	pcb_conf_hid_unreg(lesstif_cookie);
}

int pplg_init_hid_lesstif(void)
{
	static conf_hid_callbacks_t ccb;

	PCB_API_CHK_VER;

	memset(&ccb, 0, sizeof(ccb));
	ccb.val_change_post = lesstif_globconf_change_post;

	memset(&lesstif_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&lesstif_hid);

	lesstif_hid.struct_size = sizeof(pcb_hid_t);
	lesstif_hid.name = "lesstif";
	lesstif_hid.description = "LessTif - a Motif clone for X/Unix";
	lesstif_hid.gui = 1;
	lesstif_hid.heavy_term_layer_ind = 1;

	lesstif_hid.get_export_options = lesstif_get_export_options;
	lesstif_hid.do_export = lesstif_do_export;
	lesstif_hid.do_exit = lesstif_do_exit;
	lesstif_hid.uninit = lesstif_uninit;
	lesstif_hid.iterate = lesstif_iterate;
	lesstif_hid.parse_arguments = lesstif_parse_arguments;
	lesstif_hid.invalidate_lr = lesstif_invalidate_lr;
	lesstif_hid.invalidate_all = lesstif_invalidate_all;
	lesstif_hid.notify_crosshair_change = lesstif_notify_crosshair_change;
	lesstif_hid.notify_mark_change = lesstif_notify_mark_change;
	lesstif_hid.set_layer_group = lesstif_set_layer_group;
	lesstif_hid.make_gc = lesstif_make_gc;
	lesstif_hid.destroy_gc = lesstif_destroy_gc;
	lesstif_hid.set_drawing_mode = lesstif_set_drawing_mode;
	lesstif_hid.render_burst = lesstif_render_burst;
	lesstif_hid.set_color = lesstif_set_color;
	lesstif_hid.set_line_cap = lesstif_set_line_cap;
	lesstif_hid.set_line_width = lesstif_set_line_width;
	lesstif_hid.set_draw_xor = lesstif_set_draw_xor;
	lesstif_hid.draw_line = lesstif_draw_line;
	lesstif_hid.draw_arc = lesstif_draw_arc;
	lesstif_hid.draw_rect = lesstif_draw_rect;
	lesstif_hid.fill_circle = lesstif_fill_circle;
	lesstif_hid.fill_polygon = lesstif_fill_polygon;
	lesstif_hid.fill_polygon_offs = lesstif_fill_polygon_offs;
	lesstif_hid.fill_rect = lesstif_fill_rect;

	lesstif_hid.calibrate = lesstif_calibrate;
	lesstif_hid.shift_is_pressed = lesstif_shift_is_pressed;
	lesstif_hid.control_is_pressed = lesstif_control_is_pressed;
	lesstif_hid.mod1_is_pressed = lesstif_mod1_is_pressed;
	lesstif_hid.get_coords = lesstif_get_coords;
	lesstif_hid.set_crosshair = lesstif_set_crosshair;
	lesstif_hid.add_timer = lesstif_add_timer;
	lesstif_hid.stop_timer = lesstif_stop_timer;
	lesstif_hid.watch_file = lesstif_watch_file;
	lesstif_hid.unwatch_file = lesstif_unwatch_file;
	lesstif_hid.benchmark = ltf_benchmark;

	lesstif_hid.fileselect = pcb_ltf_fileselect;
	lesstif_hid.attr_dlg_new = lesstif_attr_dlg_new;
	lesstif_hid.attr_dlg_run = lesstif_attr_dlg_run;
	lesstif_hid.attr_dlg_raise = lesstif_attr_dlg_raise;
	lesstif_hid.attr_dlg_close = lesstif_attr_dlg_close;
	lesstif_hid.attr_dlg_free = lesstif_attr_dlg_free;
	lesstif_hid.attr_dlg_property = lesstif_attr_dlg_property;
	lesstif_hid.attr_dlg_widget_state = lesstif_attr_dlg_widget_state;
	lesstif_hid.attr_dlg_widget_hide = lesstif_attr_dlg_widget_hide;
	lesstif_hid.attr_dlg_set_value = lesstif_attr_dlg_set_value;
	lesstif_hid.attr_dlg_set_help = lesstif_attr_dlg_set_help;
	lesstif_hid.supports_dad_text_markup = 0;

	lesstif_hid.beep = lesstif_beep;
	lesstif_hid.edit_attributes = lesstif_attributes_dialog;
	lesstif_hid.point_cursor = PointCursor;
	lesstif_hid.command_entry = lesstif_command_entry;
	lesstif_hid.clip_set = ltf_clip_set;
	lesstif_hid.clip_get = ltf_clip_get;
	lesstif_hid.clip_free = ltf_clip_free;

	lesstif_hid.create_menu = lesstif_create_menu;
	lesstif_hid.remove_menu = lesstif_remove_menu;
	lesstif_hid.remove_menu_node = lesstif_remove_menu_node;
	lesstif_hid.update_menu_checkbox = lesstif_update_widget_flags;
	lesstif_hid.get_menu_cfg = lesstif_get_menu_cfg;

	lesstif_hid.key_state = &lesstif_keymap;

	lesstif_hid.zoom_win = ltf_zoom_win;
	lesstif_hid.zoom = ltf_zoom;
	lesstif_hid.pan = ltf_pan;
	lesstif_hid.pan_mode = ltf_pan_mode;
	lesstif_hid.view_get = ltf_view_get;
	lesstif_hid.open_command = ltf_open_command;
	lesstif_hid.open_popup = ltf_open_popup;
	lesstif_hid.reg_mouse_cursor = ltf_reg_mouse_cursor;
	lesstif_hid.set_mouse_cursor = ltf_set_mouse_cursor;
	lesstif_hid.set_top_title = ltf_set_top_title;
	lesstif_hid.dock_enter = ltf_dock_enter;
	lesstif_hid.busy = ltf_busy;

	lesstif_hid.draw_pixmap = pcb_ltf_draw_pixmap;

	lesstif_hid.set_hidlib = ltf_set_hidlib;

	lesstif_hid.usage = lesstif_usage;

	lesstif_hid.get_dad_hidlib = ltf_attr_get_dad_hidlib;

	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, LesstifNetlistChanged, NULL, lesstif_cookie);
	pcb_event_bind(PCB_EVENT_LIBRARY_CHANGED, LesstifLibraryChanged, NULL, lesstif_cookie);

	pcb_hid_register_hid(&lesstif_hid);
	if (lesstif_conf_id < 0)
		lesstif_conf_id = pcb_conf_hid_reg(lesstif_cookie, &ccb);
/*	lesstif_conf_regs(lesstif_cookie);*/

	return 0;
}

static int lesstif_attrs_regd = 0;
static void lesstif_reg_attrs(void)
{
	if (!lesstif_attrs_regd)
		pcb_export_register_opts(lesstif_attribute_list, sizeof(lesstif_attribute_list)/sizeof(lesstif_attribute_list[0]), lesstif_cookie, 0);
	lesstif_attrs_regd = 1;
}

extern void pcb_ltf_library_init2(void);
extern void pcb_ltf_dialogs_init2(void);
extern void pcb_ltf_netlist_init2(void);


static void lesstif_begin(void)
{
	pcb_ltf_library_init2();
	lesstif_reg_attrs();
	pcb_ltf_dialogs_init2();
	pcb_ltf_netlist_init2();
	lesstif_active = 1;
}

static void lesstif_end(void)
{
	pcb_remove_actions_by_cookie(lesstif_cookie);
	pcb_export_remove_opts_by_cookie(lesstif_cookie);
	lesstif_active = 0;
}
