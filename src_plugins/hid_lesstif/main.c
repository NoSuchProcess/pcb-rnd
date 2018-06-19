/* 15 Oct 2008 Ineiev: add different crosshair shapes */

#include "xincludes.h"

#include "config.h"
#include "math_helper.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>

#include "data.h"
#include "action_helper.h"
#include "crosshair.h"
#include "conf_hid.h"
#include "layer.h"
#include "pcb-printf.h"
#include "event.h"
#include "error.h"
#include "plugins.h"
#include "macro.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"
#include "hid_cfg.h"
#include "lesstif.h"
#include "hid_cfg_input.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "hid_init.h"
#include "hid_color.h"
#include "hid_extents.h"
#include "hid_flags.h"
#include "hid_actions.h"
#include "stdarg.h"
#include "grid.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "layer_vis.h"
#include "tool.h"

#include "../src_plugins/lib_hid_common/clip.h"

#include <sys/poll.h>

const char *lesstif_cookie = "lesstif HID";

pcb_hid_cfg_mouse_t lesstif_mouse;
pcb_hid_cfg_keys_t lesstif_keymap;
int lesstif_active = 0;

static int idle_proc_set = 0;
static int need_redraw = 0;

#ifndef XtRDouble
#define XtRDouble "Double"
#endif

/* How big the viewport can be relative to the pcb size.  */
#define MAX_ZOOM_SCALE	10
#define UUNIT	conf_core.editor.grid_unit->allow

typedef struct hid_gc_s {
	pcb_core_gc_t core_gc;
	pcb_hid_t *me_pointer;
	Pixel color;
	char *colorname;
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
static Cursor my_cursor = 0;
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
static GC my_gc = 0, bg_gc, clip_gc = 0, bset_gc = 0, bclear_gc = 0, mask_gc = 0;
static Pixel bgcolor, offlimit_color, grid_color;
static int bgred, bggreen, bgblue;

static GC arc1_gc, arc2_gc;

/* These are for the pinout windows. */
typedef struct PreviewData {
	struct PreviewData *prev, *next;
	Widget form;
	Window window;
	pcb_coord_t left, right, top, bottom;	/* PCB extents of item */
	pcb_coord_t x, y;										/* PCB coordinates of upper right corner of window */
	double zoom;									/* PCB units per screen pixel */
	int v_width, v_height;				/* pixels */

	pcb_hid_expose_ctx_t ctx;
	pcb_bool (*mouse_ev)(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);
	void (*pre_close)(struct PreviewData *pd);
	pcb_hid_expose_t overlay_draw;
	unsigned pan:1;
	int pan_ox, pan_oy;
	pcb_coord_t pan_opx, pan_opy;
} PreviewData;

/* Linked list of all pinout windows.  */
static PreviewData *pinouts = 0;
/* If set, we are currently updating this pinout window.  */
static PreviewData *pinout = 0;

static pcb_coord_t crosshair_x = 0, crosshair_y = 0;
static int in_move_event = 0, crosshair_in_window = 1;

Widget mainwind;
Widget work_area, messages, command, hscroll, vscroll;
static Widget m_mark, m_crosshair, m_grid, m_zoom, m_mode, m_status;
static Widget m_rats;
Widget lesstif_m_layer;
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
static double view_zoom = PCB_MIL_TO_COORD(10), prev_view_zoom = PCB_MIL_TO_COORD(10);
static pcb_bool autofade = 0;
static pcb_bool crosshair_on = pcb_true;

static void lesstif_reg_attrs(void);
static void lesstif_begin(void);
static void lesstif_end(void);

static void ShowCrosshair(pcb_bool show)
{
	if (crosshair_on == show)
		return;

	pcb_notify_crosshair_change(pcb_false);
	if (pcb_marked.status)
		pcb_notify_mark_change(pcb_false);

	crosshair_on = show;

	pcb_notify_crosshair_change(pcb_true);
	if (pcb_marked.status)
		pcb_notify_mark_change(pcb_true);
}

/* This is the size of the current PCB work area.  */
/* Use PCB->MaxWidth, PCB->MaxHeight.  */
/* static int pcb_width, pcb_height; */
static int use_private_colormap = 0;
static int stdin_listen = 0;
static char *background_image_file = 0;

pcb_hid_attribute_t lesstif_attribute_list[] = {
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

/* %start-doc options "22 lesstif GUI Options"
@ftable @code
@item --pcb-menu <string>
Location of the @file{pcb-menu.res} file which defines the menu for the lesstif GUI.
@end ftable
%end-doc
*/
#warning TODO#1: this should be generic and not depend on the HID
/*
	{"pcb-menu", "Location of pcb-menu.res file",
		PCB_HATT_STRING, 0, 0, {0, PCBSHAREDIR "/pcb-menu.res", 0}, 0, &lesstif_pcbmenu_path}
#define HA_pcbmenu 3
*/
};

PCB_REGISTER_ATTRIBUTES(lesstif_attribute_list, lesstif_cookie)

static int lesstif_direct = 0;
static pcb_composite_op_t lesstif_drawing_mode = 0;
#define use_mask() ((!lesstif_direct) && ((lesstif_drawing_mode == PCB_HID_COMP_POSITIVE) || (lesstif_drawing_mode == PCB_HID_COMP_POSITIVE_XOR) || (lesstif_drawing_mode == PCB_HID_COMP_NEGATIVE)))

static void zoom_max();
static void zoom_to(double factor, pcb_coord_t x, pcb_coord_t y);
static void zoom_win(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
static void zoom_by(double factor, pcb_coord_t x, pcb_coord_t y);
static void zoom_toggle(pcb_coord_t x, pcb_coord_t y);
static void pinout_callback(Widget, PreviewData *, XmDrawingAreaCallbackStruct *);
static void pinout_unmap(Widget, PreviewData *, void *);
static void Pan(int mode, pcb_coord_t x, pcb_coord_t y);

/* Px converts view->pcb, Vx converts pcb->view */

static inline int Vx(pcb_coord_t x)
{
	int rv = (x - view_left_x) / view_zoom + 0.5;
	if (conf_core.editor.view.flip_x)
		rv = view_width - rv;
	return rv;
}

static inline int Vy(pcb_coord_t y)
{
	int rv = (y - view_top_y) / view_zoom + 0.5;
	if (conf_core.editor.view.flip_y)
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
	if (conf_core.editor.view.flip_x)
		x = view_width - x;
	return x * view_zoom + view_left_x;
}

static inline pcb_coord_t Py(int y)
{
	if (conf_core.editor.view.flip_y)
		y = view_height - y;
	return y * view_zoom + view_top_y;
}

static inline pcb_coord_t Pz(int z)
{
	return z * view_zoom;
}

void lesstif_coords_to_pcb(int vx, int vy, pcb_coord_t * px, pcb_coord_t * py)
{
	*px = Px(vx);
	*py = Py(vy);
}

Pixel lesstif_parse_color(const char *value)
{
	XColor color;
	if (XParseColor(display, lesstif_colormap, value, &color))
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

/* Called from the core when it's busy doing something and we need to
   indicate that to the user.  */
static void LesstifBusy(void *user_data, int argc, pcb_event_arg_t argv[])
{
	static Cursor busy_cursor = 0;
	if (!lesstif_active)
		return;
	if (busy_cursor == 0)
		busy_cursor = XCreateFontCursor(display, XC_watch);
	XDefineCursor(display, window, busy_cursor);
	XFlush(display);
	old_cursor_mode = -1;
	return;
}

/* ---------------------------------------------------------------------- */

/* Local actions.  */

static void PointCursor(pcb_bool grabbed)
{
	if (grabbed > 0)
		over_point = 1;
	else
		over_point = 0;
	old_cursor_mode = -1;
}

extern void LesstifNetlistChanged(void *user_data, int argc, pcb_event_arg_t argv[]);
extern void LesstifRouteStylesChanged(void *user_data, int argc, pcb_event_arg_t argv[]);
extern void LesstifLibraryChanged(void *user_data, int argc, pcb_event_arg_t argv[]);


static void ev_pcb_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (work_area == 0)
		return;
	/*pcb_printf("PCB Changed! %$mD\n", PCB->MaxWidth, PCB->MaxHeight); */
	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNsliderSize, PCB->MaxWidth ? PCB->MaxWidth : 1);
	stdarg(XmNmaximum, PCB->MaxWidth ? PCB->MaxWidth : 1);
	XtSetValues(hscroll, stdarg_args, stdarg_n);
	stdarg_n = 0;
	stdarg(XmNminimum, 0);
	stdarg(XmNvalue, 0);
	stdarg(XmNsliderSize, PCB->MaxHeight ? PCB->MaxHeight : 1);
	stdarg(XmNmaximum, PCB->MaxHeight ? PCB->MaxHeight : 1);
	XtSetValues(vscroll, stdarg_args, stdarg_n);
	zoom_max();

	LesstifNetlistChanged(NULL, 0, NULL);
	LesstifRouteStylesChanged(NULL, 0, NULL);
	lesstif_sizes_reset();
	lesstif_update_layer_groups();
	while (pinouts)
		pinout_unmap(0, pinouts, 0);
	if (PCB->Filename) {
		char *cp = strrchr(PCB->Filename, '/');
		stdarg_n = 0;
		stdarg(XmNtitle, cp ? cp + 1 : PCB->Filename);
		XtSetValues(appwidget, stdarg_args, stdarg_n);
	}
	return;
}

static const char pcb_acts_Zoom[] = "Zoom()\n" "Zoom(factor)";

static const char pcb_acth_Zoom[] = "Various zoom factor changes.";

/* %start-doc actions Zoom

Changes the zoom (magnification) of the view of the board.  If no
arguments are passed, the view is scaled such that the board just fits
inside the visible window (i.e. ``view all'').  Otherwise,
@var{factor} specifies a change in zoom factor.  It may be prefixed by
@code{+}, @code{-}, or @code{=} to change how the zoom factor is
modified.  The @var{factor} is a floating point number, such as
@code{1.5} or @code{0.75}.

@table @code

@item +@var{factor}
Values greater than 1.0 cause the board to be drawn smaller; more of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn bigger; less of the board will be visible.

@item -@var{factor}
Values greater than 1.0 cause the board to be drawn bigger; less of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn smaller; more of the board will be visible.

@item =@var{factor}

The @var{factor} is an absolute zoom factor; the unit for this value
is "PCB units per screen pixel".  Since PCB units are 0.01 mil, a
@var{factor} of 1000 means 10 mils (0.01 in) per pixel, or 100 DPI,
about the actual resolution of most screens - resulting in an "actual
size" board.  Similarly, a @var{factor} of 100 gives you a 10x actual
size.

@end table

Note that zoom factors of zero are silently ignored.

%end-doc */

static int pcb_act_Zoom(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x, y;
	const char *vp;
	double v;

	pcb_hid_get_coords("Click on a place to zoom in", &x, &y);

	if (x == 0 && y == 0) {
		x = view_width / 2;
		y = view_height / 2;
	}
	else {
		x = Vx(x);
		y = Vy(y);
	}
	if (argc < 1) {
		zoom_max();
		return 0;
	}
	vp = argv[0];
	if (pcb_strcasecmp(vp, "toggle") == 0) {
		zoom_toggle(x, y);
		return 0;
	}
	if (*vp == '+' || *vp == '-' || *vp == '=')
		vp++;
	pcb_setlocale(LC_ALL, "C");
	v = strtod(vp, NULL);
	pcb_setlocale(LC_ALL, "");
	if (v <= 0)
		return 1;
	switch (argv[0][0]) {
	case '-':
		zoom_by(1 / v, x, y);
		break;
	default:
	case '+':
		zoom_by(v, x, y);
		break;
	case '=':
		zoom_to(v, x, y);
		break;
	}
	return 0;
	PCB_OLD_ACT_END;
}


static const char pcb_acts_ZoomTo[] = "ZoomTo(x1, y1, x2, y2)";

static const char pcb_acth_ZoomTo[] = "Zoom to a specific window on the board.";

/* %start-doc actions ZoomTo

Changes the zoom (magnification) of the view of the board.  If no
arguments are passed, the view is scaled such that the board just fits
inside the visible window (i.e. ``view all'').  Otherwise,
@var{factor} specifies a change in zoom factor.  It may be prefixed by
@code{+}, @code{-}, or @code{=} to change how the zoom factor is
modified.  The @var{factor} is a floating point number, such as
@code{1.5} or @code{0.75}.

@table @code

@item @var{x1}, @var{y1}, @var{x2}, @var{y2}
Zoom and pan so that a specific box of the board is shown.
@end table

Note that zoom factors of zero are silently ignored.

%end-doc */

static int pcb_act_ZoomTo(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x1, y1, x2, y2;
	pcb_bool succ;

	if (argc != 4)
		return -1;

	x1 = pcb_get_value(argv[0], NULL, NULL, &succ);
	if (!succ)
		return -1;

	y1 = pcb_get_value(argv[1], NULL, NULL, &succ);
	if (!succ)
		return -1;

	x2 = pcb_get_value(argv[2], NULL, NULL, &succ);
	if (!succ)
		return -1;

	y2 = pcb_get_value(argv[3], NULL, NULL, &succ);
	if (!succ)
		return -1;

	zoom_win(x1, y1, x2, y2);

	return 0;
	PCB_OLD_ACT_END;
}

static int pan_thumb_mode;

static int pcb_act_Pan(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x, y;
	int mode;

	pcb_hid_get_coords("Click on a place to pan", &x, &y);

	if (argc == 2) {
		pan_thumb_mode = (pcb_strcasecmp(argv[0], "thumb") == 0) ? 1 : 0;
		mode = atoi(argv[1]);
	}
	else {
		pan_thumb_mode = 0;
		mode = atoi(argv[0]);
	}
	Pan(mode, Vx(x), Vy(y));

	return 0;
	PCB_OLD_ACT_END;
}

static const char pcb_acts_SwapSides[] = "SwapSides(|v|h|r)";

static const char pcb_acth_SwapSides[] = "Swaps the side of the board you're looking at.";

/* %start-doc actions SwapSides

This action changes the way you view the board.

@table @code

@item v
Flips the board over vertically (up/down).

@item h
Flips the board over horizontally (left/right), like flipping pages in
a book.

@item r
Rotates the board 180 degrees without changing sides.

@end table

If no argument is given, the board isn't moved but the opposite side
is shown.

Normally, this action changes which pads and silk layer are drawn as
pcb_true silk, and which are drawn as the "invisible" layer.  It also
determines which solder mask you see.

As a special case, if the layer group for the side you're looking at
is visible and currently active, and the layer group for the opposite
is not visible (i.e. disabled), then this action will also swap which
layer group is visible and active, effectively swapping the ``working
side'' of the board.

%end-doc */
static int group_showing(int g, int *c)
{
	int i, l;
	*c = PCB->LayerGroups.grp[g].lid[0];
	for (i = 0; i < PCB->LayerGroups.grp[g].len; i++) {
		l = PCB->LayerGroups.grp[g].lid[i];
		if (l >= 0 && l < pcb_max_layer && (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK))) {
			*c = l;
			if (PCB->Data->Layer[l].meta.real.vis)
				return 1;
		}
	}
	return 0;
}

