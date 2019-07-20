#ifndef PCB_HID_H
#define PCB_HID_H

#include <stdarg.h>
#include <liblihata/dom.h>

#include "config.h"
#include "error.h"
#include "global_typedefs.h"
#include "attrib.h"
#include "layer.h"

/* attribute dialog properties */
typedef enum pcb_hat_property_e {
	PCB_HATP_GLOBAL_CALLBACK,
	PCB_HATP_max
} pcb_hat_property_t;


typedef enum {
	PCB_HID_MOUSE_PRESS,
	PCB_HID_MOUSE_RELEASE,
	PCB_HID_MOUSE_MOTION,
	PCB_HID_MOUSE_POPUP  /* "right click", open context-specific popup */
} pcb_hid_mouse_ev_t;


typedef struct pcb_hid_attr_val_s  pcb_hid_attr_val_t;
typedef struct pcb_hid_attribute_s pcb_hid_attribute_t;

/* Human Interface Device */

/*

The way the HID layer works is that you instantiate a HID device
structure, and invoke functions through its members.  Code in the
common part of PCB may *not* rely on *anything* other than what's
defined in this file.  Code in the HID layers *may* rely on data and
functions in the common code (like, board size and such) but it's
considered bad form to do so when not needed.

Coordinates are ALWAYS in pcb's internal units (nanometer at the
moment).  Positive X is right, positive Y is down.  Angles are
degrees, with 0 being right (positive X) and 90 being up (negative Y).
All zoom, scaling, panning, and conversions are hidden inside the HID
layers.

The main structure is at the end of this file.

Data structures passed to the HIDs will be copied if the HID needs to
save them.  Data structures returned from the HIDs must not be freed,
and may be changed by the HID in response to new information.

*/

/* Line end cap styles.  The cap *always* extends beyond the
   coordinates given, by half the width of the line. */
typedef enum {
	pcb_cap_invalid = -1,
	pcb_cap_square,        /* square pins or pads when drawn using a line */
	pcb_cap_round          /* for normal traces, round pins */
} pcb_cap_style_t;

/* The HID may need something more than an "int" for colors, timers,
   etc.  So it passes/returns one of these, which is castable to a
   variety of things.  */
typedef union {
	long lval;
	void *ptr;
} pcb_hidval_t;

typedef struct {
	pcb_coord_t width;     /* as set by set_line_width */
	pcb_cap_style_t cap;   /* as set by set_line_cap */
	int xor;               /* as set by set_draw_xor */
	int faded;             /* as set by set_draw_faded */
	pcb_hid_t *hid;        /* the HID that owns this gc */
} pcb_core_gc_t;


#define PCB_HIDCONCAT(a,b) a##b

/* File Watch flags */
/* Based upon those in dbus/dbus-connection.h */
typedef enum {
	PCB_WATCH_READABLE = 1 << 0,  /* As in POLLIN */
	PCB_WATCH_WRITABLE = 1 << 1,  /* As in POLLOUT */
	PCB_WATCH_ERROR = 1 << 2,     /* As in POLLERR */
	PCB_WATCH_HANGUP = 1 << 3     /* As in POLLHUP */
} pcb_watch_flags_t;

typedef enum pcb_composite_op_s {
	PCB_HID_COMP_RESET,         /* reset (allocate and clear) the sketch canvas */
	PCB_HID_COMP_POSITIVE,      /* draw subsequent objects in positive, with color, no XOR allowed */
	PCB_HID_COMP_POSITIVE_XOR,  /* draw subsequent objects in positive, with color, XOR allowed */
	PCB_HID_COMP_NEGATIVE,      /* draw subsequent objects in negative, ignore color */
	PCB_HID_COMP_FLUSH          /* combine the sketch canvas onto the output buffer and discard the sketch canvas */
} pcb_composite_op_t;

typedef enum pcb_burst_op_s {
	PCB_HID_BURST_START,
	PCB_HID_BURST_END
} pcb_burst_op_t;

