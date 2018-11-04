#ifndef PCB_HID_H
#define PCB_HID_H

#include <stdarg.h>
#include <liblihata/dom.h>

#include "config.h"
#include "error.h"
#include "drc.h"
#include "global_typedefs.h"
#include "attrib.h"
#include "layer.h"
#include "layer_grp.h"

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
} pcb_core_gc_t;

/* This graphics context is an opaque pointer defined by the HID.  GCs
   are HID-specific; attempts to use one HID's GC for a different HID
   will result in a fatal error.  The first field must be:
   pcb_core_gc_t core_gc; */
typedef struct hid_gc_s *pcb_hid_gc_t;


#define PCB_HIDCONCAT(a,b) a##b

/* File Watch flags */
/* Based upon those in dbus/dbus-connection.h */
typedef enum {
	PCB_WATCH_READABLE = 1 << 0,
														 /**< As in POLLIN */
	PCB_WATCH_WRITABLE = 1 << 1,
														 /**< As in POLLOUT */
	PCB_WATCH_ERROR = 1 << 2,	 /**< As in POLLERR */
	PCB_WATCH_HANGUP = 1 << 3	 /**< As in POLLHUP */
} pcb_watch_flags_t;

/* DRC GUI Hooks */
typedef struct {
	int log_drc_overview;
	int log_drc_violations;
	void (*reset_drc_dialog_message) (void);
	void (*append_drc_violation) (pcb_drc_violation_t *violation);
	int (*throw_drc_dialog) (void);
} pcb_hid_drc_gui_t;

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
	PCB_HID_ATTR_EV_CANCEL = 0,
	PCB_HID_ATTR_EV_OK = 1,
	PCB_HID_ATTR_EV_WINCLOSE
} pcb_hid_attr_ev_t;

/* Optional fields of a menu item; all non-NULL fields are strdup'd in the HID. */
typedef struct pcb_menu_prop_s {
	const char *action;
	const char *accel;
	const char *tip;        /* tooltip */
	const char *checked;
	const char *update_on;
	const char *foreground;
	const char *background;
	const char *cookie;     /* used for cookie based removal */
} pcb_menu_prop_t;