#warning TODO: ui_zoomplan.c does the same, maybe make the code common?
static int pcb_act_SwapSides(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	int old_shown_side = conf_core.editor.show_solder_side;
	pcb_layergrp_id_t comp_group = -1, solder_group = -1;
	pcb_layergrp_id_t active_group = pcb_layer_get_group(PCB, pcb_layer_stack[0]);
	int comp_layer;
	int solder_layer;
	int comp_showing = 0, solder_showing = 0;

	if (pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder_group, 1) > 0)
		solder_showing = group_showing(solder_group, &solder_layer);

	if (pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_group, 1) > 0)
		comp_showing = group_showing(comp_group, &comp_layer);

	if (argc > 0) {
		switch (argv[0][0]) {
		case 'h':
		case 'H':
			conf_toggle_editor_("view/flip_x", view.flip_x);
			break;
		case 'v':
		case 'V':
			conf_toggle_editor_("view/flip_y", view.flip_y);
			break;
		case 'r':
		case 'R':
			conf_toggle_editor_("view/flip_x", view.flip_x);
			conf_toggle_editor_("view/flip_y", view.flip_y);
			break;
		default:
			return 1;
		}
		/* SwapSides will swap this */
		conf_set_editor(show_solder_side, (conf_core.editor.view.flip_x == conf_core.editor.view.flip_y));
	}

	stdarg_n = 0;
	if (conf_core.editor.view.flip_x)
		stdarg(XmNprocessingDirection, XmMAX_ON_LEFT);
	else
		stdarg(XmNprocessingDirection, XmMAX_ON_RIGHT);
	XtSetValues(hscroll, stdarg_args, stdarg_n);

	stdarg_n = 0;
	if (conf_core.editor.view.flip_y)
		stdarg(XmNprocessingDirection, XmMAX_ON_TOP);
	else
		stdarg(XmNprocessingDirection, XmMAX_ON_BOTTOM);
	XtSetValues(vscroll, stdarg_args, stdarg_n);

	conf_toggle_editor(show_solder_side);

	/* The idea is that if we're looking at the front side and the front
	   layer is active (or visa versa), switching sides should switch
	   layers too.  We used to only do this if the other layer wasn't
	   shown, but we now do it always.  Change it back if users get
	   confused.  */
	if (conf_core.editor.show_solder_side != old_shown_side) {
		if (conf_core.editor.show_solder_side) {
			if (active_group == comp_group) {
				if (comp_showing && !solder_showing)
					pcb_layervis_change_group_vis(comp_layer, 0, 0);
				pcb_layervis_change_group_vis(solder_layer, 1, 1);
			}
		}
		else {
			if (active_group == solder_group) {
				if (solder_showing && !comp_showing)
					pcb_layervis_change_group_vis(solder_layer, 0, 0);
				pcb_layervis_change_group_vis(comp_layer, 1, 1);
			}
		}
	}
	lesstif_invalidate_all();
	return 0;
	PCB_OLD_ACT_END;
}

static Widget m_cmd = 0, m_cmd_label;

static void command_callback(Widget w, XtPointer uptr, XmTextVerifyCallbackStruct * cbs)
{
	char *s;
	switch (cbs->reason) {
	case XmCR_ACTIVATE:
		s = XmTextGetString(w);
		lesstif_show_crosshair(0);
		pcb_hid_parse_command(s);
		XtFree(s);
		XmTextSetString(w, XmStrCast(""));
	case XmCR_LOSING_FOCUS:
		XtUnmanageChild(m_cmd);
		XtUnmanageChild(m_cmd_label);
		break;
	}
}

static void command_event_handler(Widget w, XtPointer p, XEvent * e, Boolean * cont)
{
	char buf[10];
	KeySym sym;

	switch (e->type) {
	case KeyPress:
		XLookupString((XKeyEvent *) e, buf, sizeof(buf), &sym, NULL);
		switch (sym) {
		case XK_Escape:
			XtUnmanageChild(m_cmd);
			XtUnmanageChild(m_cmd_label);
			XmTextSetString(w, XmStrCast(""));
			*cont = False;
			break;
		}
		break;
	}
}

static const char pcb_acts_Command[] = "Command()";

static const char pcb_acth_Command[] = "Displays the command line input window.";

/* %start-doc actions Command

The command window allows the user to manually enter actions to be
executed.  Action syntax can be done one of two ways:

@table @code

@item
Follow the action name by an open parenthesis, arguments separated by
commas, end with a close parenthesis.  Example: @code{Abc(1,2,3)}

@item
Separate the action name and arguments by spaces.  Example: @code{Abc
1 2 3}.

@end table

The first option allows you to have arguments with spaces in them,
but the second is more ``natural'' to type for most people.

Note that action names are not case sensitive, but arguments normally
are.  However, most actions will check for ``keywords'' in a case
insensitive way.

There are three ways to finish with the command window.  If you press
the @code{Enter} key, the command is invoked, the window goes away,
and the next time you bring up the command window it's empty.  If you
press the @code{Esc} key, the window goes away without invoking
anything, and the next time you bring up the command window it's
empty.  If you change focus away from the command window (i.e. click
on some other window), the command window goes away but the next time
you bring it up it resumes entering the command you were entering
before.

%end-doc */

static int pcb_act_Command(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	XtManageChild(m_cmd_label);
	XtManageChild(m_cmd);
	XmProcessTraversal(m_cmd, XmTRAVERSE_CURRENT);
	return 0;
	PCB_OLD_ACT_END;
}

static const char pcb_acts_Benchmark[] = "Benchmark()";

static const char pcb_acth_Benchmark[] = "Benchmark the GUI speed.";

/* %start-doc actions Benchmark

This action is used to speed-test the Lesstif graphics subsystem.  It
redraws the current screen as many times as possible in ten seconds.
It reports the amount of time needed to draw the screen once.

%end-doc */

static int pcb_act_Benchmark(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	int i = 0;
	time_t start, end;
	pcb_hid_expose_ctx_t ctx;
	Drawable save_main;


	save_main = main_pixmap;
	main_pixmap = window;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->MaxWidth;
	ctx.view.Y2 = PCB->MaxHeight;

	pixmap = window;
	XSync(display, 0);
	time(&start);
	do {
		XFillRectangle(display, pixmap, bg_gc, 0, 0, view_width, view_height);
		pcb_hid_expose_all(&lesstif_hid, &ctx);
		XSync(display, 0);
		time(&end);
		i++;
	}
	while (end - start < 10);

	printf("%g redraws per second\n", i / 10.0);

	main_pixmap = save_main;
	return 0;
	PCB_OLD_ACT_END;
}

static int pcb_act_Center(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x, y;

	pcb_hid_get_coords("Click on a location to center", &x, &y);

	x = pcb_grid_fit(x, PCB->Grid, PCB->GridOffsetX);
	y = pcb_grid_fit(y, PCB->Grid, PCB->GridOffsetY);
	view_left_x = x - (view_width * view_zoom) / 2;
	view_top_y = y - (view_height * view_zoom) / 2;
	lesstif_pan_fixup();
	/* Move the pointer to the center of the window, but only if it's
	   currently within the window already.  Watch out for edges,
	   though.  */
	XWarpPointer(display, window, window, 0, 0, view_width, view_height, Vx(x), Vy(y));
	return 0;
	PCB_OLD_ACT_END;
}