typedef enum pcb_hid_attr_ev_e {
	PCB_HID_ATTR_EV_WINCLOSE = 2, /* window closed (window manager close) */
	PCB_HID_ATTR_EV_CODECLOSE     /* closed by the code, including standard close buttons */
} pcb_hid_attr_ev_t;

typedef enum pcb_hid_fsd_flags_e {
	/* Prompts the user for a filename or directory name.  For GUI
	   HID's this would mean a file select dialog box.  The 'flags'
	   argument is the bitwise OR of the following values.  */
	PCB_HID_FSD_READ = 1,

	/* The function calling hid->fileselect will deal with the case
	   where the selected file already exists.  If not given, then the
	   GUI will prompt with an "overwrite?" prompt.  Only used when
	   writing.
	 */
	PCB_HID_FSD_MAY_NOT_EXIST = 2,

	/* The call is supposed to return a file template (for gerber
	   output for example) instead of an actual file.  Only used when
	   writing.
	 */
	PCB_HID_FSD_IS_TEMPLATE = 4
} pcb_hid_fsd_flags_t;

typedef struct {
	const char *name;
	const char *mime;
	const char **pat; /* NULL terminated array of file name patterns */
} pcb_hid_fsd_filter_t;

extern const pcb_hid_fsd_filter_t pcb_hid_fsd_filter_any[];

/* Optional fields of a menu item; all non-NULL fields are strdup'd in the HID. */
typedef struct pcb_menu_prop_s {
	const char *action;
	const char *accel;
	const char *tip;        /* tooltip */
	const char *checked;
	const char *update_on;
	const pcb_color_t *foreground;
	const pcb_color_t *background;
	const char *cookie;     /* used for cookie based removal */
} pcb_menu_prop_t;

typedef enum pcb_hid_clipfmt_e {
	PCB_HID_CLIPFMT_TEXT              /* plain text (c string with the \0 included) */
} pcb_hid_clipfmt_t;

typedef enum {
	PCB_HID_DOCK_TOP_LEFT,       /*  hbox on the top, below the menubar */
	PCB_HID_DOCK_TOP_RIGHT,      /*  hbox on the top, next to the menubar */
	PCB_HID_DOCK_TOP_INFOBAR,    /*  vbox for horizontally aligned important messages, above ("on top of") the drawing area for critical warnings - may have bright background color */
	PCB_HID_DOCK_LEFT,
	PCB_HID_DOCK_BOTTOM,
	PCB_HID_DOCK_FLOAT,          /* separate window */

	PCB_HID_DOCK_max
} pcb_hid_dock_t;

extern int pcb_dock_is_vert[PCB_HID_DOCK_max];    /* 1 if a new dock box (parent of a new sub-DAD) should be a vbox, 0 if hbox */
extern int pcb_dock_has_frame[PCB_HID_DOCK_max];  /* 1 if a new dock box (parent of a new sub-DAD) should be framed */

/* This is the main HID structure.  */
struct pcb_hid_s {
	/* The size of this structure.  We use this as a compatibility
	   check; a HID built with a different hid.h than we're expecting
	   should have a different size here.  */
	int struct_size;

	/* The name of this HID.  This should be suitable for
	   command line options, multi-selection menus, file names,
	   etc. */
	const char *name;

	/* Likewise, but allowed to be longer and more descriptive.  */
	const char *description;

	/* The hid may use this field to store its context. */
	void *user_context;

	/* If set, this is the GUI HID.  Exactly one of these three flags
	   must be set; setting "gui" lets the expose callback optimize and
	   coordinate itself.  */
	unsigned gui:1;

	/* If set, this is the printer-class HID.  The common part of PCB
	   may use this to do command-line printing, without having
	   instantiated any GUI HIDs.  Only one printer HID is normally
	   defined at a time.  */
	unsigned printer:1;

	/* If set, this HID provides an export option, and should be used as
	   part of the File->Export menu option.  Examples are PNG, Gerber,
	   and EPS exporters.  */
	unsigned exporter:1;

	/* Export plugin should not be listed in the export dialog */
	unsigned hide_from_gui:1;