typedef struct pcb_hid_s pcb_hid_t;

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

	/* If set, draw the mask layer inverted. Normally the mask is a filled
	   rectangle over the board with cutouts at pins/pads. The HIDs
	   use render in normal mode, gerber renders in inverse mode. */
	unsigned mask_invert:1;

	/* lesstif allows positive AND negative drawing in HID_MASK_CLEAR.
	   gtk only allows negative drawing.
	   using the mask is to get rat transparency */
	unsigned can_mask_clear_rats:1;

	/* Always draw layers in compositing mode - no base layer */
	unsigned force_compositing:1;

	/* When enabled, indicate layer of heavy terminals graphically */
	unsigned heavy_term_layer_ind:1;

	/* Returns a set of resources describing options the export or print
	   HID supports.  In GUI mode, the print/export dialogs use this to
	   set up the selectable options.  In command line mode, these are
	   used to interpret command line options.  If n_ret_ is non-NULL,
	   the number of attributes is stored there.  */
	pcb_hid_attribute_t *(*get_export_options)(int *n_ret);

	/* Exports (or print) the current PCB.  The options given represent
	   the choices made from the options returned from
	   get_export_options.  Call with options_ == NULL to start the
	   primary GUI (create a main window, print, export, etc)  */
	void (*do_export)(pcb_hid_attr_val_t *options);

	/* uninit a GUI hid */
	void (*uninit)(pcb_hid_t *hid);

	/* uninit a GUI hid */
	void (*do_exit)(pcb_hid_t *hid);

	/* Parses the command line.  Call this early for whatever HID will be
	   the primary HID, as it will set all the registered attributes.
	   The HID should remove all arguments, leaving any possible file
	   names behind. Returns 0 on success, positive for recoverable errors
	   (no change in argc or argv) and negative for unrecoverable errors.  */
	int (*parse_arguments)(int *argc, char ***argv);

	/* This may be called to ask the GUI to force a redraw of a given area */
	void (*invalidate_lr)(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom);
	void (*invalidate_all)(void);
	void (*notify_crosshair_change)(pcb_bool changes_complete);
	void (*notify_mark_change)(pcb_bool changes_complete);

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
	int (*set_layer_group)(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform);

	/* Tell the GUI the layer last selected has been finished with. */
	void (*end_layer)(void);

	/*** Drawing Functions. ***/

	/* Make an empty graphics context.  */
	pcb_hid_gc_t (*make_gc)(void);
	void (*destroy_gc)(pcb_hid_gc_t gc);

	/* Composite layer drawing: manipulate the sketch canvas and set
	   positive or negative drawing mode. The canvas covers the screen box. */
	void (*set_drawing_mode)(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen);

	/* Announce start/end of a render burst for a specific screen screen box;
	   A GUI hid should set the coord_per_pix value here for proper optimization. */
	void (*render_burst)(pcb_burst_op_t op, const pcb_box_t *screen);

	/* Sets a color.  Names can be like "red" or "#rrggbb" or special
	   names like "drill". Always use the "drill" color to draw holes.
	   You may assume this is cheap enough to call inside the redraw
	   callback, but not cheap enough to call for each item drawn. */
	void (*set_color)(pcb_hid_gc_t gc, const char *name);

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
	void (*fill_pcb_polygon)(pcb_hid_gc_t gc, pcb_poly_t *poly, const pcb_box_t *clip_box);
	void (*thindraw_pcb_polygon)(pcb_hid_gc_t gc, pcb_poly_t *poly, const pcb_box_t *clip_box);
	void (*fill_rect)(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);


	/* This is for the printer.  If you call this for the GUI, xval_ and
	   yval_ are ignored, and a dialog pops up to lead you through the
	   calibration procedure.  For the printer, if xval_ and yval_ are
	   zero, a calibration page is printed with instructions for
	   calibrating your printer.  After calibrating, nonzero xval_ and
	   yval_ are passed according to the instructions.  Metric is nonzero
	   if the user prefers metric units, else inches are used. */
	void (*calibrate)(double xval, double yval);


	/* GUI layout functions.  Not used or defined for print/export
	   HIDs.  */

	/* Temporary */
	int (*shift_is_pressed)(void);
	int (*control_is_pressed)(void);
	int (*mod1_is_pressed)(void);

	void (*get_coords)(const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force);

	/* Fill in width and height with the sizes of the current view in
	   pcb coordinates. used by action "Cursor" to determine max cursor pos.
	   Width and height are never NULL. */
	void (*get_view_size)(pcb_coord_t *width, pcb_coord_t *height);

	/* Sets the crosshair, which may differ from the pointer depending
	   on grid and pad snap.  Note that the HID is responsible for
	   hiding, showing, redrawing, etc.  The core just tells it what
	   coordinates it's actually using.  Note that this routine may
	   need to know what "pcb units" are so it can display them in mm
	   or mils accordingly.  If cursor_action_ is set, the cursor or
	   screen may be adjusted so that the cursor and the crosshair are
	   at the same point on the screen.  */
	void (*set_crosshair)(pcb_coord_t x, pcb_coord_t y, int cursor_action);