pcb_hid_action_t lesstif_main_action_list[] = {
	{"Zoom", pcb_act_Zoom, pcb_acth_Zoom, pcb_acts_Zoom},
	{"ZoomTo", pcb_act_ZoomTo, pcb_acth_ZoomTo, pcb_acts_ZoomTo},
	{"Pan", pcb_act_Pan, pcb_acth_Zoom, pcb_acts_Zoom},
	{"SwapSides", pcb_act_SwapSides, pcb_acth_SwapSides, pcb_acts_SwapSides},
	{"Command", pcb_act_Command, pcb_acth_Command, pcb_acts_Command},
	{"Benchmark", pcb_act_Benchmark, pcb_acth_Benchmark, pcb_acts_Benchmark},
	{"Center", pcb_act_Center}
};

PCB_REGISTER_ACTIONS(lesstif_main_action_list, lesstif_cookie)


/* ----------------------------------------------------------------------
 * redraws the background image
 */
		 static int bg_w, bg_h, bgi_w, bgi_h;
		 static Pixel **bg = 0;
		 static XImage *bgi = 0;
		 static enum {
			 PT_unknown,
			 PT_RGB565,
			 PT_RGB888
		 } pixel_type = PT_unknown;

		 static void
		   LoadBackgroundFile(FILE * f, char *filename)
{
	XVisualInfo vinfot, *vinfo;
	Visual *vis;
	int c, r, b;
	int i, nret;
	int p[3], rows, cols, maxval;

	if (fgetc(f) != 'P') {
		printf("bgimage: %s signature not P6\n", filename);
		return;
	}
	if (fgetc(f) != '6') {
		printf("bgimage: %s signature not P6\n", filename);
		return;
	}
	for (i = 0; i < 3; i++) {
		do {
			b = fgetc(f);
			if (feof(f))
				return;
			if (b == '#')
				while (!feof(f) && b != '\n')
					b = fgetc(f);
		} while (!isdigit(b));
		p[i] = b - '0';
		while (isdigit(b = fgetc(f)))
			p[i] = p[i] * 10 + b - '0';
	}
	bg_w = cols = p[0];
	bg_h = rows = p[1];
	maxval = p[2];

	setbuf(stdout, 0);
	bg = (Pixel **) malloc(rows * sizeof(Pixel *));
	if (!bg) {
		printf("Out of memory loading %s\n", filename);
		return;
	}
	for (i = 0; i < rows; i++) {
		bg[i] = (Pixel *) malloc(cols * sizeof(Pixel));
		if (!bg[i]) {
			printf("Out of memory loading %s\n", filename);
			while (--i >= 0)
				free(bg[i]);
			free(bg);
			bg = 0;
			return;
		}
	}

	vis = DefaultVisual(display, DefaultScreen(display));

	vinfot.visualid = XVisualIDFromVisual(vis);
	vinfo = XGetVisualInfo(display, VisualIDMask, &vinfot, &nret);

#if 0
	/* If you want to support more visuals below, you'll probably need
	   this. */
	printf("vinfo: rm %04x gm %04x bm %04x depth %d class %d\n",
				 vinfo->red_mask, vinfo->green_mask, vinfo->blue_mask, vinfo->depth, vinfo->class);
#endif

#if !defined(__cplusplus)
#define c_class class
#endif

	if (vinfo->c_class == TrueColor
			&& vinfo->depth == 16 && vinfo->red_mask == 0xf800 && vinfo->green_mask == 0x07e0 && vinfo->blue_mask == 0x001f)
		pixel_type = PT_RGB565;

	if (vinfo->c_class == TrueColor
			&& vinfo->depth == 24 && vinfo->red_mask == 0xff0000 && vinfo->green_mask == 0x00ff00 && vinfo->blue_mask == 0x0000ff)
		pixel_type = PT_RGB888;

	for (r = 0; r < rows; r++) {
		for (c = 0; c < cols; c++) {
			XColor pix;
			unsigned int pr = (unsigned) fgetc(f);
			unsigned int pg = (unsigned) fgetc(f);
			unsigned int pb = (unsigned) fgetc(f);

			switch (pixel_type) {
			case PT_unknown:
				pix.red = pr * 65535 / maxval;
				pix.green = pg * 65535 / maxval;
				pix.blue = pb * 65535 / maxval;
				pix.flags = DoRed | DoGreen | DoBlue;
				XAllocColor(display, lesstif_colormap, &pix);
				bg[r][c] = pix.pixel;
				break;
			case PT_RGB565:
				bg[r][c] = (pr >> 3) << 11 | (pg >> 2) << 5 | (pb >> 3);
				break;
			case PT_RGB888:
				bg[r][c] = (pr << 16) | (pg << 8) | (pb);
				break;
			}
		}
	}
}

void LoadBackgroundImage(char *filename)
{
	FILE *f = pcb_fopen(filename, "rb");
	if (!f) {
		if (PCB_NSTRCMP(filename, "pcb-background.ppm"))
			perror(filename);
		return;
	}
	LoadBackgroundFile(f, filename);
	fclose(f);
}

static void DrawBackgroundImage()
{
	int x, y, w, h;
	double xscale, yscale;
	int pcbwidth = PCB->MaxWidth / view_zoom;
	int pcbheight = PCB->MaxHeight / view_zoom;

	if (!window || !bg)
		return;

	if (!bgi || view_width != bgi_w || view_height != bgi_h) {
		if (bgi)
			XDestroyImage(bgi);
		/* Cheat - get the image, which sets up the format too.  */
		bgi = XGetImage(XtDisplay(work_area), window, 0, 0, view_width, view_height, -1, ZPixmap);
		bgi_w = view_width;
		bgi_h = view_height;
	}

	w = MIN(view_width, pcbwidth);
	h = MIN(view_height, pcbheight);

	xscale = (double) bg_w / PCB->MaxWidth;
	yscale = (double) bg_h / PCB->MaxHeight;

	for (y = 0; y < h; y++) {
		int pr = Py(y);
		int ir = pr * yscale;
		for (x = 0; x < w; x++) {
			int pc = Px(x);
			int ic = pc * xscale;
			XPutPixel(bgi, x, y, bg[ir][ic]);
		}
	}
	XPutImage(display, main_pixmap, bg_gc, bgi, 0, 0, 0, 0, w, h);
}

/* ---------------------------------------------------------------------- */

static pcb_hid_attribute_t *lesstif_get_export_options(int *n)
{
	if (n != NULL)
		*n = sizeof(lesstif_attribute_list) / sizeof(pcb_hid_attribute_t);
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
	if (view_left_x > PCB->MaxWidth + (view_width * view_zoom))
		view_left_x = PCB->MaxWidth + (view_width * view_zoom);
	if (view_top_y > PCB->MaxHeight + (view_height * view_zoom))
		view_top_y = PCB->MaxHeight + (view_height * view_zoom);
	if (view_left_x < -(view_width * view_zoom))
		view_left_x = -(view_width * view_zoom);
	if (view_top_y < -(view_height * view_zoom))
		view_top_y = -(view_height * view_zoom);

	set_scroll(hscroll, view_left_x, view_width, PCB->MaxWidth);
	set_scroll(vscroll, view_top_y, view_height, PCB->MaxHeight);

	lesstif_invalidate_all();
}

static void zoom_max()
{
	double new_zoom = PCB->MaxWidth / view_width;
	if (new_zoom < PCB->MaxHeight / view_height)
		new_zoom = PCB->MaxHeight / view_height;

	view_left_x = -(view_width * new_zoom - PCB->MaxWidth) / 2;
	view_top_y = -(view_height * new_zoom - PCB->MaxHeight) / 2;
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

	if (conf_core.editor.view.flip_x)
		xfrac = 1 - xfrac;
	if (conf_core.editor.view.flip_y)
		yfrac = 1 - yfrac;

	max_zoom = PCB->MaxWidth / view_width;
	if (max_zoom < PCB->MaxHeight / view_height)
		max_zoom = PCB->MaxHeight / view_height;

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

static void zoom_win(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
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
}

static void zoom_toggle(pcb_coord_t x, pcb_coord_t y)
{
	double tmp;

	tmp = prev_view_zoom;
	prev_view_zoom = view_zoom;
	zoom_to(tmp, x, y);
}

void zoom_by(double factor, pcb_coord_t x, pcb_coord_t y)
{
	zoom_to(view_zoom * factor, x, y);
}

static int panning = 0;
static int shift_pressed;
static int ctrl_pressed;
static int alt_pressed;

/* X and Y are in screen coordinates.  */
static void Pan(int mode, pcb_coord_t x, pcb_coord_t y)
{
	static pcb_coord_t ox, oy;
	static pcb_coord_t opx, opy;

	panning = mode;
	/* This is for ctrl-pan, where the viewport's position is directly
	   proportional to the cursor position in the window (like the Xaw
	   thumb panner) */
	if (pan_thumb_mode) {
		opx = x * PCB->MaxWidth / view_width;
		opy = y * PCB->MaxHeight / view_height;
		if (conf_core.editor.view.flip_x)
			opx = PCB->MaxWidth - opx;
		if (conf_core.editor.view.flip_y)
			opy = PCB->MaxHeight - opy;
		view_left_x = opx - view_width / 2 * view_zoom;
		view_top_y = opy - view_height / 2 * view_zoom;
		lesstif_pan_fixup();
	}
	/* This is the start of a regular pan.  On the first click, we
	   remember the coordinates where we "grabbed" the screen.  */
	else if (mode == 1) {
		ox = x;
		oy = y;
		opx = view_left_x;
		opy = view_top_y;
	}
	/* continued drag, we calculate how far we've moved the cursor and
	   set the position accordingly.  */
	else {
		if (conf_core.editor.view.flip_x)
			view_left_x = opx + (x - ox) * view_zoom;
		else
			view_left_x = opx - (x - ox) * view_zoom;
		if (conf_core.editor.view.flip_y)
			view_top_y = opy + (y - oy) * view_zoom;
		else
			view_top_y = opy - (y - oy) * view_zoom;
		lesstif_pan_fixup();
	}
}

static void mod_changed(XKeyEvent * e, int set)
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
		return;
	}
	in_move_event = 1;
	pcb_notify_crosshair_change(pcb_false);
	if (panning)
		Pan(2, e->x, e->y);
	pcb_event_move_crosshair(Px(e->x), Py(e->y));
	pcb_adjust_attached_objects();
	pcb_notify_crosshair_change(pcb_true);
	in_move_event = 0;
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

			pcb_notify_crosshair_change(pcb_false);
			pressed_button = e->xbutton.button;
			mods = ((e->xbutton.state & ShiftMask) ? PCB_M_Shift : 0)
				+ ((e->xbutton.state & ControlMask) ? PCB_M_Ctrl : 0)
#ifdef __APPLE__
				+ ((e->xbutton.state & (1 << 13)) ? PCB_M_Alt : 0);
#else
				+ ((e->xbutton.state & Mod1Mask) ? PCB_M_Alt : 0);