	/* If set, draw the mask layer inverted. Normally the mask is a filled
	   rectangle over the board with cutouts at pins/pads. The HIDs
	   use render in normal mode, gerber renders in inverse mode. */
	unsigned mask_invert:1;

	/* Always draw layers in compositing mode - no base layer */
	unsigned force_compositing:1;

	/* When enabled, indicate layer of heavy terminals graphically */
	unsigned heavy_term_layer_ind:1;

	/* When 1, HID supports markup (e.g. color) in DAD text widgets  */
	unsigned supports_dad_text_markup:1;

	/* it is possible to create DAD dialogs before the GUI_INIT event is emitted
	   (e.g. draw progress bar while loading a command line file) */
	unsigned allow_dad_before_init:1;

	/* Returns a set of resources describing options the export or print
	   HID supports.  In GUI mode, the print/export dialogs use this to
	   set up the selectable options.  In command line mode, these are
	   used to interpret command line options.  If n_ret_ is non-NULL,
	   the number of attributes is stored there.  */
	pcb_hid_attribute_t *(*get_export_options)(pcb_hid_t *hid, int *n_ret);

	/* Exports (or print) the current PCB.  The options given represent
	   the choices made from the options returned from
	   get_export_options.  Call with options_ == NULL to start the
	   primary GUI (create a main window, print, export, etc)  */
	void (*do_export)(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_hid_attr_val_t *options);

	/* uninit a GUI hid */
	void (*uninit)(pcb_hid_t *hid);

	/* uninit a GUI hid */
	void (*do_exit)(pcb_hid_t *hid);

	/* Process pending events, flush output, but never block. Called from busy
	   loops from time to time. */
	void (*iterate)(pcb_hid_t *hid);

	/* Parses the command line.  Call this early for whatever HID will be
	   the primary HID, as it will set all the registered attributes.
	   The HID should remove all arguments, leaving any possible file
	   names behind. Returns 0 on success, positive for recoverable errors
	   (no change in argc or argv) and negative for unrecoverable errors.  */
	int (*parse_arguments)(pcb_hid_t *hid, int *argc, char ***argv);

	/* This may be called to ask the GUI to force a redraw of a given area */
	void (*invalidate_lr)(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom);
	void (*invalidate_all)(pcb_hid_t *hid, pcb_hidlib_t *hidlib);
	void (*notify_crosshair_change)(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_bool changes_complete);
	void (*notify_mark_change)(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_bool changes_complete);

	/* During redraw or print/export cycles, this is called once per layer group
	   (physical layer); pusrpose/purpi are the extracted purpose field and its
	   keyword/function version; layer is the first layer in the group.
	   TODO: The group may be -1 until the layer rewrite is finished.
	   If it returns false (zero), the HID does not want that layer, and none of
	   the drawing functions should be called.  If it returns pcb_true (nonzero),
	   the items in that layer [group] should be drawn using the various drawing
	   functions.  In addition to the copper layer groups, you may select virtual
	   layers. The is_empty argument is a hint - if set, the layer is empty, if
	   zero it may be non-empty. */
	int (*set_layer_group)(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform);

	/* Tell the GUI the layer last selected has been finished with. */
	void (*end_layer)(pcb_hid_t *hid);

	/*** Drawing Functions. ***/

	/* Make an empty graphics context.  */
	pcb_hid_gc_t (*make_gc)(pcb_hid_t *hid);
	void (*destroy_gc)(pcb_hid_t *hid, pcb_hid_gc_t gc);

	/* Composite layer drawing: manipulate the sketch canvas and set
	   positive or negative drawing mode. The canvas covers the screen box. */
	void (*set_drawing_mode)(pcb_hid_t *hid, pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen);

	/* Announce start/end of a render burst for a specific screen screen box;
	   A GUI hid should set the coord_per_pix value here for proper optimization. */
	void (*render_burst)(pcb_hid_t *hid, pcb_burst_op_t op, const pcb_box_t *screen);

	/*** gc vs. pcb_hid_t *: pcb_core_gc_t contains ->hid, so these calls don't
	     need to get it as first arg. ***/

