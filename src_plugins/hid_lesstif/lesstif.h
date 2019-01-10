#include "hid_cfg_input.h"
#include "compat_nls.h"
#include "board.h"

#define app_context lesstif_app_context
#define appwidget lesstif_appwidget
#define display lesstif_display
#define screen_s lesstif_screen_s
#define screen lesstif_screen
#define mainwind lesstif_mainwind
#define work_area lesstif_work_area
#define messages lesstif_messages
#define command lesstif_command
#define hscroll lesstif_hscroll
#define vscroll lesstif_vscroll
#define m_click lesstif_message_click

extern XtAppContext app_context;
extern Widget appwidget;
extern Display *display;
extern Screen *screen_s;
extern int screen;

extern pcb_hid_cfg_mouse_t lesstif_mouse;
extern pcb_hid_cfg_keys_t lesstif_keymap;

extern Widget mainwind, work_area, command, hscroll, vscroll;
extern Widget m_click;

extern Widget lesstif_menu(Widget, const char *, Arg *, int);
extern int lesstif_key_event(XKeyEvent *);
extern int lesstif_button_event(Widget w, XEvent * e);

/* Returns TRUE if the point mapped to the PCB region, FALSE (=0) if
   we're off-board.  Note that *pcbxy is always written to, even if
   out of range.  */
extern int lesstif_winxy_to_pcbxy(int winx, int winy, int *pcbx, int *pcby);

/* Returns TRUE if the point is in the window, FALSE (=0) otherwise. */
extern int lesstif_pcbxy_to_winxy(int pcbx, int pcby, int *winx, int *winy);

extern void lesstif_need_idle_proc(void);
extern void lesstif_show_crosshair(int);
extern void lesstif_invalidate_all(void);
extern void lesstif_coords_to_pcb(int, int, pcb_coord_t *, pcb_coord_t *);
extern void lesstif_get_xy(const char *msg);
extern void lesstif_update_widget_flags(const char *cookie);
extern int lesstif_call_action(const char *, int, char **);
extern void lesstif_sizes_reset(void);
extern void lesstif_pan_fixup(void);
extern void lesstif_show_library(void);
extern void lesstif_show_netlist(void);
extern Pixel lesstif_parse_color(const pcb_color_t *value);
extern Pixel lesstif_parse_color_str(const char *value);
extern void lesstif_styles_update_values();
extern void lesstif_update_layer_groups();
extern void lesstif_update_status_line();
extern char *lesstif_fileselect(const char *, const char *, const char *, const char *, const char *, int);
extern void lesstif_log(const char *fmt, ...);
extern void lesstif_attributes_dialog(const char *, pcb_attribute_list_t *);


#ifndef XtRPCBCoord
#define XtRPCBCoord	"PCBCoord"
#endif

#define need_idle_proc lesstif_need_idle_proc
#define show_crosshair lesstif_show_crosshair

/*
 * Motif comes from a time when even constant strings where
 * passed as char*. These days, this requires to do ugly
 * type-casting. To better identify all the places where this
 * is necessary, we make this cast even more ugly but unique
 * enough that it is simple to grep.
 */
#define XmStrCast(s) ((char*)(s))

static XmString XmStringCreatePCB(const char *x)
{
	if (x && x[0])
		x = gettext(x);
	return XmStringCreateLtoR(XmStrCast(x), XmFONTLIST_DEFAULT_TAG);
}

extern const char *lesstif_cookie;

void pcb_ltf_winplace(Display *dsp, Window w, const char *id, int defx, int defy);
void pcb_ltf_wplc_config_cb(Widget shell, XtPointer data, XEvent *xevent, char *dummy);