#endif
			hid_cfg_mouse_action(&lesstif_mouse, lesstif_mb2cfg(e->xbutton.button) | mods);

			pcb_notify_crosshair_change(pcb_true);
			break;
		}

	case ButtonRelease:
		{
			int mods;
			if (e->xbutton.button != pressed_button)
				return;
			lesstif_button_event(w, e);
			pcb_notify_crosshair_change(pcb_false);
			pressed_button = 0;
			mods = ((e->xbutton.state & ShiftMask) ? PCB_M_Shift : 0)
				+ ((e->xbutton.state & ControlMask) ? PCB_M_Ctrl : 0)
#ifdef __APPLE__
				+ ((e->xbutton.state & (1 << 13)) ? PCB_M_Alt : 0)
#else
				+ ((e->xbutton.state & Mod1Mask) ? PCB_M_Alt : 0)
#endif
				+ PCB_M_Release;
			hid_cfg_mouse_action(&lesstif_mouse, lesstif_mb2cfg(e->xbutton.button) | mods);
			pcb_notify_crosshair_change(pcb_true);
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
			pcb_event_move_crosshair(Px(pos_x), Py(pos_y));
			in_move_event = 0;
		}
		break;

	case LeaveNotify:
		crosshair_in_window = 0;
		if (crosshair_on)
			pcb_draw_attached(1);
		pcb_draw_mark(1);
		ShowCrosshair(pcb_false);
		need_idle_proc();
		break;

	case EnterNotify:
		crosshair_in_window = 1;
		in_move_event = 1;
		pcb_event_move_crosshair(Px(e->xcrossing.x), Py(e->xcrossing.y));
		ShowCrosshair(pcb_true);
		in_move_event = 0;
		need_redraw = 1;
		need_idle_proc();
		break;

	default:
		printf("work_area: unknown event %d\n", e->type);
		break;
	}
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
	if (pcb_crosshair.shape == pcb_ch_shape_union_jack)
		draw_slanted_cross(xor_gc, x, y, view_width, view_height);
	if (pcb_crosshair.shape == pcb_ch_shape_dozen)
		draw_dozen_cross(xor_gc, x, y, view_width, view_height);
}

void lesstif_show_crosshair(int show)
{
	static int showing = 0;
	static int sx, sy;
	static GC xor_gc = 0;
	Pixel crosshair_color;

	if (!crosshair_in_window || !window)
		return;
	if (xor_gc == 0) {
		crosshair_color = lesstif_parse_color(conf_core.appearance.color.crosshair) ^ bgcolor;
		xor_gc = XCreateGC(display, window, 0, 0);
		XSetFunction(display, xor_gc, GXxor);
		XSetForeground(display, xor_gc, crosshair_color);
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
	lesstif_invalidate_all();
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
		main_picture = XRenderCreatePicture(display, main_pixmap,
																				XRenderFindVisualFormat(display, DefaultVisual(display, screen)), 0, 0);
		mask_picture = XRenderCreatePicture(display, mask_pixmap,
																				XRenderFindVisualFormat(display, DefaultVisual(display, screen)), 0, 0);
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
	int c;
	Dimension width, height;
	static char dashes[] = { 4, 4 };

	window = XtWindow(work_area);
	my_gc = XCreateGC(display, window, 0, 0);

	arc1_gc = XCreateGC(display, window, 0, 0);
	c = lesstif_parse_color("#804000");
	XSetForeground(display, arc1_gc, c);
	arc2_gc = XCreateGC(display, window, 0, 0);
	c = lesstif_parse_color("#004080");
	XSetForeground(display, arc2_gc, c);
	XSetLineAttributes(display, arc1_gc, 1, LineOnOffDash, 0, 0);
	XSetLineAttributes(display, arc2_gc, 1, LineOnOffDash, 0, 0);
	XSetDashes(display, arc1_gc, 0, dashes, 2);
	XSetDashes(display, arc2_gc, 0, dashes, 2);

	stdarg_n = 0;
	stdarg(XtNwidth, &width);
	stdarg(XtNheight, &height);
	stdarg(XmNbackground, &bgcolor);
	XtGetValues(work_area, stdarg_args, stdarg_n);
	view_width = width;
	view_height = height;

	offlimit_color = lesstif_parse_color(conf_core.appearance.color.off_limit);
	grid_color = lesstif_parse_color(conf_core.appearance.color.grid);

	bg_gc = XCreateGC(display, window, 0, 0);
	XSetForeground(display, bg_gc, bgcolor);

	work_area_make_pixmaps(width, height);

#ifdef HAVE_XRENDER
	if (use_xrender) {
		double l_alpha = conf_core.appearance.layer_alpha;
		XRenderPictureAttributes pa;
		XRenderColor a = { 0, 0, 0,  0x8000};
		
		if (l_alpha < 0)
			l_alpha = 0;
		else if (l_alpha > 1)
			l_alpha = 1;
		a.alpha = (int)(l_alpha * (double)0xFFFF);

		pale_pixmap = XCreatePixmap(display, window, 1, 1, 8);
		pa.repeat = True;
		pale_picture = XRenderCreatePicture(display, pale_pixmap,
																				XRenderFindStandardFormat(display, PictStandardA8), CPRepeat, &pa);
		if (pale_picture)
			XRenderFillRectangle(display, PictOpSrc, pale_picture, &a, 0, 0, 1, 1);
		else
			use_xrender = 0;
	}
#endif /* HAVE_XRENDER */

	clip_gc = XCreateGC(display, window, 0, 0);
	bset_gc = XCreateGC(display, mask_bitmap, 0, 0);
	XSetForeground(display, bset_gc, 1);
	bclear_gc = XCreateGC(display, mask_bitmap, 0, 0);
	XSetForeground(display, bclear_gc, 0);

	XtRemoveCallback(work_area, XmNexposeCallback, (XtCallbackProc) work_area_first_expose, 0);
	XtAddCallback(work_area, XmNexposeCallback, (XtCallbackProc) work_area_expose, 0);
	lesstif_invalidate_all();
}

static Widget make_message(const char *name, Widget left, int resizeable)
{
	Widget w, f;
	stdarg_n = 0;
	if (left) {
		stdarg(XmNleftAttachment, XmATTACH_WIDGET);
		stdarg(XmNleftWidget, XtParent(left));
	}
	else {
		stdarg(XmNleftAttachment, XmATTACH_FORM);
	}
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNshadowType, XmSHADOW_IN);
	stdarg(XmNshadowThickness, 1);
	stdarg(XmNalignment, XmALIGNMENT_CENTER);
	stdarg(XmNmarginWidth, 4);
	stdarg(XmNmarginHeight, 1);
	if (!resizeable)
		stdarg(XmNresizePolicy, XmRESIZE_GROW);
	f = XmCreateForm(messages, XmStrCast(name), stdarg_args, stdarg_n);
	XtManageChild(f);
	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	w = XmCreateLabel(f, XmStrCast(name), stdarg_args, stdarg_n);
	XtManageChild(w);
	return w;
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
#warning TODO#3: do not ingore key_tr (either of them is 0)
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

static void lesstif_do_export(pcb_hid_attr_val_t * options)
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
	stdarg_do_color(conf_core.appearance.color.background, XmNbackground);
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
	stdarg(XmNmaximum, PCB->MaxHeight ? PCB->MaxHeight : 1);
	vscroll = XmCreateScrollBar(mainwind, XmStrCast("vscroll"), stdarg_args, stdarg_n);
	XtAddCallback(vscroll, XmNvalueChangedCallback, (XtCallbackProc) scroll_callback, (XtPointer) & view_top_y);
	XtAddCallback(vscroll, XmNdragCallback, (XtCallbackProc) scroll_callback, (XtPointer) & view_top_y);
	XtManageChild(vscroll);

	stdarg_n = 0;
	stdarg(XmNorientation, XmHORIZONTAL);
	stdarg(XmNmaximum, PCB->MaxWidth ? PCB->MaxWidth : 1);
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
	stdarg(XmNlabelString, XmStringCreatePCB("Command: "));
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
	XtAddEventHandler(m_cmd, KeyPressMask, 0, command_event_handler, 0);

	m_mark = make_message("m_mark", 0, 0);
	m_crosshair = make_message("m_crosshair", m_mark, 0);
	m_grid = make_message("m_grid", m_crosshair, 1);
	m_zoom = make_message("m_zoom", m_grid, 1);
	lesstif_m_layer = make_message("m_layer", m_zoom, 0);
	m_mode = make_message("m_mode", lesstif_m_layer, 1);
	m_rats = make_message("m_rats", m_mode, 1);
	m_status = make_message("m_status", m_mode, 1);

	XtUnmanageChild(XtParent(m_mark));
	XtUnmanageChild(XtParent(m_rats));

	stdarg_n = 0;
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	XtSetValues(XtParent(m_status), stdarg_args, stdarg_n);

	/* We'll use this later.  */
	stdarg_n = 0;
	stdarg(XmNleftWidget, XtParent(m_mark));
	XtSetValues(XtParent(m_crosshair), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNmessageWindow, messages);
	XtSetValues(mainwind, stdarg_args, stdarg_n);

	if (background_image_file)
		LoadBackgroundImage(background_image_file);

	XtRealizeWidget(appwidget);

	while (!window) {
		XEvent e;
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}

	pcb_board_changed(0);

	lesstif_menubar = menu;
	pcb_event(PCB_EVENT_GUI_INIT, NULL);


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
		lesstif_hid_inited = 0;
	}
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
	pcb_hid_action("Quit");
}

