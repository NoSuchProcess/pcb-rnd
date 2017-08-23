#ifndef PCB_HID_H
#define PCB_HID_H

#include <stdarg.h>

#include "config.h"
#include "error.h"
#include "drc.h"
#include "global_typedefs.h"
#include "attrib.h"
#include "layer.h"
#include "layer_grp.h"

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

Coordinates are ALWAYS in pcb's default resolution (1/100 mil at the
moment).  Positive X is right, positive Y is down.  Angles are
degrees, with 0 being right (positive X) and 90 being up (negative Y).
All zoom, scaling, panning, and conversions are hidden inside the HID
layers.

The main structure is at the end of this file.

Data structures passed to the HIDs will be copied if the HID needs to
save them.  Data structures returned from the HIDs must not be freed,
and may be changed by the HID in response to new information.

*/

/* Like end cap styles.  The cap *always* extends beyond the
   coordinates given, by half the width of the line.  Beveled ends can
   be used to make octagonal pads by giving the same x,y coordinate
   twice.  */
typedef enum {
	Trace_Cap,									/* This means we're drawing a trace, which has round caps.  */
	Square_Cap,									/* Square pins or pads. */
	Round_Cap,									/* Round pins or round-ended pads, thermals.  */
	Beveled_Cap									/* Octagon pins or bevel-cornered pads.  */
} pcb_cap_style_t;

/* The HID may need something more than an "int" for colors, timers,
   etc.  So it passes/returns one of these, which is castable to a
   variety of things.  */
typedef union {
	long lval;
	void *ptr;
} pcb_hidval_t;

/* This graphics context is an opaque pointer defined by the HID.  GCs
   are HID-specific; attempts to use one HID's GC for a different HID
   will result in a fatal error.  */
typedef struct hid_gc_s *pcb_hid_gc_t;

#define PCB_HIDCONCAT(a,b) a##b

