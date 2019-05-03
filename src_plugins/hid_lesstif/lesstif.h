#include "hid_cfg_input.h"
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
extern void lesstif_invalidate_all(pcb_hidlib_t *hidlib);
extern void lesstif_coords_to_pcb(int, int, pcb_coord_t *, pcb_coord_t *);
extern void lesstif_get_xy(const char *msg);
extern void lesstif_update_widget_flags(const char *cookie);
extern int lesstif_call_action(const char *, int, char **);
extern void lesstif_pan_fixup(void);
extern void lesstif_show_library(void);
extern void lesstif_show_netlist(void);
extern Pixel lesstif_parse_color(const pcb_color_t *value);
extern Pixel lesstif_parse_color_str(const char *value);
extern void lesstif_update_layer_groups();
extern void lesstif_update_status_line();
extern void lesstif_attributes_dialog(const char *, pcb_attribute_list_t *);
void *lesstif_attr_sub_new(Widget parent_box, pcb_hid_attribute_t *attrs, int n_attrs, void *caller_data);
char *pcb_ltf_fileselect(const char *title, const char *descr, const char *default_file, const char *default_ext, const pcb_hid_fsd_filter_t *flt, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub);

extern int pcb_ltf_ok;
int pcb_ltf_wait_for_dialog(Widget w);


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
	return XmStringCreateLtoR(XmStrCast(x), XmFONTLIST_DEFAULT_TAG);
}

extern const char *lesstif_cookie;

void pcb_ltf_winplace(Display *dsp, Window w, const char *id, int defx, int defy);
void pcb_ltf_wplc_config_cb(Widget shell, XtPointer data, XEvent *xevent, char *dummy);