static void lesstif_listener_cb(XtPointer client_data, int *fid, XtInputId * id)
{
	char buf[BUFSIZ];
	int nbytes;

	if ((nbytes = read(*fid, buf, BUFSIZ)) == -1)
		perror("lesstif_listener_cb");

	if (nbytes) {
		buf[nbytes] = '\0';
		pcb_hid_parse_actions(buf);
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


static int lesstif_parse_arguments(int *argc, char ***argv)
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
			pcb_hid_attribute_t *a = ha->attributes + i;
			switch (a->type) {
			case PCB_HATT_INTEGER:
			case PCB_HATT_COORD:
			case PCB_HATT_REAL:
			case PCB_HATT_STRING:
			case PCB_HATT_PATH:
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

#if 0
	rmax = rcount + XtNumber(lesstif_resources);
#else
	rmax = rcount;
#endif

	new_resources = (XtResource *) malloc((rmax + 1) * sizeof(XtResource));
	new_values = (val_union *) malloc((rmax + 1) * sizeof(val_union));
#if 0
	memcpy(new_resources + acount, lesstif_resources, sizeof(lesstif_resources));
#endif
	rcount = 0;

	for (ha = hid_attr_nodes; ha; ha = ha->next)
		for (i = 0; i < ha->n; i++) {
			pcb_hid_attribute_t *a = ha->attributes + i;
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
			case PCB_HATT_PATH:
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
				r->default_addr = &(a->default_val.int_value);
				rcount++;
				break;
			case PCB_HATT_COORD:
				r->resource_type = XmStrCast(XtRPCBCoord);
				r->default_type = XmStrCast(XtRPCBCoord);
				r->resource_size = sizeof(pcb_coord_t);
				r->default_addr = &(a->default_val.coord_value);
				rcount++;
				break;
			case PCB_HATT_REAL:
				r->resource_type = XmStrCast(XtRDouble);
				r->default_type = XmStrCast(XtRDouble);
				r->resource_size = sizeof(double);
				r->default_addr = &(a->default_val.real_value);
				rcount++;
				break;
			case PCB_HATT_STRING:
			case PCB_HATT_PATH:
				r->resource_type = XtRString;
				r->default_type = XtRString;
				r->resource_size = sizeof(char *);
				r->default_addr = (char *) a->default_val.str_value;
				rcount++;
				break;
			case PCB_HATT_BOOL:
				r->resource_type = XtRBoolean;
				r->default_type = XtRInt;
				r->resource_size = sizeof(int);
				r->default_addr = &(a->default_val.int_value);
				rcount++;
				break;
			default:
				break;
			}
		}
#if 0
	for (i = 0; i < XtNumber(lesstif_resources); i++) {
		XtResource *r = new_resources + rcount;
		r->resource_offset = sizeof(val_union) * rcount;
		rcount++;
	}
#endif

	stdarg_n = 0;
	stdarg(XmNdeleteResponse, XmDO_NOTHING);

	XtSetErrorMsgHandler(lesstif_err_msg);
	err = setjmp(lesstif_err_jmp);
	if (err != 0)
		return err;
	appwidget = XtAppInitialize(&app_context, "Pcb", new_options, amax, argc, *argv, 0, stdarg_args, stdarg_n);
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
			pcb_hid_attribute_t *a = ha->attributes + i;
			val_union *v = new_values + rcount;
			switch (a->type) {
			case PCB_HATT_INTEGER:
				if (a->value)
					*(int *) a->value = v->i;
				else
					a->default_val.int_value = v->i;
				rcount++;
				break;
			case PCB_HATT_COORD:
				if (a->value)
					*(pcb_coord_t *) a->value = v->c;
				else
					a->default_val.coord_value = v->c;
				rcount++;
				break;
			case PCB_HATT_BOOL:
				if (a->value)
					*(char *) a->value = v->i;
				else
					a->default_val.int_value = v->i;
				rcount++;
				break;
			case PCB_HATT_REAL:
				if (a->value)
					*(double *) a->value = v->f;
				else
					a->default_val.real_value = v->f;
				rcount++;
				break;
			case PCB_HATT_STRING:
			case PCB_HATT_PATH:
				if (a->value)
					*(char **) a->value = v->s;
				else
					a->default_val.str_value = v->s;
				rcount++;
				break;
			default:
				break;
			}
		}

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

	if (!conf_core.editor.draw_grid)
		return;
	if (Vz(PCB->Grid) < PCB_MIN_GRID_DISTANCE)
		return;
	if (!grid_gc) {
		grid_gc = XCreateGC(display, window, 0, 0);
		XSetFunction(display, grid_gc, GXxor);
		XSetForeground(display, grid_gc, grid_color);
	}
	if (conf_core.editor.view.flip_x) {
		x2 = pcb_grid_fit(Px(0), PCB->Grid, PCB->GridOffsetX);
		x1 = pcb_grid_fit(Px(view_width), PCB->Grid, PCB->GridOffsetX);
		if (Vx(x2) < 0)
			x2 -= PCB->Grid;
		if (Vx(x1) >= view_width)
			x1 += PCB->Grid;
	}
	else {
		x1 = pcb_grid_fit(Px(0), PCB->Grid, PCB->GridOffsetX);
		x2 = pcb_grid_fit(Px(view_width), PCB->Grid, PCB->GridOffsetX);
		if (Vx(x1) < 0)
			x1 += PCB->Grid;
		if (Vx(x2) >= view_width)
			x2 -= PCB->Grid;
	}
	if (conf_core.editor.view.flip_y) {
		y2 = pcb_grid_fit(Py(0), PCB->Grid, PCB->GridOffsetY);
		y1 = pcb_grid_fit(Py(view_height), PCB->Grid, PCB->GridOffsetY);
		if (Vy(y2) < 0)
			y2 -= PCB->Grid;
		if (Vy(y1) >= view_height)
			y1 += PCB->Grid;
	}
	else {
		y1 = pcb_grid_fit(Py(0), PCB->Grid, PCB->GridOffsetY);
		y2 = pcb_grid_fit(Py(view_height), PCB->Grid, PCB->GridOffsetY);
		if (Vy(y1) < 0)
			y1 += PCB->Grid;
		if (Vy(y2) >= view_height)
			y2 -= PCB->Grid;
	}
	n = (x2 - x1) / PCB->Grid + 1;
	if (n > npoints) {
		npoints = n + 10;
		points = (XPoint *) realloc(points, npoints * sizeof(XPoint));
	}
	n = 0;
	prevx = 0;
	for (x = x1; x <= x2; x += PCB->Grid) {
		int temp = Vx(x);
		points[n].x = temp;
		if (n) {
			points[n].x -= prevx;
			points[n].y = 0;
		}
		prevx = temp;
		n++;
	}
	for (y = y1; y <= y2; y += PCB->Grid) {
		int vy = Vy(y);
		points[0].y = vy;
		XDrawPoints(display, pixmap, grid_gc, points, n, CoordModePrevious);
	}
}

static void mark_delta_to_widget(pcb_coord_t dx, pcb_coord_t dy, Widget w)
{
	char *buf;
	double g = pcb_coord_to_unit(conf_core.editor.grid_unit, PCB->Grid);
	int prec;
	XmString ms;

	/* Integer-sized grid? */
	if (((int) (g * 10000 + 0.5) % 10000) == 0)
		prec = 0;
	else
		prec = conf_core.editor.grid_unit->default_prec;

	if (dx == 0 && dy == 0)
		buf = pcb_strdup_printf("%m+%+.*mS, %+.*mS", UUNIT, prec, dx, prec, dy);
	else {
		pcb_angle_t angle = atan2(dy, -dx) * 180 / M_PI;
		pcb_coord_t dist = pcb_distance(0, 0, dx, dy);

		buf = pcb_strdup_printf("%m+%+.*mS, %+.*mS (%.*mS, %.2f\260)", UUNIT, prec, dx, prec, dy, prec, dist, angle);
	}

	ms = XmStringCreatePCB(buf);
	stdarg_n = 0;
	stdarg(XmNlabelString, ms);
	XtSetValues(w, stdarg_args, stdarg_n);
	free(buf);
}

static int cursor_pos_to_widget(pcb_coord_t x, pcb_coord_t y, Widget w, int prev_state)
{
	int this_state = prev_state;
	char *buf = NULL;
	const char *msg = "";
	double g = pcb_coord_to_unit(conf_core.editor.grid_unit, PCB->Grid);
	XmString ms;
	int prec;

	/* Determine necessary precision (and state) based
	 * on the user's grid setting */
	if (((int) (g * 10000 + 0.5) % 10000) == 0) {
		prec = 0;
		this_state = conf_core.editor.grid_unit->allow;
	}
	else {
		prec = conf_core.editor.grid_unit->default_prec;
		this_state = -conf_core.editor.grid_unit->allow;
	}

	if (x >= 0) {
		buf = pcb_strdup_printf("%m+%.*mS, %.*mS", UUNIT, prec, x, prec, y);
		msg = buf;
	}

	ms = XmStringCreatePCB(msg);
	stdarg_n = 0;
	stdarg(XmNlabelString, ms);
	XtSetValues(w, stdarg_args, stdarg_n);
	free(buf);
	return this_state;
}

