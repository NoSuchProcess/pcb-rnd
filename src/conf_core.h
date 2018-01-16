#ifndef PCB_CONF_CORE_H
#define PCB_CONF_CORE_H

#include "conf.h"
#include "globalconst.h"

/* NOTE: this struct has a strict format because a code generator needs to
   read it. Please always keep the format (preferably even whitespace style).
   Use only the CFT_* prefixed types declared in conf.h.
   */

typedef struct {

	struct temp {
		CFT_BOOLEAN rat_warn;              /* rats nest has set warnings */
	} temp;

	const struct editor {
		CFT_UNIT grid_unit;                /* select whether you draw in mm or mil */
		CFT_COORD grid;                    /* grid in pcb-units */
		CFT_INCREMENTS increments_mm;      /* increments (size deltas) when drawing in mil */
		CFT_INCREMENTS increments_mil;     /* increments (size deltas) when drawing in mil */
		CFT_REAL zoom;                     /* default zoom */
		CFT_INTEGER mode;                  /* currently active mode */
		CFT_INTEGER buffer_number;         /* number of the current buffer */
		CFT_BOOLEAN clear_line;            /* new lines/arc clear polygons. */
		CFT_BOOLEAN clear_polypoly;        /* new polygons clear polygons. */
		CFT_BOOLEAN full_poly;             /* new polygons are full polygons. */
		CFT_BOOLEAN unique_names;          /* force unique names */
		CFT_BOOLEAN snap_pin;              /* snap to pins and pads */
		CFT_BOOLEAN snap_offgrid_line;     /* Snap to certain off-grid points along a line. */
		CFT_BOOLEAN marker_snaps;          /* marker snaps to grid or snp points, as any other click */
		CFT_BOOLEAN highlight_on_point;    /* Highlight if crosshair is on endpoints. */
		CFT_BOOLEAN show_solder_side;      /* mirror output */
		CFT_BOOLEAN save_last_command;     /* the command entry editline always starts with the last command entered by user in the current session */
		CFT_INTEGER line_refraction;       /* value for line lookahead setting */
		CFT_BOOLEAN save_in_tmp;           /* emergency save unsaved PCB data (despite the user clicks don't save) when: user starts a new PCB; user quits pcb-rnd. Does not affect the on-crash emergency save. */
		CFT_BOOLEAN draw_grid;             /* draw grid points */
		CFT_BOOLEAN all_direction_lines;   /* enable lines to all directions */
		CFT_BOOLEAN rubber_band_mode;      /* move, rotate use rubberband connections */
		CFT_BOOLEAN rubber_band_keep_midlinedir; /* keep line direction when a middle line is moved */
		CFT_BOOLEAN swap_start_direction;  /* change starting direction after each click */
		CFT_BOOLEAN show_drc;              /* show drc region on crosshair */
		CFT_BOOLEAN auto_drc;              /* when set, PCB doesn't let you place copper that violates DRC. */
		CFT_BOOLEAN show_number;           /* pinout shows number */
		CFT_BOOLEAN orthogonal_moves;      /* move items orthogonally. */
		CFT_BOOLEAN reset_after_element;   /* reset connections after each element while saving all connections */
		CFT_BOOLEAN auto_place;            /* flag which says we should force placement of the windows on startup */
		CFT_BOOLEAN lock_names;            /* lock down text so they can not be moved or selected */
		CFT_BOOLEAN only_names;            /* lock down everything else but text so only text objects can be moved or selected */
		CFT_BOOLEAN thin_draw;             /* if set, objects on the screen are drawn as outlines (lines are drawn as center-lines).  This lets you see line endpoints hidden under pins, for example. */
		CFT_BOOLEAN thin_draw_poly;        /* if set, polygons on the screen are drawn as outlines. */
		CFT_BOOLEAN wireframe_draw;        /* if set, lines and arcs on the screen are drawn as outlines. */
		CFT_BOOLEAN local_ref;             /* use local reference for moves, by setting the mark at the beginning of each move. */
		CFT_BOOLEAN check_planes;          /* when set, only polygons and their clearances are drawn, to see if polygons have isolated regions. */
		CFT_BOOLEAN hide_names;            /* when set, element names are not drawn. */
		CFT_BOOLEAN description;           /* display element description as element name, instead of value */
		CFT_BOOLEAN name_on_pcb;           /* display Reference Designator as element name, instead of value */
		CFT_BOOLEAN fullscreen;            /* hide widgets to make more room for the drawing */
		CFT_BOOLEAN move_linepoint_uses_route;	/* Moving a line point calculates a new line route. This allows 45/90 line modes when editing lines. */
		CFT_BOOLEAN auto_via;              /* when drawing traces and switching layers or when moving an object from one layer to another, try to keep connections by automatically inserting vias. */
		CFT_REAL route_radius;             /* temporary: route draw helper's arc radius at corners (factor of the trace thickness) */

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
	} editor;

	const struct rc {
		CFT_INTEGER verbose;
		CFT_INTEGER quiet;                 /* print only errors on stderr */
		CFT_INTEGER backup_interval;       /* time between two backups in seconds; 0 means disabled (no backups) */
		CFT_STRING brave;                  /* brave mode flags: when non-empty, enable various experimental (unstable) features - useful for testers */
		CFT_STRING font_command;           /* file name template; if not empty, run this command and read its output for loading the font; %f is the file name  */
		CFT_STRING file_command;           /* file name template; if not empty, run this command and read its output for loading a pcb file; %f is the file name, %p is the conf setting rc.file_path */
		CFT_STRING file_path;
		CFT_STRING library_shell;
		CFT_LIST library_search_paths;

		CFT_STRING emergency_name;         /* file name template for emergency save anonymous .pcb files (when pcb-rnd crashes); optional field: %ld --> pid; must be shorter than 240 characters. Don't do emergency save if this item is empty. */
		CFT_STRING emergency_format;       /* if set, use this format for the backups; if unset, use the default format */
		CFT_STRING backup_name;            /* file name template for periodic backup anonymous .pcb files; optional fields: %P --> pid */
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

		CFT_COORD max_width;
		CFT_COORD max_height;
		CFT_COORD alignment_distance;/* default drc size */
		CFT_COORD bloat; /* default drc size */
		CFT_COORD shrink;
		CFT_COORD min_wid;
		CFT_COORD min_slk;
		CFT_COORD min_drill;
		CFT_COORD min_ring;
		CFT_INTEGER text_scale;   /* text scaling in % */
		CFT_INTEGER text_font_id;
		CFT_REAL poly_isle_area;  /* polygon min area */
		CFT_STRING default_layer_name[PCB_MAX_LAYER];
		CFT_STRING fab_author;                  /* Full name of author for FAB drawings */
		CFT_STRING initial_layer_stack;         /* deprecated. */

		CFT_STRING groups;                 /* string with layergroups */
		CFT_STRING routes;                 /* string with route styles */

		CFT_COORD paste_adjust;            /* Adjust paste thickness */
	} design;

/* @path appearance/color */
	const struct appearance {
		CFT_COORD rat_thickness;
		CFT_COORD mark_size;               /* relative marker size */
		CFT_REAL layer_alpha;              /* alpha value for layer drawing */
		CFT_REAL drill_alpha;              /* alpha value for drill drawing */
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
			CFT_COLOR black;
			CFT_COLOR white;
			CFT_COLOR background;	/* background and cursor color ... */
			CFT_COLOR crosshair;							/* different object colors */
			CFT_COLOR cross;
			CFT_COLOR via;
			CFT_COLOR via_selected;
			CFT_COLOR pin;
			CFT_COLOR pin_selected;
			CFT_COLOR pin_name;
			CFT_COLOR element;
			CFT_COLOR element_nonetlist;
			CFT_COLOR subc;
			CFT_COLOR subc_selected;
			CFT_COLOR subc_nonetlist;
			CFT_COLOR padstackmark;          /* color of the center mark cross for padstacks */
			CFT_COLOR rat;
			CFT_COLOR invisible_objects;
			CFT_COLOR invisible_mark;
			CFT_COLOR element_selected;
			CFT_COLOR rat_selected;
			CFT_COLOR connected;
			CFT_COLOR off_limit;
			CFT_COLOR grid;
			CFT_COLOR layer[PCB_MAX_LAYER];
			CFT_COLOR layer_selected[PCB_MAX_LAYER];
			CFT_COLOR warn;
			CFT_COLOR mask;
			CFT_COLOR paste;
		} color;
		struct pinout {
			CFT_INTEGER name_length;
			CFT_REAL zoom;
			CFT_COORD offset_x;           /* X offset of origin */
			CFT_COORD offset_y;           /* Y offset of origin */
			CFT_COORD text_offset_x;      /* X offset of text from pin center */
			CFT_COORD text_offset_y;      /* Y offset of text from pin center */
		} pinout;
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
void conf_core_postproc();
void pcb_conf_ro(const char *path);
#endif
