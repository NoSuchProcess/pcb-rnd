#ifndef PCB_CONF_CORE_H
#define PCB_CONF_CORE_H

#include "conf.h"
#include "globalconst.h"
#include "color.h"

/* NOTE: this struct has a strict format because a code generator needs to
   read it. Please always keep the format (preferably even whitespace style).
   Use only the CFT_* prefixed types declared in conf.h.
   */

typedef struct {

	struct temp {
		CFT_BOOLEAN rat_warn;              /* rats nest has set warnings */
		CFT_BOOLEAN clip_inhibit_chg;      /* dummy node to inform the menu about clip inhibit change */
	} temp;

	const struct editor {
		CFT_UNIT grid_unit;                /* select whether you draw in mm or mil */
		CFT_COORD grid;                    /* grid in pcb-units */
		CFT_LIST grids;                    /* grid in grid-string format */
		CFT_INTEGER grids_idx;             /* the index of the currently active grid from grids */
		CFT_REAL zoom;                     /* default zoom */
		CFT_INTEGER mode;                  /* currently active mode */
		CFT_INTEGER buffer_number;         /* number of the current buffer */
		CFT_BOOLEAN clear_line;            /* new lines/arc clear polygons. */
		CFT_BOOLEAN clear_polypoly;        /* new polygons clear polygons. */
		CFT_BOOLEAN full_poly;             /* new polygons are full polygons. */
		CFT_BOOLEAN unique_names;          /* OBSOLETE: force unique names */
		CFT_BOOLEAN snap_pin;              /* snap to pins and pads */
		CFT_BOOLEAN snap_offgrid_line;     /* Snap to certain off-grid points along a line. */
		CFT_BOOLEAN marker_snaps;          /* marker snaps to grid or snap points, as any other click */
		CFT_BOOLEAN highlight_on_point;    /* Highlight if crosshair is on endpoints. */
		CFT_BOOLEAN show_solder_side;      /* mirror output */
		CFT_BOOLEAN save_last_command;     /* OBSOLETE: use the session-persistent command line history instead (press the up arrow) */
		CFT_INTEGER line_refraction;       /* value for line lookahead setting */
		CFT_BOOLEAN save_in_tmp;           /* emergency save unsaved PCB data (despite the user clicks don't save) when: user starts a new PCB; user quits pcb-rnd. Does not affect the on-crash emergency save. */
		CFT_BOOLEAN draw_grid;             /* draw grid points */
		CFT_BOOLEAN all_direction_lines;   /* enable lines to all directions */
		CFT_BOOLEAN rubber_band_mode;      /* move, rotate use rubberband connections */
		CFT_BOOLEAN rubber_band_keep_midlinedir; /* keep line direction when a middle line is moved */
		CFT_BOOLEAN swap_start_direction;  /* change starting direction after each click */
		CFT_BOOLEAN show_drc;              /* show drc region on crosshair */
		CFT_BOOLEAN auto_drc;              /* when set, PCB doesn't let you place copper that violates DRC. */
		CFT_BOOLEAN conn_find_rat;         /* connection find includes rats; when off, only existing galvanic connections are mapped */
		CFT_BOOLEAN show_number;           /* OBSOLETE: pinout shows number */
		CFT_BOOLEAN orthogonal_moves;      /* move items orthogonally. */
		CFT_BOOLEAN reset_after_element;   /* OBSOLETE: reset connections after each element while saving all connections */
		CFT_BOOLEAN auto_place;            /* force placement of GUI windows (dialogs), trying to override the window manager */
		CFT_BOOLEAN lock_names;            /* lock down text so they can not be moved or selected */
		CFT_BOOLEAN only_names;            /* lock down everything else but text so only text objects can be moved or selected */
		CFT_BOOLEAN thin_draw;             /* if set, objects on the screen are drawn as outlines (lines are drawn as center-lines).  This lets you see line endpoints hidden under pins, for example. */
		CFT_BOOLEAN thin_draw_poly;        /* if set, polygons on the screen are drawn as outlines. */
		CFT_BOOLEAN as_drawn_poly;         /* if set, also draw the as-drawn outline of polygons */
		CFT_BOOLEAN wireframe_draw;        /* if set, lines and arcs on the screen are drawn as outlines. */
		CFT_BOOLEAN local_ref;             /* use local reference for moves, by setting the mark at the beginning of each move. */
		CFT_BOOLEAN check_planes;          /* when set, only polygons and their clearances are drawn, to see if polygons have isolated regions. */
		CFT_BOOLEAN hide_names;            /* when set, subc floater text objects (typical use case: refdes text) are not drawn. */
		CFT_BOOLEAN description;           /* obsolete - DO NOT USE - kept for compatibility */
		CFT_BOOLEAN name_on_pcb;           /* obsolete - DO NOT USE - kept for compatibility */
		CFT_STRING  subc_id;               /* subcircuit ID template for diplaying the subcircuit label on the subcircuit layer; default to displaying the refes, if empty; syntax if the same as for DYNTEXT */
		CFT_STRING  term_id;               /* terminal ID template for diplaying the subcircuit label on the subcircuit layer; default to displaying termid[intconn], if empty; syntax if the same as for DYNTEXT */
		CFT_BOOLEAN fullscreen;            /* hide widgets to make more room for the drawing */
		CFT_BOOLEAN move_linepoint_uses_route;	/* Moving a line point calculates a new line route. This allows 45/90 line modes when editing lines. */
		CFT_BOOLEAN auto_via;              /* when drawing traces and switching layers or when moving an object from one layer to another, try to keep connections by automatically inserting vias. */
		CFT_REAL route_radius;             /* temporary: route draw helper's arc radius at corners (factor of the trace thickness) */

		CFT_BOOLEAN io_incomp_popup;       /* wether to enable popping up the io incompatibility list dialog on save incompatibility errors */
		CFT_STRING io_incomp_style;        /* view listing style (list or simple), when io_incomp_popup is true */

		CFT_INTEGER click_time;            /* default time for click expiration, in ms */

		struct view {
			CFT_BOOLEAN flip_x;              /* view: flip the board along the X (horizontal) axis */
			CFT_BOOLEAN flip_y;              /* view: flip the board along the Y (vertical) axis */
		} view;

		struct selection {
			CFT_BOOLEAN disable_negative;    /* selection box behaviour: disable the negative-direction selection - any selection box will select only what's fully within the box */
			CFT_BOOLEAN symmetric_negative;  /* selection box behaviour: when set, the selection direction is considered negative only if the box has negative size in the X direction */
		} selection;

		/* these two would need to be moved in the router plugin.... There are two
		   reasons to keep them here:
		   - the original pcb and pcb-rnd file formats already have named/numbered flags for these, so io_pcb needs these
		   - more than one router plugin may share these */
		CFT_BOOLEAN enable_stroke;         /* Enable libstroke gestures on middle mouse button when non-zero */
		CFT_BOOLEAN live_routing;          /* autorouter shows tracks in progress */

		/* Keep it here instead of the router plugin: more than one router plugin may share these */
		CFT_BOOLEAN beep_when_finished;    /* flag if a signal should be produced when searching of  connections is done */

		CFT_INTEGER undo_warning_size;     /* warn the user when undo list exceeds this amount of kilobytes in memory */

		CFT_STRING subc_conv_refdes;       /* automatic refdes value assigned to new subcircuits on conversion from objects - if empty, no refdes text or attribute is added; if the value is <?>, the refdes text object is added but no refdes attribute is created */
	} editor;

	const struct rc {
		CFT_INTEGER verbose;
		CFT_INTEGER quiet;                 /* print only errors on stderr */
		CFT_BOOLEAN dup_log_to_stderr;     /* copy log messages to stderr even if there is a HID that can show them */
		CFT_INTEGER backup_interval;       /* time between two backups in seconds; 0 means disabled (no backups) */
		CFT_BOOLEAN hid_fallback;          /* if there is no explicitly specified HID (--gui) and the preferred GUI fails, automatically fall back on other HIDs, eventually running in batch mode */
		CFT_STRING brave;                  /* brave mode flags: when non-empty, enable various experimental (unstable) features - useful for testers */
		CFT_STRING font_command;           /* file name template; if not empty, run this command and read its output for loading the font; %f is the file name  */
		CFT_STRING file_command;           /* file name template; if not empty, run this command and read its output for loading a pcb file; %f is the file name, %p is the conf setting rc.file_path */
		CFT_STRING file_path;
		CFT_STRING library_shell;
		CFT_LIST library_search_paths;
		CFT_STRING menu_file;              /* where to load the default menu file from. If empty/unset, fall back to the legacy 'per hid ow menu file' setup. If contains slash, take it as a full path, if no slash, do a normal menu search for pcb-menu-NAME.lht */
		CFT_BOOLEAN export_basename;       /* if an exported file contains the source file name, remove path from it, keeping the basename only */

		CFT_STRING emergency_name;         /* file name template for emergency save anonymous .pcb files (when pcb-rnd crashes); optional field: %ld --> pid; must be shorter than 240 characters. Don't do emergency save if this item is empty. */
		CFT_STRING emergency_format;       /* if set, use this format for the backups; if unset, use the default format */
		CFT_STRING backup_name;            /* file name template for periodic backup of board files; optional fields (the usual % substitutions work) */
		CFT_STRING backup_format;          /* if set, use this format for the backups; if unset or set to 'original', use the original format */

		CFT_STRING save_command;           /* command to pipe the pcb, footprint or buffer file into, when saving (makes lihata persist impossible) */
		CFT_BOOLEAN keep_save_backups;     /* a copy is made before a save operation overwrites an existing file; if this setting is true, keep the copy even after a successful save */
		CFT_LIST default_font_file;        /* name of default font file (list of names to search) */
		CFT_LIST default_pcb_file;

		CFT_STRING script_filename;        /* PCB Actions script to execute on startup */
		CFT_STRING action_string;          /* PCB Actions string to execute on startup */
		CFT_STRING rat_path;
		CFT_STRING rat_command;            /* file name template; if not empty, run this command and read its output for loading a rats; %f is the file name, %p is the rc.rat_path conf setting */

		CFT_LIST preferred_gui;            /* if set, try GUI HIDs in this order when no GUI is explicitly selected */

		CFT_STRING save_final_fallback_fmt;/* when a new file is created (by running pcb-rnd with the file name) there won't be a known format; pcb-rnd will guess from the file name (extension) but eventhat may fail. This format is the final fallback that'll be used if no other guessing mechanism worked. The user can override this by save as. */
		CFT_STRING save_fp_fmt;            /* when saving a buffer element/subcircuit, prefer this format by default */

		CFT_STRING cli_prompt;             /* plain text prompt to prefix the command entry */
		CFT_STRING cli_backend;            /* command parser action */

		/***** automatically set (in postproc) *****/
		CFT_BOOLEAN have_regex;            /* whether we have regex compiled in */
		struct path {
			CFT_STRING prefix;               /* e.g. /usr/local */
			CFT_STRING lib;                  /* e.g. /usr/lib/pcb-rnd */
			CFT_STRING bin;                  /* e.g. /usr/bin */
			CFT_STRING share;                /* e.g. /usr/share/pcb-rnd */
			CFT_STRING home;                 /* user's home dir, determined run-time */
			
			CFT_STRING exec_prefix;          /* exec prefix path (extracted from argv[0]) */

			CFT_STRING design;               /* directory path of the current design, or <invalid> if the current design doesn't have a file name yet */
		} path;
	} rc;

	const struct design { /* defaults of a new layout */
		CFT_COORD via_thickness;
		CFT_COORD via_drilling_hole;
		CFT_COORD line_thickness;
		CFT_COORD clearance;

		CFT_COORD bloat;                   /* minimum space between copper features on different networks */
		CFT_COORD shrink;                  /* minimum overlap between connected copper features */
		CFT_COORD min_wid;                 /* minimum copper width */
		CFT_COORD min_slk;                 /* minimum silk width */
		CFT_COORD min_drill;               /* minimum drill diameter */
		CFT_COORD min_ring;                /* minimum annular ring */
		CFT_INTEGER text_scale;            /* text scaling in % */
		CFT_COORD text_thickness;          /* override stroke font text thickness */
		CFT_INTEGER text_font_id;
		CFT_REAL poly_isle_area;           /* polygon min area */
		CFT_STRING fab_author;             /* Full name of author for FAB drawings */
		CFT_STRING initial_layer_stack;    /* deprecated. */

		CFT_COORD paste_adjust;            /* Adjust paste thickness */
	} design;

/* @path appearance/color */
	const struct appearance {
		CFT_BOOLEAN compact;               /* when set: optimize GUI widget arrangement for small screen; may be wasting some screen space on large screen */
		CFT_COORD rat_thickness;
		CFT_COORD mark_size;               /* relative marker size */
		CFT_REAL layer_alpha;              /* alpha value for layer drawing */
		CFT_REAL drill_alpha;              /* alpha value for drill drawing */
		CFT_BOOLEAN text_host_bbox;        /* when moving a text object, the outline thin-draw should also include the bounding box */
		CFT_REAL term_label_size;          /* size of terminal labels, in pcb font scale (100 is for the normal size) */
		CFT_BOOLEAN subc_layer_per_side;   /* hide top or bottom placed subcircuit annotations if the view is showing the other side */
		CFT_BOOLEAN invis_other_groups;    /* render non-current group layers with the inivisble color */
		CFT_BOOLEAN black_current_group;   /* render all layers of the current group black, for maximum contrast */
		struct loglevels {
			CFT_STRING   debug_tag;          /* log style tag of debug messages */
			CFT_BOOLEAN  debug_popup;        /* whether a debug line should pop up the log window */
			CFT_STRING   info_tag;           /* log style tag of info messages */
			CFT_BOOLEAN  info_popup;         /* whether an info line should pop up the log window */
			CFT_STRING   warning_tag;        /* log style tag of warnings */
			CFT_BOOLEAN  warning_popup;      /* whether a warning should pop up the log window */
			CFT_STRING   error_tag;          /* log style tag of errors */
			CFT_BOOLEAN  error_popup;        /* whether an error should pop up the log window */
		} loglevels;
		struct color {
			CFT_COLOR background;            /* background and cursor color ... */
			CFT_COLOR crosshair;             /* different object colors */
			CFT_COLOR cross;                 /* crosshair, drc outline color */
			CFT_COLOR selected;              /* generic object selection color */
			CFT_COLOR via;                   /* non-terminal padstack shape on current layer */
			CFT_COLOR via_far;               /* non-terminal padstack shape on non-current ('far side') layer */
			CFT_COLOR pin;                   /* terminal padstack shape on current layer */
			CFT_COLOR pin_far;               /* terminal padstack shape on non-current ('far side') layer */
			CFT_COLOR pin_name;              /* on-screen terminal number/name labels */
			CFT_COLOR subc;                  /* on-screen subcircuit marks */
			CFT_COLOR subc_nonetlist;        /* on-screen subcircuit marks for subcircuits with the nonetlist flag */
			CFT_COLOR padstackmark;          /* on-screen center mark cross for padstacks */
			CFT_COLOR rat;                   /* on-screen rat lines */
			CFT_COLOR invisible_objects;     /* other-side objects and padstack shapes on non-current layer */
			CFT_COLOR connected;             /* 'connected' highlight (galvanic connections found) */
			CFT_COLOR warn;                  /* warning highlight (e.g. object found to cause a short) */
			CFT_COLOR off_limit;             /* on-screen background beyond the configured drawing area */
			CFT_COLOR grid;                  /* on-screen grid */
			CFT_COLOR layer[PCB_MAX_LAYER];  /* default layer colors; when a new layer is created, a color from this list is assigned initially */
			CFT_COLOR mask;                  /* default mask layer color (when a new mask layer is created) */
			CFT_COLOR paste;                 /* default paste layer color (when a new paste layer is created) */
			CFT_COLOR element;               /* default silk layer color (when a new silk layer is created) */
		} color;
		struct padstack {
			CFT_INTEGER cross_thick;      /* cross thickness in pixels - 0 means disable crosses */
			CFT_COORD cross_size;         /* cross size in word coords - size of one arm of the cross (minus the hole radius) */
		} padstack;
		struct subc {
			CFT_INTEGER dash_freq;        /* how dense the dashed outline should be; -1 means do not display the dashed outline; 0 means solid outline; 1..32 means dashed outline */
		} subc;
		struct messages {
			CFT_INTEGER char_per_line;   /* width of an output line in characters (used by separator drawing in find.c) */		
		} messages;
		struct misc {
			CFT_INTEGER volume;          /* the speakers volume -100..100 */
		} misc;
	} appearance;

} conf_core_t;

extern conf_core_t conf_core;
void conf_core_init();
void pcb_conf_ro(const char *path);
#endif