void lesstif_update_status_line()
{
	const char *msg = "";
	char *buf = NULL;
	const char *s45 = cur_clip();
	XmString xs;

	switch (conf_core.editor.mode) {
	case PCB_MODE_VIA:
		buf = pcb_strdup_printf("%m+%.2mS/%.2mS \370=%.2mS", UUNIT, conf_core.design.via_thickness, conf_core.design.clearance, conf_core.design.via_drilling_hole);
		break;
	case PCB_MODE_LINE:
	case PCB_MODE_ARC:
		buf = pcb_strdup_printf("%m+%.2mS/%.2mS %s", UUNIT, conf_core.design.line_thickness, conf_core.design.clearance, s45);
		break;
	case PCB_MODE_RECTANGLE:
	case PCB_MODE_POLYGON:
		buf = pcb_strdup_printf("%m+%.2mS %s", UUNIT, conf_core.design.clearance, s45);
		break;
	case PCB_MODE_TEXT:
		buf = pcb_strdup_printf("%d %%", conf_core.design.text_scale);
		break;
	case PCB_MODE_MOVE:
	case PCB_MODE_COPY:
	case PCB_MODE_INSERT_POINT:
	case PCB_MODE_RUBBERBAND_MOVE:
		if (s45 != NULL)
			buf = pcb_strdup(s45);
		break;
	case PCB_MODE_NO:
	case PCB_MODE_PASTE_BUFFER:
	case PCB_MODE_ROTATE:
	case PCB_MODE_REMOVE:
	case PCB_MODE_THERMAL:
	case PCB_MODE_ARROW:
	case PCB_MODE_LOCK:
	default:
		break;
	}

	if (buf != NULL) {
		msg = buf;
	}
	xs = XmStringCreatePCB(msg);
	stdarg_n = 0;
	stdarg(XmNlabelString, xs);
	XtSetValues(m_status, stdarg_args, stdarg_n);
	free(buf);
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
		if (conf_core.editor.view.flip_x) {
			pcb_coord_t tmp = ctx.view.X1;
			ctx.view.X1 = ctx.view.X2;
			ctx.view.X2 = tmp;
		}
		if (conf_core.editor.view.flip_y) {
			pcb_coord_t tmp = ctx.view.Y1;
			ctx.view.Y1 = ctx.view.Y2;
			ctx.view.Y2 = tmp;
		}
		XSetForeground(display, bg_gc, bgcolor);
		XFillRectangle(display, main_pixmap, bg_gc, 0, 0, mx, my);

		if (ctx.view.X1 < 0 || ctx.view.Y1 < 0 || ctx.view.X2 > PCB->MaxWidth || ctx.view.Y2 > PCB->MaxHeight) {
			int leftmost, rightmost, topmost, bottommost;

			leftmost = Vx(0);
			rightmost = Vx(PCB->MaxWidth);
			topmost = Vy(0);
			bottommost = Vy(PCB->MaxHeight);
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
		pcb_hid_expose_all(&lesstif_hid, &ctx);
		lesstif_drawing_mode = PCB_HID_COMP_POSITIVE;
		draw_grid();
		show_crosshair(0);					/* To keep the drawn / not drawn info correct */
		XSetFunction(display, my_gc, GXcopy);
		XCopyArea(display, main_pixmap, window, my_gc, 0, 0, view_width, view_height, 0, 0);
		pixmap = window;
		need_redraw = 0;
		if (crosshair_on) {
			pcb_draw_attached(1);
			pcb_draw_mark(1);
		}
	}

	{
		static int c_x = -2, c_y = -2;
		static pcb_mark_t saved_mark;
		static const pcb_unit_t *old_grid_unit = NULL;
		if (crosshair_x != c_x || crosshair_y != c_y
				|| conf_core.editor.grid_unit != old_grid_unit || memcmp(&saved_mark, &pcb_marked, sizeof(pcb_mark_t))) {
			static int last_state = 0;
			static int this_state = 0;

			c_x = crosshair_x;
			c_y = crosshair_y;

			this_state = cursor_pos_to_widget(crosshair_x, crosshair_y, m_crosshair, this_state);
			if (pcb_marked.status)
				mark_delta_to_widget(crosshair_x - pcb_marked.X, crosshair_y - pcb_marked.Y, m_mark);

			if (pcb_marked.status != saved_mark.status) {
				if (pcb_marked.status) {
					XtManageChild(XtParent(m_mark));
					XtManageChild(m_mark);
					stdarg_n = 0;
					stdarg(XmNleftAttachment, XmATTACH_WIDGET);
					stdarg(XmNleftWidget, XtParent(m_mark));
					XtSetValues(XtParent(m_crosshair), stdarg_args, stdarg_n);
				}
				else {
					stdarg_n = 0;
					stdarg(XmNleftAttachment, XmATTACH_FORM);
					XtSetValues(XtParent(m_crosshair), stdarg_args, stdarg_n);
					XtUnmanageChild(XtParent(m_mark));
				}
				last_state = this_state + 100;
			}
			memcpy(&saved_mark, &pcb_marked, sizeof(pcb_mark_t));

			if (old_grid_unit != conf_core.editor.grid_unit) {
				old_grid_unit = conf_core.editor.grid_unit;
				/* Force a resize on units change.  */
				last_state++;
			}

			/* This is obtuse.  We want to enable XmRESIZE_ANY long enough
			   to shrink to fit the new format (if any), then switch it
			   back to XmRESIZE_GROW to prevent it from shrinking due to
			   changes in the number of actual digits printed.  Thus, when
			   you switch from a small grid and %.2f formats to a large
			   grid and %d formats, you aren't punished with a wide and
			   mostly white-space label widget.  "this_state" indicates
			   which of the above formats we're using.  "last_state" is
			   either zero (when resizing) or the same as "this_state"
			   (when grow-only), or a non-zero but not "this_state" which
			   means we need to start a resize cycle.  */
			if (this_state != last_state && last_state) {
				stdarg_n = 0;
				stdarg(XmNresizePolicy, XmRESIZE_ANY);
				XtSetValues(XtParent(m_mark), stdarg_args, stdarg_n);
				XtSetValues(XtParent(m_crosshair), stdarg_args, stdarg_n);
				last_state = 0;
			}
			else if (this_state != last_state) {
				stdarg_n = 0;
				stdarg(XmNresizePolicy, XmRESIZE_GROW);
				XtSetValues(XtParent(m_mark), stdarg_args, stdarg_n);
				XtSetValues(XtParent(m_crosshair), stdarg_args, stdarg_n);
				last_state = this_state;
			}
		}
	}

	{
		static pcb_coord_t old_grid = -1;
		static pcb_coord_t old_gx, old_gy;
		static const pcb_unit_t *old_unit;
		XmString ms;
		if (PCB->Grid != old_grid || PCB->GridOffsetX != old_gx || PCB->GridOffsetY != old_gy || conf_core.editor.grid_unit != old_unit) {
			static char buf[100];
			old_grid = PCB->Grid;
			old_unit = conf_core.editor.grid_unit;
			old_gx = PCB->GridOffsetX;
			old_gy = PCB->GridOffsetY;
			if (old_grid == 1) {
				strcpy(buf, "No Grid");
			}
			else {
				if (old_gx || old_gy)
					pcb_snprintf(buf, sizeof(buf), "%m+%$mS @%mS,%mS", UUNIT, old_grid, old_gx, old_gy);
				else
					pcb_snprintf(buf, sizeof(buf), "%m+%$mS", UUNIT, old_grid);
			}
			ms = XmStringCreatePCB(buf);
			stdarg_n = 0;
			stdarg(XmNlabelString, ms);
			XtSetValues(m_grid, stdarg_args, stdarg_n);
		}
	}

	{
		static double old_zoom = -1;
		static const pcb_unit_t *old_grid_unit = NULL;
		if (view_zoom != old_zoom || conf_core.editor.grid_unit != old_grid_unit) {
			char *buf = pcb_strdup_printf("%m+%$mS/pix",
																			 conf_core.editor.grid_unit->allow, (pcb_coord_t) view_zoom);
			XmString ms;

			old_zoom = view_zoom;
			old_grid_unit = conf_core.editor.grid_unit;

			ms = XmStringCreatePCB(buf);
			stdarg_n = 0;
			stdarg(XmNlabelString, ms);
			XtSetValues(m_zoom, stdarg_args, stdarg_n);
			free(buf);
		}
	}

	{
		if (old_cursor_mode != conf_core.editor.mode) {
			const char *s = "None";
			XmString ms;
			int cursor = -1;
			static int free_cursor = 0;

			old_cursor_mode = conf_core.editor.mode;
			switch (conf_core.editor.mode) {
			case PCB_MODE_NO:
				s = "None";
				cursor = XC_X_cursor;
				break;
			case PCB_MODE_VIA:
				s = "Via";
				cursor = -1;
				break;
			case PCB_MODE_LINE:
				s = "Line";
				cursor = XC_pencil;
				break;
			case PCB_MODE_RECTANGLE:
				s = "Rectangle";
				cursor = XC_ul_angle;
				break;
			case PCB_MODE_POLYGON:
				s = "Polygon";
				cursor = XC_sb_up_arrow;
				break;
			case PCB_MODE_POLYGON_HOLE:
				s = "Polygon Hole";
				cursor = XC_sb_up_arrow;
				break;
			case PCB_MODE_PASTE_BUFFER:
				s = "Paste";
				cursor = XC_hand1;
				break;
			case PCB_MODE_TEXT:
				s = "Text";
				cursor = XC_xterm;
				break;
			case PCB_MODE_ROTATE:
				s = "Rotate";
				cursor = XC_exchange;
				break;
			case PCB_MODE_REMOVE:
				s = "Remove";
				cursor = XC_pirate;
				break;
			case PCB_MODE_MOVE:
				s = "Move";
				cursor = XC_crosshair;
				break;
			case PCB_MODE_COPY:
				s = "Copy";
				cursor = XC_crosshair;
				break;
			case PCB_MODE_INSERT_POINT:
				s = "Insert";
				cursor = XC_dotbox;
				break;
			case PCB_MODE_RUBBERBAND_MOVE:
				s = "RBMove";
				cursor = XC_top_left_corner;
				break;
			case PCB_MODE_THERMAL:
				s = "Thermal";
				cursor = XC_iron_cross;
				break;
			case PCB_MODE_ARC:
				s = "Arc";
				cursor = XC_question_arrow;
				break;
			case PCB_MODE_ARROW:
				s = "Arrow";
				if (over_point)
					cursor = XC_draped_box;
				else
					cursor = XC_left_ptr;
				break;
			case PCB_MODE_LOCK:
				s = "Lock";
				cursor = XC_hand2;
				break;
			}
			ms = XmStringCreatePCB(s);
			stdarg_n = 0;
			stdarg(XmNlabelString, ms);
			XtSetValues(m_mode, stdarg_args, stdarg_n);

			if (free_cursor) {
				XFreeCursor(display, my_cursor);
				free_cursor = 0;
			}
			if (cursor == -1) {
				static Pixmap nocur_source = 0;
				static Pixmap nocur_mask = 0;
				static Cursor nocursor = 0;
				if (nocur_source == 0) {
					XColor fg, bg;
					nocur_source = XCreateBitmapFromData(display, window, "\0", 1, 1);
					nocur_mask = XCreateBitmapFromData(display, window, "\0", 1, 1);

					fg.red = fg.green = fg.blue = 65535;
					bg.red = bg.green = bg.blue = 0;
					fg.flags = bg.flags = DoRed | DoGreen | DoBlue;
					nocursor = XCreatePixmapCursor(display, nocur_source, nocur_mask, &fg, &bg, 0, 0);
				}
				my_cursor = nocursor;
			}
			else {
				my_cursor = XCreateFontCursor(display, cursor);
				free_cursor = 1;
			}
			XDefineCursor(display, window, my_cursor);
			lesstif_update_status_line();
		}
	}
	{
		static const char *old_clip = NULL;
		static int old_tscale = -1;
		const char *new_clip = cur_clip();

		if (new_clip != old_clip || conf_core.design.text_scale != old_tscale) {
			lesstif_update_status_line();
			old_clip = new_clip;
			old_tscale = conf_core.design.text_scale;
		}
	}

	{
		static int old_nrats = -1;
		static char buf[20];

		if (old_nrats != ratlist_length(&PCB->Data->Rat)) {
			old_nrats = ratlist_length(&PCB->Data->Rat);
			sprintf(buf, "%ld rat%s", (long)ratlist_length(&PCB->Data->Rat), ratlist_length(&PCB->Data->Rat) == 1 ? "" : "s");
			if (ratlist_length(&PCB->Data->Rat)) {
				XtManageChild(XtParent(m_rats));
				XtManageChild(m_rats);
				stdarg_n = 0;
				stdarg(XmNleftWidget, m_rats);
				XtSetValues(XtParent(m_status), stdarg_args, stdarg_n);
			}

			stdarg_n = 0;
			stdarg(XmNlabelString, XmStringCreatePCB(buf));
			XtSetValues(m_rats, stdarg_args, stdarg_n);

			if (!ratlist_length(&PCB->Data->Rat)) {
				stdarg_n = 0;
				stdarg(XmNleftWidget, m_mode);
				XtSetValues(XtParent(m_status), stdarg_args, stdarg_n);
				XtUnmanageChild(XtParent(m_rats));
			}
		}
	}

#warning TODO: remove this, update-on should handle all cases
	lesstif_update_widget_flags(NULL);

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

static void lesstif_invalidate_lr(pcb_coord_t l, pcb_coord_t r, pcb_coord_t t, pcb_coord_t b)
{
	if (!window)
		return;

	need_redraw = 1;
	need_idle_proc();
}

void lesstif_invalidate_all(void)
{
	lesstif_invalidate_lr(0, PCB->MaxWidth, 0, PCB->MaxHeight);
}

static void lesstif_notify_crosshair_change(pcb_bool changes_complete)
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
		pcb_draw_attached(1);
		pixmap = save_pixmap;
	}

	if (!changes_complete)
		invalidate_depth++;
}

static void lesstif_notify_mark_change(pcb_bool changes_complete)
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
		pcb_draw_mark(1);
		pixmap = save_pixmap;
	}

	if (!changes_complete)
		invalidate_depth++;
}

static int lesstif_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	int idx = group;
	if (idx >= 0 && idx < pcb_max_group(PCB)) {
		int n = PCB->LayerGroups.grp[group].len;
		for (idx = 0; idx < n - 1; idx++) {
			int ni = PCB->LayerGroups.grp[group].lid[idx];
			if (ni >= 0 && ni < pcb_max_layer && PCB->Data->Layer[ni].meta.real.vis)
				break;
		}
		idx = PCB->LayerGroups.grp[group].lid[idx];
#if 0
		if (idx == pcb_layer_stack[0]
				|| pcb_layer_get_group(PCB, idx) == pcb_layer_get_group(PCB, pcb_layer_stack[0]))
			autofade = 0;
		else
			autofade = 1;
#endif
	}
#if 0
	else
		autofade = 0;