#define HID_SC_DO_NOTHING	0
#define HID_SC_WARP_POINTER	1
#define HID_SC_PAN_VIEWPORT	2

	/* Causes func to be called at some point in the future.  Timers are
	   only good for *one* call; if you want it to repeat, add another
	   timer during the callback for the first.  user_data_ can be
	   anything, it's just passed to func.  Times are not guaranteed to
	   be accurate.  */
	pcb_hidval_t (*add_timer) (void (*func)(pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data);
	/* Use this to stop a timer that hasn't triggered yet.  */
	void (*stop_timer) (pcb_hidval_t timer);

	/* Causes func_ to be called when some condition occurs on the file
	   descriptor passed. Conditions include data for reading, writing,
	   hangup, and errors. user_data_ can be anything, it's just passed
	   to func. If the watch function returns pcb_true, the watch is kept, else
	   it is removed. */
	pcb_hidval_t (*watch_file)(int fd, unsigned int condition, pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data), pcb_hidval_t user_data);

	/* Use this to stop a file watch; must not be called from within a GUI callback! */
	void (*unwatch_file) (pcb_hidval_t watch);

	/* Causes func_ to be called in the mainloop prior to blocking */
	pcb_hidval_t (*add_block_hook)(void (*func)(pcb_hidval_t data), pcb_hidval_t data);
	/* Use this to stop a mainloop block hook. */
	void (*stop_block_hook) (pcb_hidval_t block_hook);

	/* Various dialogs */

	/* Logs a message to the log window.  */
	void (*log)(const char *fmt, ...);
	void (*logv)(enum pcb_message_level, const char *fmt, va_list args);

	/* A generic yes/no dialog.  Returns zero if the cancel button is
	   pressed, one for the OK button.  If you specify alternate labels
	   for ..., they are used instead of the default OK/Cancel ones, and
	   the return value is the index of the label chosen.  You MUST pass
	   NULL as the last parameter to this.  */
	int (*confirm_dialog)(const char *msg, ...);

	/* A close confirmation dialog for unsaved pages, for example, with
	   options "Close without saving", "Cancel" and "Save". Returns zero
	   if the close is cancelled, or one if it should proceed. The HID
	   is responsible for any "Save" action the user may wish before
	   confirming the close.
	 */
	int (*close_confirm_dialog)();
#define HID_CLOSE_CONFIRM_CANCEL 0
#define HID_CLOSE_CONFIRM_OK     1

	/* Just prints text.  */
	void (*report_dialog)(const char *title, const char *msg);

	/* Prompts the user to enter a string, returns the string.  If
	   default_string isn't NULL, the form is pre-filled with this
	   value.  "msg" is like "Enter value:".  Returns NULL on cancel. */
	char *(*prompt_for)(const char *msg, const char *default_string);

	/* Prompts the user for a filename or directory name.  For GUI
	   HID's this would mean a file select dialog box.  The 'flags'
	   argument is the bitwise OR of the following values.  */
#define HID_FILESELECT_READ  0x01

	/* The function calling hid->fileselect will deal with the case
	   where the selected file already exists.  If not given, then the
	   GUI will prompt with an "overwrite?" prompt.  Only used when
	   writing.
	 */
#define HID_FILESELECT_MAY_NOT_EXIST 0x02

	/* The call is supposed to return a file template (for gerber
	   output for example) instead of an actual file.  Only used when
	   writing.
	 */