/* This is used to register the action callbacks (for menus and
   whatnot).  HID assumes the following actions are available for its
   use:
	SaveAs(filename);
	Quit();
*/
typedef struct {
	/* This is matched against action names in the GUI configuration */
	const char *name;
	/* If this string is non-NULL, the action needs to know the X,Y
	   coordinates to act on, and this string may be used to prompt
	   the user to select a coordinate.  If NULL, the coordinates may
	   be 0,0 if none are known.  */
	const char *need_coord_msg;
	/* Called when the action is triggered.  If this function returns
	   non-zero, no further actions will be invoked for this key/mouse
	   event.  */
	int (*trigger_cb) (int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
	/* Short description that sometimes accompanies the name.  */
	const char *description;
	/* Full allowed syntax; use \n to separate lines.  */
	const char *syntax;
} pcb_hid_action_t;

extern void pcb_hid_register_action(const pcb_hid_action_t *a, const char *cookie, int copy);

extern void pcb_hid_register_actions(const pcb_hid_action_t *a, int, const char *cookie, int copy);
#define PCB_REGISTER_ACTIONS(a, cookie) PCB_HIDCONCAT(void register_,a) ()\
{ pcb_hid_register_actions(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* Note that PCB expects the GUI to provide the following actions:

   LibraryChanged()
 */

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
	void (*append_drc_violation) (pcb_drc_violation_t * violation);
	int (*throw_drc_dialog) (void);
} pcb_hid_drc_gui_t;

typedef enum pcb_mask_op_s {
	HID_MASK_OFF,    /* Flushes the buffer and return to non-mask operation.  */

	HID_MASK_BEFORE, /* Polygons being drawn before clears.  */

	HID_MASK_INIT,   /* Initialize a new rendering layer for compositing layers  */
	HID_MASK_CLEAR,  /* negative-draw on the rendering layer  */
	HID_MASK_SET,    /* positive-draw on the rendering layer */

	HID_MASK_AFTER   /* Polygons being drawn after clears.  */
} pcb_mask_op_t;

typedef enum pcb_composite_op_s {
	PCB_HID_COMP_RESET,         /* reset (allocate and clear) the sketch canvas */
	PCB_HID_COMP_POSITIVE,      /* draw subsequent objects in positive, with color */
	PCB_HID_COMP_NEGATIVE,      /* draw subsequent objects in negative, ignore color */
	PCB_HID_COMP_FLUSH          /* combine the sketch canvas onto the output buffer and discard the sketch canvas */
} pcb_composite_op_t;

typedef enum pcb_burst_op_s {
	PCB_HID_BURST_START,
	PCB_HID_BURST_END
} pcb_burst_op_t;

typedef struct hid_s pcb_hid_t;

/* This is the main HID structure.  */
struct hid_s {
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

	/* If set, the redraw code will draw polygons before erasing the
	   clearances.  */
	unsigned poly_before:1;

	/* If set, the redraw code will draw polygons after erasing the
	   clearances.  Note that HIDs may set both of these, in which case
	   polygons will be drawn twice.  */
	unsigned poly_after:1;

	/* If set, draw the mask layer inverted. Normally the mask is a filled
	   rectangle over the board with cutouts at pins/pads. The HIDs
	   use render in normal mode, gerber renders in inverse mode. */
	unsigned mask_invert:1;


	/* If set, draws holes after copper, silk and mask, to make sure it
	   punches through everything. */
	unsigned holes_after:1;

	/* If set, enable the fake composite layer draw optimization, that is:
	   if all layers in a compositing layer group are positive, no composition
	   is done but layers are just drawn. Useful for software render HIDs with
	   no transluency; causes problems with transluent rendering: different
	   layers within the same group will be darker on overlap */
	unsigned enable_fake_composite:1;


	/* lesstif allows positive AND negative drawing in HID_MASK_CLEAR.
	   gtk only allows negative drawing.
	   using the mask is to get rat transparency */
	unsigned can_mask_clear_rats:1;

	/* Returns a set of resources describing options the export or print
	   HID supports.  In GUI mode, the print/export dialogs use this to
	   set up the selectable options.  In command line mode, these are
	   used to interpret command line options.  If n_ret_ is non-NULL,
	   the number of attributes is stored there.  */
	pcb_hid_attribute_t *(*get_export_options) (int *n_ret_);

	/* Exports (or print) the current PCB.  The options given represent
	   the choices made from the options returned from
	   get_export_options.  Call with options_ == NULL to start the
	   primary GUI (create a main window, print, export, etc)  */
	void (*do_export) (pcb_hid_attr_val_t * options_);

	/* uninit a GUI hid */
	void (*uninit) (pcb_hid_t *hid);

	/* uninit a GUI hid */
	void (*do_exit) (pcb_hid_t *hid);

	/* Parses the command line.  Call this early for whatever HID will be
	   the primary HID, as it will set all the registered attributes.
	   The HID should remove all arguments, leaving any possible file
	   names behind.  */
	void (*parse_arguments) (int *argc_, char ***argv_);

	/* This may be called to ask the GUI to force a redraw of a given area */
	void (*invalidate_lr) (pcb_coord_t left_, pcb_coord_t right_, pcb_coord_t top_, pcb_coord_t bottom_);
	void (*invalidate_all) (void);
	void (*notify_crosshair_change) (pcb_bool changes_complete);
	void (*notify_mark_change) (pcb_bool changes_complete);

	/* During redraw or print/export cycles, this is called once per layer group
	   (physical layer); layer is the first layer in the group.
	   TODO: The group may be -1 until the layer rewrite is finished.
	   If it returns false (zero), the HID does not want that layer, and none of
	   the drawing functions should be called.  If it returns pcb_true (nonzero),
	   the items in that layer [group] should be drawn using the various drawing
	   functions.  In addition to the copper layer groups, you may select virtual
	   layers. The is_empty argument is a hint - if set, the layer is empty, if
	   zero it may be non-empty. */
	int (*set_layer_group)(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty);

	/* Tell the GUI the layer last selected has been finished with. */
	void (*end_layer) (void);

	/* Drawing Functions.  Coordinates and distances are ALWAYS in PCB's
	   default coordinates (1/100 mil at the time this comment was
	   written).  Angles are always in degrees, with 0 being "right"
	   (positive X) and 90 being "up" (positive Y).  */

	/* Make an empty graphics context.  */
	pcb_hid_gc_t (*make_gc) (void);
	void (*destroy_gc) (pcb_hid_gc_t gc_);

	/* Special note about the "erase" color: To use this color, you must
	   use this function to tell the HID when you're using it.  At the
	   beginning of a layer redraw cycle (i.e. after set_layer), call
	   use_mask() to redirect output to a buffer.  Draw to the buffer
	   (using regular HID calls) using regular and "erase" colors.  Then
	   call use_mask(HID_MASK_OFF) to flush the buffer to the HID.  If
	   you use the "erase" color when use_mask is disabled, it simply
	   draws in the background color.

	   OBSOLETE, do not use it, use set_drawing_mode() and render_burst()
	   */
	void (*use_mask) (pcb_mask_op_t use_it_);

	/* Composite layer drawing: manipulate the sketch canvas and set
	   positive or negative drawing mode. The canvas covers the screen box. */
	void (*set_drawing_mode)(pcb_composite_op_t op, const pcb_box_t *screen);

	/* Announce start/end of a render burst for a specific screen screen box. */
	void (*render_burst)(pcb_burst_op_t op, const pcb_box_t *screen);

	/* Sets a color.  Names can be like "red" or "#rrggbb" or special
	   names like "erase".  *Always* use the "erase" color for removing
	   ink (like polygon reliefs or thermals), as you cannot rely on
	   knowing the background color or special needs of the HID.  Always
	   use the "drill" color to draw holes.  You may assume this is
	   cheap enough to call inside the redraw callback, but not cheap
	   enough to call for each item drawn. */
	void (*set_color) (pcb_hid_gc_t gc_, const char *name_);

	/* Sets the line style.  While calling this is cheap, calling it with
	   different values each time may be expensive, so grouping items by
	   line style is helpful.  */
	void (*set_line_cap) (pcb_hid_gc_t gc_, pcb_cap_style_t style_);
	void (*set_line_width) (pcb_hid_gc_t gc_, pcb_coord_t width_);
	void (*set_draw_xor) (pcb_hid_gc_t gc_, int xor_);
	/* Blends 20% or so color with 80% background.  Only used for
	   assembly drawings so far. */
	void (*set_draw_faded) (pcb_hid_gc_t gc_, int faded_);

	/* The usual drawing functions.  "draw" means to use segments of the
	   given width, whereas "fill" means to fill to a zero-width
	   outline.  */
	void (*draw_line) (pcb_hid_gc_t gc_, pcb_coord_t x1_, pcb_coord_t y1_, pcb_coord_t x2_, pcb_coord_t y2_);
	void (*draw_arc) (pcb_hid_gc_t gc_, pcb_coord_t cx_, pcb_coord_t cy_, pcb_coord_t xradius_, pcb_coord_t yradius_, pcb_angle_t start_angle_, pcb_angle_t delta_angle_);
	void (*draw_rect) (pcb_hid_gc_t gc_, pcb_coord_t x1_, pcb_coord_t y1_, pcb_coord_t x2_, pcb_coord_t y2_);
	void (*fill_circle) (pcb_hid_gc_t gc_, pcb_coord_t cx_, pcb_coord_t cy_, pcb_coord_t radius_);
	void (*fill_polygon) (pcb_hid_gc_t gc_, int n_coords_, pcb_coord_t * x_, pcb_coord_t * y_);
	void (*fill_pcb_polygon) (pcb_hid_gc_t gc_, pcb_polygon_t * poly, const pcb_box_t * clip_box);
	void (*thindraw_pcb_polygon) (pcb_hid_gc_t gc_, pcb_polygon_t * poly, const pcb_box_t * clip_box);
	void (*fill_pcb_pad) (pcb_hid_gc_t gc_, pcb_pad_t * pad, pcb_bool clip, pcb_bool mask);
	void (*thindraw_pcb_pad) (pcb_hid_gc_t gc_, pcb_pad_t * pad, pcb_bool clip, pcb_bool mask);
	void (*fill_pcb_pv) (pcb_hid_gc_t fg_gc, pcb_hid_gc_t bg_gc, pcb_pin_t * pv, pcb_bool drawHole, pcb_bool mask);
	void (*thindraw_pcb_pv) (pcb_hid_gc_t fg_gc, pcb_hid_gc_t bg_gc, pcb_pin_t * pv, pcb_bool drawHole, pcb_bool mask);
	void (*fill_rect) (pcb_hid_gc_t gc_, pcb_coord_t x1_, pcb_coord_t y1_, pcb_coord_t x2_, pcb_coord_t y2_);


	/* This is for the printer.  If you call this for the GUI, xval_ and
	   yval_ are ignored, and a dialog pops up to lead you through the
	   calibration procedure.  For the printer, if xval_ and yval_ are
	   zero, a calibration page is printed with instructions for
	   calibrating your printer.  After calibrating, nonzero xval_ and
	   yval_ are passed according to the instructions.  Metric is nonzero
	   if the user prefers metric units, else inches are used. */
	void (*calibrate) (double xval_, double yval_);


	/* GUI layout functions.  Not used or defined for print/export
	   HIDs.  */

	/* Temporary */
	int (*shift_is_pressed) (void);
	int (*control_is_pressed) (void);
	int (*mod1_is_pressed) (void);

	void (*get_coords) (const char *msg_, pcb_coord_t * x_, pcb_coord_t * y_);

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
	void (*set_crosshair) (int x_, int y_, int cursor_action_);
#define HID_SC_DO_NOTHING	0
#define HID_SC_WARP_POINTER	1
#define HID_SC_PAN_VIEWPORT	2

	/* Causes func to be called at some point in the future.  Timers are
	   only good for *one* call; if you want it to repeat, add another
	   timer during the callback for the first.  user_data_ can be
	   anything, it's just passed to func.  Times are not guaranteed to
	   be accurate.  */
	  pcb_hidval_t(*add_timer) (void (*func) (pcb_hidval_t user_data_), unsigned long milliseconds_, pcb_hidval_t user_data_);
	/* Use this to stop a timer that hasn't triggered yet.  */
	void (*stop_timer) (pcb_hidval_t timer_);

	/* Causes func_ to be called when some condition occurs on the file
	   descriptor passed. Conditions include data for reading, writing,
	   hangup, and errors. user_data_ can be anything, it's just passed
	   to func. */
	  pcb_hidval_t(*watch_file) (int fd_, unsigned int condition_,
												 void (*func_) (pcb_hidval_t watch_, int fd_, unsigned int condition_, pcb_hidval_t user_data_),
												 pcb_hidval_t user_data);
	/* Use this to stop a file watch. */
	void (*unwatch_file) (pcb_hidval_t watch_);

	/* Causes func_ to be called in the mainloop prior to blocking */
	  pcb_hidval_t(*add_block_hook) (void (*func_) (pcb_hidval_t data_), pcb_hidval_t data_);
	/* Use this to stop a mainloop block hook. */
	void (*stop_block_hook) (pcb_hidval_t block_hook_);

	/* Various dialogs */

	/* Logs a message to the log window.  */
	void (*log) (const char *fmt_, ...);
	void (*logv) (enum pcb_message_level, const char *fmt_, va_list args_);

	/* A generic yes/no dialog.  Returns zero if the cancel button is
	   pressed, one for the OK button.  If you specify alternate labels
	   for ..., they are used instead of the default OK/Cancel ones, and
	   the return value is the index of the label chosen.  You MUST pass
	   NULL as the last parameter to this.  */
	int (*confirm_dialog) (const char *msg_, ...);

	/* A close confirmation dialog for unsaved pages, for example, with
	   options "Close without saving", "Cancel" and "Save". Returns zero
	   if the close is cancelled, or one if it should proceed. The HID
	   is responsible for any "Save" action the user may wish before
	   confirming the close.
	 */
	int (*close_confirm_dialog) ();
#define HID_CLOSE_CONFIRM_CANCEL 0
#define HID_CLOSE_CONFIRM_OK     1

	/* Just prints text.  */
	void (*report_dialog) (const char *title_, const char *msg_);

	/* Prompts the user to enter a string, returns the string.  If
	   default_string isn't NULL, the form is pre-filled with this
	   value.  "msg" is like "Enter value:".  Returns NULL on cancel. */
	char *(*prompt_for) (const char *msg_, const char *default_string_);

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

	char *(*fileselect) (const char *title_, const char *descr_,
											 const char *default_file_, const char *default_ext_, const char *history_tag_, int flags_);

	/* A generic dialog to ask for a set of attributes.  If n_attrs_ is
	   zero, attrs_(.name) must be NULL terminated.  Returns non-zero if
	   an error occurred (usually, this means the user cancelled the
	   dialog or something). title_ is the title of the dialog box
	   descr_ (if not NULL) can be a longer description of what the
	   attributes are used for.  The HID may choose to ignore it or it
	   may use it for a tooltip or text in a dialog box, or a help
	   string.
	 */
	int (*attribute_dialog) (pcb_hid_attribute_t * attrs_,
													 int n_attrs_, pcb_hid_attr_val_t * results_, const char *title_, const char *descr_);

	/* This causes a second window to display, which only shows the
	   selected item_. The expose callback is called twice; once to size
	   the extents of the item, and once to draw it.  To pass magic
	   values, pass the address of a variable created for this
	   purpose.  */
	void (*show_item) (void *item_);

	/* Something to alert the user.  */
	void (*beep) (void);

	/* Used by optimizers and autorouter to show progress to the user.
	   Pass all zeros to flush display and remove any dialogs.
	   Returns nonzero if the user wishes to cancel the operation.  */
	int (*progress) (int so_far_, int total_, const char *message_);

	pcb_hid_drc_gui_t *drc_gui;

	void (*edit_attributes) (const char *owner, pcb_attribute_list_t * attrlist_);

	/* Debug drawing support. These APIs must be implemented (non NULL),
	 * but they do not have to be functional. request_debug_draw can
	 * return NULL to indicate debug drawing is not permitted.
	 *
	 * Debug drawing is not guaranteed to be re-entrant.
	 * The caller must not nest requests for debug drawing.
	 */

	/* Request permission for debug drawing
	 *
	 * Returns a HID pointer which should be used rather than the global
	 * gui-> for making drawing calls. If the return value is NULL, then
	 * permission has been denied, and the drawing must not continue.
	 */
	pcb_hid_t *(*request_debug_draw) (void);

	/* Flush pending drawing to the screen
	 *
	 * May be implemented as a NOOP if the GUI has chosen to send the
	 * debug drawing directly to the screen.
	 */
	void (*flush_debug_draw) (void);

	/* When finished, the user must inform the GUI to clean up resources
	 *
	 * Any remaining rendering will be flushed to the screen.
	 */
	void (*finish_debug_draw) (void);

	/* Notification to the GUI around saving the PCB file.
	 *
	 * Called with a false parameter before the save, called again
	 * with pcb_true after the save.
	 *
	 * Allows GUIs which watch for file-changes on disk to ignore
	 * our deliberate changes.
	 */
	void (*notify_save_pcb) (const char *filename, pcb_bool done);

	/* Notification to the GUI that the PCB file has been renamed. */
	void (*notify_filename_changed) (void);

	/* Creates a new menu and/or submenus
	 * menu_path is a / separated path to the new menu (parents are silently created).
	 * The last non-NULL item is the new menu item.
	 * action, mnemonic, accel and tip affect the new menu item.
	 * Cookie is strdup()'d into the lihata tree and can be used later to search
	 * and remove menu items that are no longer needed.
	 * If action is NULL, the menu may get submenus.
	 */
	void (*create_menu) (const char *menu_path, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie);

	/* Removes a menu recursively */
	int (*remove_menu) (const char *menu_path);

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
};

/* One of these functions (in the common code) will be called whenever the GUI
   needs to redraw the screen, print the board, or export a layer.

   Each time func is called, it should do the following:

   * allocate any colors needed, via get_color.

   * cycle through the layers, calling set_layer for each layer to be
     drawn, and only drawing elements (all or specified) of desired
     layers.

   Do *not* assume that the hid that is passed is the GUI hid.  This
   callback is also used for printing and exporting. */

typedef struct pcb_hid_expose_ctx_s pcb_hid_expose_ctx_t;

typedef void (*hid_dialog_draw_t)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);

struct pcb_hid_expose_ctx_s {
	pcb_box_t view;
	unsigned force:1; /* draw even if layer set fails */
	union {
		pcb_layer_id_t layer_id;
		pcb_element_t *elem;
	} content;

	/* for PCB_LYT_DIALOG */
	hid_dialog_draw_t dialog_draw;
};

typedef void (*pcb_hid_expose_t)(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx);

/* Normal expose: draw all layers with all flags (no .content is used) */
void pcb_hid_expose_all(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *region);

/* Pinout preview expose: draws an element; content.elem is used */
void pcb_hid_expose_pinout(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *element);


/* Layer preview expose: draws a single layer; content.layer_id is used */
void pcb_hid_expose_layer(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ly);


/* This is initially set to a "no-gui" GUI, and later reset by
   main. hid_expose_callback also temporarily set it for drawing. */
extern pcb_hid_t *pcb_gui;

/* When not NULL, auto-start the next GUI after the current one quits */
extern pcb_hid_t *pcb_next_gui;

/* This is either NULL or points to the current HID that is being called to
   do the exporting. The GUI HIDs set and unset this var.*/
extern pcb_hid_t *pcb_exporter;

/* This is either NULL or points to the current pcb_hid_action_t that is being
   called. The action launcher sets and unsets this variable. */
extern const pcb_hid_action_t *pcb_current_action;

/* The GUI may set this to be approximately the PCB size of a pixel,
   to allow for near-misses in selection and changes in drawing items
   smaller than a screen pixel.  */
extern int pcb_pixel_slop;

/* Inits and uninits the whole action framework */
void pcb_hid_actions_init(void);
void pcb_hid_actions_uninit(void);

#endif