#endif

	if ((flags & PCB_LYT_MASK) || (flags & PCB_LYT_PASTE)) {
		if (pinout)
			return 0;
		return (PCB->Data->Layer[idx].meta.real.vis && PCB_LAYERFLG_ON_VISIBLE_SIDE(flags));
	}

	if (flags & PCB_LYT_COPPER)
		return pinout ? 1 : PCB->Data->Layer[idx].meta.real.vis;

	/* virtual layers */
	{
		switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_INVIS:
			return pinout ? 0 : PCB->InvisibleObjectsOn;
		case PCB_LYT_SILK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_silk_on(PCB);
			return 0;
		case PCB_LYT_ASSY:
			return 0;
		case PCB_LYT_UDRILL:
		case PCB_LYT_PDRILL:
			return 1;
		case PCB_LYT_UI:
			return 1;
		case PCB_LYT_RAT:
			return PCB->RatOn;
		}
	}
	return 0;
}

static pcb_hid_gc_t lesstif_make_gc(void)
{
	pcb_hid_gc_t rv = (hid_gc_s *) malloc(sizeof(hid_gc_s));
	memset(rv, 0, sizeof(hid_gc_s));
	rv->me_pointer = &lesstif_hid;
	rv->colorname = NULL;
	return rv;
}

static void lesstif_destroy_gc(pcb_hid_gc_t gc)
{
	if (gc->colorname != NULL)
		free(gc->colorname);
	free(gc);
}

static void lesstif_render_burst(pcb_burst_op_t op, const pcb_box_t *screen)
{
	pcb_gui->coord_per_pix = view_zoom;
}

static void lesstif_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *drw_screen)
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



static void lesstif_set_color(pcb_hid_gc_t gc, const char *name)
{
	static void *cache = 0;
	pcb_hidval_t cval;
	static XColor color, exact_color;

	if (!display)
		return;
	if (!name)
		name = "red";

	if (name != gc->colorname) {
		if (gc->colorname != NULL)
			free(gc->colorname);
		gc->colorname = pcb_strdup(name);
	}

	if (strcmp(name, "drill") == 0) {
		gc->color = offlimit_color;
		gc->erase = 0;
	}
	else if (pcb_hid_cache_color(0, name, &cval, &cache)) {
		gc->color = cval.lval;
		gc->erase = 0;
	}
	else {
		if (!XAllocNamedColor(display, lesstif_colormap, name, &color, &exact_color))
			color.pixel = WhitePixel(display, screen);
#if 0
		printf("lesstif_set_color `%s' %08x rgb/%d/%d/%d\n", name, color.pixel, color.red, color.green, color.blue);
#endif
		cval.lval = gc->color = color.pixel;
		pcb_hid_cache_color(1, name, &cval, &cache);
		gc->erase = 0;
	}
	if (autofade) {
		static int lastcolor = -1, lastfade = -1;
		if (gc->color == lastcolor)
			gc->color = lastfade;
		else {
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
	if ((pinout || conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly) && gc->erase)
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
	if ((pinout || conf_core.editor.thin_draw) && gc->erase)
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

	if (conf_core.editor.view.flip_x) {
		start_angle = 180 - start_angle;
		delta_angle = -delta_angle;
	}
	if (conf_core.editor.view.flip_y) {
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
#warning TODO: make this #if a flag and add it in the gtk hid as well
#if 0
	/* Enable this if you want to see the center and radii of drawn
	   arcs, for debugging.  */
	if (conf_core.editor.thin_draw && (delta_angle != 360)) {
		cx += width;
		cy += height;
		XDrawLine(display, pixmap, arc1_gc, cx, cy,
							cx - width * cos(start_angle * M_PI / 180), cy + width * sin(start_angle * M_PI / 180));
		XDrawLine(display, pixmap, arc2_gc, cx, cy,
							cx - width * cos((start_angle + delta_angle) * M_PI / 180),
							cy + width * sin((start_angle + delta_angle) * M_PI / 180));
	}
#endif
}

static void lesstif_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	int vw = Vw(gc->width);
	if ((pinout || conf_core.editor.thin_draw) && gc->erase)
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
	if (pinout && use_mask() && gc->erase)
		return;
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
	if ((pinout || conf_core.editor.thin_draw) && gc->erase)
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

static void lesstif_calibrate(double xval, double yval)
{
	CRASH("lesstif_calibrate");
}

static int lesstif_shift_is_pressed(void)
{
	return shift_pressed;
}

static int lesstif_control_is_pressed(void)
{
	return ctrl_pressed;
}

static int lesstif_mod1_is_pressed(void)
{
	return alt_pressed;
}

extern void lesstif_get_coords(const char *msg, pcb_coord_t * x, pcb_coord_t * y);

static void lesstif_set_crosshair(pcb_coord_t x, pcb_coord_t y, int action)
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
		if (conf_core.editor.view.flip_x)
			view_left_x = x - (view_width - pos_x) * view_zoom;
		else
			view_left_x = x - pos_x * view_zoom;
		if (conf_core.editor.view.flip_y)
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

static pcb_hidval_t lesstif_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data)
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

static void lesstif_stop_timer(pcb_hidval_t hv)
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

void lesstif_unwatch_file(pcb_hidval_t data)
{
	WatchStruct *watch = (WatchStruct *) data.ptr;
	XtRemoveInput(watch->id);
	free(watch);
}

	/* We need a wrapper around the hid file watch because to pass the correct flags
	 */
static void lesstif_watch_cb(XtPointer client_data, int *fid, XtInputId * id)
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
		lesstif_unwatch_file(x);
	return;
}

pcb_hidval_t
lesstif_watch_file(int fd, unsigned int condition,
									 pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data), pcb_hidval_t user_data)
{
	WatchStruct *watch = (WatchStruct *) malloc(sizeof(WatchStruct));
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


typedef struct {
	XtBlockHookId id;
	void (*func) (pcb_hidval_t user_data);
	pcb_hidval_t user_data;
} BlockHookStruct;

static void lesstif_block_hook_cb(XtPointer user_data);

static void lesstif_block_hook_cb(XtPointer user_data)
{
	BlockHookStruct *block_hook = (BlockHookStruct *) user_data;
	block_hook->func(block_hook->user_data);
}

static pcb_hidval_t lesstif_add_block_hook(void (*func) (pcb_hidval_t data), pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	BlockHookStruct *block_hook = (BlockHookStruct *) malloc(sizeof(BlockHookStruct));

	block_hook->func = func;
	block_hook->user_data = user_data;

	block_hook->id = XtAppAddBlockHook(app_context, lesstif_block_hook_cb, (XtPointer) block_hook);

	ret.ptr = (void *) block_hook;
	return ret;
}

static void lesstif_stop_block_hook(pcb_hidval_t mlpoll)
{
	BlockHookStruct *block_hook = (BlockHookStruct *) mlpoll.ptr;
	XtRemoveBlockHook(block_hook->id);
	free(block_hook);
}


extern void lesstif_logv(enum pcb_message_level level, const char *fmt, va_list ap);

extern int lesstif_confirm_dialog(const char *msg, ...);

extern int lesstif_close_confirm_dialog();

extern void lesstif_report_dialog(const char *title, const char *msg);

extern void *lesstif_attr_dlg_new(pcb_hid_attribute_t *attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, const char *descr_, void *caller_data, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev));
extern int lesstif_attr_dlg_run(void *hid_ctx);
extern void lesstif_attr_dlg_free(void *hid_ctx);
extern void lesstif_attr_dlg_property(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val);
extern int lesstif_attribute_dialog(pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data);
extern int lesstif_attr_dlg_widget_state(void *hid_ctx, int idx, pcb_bool enabled);
extern int lesstif_attr_dlg_widget_hide(void *hid_ctx, int idx, pcb_bool hide);
extern int lesstif_attr_dlg_set_value(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);


static void pinout_callback(Widget da, PreviewData * pd, XmDrawingAreaCallbackStruct * cbs)
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
	save_px = main_pixmap;
	pinout = pd;
	main_pixmap = pd->window;
	view_left_x = pd->x;
	view_top_y = pd->y;
	view_zoom = pd->zoom;
	view_width = pd->v_width;
	view_height = pd->v_height;
	conf_force_set_bool(conf_core.editor.view.flip_x, 0);
	conf_force_set_bool(conf_core.editor.view.flip_y, 0);
/*	region.X1 = 0;
	region.Y1 = 0;
	region.X2 = PCB->MaxWidth;
	region.Y2 = PCB->MaxHeight;*/

	XFillRectangle(display, pixmap, bg_gc, 0, 0, pd->v_width, pd->v_height);
	pcb_hid_expose_pinout(&lesstif_hid, &pd->ctx);

	pinout = 0;
	view_left_x = save_vx;
	view_top_y = save_vy;
	view_zoom = save_vz;
	view_width = save_vw;
	view_height = save_vh;
	main_pixmap = save_px;
	conf_force_set_bool(conf_core.editor.view.flip_x, save_fx);
	conf_force_set_bool(conf_core.editor.view.flip_y, save_fy);
}

static void pinout_unmap(Widget w, PreviewData * pd, void *v)
{
	if (pd->prev)
		pd->prev->next = pd->next;
	else
		pinouts = pd->next;
	if (pd->next)
		pd->next->prev = pd->prev;
	XtDestroyWidget(XtParent(pd->form));
	free(pd);
}

static void lesstif_show_item(void *item)
{
	double scale;
	Widget da;
	pcb_box_t *extents;
	PreviewData *pd;

	for (pd = pinouts; pd; pd = pd->next)
		if (pd->ctx.content.obj == item)
			return;
	if (!mainwind)
		return;

	pd = (PreviewData *) calloc(1, sizeof(PreviewData));

	pd->ctx.content.obj = item;

	extents = pcb_hid_get_extents_pinout(&pd->ctx);
	pd->left = extents->X1;
	pd->right = extents->X2;
	pd->top = extents->Y1;
	pd->bottom = extents->Y2;

	if (pd->left > pd->right) {
		free(pd);
		return;
	}
	pd->prev = 0;
	pd->next = pinouts;
	if (pd->next)
		pd->next->prev = pd;
	pinouts = pd;
	pd->zoom = 0;

	stdarg_n = 0;
	pd->form = XmCreateFormDialog(mainwind, XmStrCast("pinout"), stdarg_args, stdarg_n);
	pd->window = 0;
	XtAddCallback(pd->form, XmNunmapCallback, (XtCallbackProc) pinout_unmap, (XtPointer) pd);

	scale = sqrt(200.0 * 200.0 / ((pd->right - pd->left + 1.0) * (pd->bottom - pd->top + 1.0)));

	stdarg_n = 0;
	stdarg(XmNwidth, (int) (scale * (pd->right - pd->left + 1)));
	stdarg(XmNheight, (int) (scale * (pd->bottom - pd->top + 1)));
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	da = XmCreateDrawingArea(pd->form, XmStrCast("pinout"), stdarg_args, stdarg_n);
	XtManageChild(da);

	XtAddCallback(da, XmNexposeCallback, (XtCallbackProc) pinout_callback, (XtPointer) pd);
	XtAddCallback(da, XmNresizeCallback, (XtCallbackProc) pinout_callback, (XtPointer) pd);

	XtManageChild(pd->form);
	pinout = 0;
}

#include "dlg_preview.c"

static void lesstif_beep(void)
{
	putchar(7);
	fflush(stdout);
}