	/* Sets a color. Can be one of the special colors like pcb_color_drill.
	   (Always use the drill color to draw holes and slots).
	   You may assume this is cheap enough to call inside the redraw
	   callback, but not cheap enough to call for each item drawn. */
	void (*set_color)(pcb_hid_gc_t gc, const pcb_color_t *color);

	/* Sets the line style.  While calling this is cheap, calling it with
	   different values each time may be expensive, so grouping items by
	   line style is helpful.  */
	void (*set_line_cap)(pcb_hid_gc_t gc, pcb_cap_style_t style);
	void (*set_line_width)(pcb_hid_gc_t gc, pcb_coord_t width);
	void (*set_draw_xor)(pcb_hid_gc_t gc, int xor);
	/* Blends 20% or so color with 80% background.  Only used for
	   assembly drawings so far. */
	void (*set_draw_faded)(pcb_hid_gc_t gc, int faded);

	/* The usual drawing functions.  "draw" means to use segments of the
	   given width, whereas "fill" means to fill to a zero-width
	   outline.  */
	void (*draw_line)(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
	void (*draw_arc)(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle);
	void (*draw_rect)(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
	void (*fill_circle)(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius);
	void (*fill_polygon)(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y);
	void (*fill_polygon_offs)(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy);
	void (*fill_rect)(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);

	/* This is for the printer. If xval_ and yval_ are
	   zero, a calibration page is printed with instructions for
	   calibrating your printer.  After calibrating, nonzero xval_ and
	   yval_ are passed according to the instructions.  Metric is nonzero
	   if the user prefers metric units, else inches are used. */
	void (*calibrate)(pcb_hid_t *hid, double xval, double yval);


	/*** GUI layout functions.  Not used or defined for print/export HIDs. ***/

	/* Temporary */
	int (*shift_is_pressed)(pcb_hid_t *hid);
	int (*control_is_pressed)(pcb_hid_t *hid);
	int (*mod1_is_pressed)(pcb_hid_t *hid);

	void (*get_coords)(pcb_hid_t *hid, const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force);

	/* Sets the crosshair, which may differ from the pointer depending
	   on grid and pad snap.  Note that the HID is responsible for
	   hiding, showing, redrawing, etc.  The core just tells it what
	   coordinates it's actually using.  Note that this routine may
	   need to know what "pcb units" are so it can display them in mm
	   or mils accordingly.  If cursor_action_ is set, the cursor or
	   screen may be adjusted so that the cursor and the crosshair are
	   at the same point on the screen.  */
	void (*set_crosshair)(pcb_hid_t *hid, pcb_coord_t x, pcb_coord_t y, int cursor_action);
#define HID_SC_DO_NOTHING    0
#define HID_SC_WARP_POINTER  1
#define HID_SC_PAN_VIEWPORT  2

	/* Causes func to be called at some point in the future.  Timers are
	   only good for *one* call; if you want it to repeat, add another
	   timer during the callback for the first.  user_data_ can be
	   anything, it's just passed to func.  Times are not guaranteed to
	   be accurate.  */
	pcb_hidval_t (*add_timer)(pcb_hid_t *hid, void (*func)(pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data);
	/* Use this to stop a timer that hasn't triggered yet. */
	void (*stop_timer)(pcb_hid_t *hid, pcb_hidval_t timer);

	/* Causes func_ to be called when some condition occurs on the file
	   descriptor passed. Conditions include data for reading, writing,
	   hangup, and errors. user_data_ can be anything, it's just passed
	   to func. If the watch function returns pcb_true, the watch is kept, else
	   it is removed. */
	pcb_hidval_t (*watch_file)(pcb_hid_t *hid, int fd, unsigned int condition, pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data), pcb_hidval_t user_data);

	/* Use this to stop a file watch; must not be called from within a GUI callback! */
	void (*unwatch_file)(pcb_hid_t *hid, pcb_hidval_t watch);

	/* title may be used as a dialog box title.  Ignored if NULL.
	 *
	 * descr is a longer help string.  Ignored if NULL.
	 *
	 * default_file is the default file name.  Ignored if NULL.
	 *
	 * default_ext is the default file extension, like ".pdf".
	 * Ignored if NULL.
	 *
	 * flt is a {NULL} terminated array of file filters; HID support is optional.
	 * Ignored if NULL. If NULL and default_ext is not NULL, the HID may make
	 * up a minimalistic filter from the default_ext also allowing *.*.
	 *
	 * history_tag may be used by the GUI to keep track of file
	 * history.  Examples would be "board", "vendor", "renumber",
	 * etc.  If NULL, no specific history is kept.
	 *
	 * flags_ are the bitwise OR
	 *
	 * sub is an optional DAD sub-dialog, can be NULL; its parent_poke commands:
	 *  close          close the dialog
	 *  get_path       returns the current full path in res as an strdup'd string (caller needs to free it)
	 *  set_file_name  replaces the file name portion of the current path from arg[0].d.s
	 */
	char *(*fileselect)(pcb_hid_t *hid, const char *title, const char *descr, const char *default_file, const char *default_ext, const pcb_hid_fsd_filter_t *flt, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub);

	/* A generic dialog to ask for a set of attributes. If n_attrs_ is
	   zero, attrs_(.name) must be NULL terminated. attr_dlg_run returns
	   non-zero if an error occurred (usually, this means the user cancelled
	   the dialog or something). title is the title of the dialog box
	   The HID may choose to ignore it or it may use it for a tooltip or
	   text in a dialog box, or a help string. Id is used in some events (e.g.
	   for window placement) and is strdup'd. If defx and defy are larger than 0,
	   they are hints for the default (starting) window size - can be overridden
	   by window placement. Returns opaque hid_ctx.
	   (Hid_ctx shall save pcb_hid_t so subsequent attr_dlg_*() calls don't have
	   it as an argument) */
	void *(*attr_dlg_new)(pcb_hid_t *hid, const char *id, pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny);
	int (*attr_dlg_run)(void *hid_ctx);
	void (*attr_dlg_raise)(void *hid_ctx); /* raise the window to top */
	void (*attr_dlg_free)(void *hid_ctx); /* results_ is avalibale after this call */

	/* Set a property of an attribute dialog (typical call is between
	   attr_dlg_new() and attr_dlg_run()) */
	void (*attr_dlg_property)(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val);


	/* Disable or enable a widget of an active attribute dialog; if enabled is
	   2, the widget is put to its second enabled mode (pressed state for buttons, highlight on label) */
	int (*attr_dlg_widget_state)(void *hid_ctx, int idx, int enabled);

	/* hide or show a widget of an active attribute dialog */
	int (*attr_dlg_widget_hide)(void *hid_ctx, int idx, pcb_bool hide);

	/* Change the current value of a widget; same as if the user chaged it,
	   except the value-changed callback is inhibited */
	int (*attr_dlg_set_value)(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);

	/* Change the help text (tooltip) string of a widget; NULL means remove it.
	   NOTE: does _not_ change the help_text field of the attribute. */
	void (*attr_dlg_set_help)(void *hid_ctx, int idx, const char *val);

	/* top window docking: enter a new docked part by registering a
	   new subdialog or leave (remove a docked part) from a subdialog. Return 0
	   on success. */
	int (*dock_enter)(pcb_hid_t *hid, pcb_hid_dad_subdialog_t *sub, pcb_hid_dock_t where, const char *id);
	void (*dock_leave)(pcb_hid_t *hid, pcb_hid_dad_subdialog_t *sub);

	/* Something to alert the user.  */
	void (*beep)(pcb_hid_t *hid);

	void (*edit_attributes)(pcb_hid_t *hid, const char *owner, pcb_attribute_list_t *attrlist);

	/* Creates a new menu and/or submenus
	 * menu_path is a / separated path to the new menu (parents are silently created).
	 * The last non-NULL item is the new menu item.
	 * action, accel and tip affect the new menu item.
	 * Cookie is strdup()'d into the lihata tree and can be used later to search
	 * and remove menu items that are no longer needed.
	 * If action is NULL, the menu may get submenus.
	 */
	void (*create_menu)(const char *menu_path, const pcb_menu_prop_t *props);

	/* Removes a menu recursively */
	int (*remove_menu)(const char *menu_path);
	int (*remove_menu_node)(lht_node_t *nd);

	/* At the moment HIDs load the menu file. Some plugin code, like the toolbar
	   code needs to traverse the menu tree too. This call exposes the
	   HID-internal menu struct */
	pcb_hid_cfg_t *(*get_menu_cfg)(void);

	/* Update the state of all checkboxed menus whose luhata
	   node cookie matches cookie (or all checkboxed menus globally if cookie
	   is NULL) */
	void (*update_menu_checkbox)(const char *cookie);

	/* Pointer to the hid's configuration - useful for plugins and core wanting to install menus at anchors */
	pcb_hid_cfg_t *hid_cfg;


	/* Optional: print usage string (if accepts command line arguments)
	   Subtopic:
	     NULL    print generic help
	     string  print summary for the topic in string
	   Return 0 on success.
	*/
	int (*usage)(const char *subtopic);


	/* Optional: change cursor to indicate if an object is grabbed (or not) */
	void (*point_cursor)(pcb_bool grabbed);

	/* Optional: when non-zero, the core renderer may decide to draw cheaper
	   (simplified) approximation of some objects that would end up being too
	   small. For a GUI, this should depend on the zoom level */
	pcb_coord_t coord_per_pix;

	/* If ovr is not NULL:
	    - overwrite the command etry with ovr
	    - if cursor is not NULL, set the cursor after the overwrite
	   If ovr is NULL:
	    - if cursor is not NULL, load the value with the cursor (or -1 if not supported)
	   Return the current command entry content in a read-only string */
	const char *(*command_entry)(const char *ovr, int *cursor);

	/*** clipboard handling for GUI HIDs ***/
	/* Place format/data/len on the clipboard; return 0 on success */
	int (*clip_set)(pcb_hid_clipfmt_t format, const void *data, size_t len);

	/* retrieve format/data/len from the clipboard; return 0 on success;
	   data is a copy of the data, modifiable by the caller */
	int (*clip_get)(pcb_hid_clipfmt_t *format, void **data, size_t *len);

	/* release the data from the last clip_get(); clip_get() and clip_free() should
	   be called in pair */
	void (*clip_free)(pcb_hid_clipfmt_t format, void *data, size_t len);

	/* run redraw-benchmark and return an FPS value (optional) */
	double (*benchmark)(void);

	/* (pcb_hid_cfg_keys_t *): key state */
	void *key_state;

	/*** zoom/pan calls ***/

	/* side-correct zoom to show a window of the board. If set_crosshair
	   is true, also update the crosshair to be on the center of the window */
	void (*zoom_win)(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_bool set_crosshair);

	/* Zoom relative or absolute by factor; relative means current zoom is
	   multiplied by factor */
	void (*zoom)(pcb_coord_t center_x, pcb_coord_t center_y, double factor, int relative);

	/* Pan relative/absolute by x and y; relative means x and y are added to
	   the current pan */
	void (*pan)(pcb_coord_t x, pcb_coord_t y, int relative);

	/* Start or stop panning at x;y - stop is mode=0, start is mode=1 */
	void (*pan_mode)(pcb_coord_t x, pcb_coord_t y, pcb_bool mode);

	/* Load viewbox with the extents of visible pixels translated to board coords */
	void (*view_get)(pcb_hidlib_t *hidlib, pcb_box_t *viewbox);

	/*** misc GUI ***/
	/* Open the command line */
	void (*open_command)(void);

	/* Open a popup menu addressed by full menu path (starting with "/popups/").
	   Return 0 on success. */
	int (*open_popup)(const char *menupath);

	/* optional: called by core when the global hidlib context changes
	   (e.g. board changed) */
	void (*set_hidlib)(pcb_hidlib_t *hidlib);

	/* Change the mouse cursor to a named cursor e.g. after the tool has changed.
	   The list of cursors names available may depend on the HID. */
	void (*reg_mouse_cursor)(pcb_hidlib_t *hidlib, int idx, const char *name, const unsigned char *pixel, const unsigned char *mask);
	void (*set_mouse_cursor)(pcb_hidlib_t *hidlib, int idx);

	/* change top window title any time the after the GUI_INIT event */
	void (*set_top_title)(pcb_hidlib_t *hidlib, const char *title);

	/* OPTIONAL: override the mouse cursor to indicate busy state */
	void (*busy)(pcb_hidlib_t *hidlib, pcb_bool busy);

	/* this field is used by that HID implementation to store its data */
	void *hid_data;
};

/* One of these functions (in the common code) will be called whenever the GUI
   needs to redraw the screen, print the board, or export a layer.

   Each time func is called, it should do the following:

   * allocate any colors needed, via get_color.

   * cycle through the layers, calling set_layer for each layer to be
     drawn, and only drawing objects (all or specified) of desired
     layers.

   Do *not* assume that the hid that is passed is the GUI hid.  This
   callback is also used for printing and exporting. */
typedef void (*pcb_hid_expose_cb_t)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);

struct pcb_hid_expose_ctx_s {
	pcb_box_t view;
	pcb_hid_expose_cb_t expose_cb;   /* function that is called on expose to get things drawn */
	void *draw_data;                 /* user data for the expose function */
};

typedef void (*pcb_hid_expose_t)(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx);

/* This is initially set to a "no-gui" GUI, and later reset by
   main. hid_expose_callback also temporarily set it for drawing. */
extern pcb_hid_t *pcb_gui;

/* This is either NULL or points to the current HID that is being called to
   do the exporting. The GUI HIDs set and unset this var.*/
extern pcb_hid_t *pcb_exporter;

/* This is either NULL or points to the current pcb_action_t that is being
   called. The action launcher sets and unsets this variable. */
extern const pcb_action_t *pcb_current_action;

/* The GUI may set this to be approximately the PCB size of a pixel,
   to allow for near-misses in selection and changes in drawing items
   smaller than a screen pixel.  */
extern int pcb_pixel_slop;

/*** Glue for GUI/CLI dialogs: wrappers around actions */

/* Prompts the user to enter a string, returns the string.  If
   default_string isn't NULL, the form is pre-filled with this
   value. "msg" is printed above the query. The optional title
   is the window title.
   Returns NULL on cancel. The caller needs to free the returned string */
char *pcb_hid_prompt_for(const char *msg, const char *default_string, const char *title);

/* Present a dialog box with a message and variable number of buttons. If icon
   is not NULL, attempt to draw the named icon on the left. The vararg part is
   one or more buttons, as a list of "char *label, int retval", terminated with
   NULL. */
int pcb_hid_message_box(const char *icon, const char *title, const char *label, ...);

/* Show modal progressbar to the user, offering cancel long running processes.
   Pass all zeros to flush display and remove the dialog.
   Returns nonzero if the user wishes to cancel the operation.  */
int pcb_hid_progress(long so_far, long total, const char *message);

/* non-zero if DAD dialogs are available currently */
#define PCB_HAVE_GUI_ATTR_DLG \
	((pcb_gui != NULL) && (pcb_gui->gui) && (pcb_gui->attr_dlg_new != NULL) && (pcb_gui->attr_dlg_new != pcb_nogui_attr_dlg_new))
void *pcb_nogui_attr_dlg_new(pcb_hid_t *hid, const char *id, pcb_hid_attribute_t *attrs_, int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev), int defx, int defy, int minx, int miny);

int pcb_hid_dock_enter(pcb_hid_dad_subdialog_t *sub, pcb_hid_dock_t where, const char *id);
void pcb_hid_dock_leave(pcb_hid_dad_subdialog_t *sub);

#define pcb_hid_redraw(pcb) pcb_gui->invalidate_all(pcb_gui, &pcb->hidlib)

#define pcb_hid_busy(pcb, is_busy) \
do { \
	pcb_event(&pcb->hidlib, PCB_EVENT_BUSY, "i", is_busy, NULL); \
	if ((pcb_gui != NULL) && (pcb_gui->busy != NULL)) \
		pcb_gui->busy(&pcb->hidlib, is_busy); \
} while(0)

#endif