#define HID_FILESELECT_IS_TEMPLATE 0x04

	/* title_ may be used as a dialog box title.  Ignored if NULL.
	 *
	 * descr_ is a longer help string.  Ignored if NULL.
	 *
	 * default_file_ is the default file name.  Ignored if NULL.
	 *
	 * default_ext_ is the default file extension, like ".pdf".
	 * Ignored if NULL.
	 *
	 * history_tag_ may be used by the GUI to keep track of file
	 * history.  Examples would be "board", "vendor", "renumber",
	 * etc.  If NULL, no specific history is kept.
	 *
	 * flags_ are the bitwise OR of the HID_FILESELECT defines above
	 */

	char *(*fileselect)(const char *title, const char *descr, const char *default_file, const char *default_ext, const char *history_tag, int flags);

	/* A generic dialog to ask for a set of attributes.  If n_attrs_ is
	   zero, attrs_(.name) must be NULL terminated.  Returns non-zero if
	   an error occurred (usually, this means the user cancelled the
	   dialog or something). title_ is the title of the dialog box
	   descr_ (if not NULL) can be a longer description of what the
	   attributes are used for.  The HID may choose to ignore it or it
	   may use it for a tooltip or text in a dialog box, or a help
	   string.
	 */
	int (*attribute_dialog)(pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data);

	/* The same API in 3 stages: */
	void *(*attr_dlg_new)(pcb_hid_attribute_t *attrs, int n_attrs, pcb_hid_attr_val_t *results, const char *title, const char *descr, void *caller_data, pcb_bool modal, void (*button_cb)(void *caller_data, pcb_hid_attr_ev_t ev)); /* returns hid_ctx */
	int (*attr_dlg_run)(void *hid_ctx);
	void (*attr_dlg_free)(void *hid_ctx); /* results_ is avalibale after this call */

	/* Set a property of an attribute dialog (typical call is between
	   attr_dlg_new() and attr_dlg_run()) */
	void (*attr_dlg_property)(void *hid_ctx, pcb_hat_property_t prop, const pcb_hid_attr_val_t *val);


	/* Disable or enable a widget of an active attribute dialog */
	int (*attr_dlg_widget_state)(void *hid_ctx, int idx, pcb_bool enabled);

	/* hide or show a widget of an active attribute dialog */
	int (*attr_dlg_widget_hide)(void *hid_ctx, int idx, pcb_bool hide);

	/* Change the current value of a widget; same as if the user chaged it,
	   except the value-changed callback is inhibited */
	int (*attr_dlg_set_value)(void *hid_ctx, int idx, const pcb_hid_attr_val_t *val);


	/* This causes a second window to display, which only shows the
	   selected item_. The expose callback is called twice; once to size
	   the extents of the item, and once to draw it.  To pass magic
	   values, pass the address of a variable created for this
	   purpose.  */
	void (*show_item)(void *item);

	/* Something to alert the user.  */
	void (*beep)(void);

	/* Used by optimizers and autorouter to show progress to the user.
	   Pass all zeros to flush display and remove any dialogs.
	   Returns nonzero if the user wishes to cancel the operation.  */
	int (*progress)(int so_far, int total, const char *message);

	pcb_hid_drc_gui_t *drc_gui;

	void (*edit_attributes)(const char *owner, pcb_attribute_list_t *attrlist);

	/* Notification to the GUI around saving the PCB file.
	 *
	 * Called with a false parameter before the save, called again
	 * with pcb_true after the save.
	 *
	 * Allows GUIs which watch for file-changes on disk to ignore
	 * our deliberate changes.
	 */
	void (*notify_save_pcb)(const char *filename, pcb_bool done);

	/* Notification to the GUI that the PCB file has been renamed. */
	void (*notify_filename_changed)(void);

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

	/*** PROPEDIT (optional) ****/
	/* Optional: start a propedit session: a series of propedit calls will follow
	   Return 0 on success; non-zero aborts the session.
	*/
	int (*propedit_start)(void *pe, int num_props, const char *(*query)(void *pe, const char *cmd, const char *key, const char *val, int idx));

	/* Optional: end a propedit session: all data has been sent, no more; this call
	   should present and run the user dialog and should return, only when the
	   propedit section can be closed. */
	void (*propedit_end)(void *pe);

	/* Optional: registers a new property
	   Returns a prop context passed with each value
	*/
	void *(*propedit_add_prop)(void *pe, const char *propname, int is_mutable, int num_vals);

	/* Optional: registers a new value for a property */
	void (*propedit_add_value)(void *pe, const char *propname, void *propctx, const char *value, int repeat_cnt);

	/* Optional: registers statistical info for a property */
	void (*propedit_add_stat)(void *pe, const char *propname, void *propctx, const char *most_common, const char *min, const char *max, const char *avg);

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

typedef struct pcb_hid_expose_ctx_s pcb_hid_expose_ctx_t;

typedef void (*pcb_hid_dialog_draw_t)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);

struct pcb_hid_expose_ctx_s {
	pcb_box_t view;
	unsigned force:1; /* draw even if layer set fails */
	union {
		pcb_layer_id_t layer_id;
		pcb_any_obj_t *obj;
		void *draw_data;
	} content;

	/* for PCB_LYT_DIALOG */
	pcb_hid_dialog_draw_t dialog_draw; /* also use for generic draw */
};

typedef void (*pcb_hid_expose_t)(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx);

/* Normal expose: draw all layers with all flags (no .content is used) */
void pcb_hid_expose_all(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *region);

/* Pinout preview expose: draws a subc; content.elem is used */
void pcb_hid_expose_pinout(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *element);


/* Layer preview expose: draws a single layer; content.layer_id is used */
void pcb_hid_expose_layer(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ly);

/* generic, dialog/callbakc based preview expose */
void pcb_hid_expose_generic(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *e);


/* This is initially set to a "no-gui" GUI, and later reset by
   main. hid_expose_callback also temporarily set it for drawing. */
extern pcb_hid_t *pcb_gui;

/* When not NULL, auto-start the next GUI after the current one quits */
extern pcb_hid_t *pcb_next_gui;

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

#endif