static pcb_bool progress_cancelled = pcb_false;

static void progress_cancel_callback(Widget w, void *v, void *cbs)
{
	progress_cancelled = pcb_true;
}

static Widget progress_dialog = 0;
static Widget progress_cancel, progress_label;
static Widget progress_scale;

static void lesstif_progress_dialog(int so_far, int total, const char *msg)
{
	XmString xs;

	if (mainwind == 0)
		return;

	if (progress_dialog == 0) {
		Atom close_atom;

		stdarg_n = 0;
		stdarg(XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON);
		stdarg(XmNtitle, "Progress");
		stdarg(XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
		stdarg(XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
		progress_dialog = XmCreateInformationDialog(mainwind, XmStrCast("progress"), stdarg_args, stdarg_n);
		XtAddCallback(progress_dialog, XmNcancelCallback, (XtCallbackProc) progress_cancel_callback, NULL);

		progress_cancel = XmMessageBoxGetChild(progress_dialog, XmDIALOG_CANCEL_BUTTON);
		progress_label = XmMessageBoxGetChild(progress_dialog, XmDIALOG_MESSAGE_LABEL);

		XtUnmanageChild(XmMessageBoxGetChild(progress_dialog, XmDIALOG_OK_BUTTON));
		XtUnmanageChild(XmMessageBoxGetChild(progress_dialog, XmDIALOG_HELP_BUTTON));

		stdarg(XmNdefaultPosition, False);
		XtSetValues(progress_dialog, stdarg_args, stdarg_n);

		stdarg_n = 0;
		stdarg(XmNminimum, 0);
		stdarg(XmNvalue, 0);
		stdarg(XmNmaximum, total > 0 ? total : 1);
		stdarg(XmNorientation, XmHORIZONTAL);
		stdarg(XmNshowArrows, pcb_false);
		progress_scale = XmCreateScrollBar(progress_dialog, XmStrCast("scale"), stdarg_args, stdarg_n);
		XtManageChild(progress_scale);

		close_atom = XmInternAtom(display, XmStrCast("WM_DELETE_WINDOW"), 0);
		XmAddWMProtocolCallback(XtParent(progress_dialog), close_atom, (XtCallbackProc) progress_cancel_callback, 0);
	}

	stdarg_n = 0;
	stdarg(XmNvalue, 0);
	stdarg(XmNsliderSize, (so_far <= total) ? (so_far < 0) ? 0 : so_far : total);
	stdarg(XmNmaximum, total > 0 ? total : 1);
	XtSetValues(progress_scale, stdarg_args, stdarg_n);

	stdarg_n = 0;
	xs = XmStringCreatePCB(msg);
	stdarg(XmNmessageString, xs);
	XtSetValues(progress_dialog, stdarg_args, stdarg_n);

	return;
}

#define MIN_TIME_SEPARATION 0.1	/* seconds */

static int lesstif_progress(int so_far, int total, const char *message)
{
	static pcb_bool started = pcb_false;
	XEvent e;
	struct timeval time;
	double time_delta, time_now;
	static double time_then = 0.0;
	int retval = 0;

	if (!lesstif_active) {
		if (message != NULL)
			fprintf(stderr, "progress: %d/%d %s\n", so_far, total, message);
		return 0;
	}

	if (so_far == 0 && total == 0 && message == NULL) {
		XtUnmanageChild(progress_dialog);
		started = pcb_false;
		progress_cancelled = pcb_false;
		return retval;
	}

	gettimeofday(&time, NULL);
	time_now = time.tv_sec + time.tv_usec / 1000000.0;

	time_delta = time_now - time_then;

	if (started && time_delta < MIN_TIME_SEPARATION)
		return retval;

	/* Create or update the progress dialog */
	lesstif_progress_dialog(so_far, total, message);

	if (!started) {
		XtManageChild(progress_dialog);
		started = pcb_true;
	}

	/* Dispatch pending events */
	while (XtAppPending(app_context)) {
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}
	idle_proc(NULL);

	/* If rendering takes a while, make sure the core has enough time to
	   do work.  */
	gettimeofday(&time, NULL);
	time_then = time.tv_sec + time.tv_usec / 1000000.0;

	return progress_cancelled;
}

static void lesstif_get_view_size(pcb_coord_t *width, pcb_coord_t *height)
{
	*width = Pz(view_width);
	*width = Pz(view_height);
}


static int lesstif_usage(const char *topic)
{
	fprintf(stderr, "\nLesstif GUI command line arguments:\n\n");
	pcb_hid_usage(lesstif_attribute_list, sizeof(lesstif_attribute_list) / sizeof(lesstif_attribute_list[0]));
	fprintf(stderr, "\nInvocation: pcb-rnd --gui lesstif [options]\n");
	return 0;
}

#include "dolists.h"

static void lesstif_globconf_change_post(conf_native_t *cfg, int arr_idx)
{
	if (!lesstif_active)
		return;
	if (strncmp(cfg->hash_path, "appearance/color/", 17) == 0)
		lesstif_invalidate_all();
}

static conf_hid_id_t lesstif_conf_id = -1;
void lesstif_confchg_grid_unit(conf_native_t *cfg, int arr_idx)
{
	/* test if PCB struct doesn't exist at startup */
	if ((PCB == NULL) || (!lesstif_active))
		return;

	lesstif_sizes_reset();
	lesstif_styles_update_values();
}

static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func)(conf_native_t *, int))
{
	conf_native_t *n = conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		conf_hid_set_cb(n, lesstif_conf_id, cbs);
	}
}

static void lesstif_conf_regs(const char *cookie)
{
	static conf_hid_callbacks_t cbs_grid_unit;
	init_conf_watch(&cbs_grid_unit, "editor/grid_unit", lesstif_confchg_grid_unit);
}


void lesstif_create_menu(const char *menu, const pcb_menu_prop_t *props);
void lesstif_remove_menu(const char *menu);
void lesstif_remove_menu_node(lht_node_t *node);

int pplg_check_ver_hid_lesstif(int version_we_need) { return 0; }

void pplg_uninit_hid_lesstif(void)
{
	pcb_hid_remove_attributes_by_cookie(lesstif_cookie);
	pcb_event_unbind_allcookie(lesstif_cookie);
	conf_hid_unreg(lesstif_cookie);
}

int pplg_init_hid_lesstif(void)
{
	static conf_hid_callbacks_t ccb;

	PCB_API_CHK_VER;

	memset(&ccb, 0, sizeof(ccb));
	ccb.val_change_post = lesstif_globconf_change_post;

	memset(&lesstif_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&lesstif_hid);
	pcb_dhlp_draw_helpers_init(&lesstif_hid);

	lesstif_hid.struct_size = sizeof(pcb_hid_t);
	lesstif_hid.name = "lesstif";
	lesstif_hid.description = "LessTif - a Motif clone for X/Unix";
	lesstif_hid.gui = 1;
	lesstif_hid.can_mask_clear_rats = 1;

	lesstif_hid.get_export_options = lesstif_get_export_options;
	lesstif_hid.do_export = lesstif_do_export;
	lesstif_hid.do_exit = lesstif_do_exit;
	lesstif_hid.uninit = lesstif_uninit;
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
	lesstif_hid.get_view_size = lesstif_get_view_size;
	lesstif_hid.set_crosshair = lesstif_set_crosshair;
	lesstif_hid.add_timer = lesstif_add_timer;
	lesstif_hid.stop_timer = lesstif_stop_timer;
	lesstif_hid.watch_file = lesstif_watch_file;
	lesstif_hid.unwatch_file = lesstif_unwatch_file;
	lesstif_hid.add_block_hook = lesstif_add_block_hook;
	lesstif_hid.stop_block_hook = lesstif_stop_block_hook;

	lesstif_hid.log = lesstif_log;
	lesstif_hid.logv = lesstif_logv;
	lesstif_hid.confirm_dialog = lesstif_confirm_dialog;
	lesstif_hid.close_confirm_dialog = lesstif_close_confirm_dialog;
	lesstif_hid.report_dialog = lesstif_report_dialog;
	lesstif_hid.prompt_for = lesstif_prompt_for;
	lesstif_hid.fileselect = lesstif_fileselect;
	lesstif_hid.attr_dlg_new = lesstif_attr_dlg_new;
	lesstif_hid.attr_dlg_run = lesstif_attr_dlg_run;
	lesstif_hid.attr_dlg_free = lesstif_attr_dlg_free;
	lesstif_hid.attr_dlg_property = lesstif_attr_dlg_property;
	lesstif_hid.attribute_dialog = lesstif_attribute_dialog;
	lesstif_hid.attr_dlg_widget_state = lesstif_attr_dlg_widget_state;
	lesstif_hid.attr_dlg_widget_hide = lesstif_attr_dlg_widget_hide;
	lesstif_hid.attr_dlg_set_value = lesstif_attr_dlg_set_value;
	lesstif_hid.show_item = lesstif_show_item;
	lesstif_hid.beep = lesstif_beep;
	lesstif_hid.progress = lesstif_progress;
	lesstif_hid.edit_attributes = lesstif_attributes_dialog;
	lesstif_hid.point_cursor = PointCursor;

	lesstif_hid.create_menu = lesstif_create_menu;
	lesstif_hid.remove_menu = lesstif_remove_menu;
	lesstif_hid.remove_menu_node = lesstif_remove_menu_node;
	lesstif_hid.update_menu_checkbox = lesstif_update_widget_flags;
	lesstif_hid.usage = lesstif_usage;

	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, lesstif_cookie);
	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, LesstifNetlistChanged, NULL, lesstif_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, LesstifRouteStylesChanged, NULL, lesstif_cookie);
	pcb_event_bind(PCB_EVENT_LIBRARY_CHANGED, LesstifLibraryChanged, NULL, lesstif_cookie);
	pcb_event_bind(PCB_EVENT_BUSY, LesstifBusy, NULL, lesstif_cookie);

	pcb_hid_register_hid(&lesstif_hid);
	if (lesstif_conf_id < 0)
		lesstif_conf_id = conf_hid_reg(lesstif_cookie, &ccb);
	lesstif_conf_regs(lesstif_cookie);

	return 0;
}

static int lesstif_attrs_regd = 0;
static void lesstif_reg_attrs(void)
{
	if (!lesstif_attrs_regd)
		PCB_REGISTER_ATTRIBUTES(lesstif_attribute_list, lesstif_cookie)
	lesstif_attrs_regd = 1;
}

static void lesstif_begin(void)
{
	PCB_REGISTER_ACTIONS(lesstif_library_action_list, lesstif_cookie)
	lesstif_reg_attrs();
	PCB_REGISTER_ACTIONS(lesstif_main_action_list, lesstif_cookie)
	PCB_REGISTER_ACTIONS(lesstif_dialog_action_list, lesstif_cookie)
	PCB_REGISTER_ACTIONS(lesstif_netlist_action_list, lesstif_cookie)
	PCB_REGISTER_ACTIONS(lesstif_styles_action_list, lesstif_cookie)

	lesstif_active = 1;
}

static void lesstif_end(void)
{
	pcb_hid_remove_actions_by_cookie(lesstif_cookie);
	pcb_hid_remove_attributes_by_cookie(lesstif_cookie);
	lesstif_active = 0;
}
