#ifndef PCB_CONF_CORE_H
#define PCB_CONF_CORE_H

#include "conf.h"

/* NOTE: this struct has a strict format because a code generator needs to
   read it. Please always keep the format (preferrably even whitespace style).
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
		CFT_BOOLEAN full_poly;             /* new polygons are full polygons. */
		CFT_BOOLEAN unique_names;          /* force unique names */
		CFT_BOOLEAN snap_pin;              /* snap to pins and pads */
		CFT_BOOLEAN snap_offgrid_line;     /* Snap to certain off-grid points along a line. */
		CFT_BOOLEAN highlight_on_point;    /* Highlight if crosshair is on endpoints. */
		CFT_BOOLEAN show_solder_side;      /* mirror output */
		CFT_BOOLEAN save_last_command;     /* save the last command entered by user */
		CFT_INTEGER line_refraction;	   /* value for line lookahead setting */
		CFT_BOOLEAN save_in_tmp;           /* always save data in /tmp */
		CFT_BOOLEAN draw_grid;             /* draw grid points */
		CFT_BOOLEAN all_direction_lines;   /* enable lines to all directions */
		CFT_BOOLEAN rubber_band_mode;      /* move, rotate use rubberband connections */
		CFT_BOOLEAN swap_start_direction;  /* change starting direction after each click */
		CFT_BOOLEAN show_drc;              /* show drc region on crosshair */
		CFT_BOOLEAN auto_drc;              /* when set, PCB doesn't let you place copper that violates DRC. */
		CFT_BOOLEAN show_number;           /* pinout shows number */
		CFT_BOOLEAN orthogonal_moves;      /* move items orthogonally. */
		CFT_BOOLEAN reset_after_element;   /* reset connections after each element */
		CFT_BOOLEAN auto_place;            /* flag which says we should force placement of the windows on startup */
		CFT_BOOLEAN lock_names;            /* lock down text so they can not be moved or selected */
		CFT_BOOLEAN only_names;            /* lock down everything else but text so only text objects can be moved or selected */
		CFT_BOOLEAN thin_draw;             /* if set, objects on the screen are drawn as outlines (lines are drawn as center-lines).  This lets you see line endpoints hidden under pins, for example. */
		CFT_BOOLEAN thin_draw_poly;        /* if set, polygons on the screen are drawn as outlines. */
		CFT_BOOLEAN local_ref;             /* use local reference for moves, by setting the mark at the beginning of each move. */
		CFT_BOOLEAN check_planes;          /* when set, only polygons and their clearances are drawn, to see if polygons have isolated regions. */
		CFT_BOOLEAN show_mask;             /* show the solder mask layer */
		CFT_BOOLEAN hide_names;            /* when set, element names are not drawn. */
		CFT_BOOLEAN description;           /* display element description as element name, instead of value */
		CFT_BOOLEAN name_on_pcb;           /* display Reference Designator as element name, instead of value */

		CFT_INTEGER click_time;            /* default time for click expiration, in ms */

		struct view {
			CFT_BOOLEAN flip_x;              /* view: flip the board along the X (horizontal) axis */
			CFT_BOOLEAN flip_y;              /* view: flip the board along the Y (vertical) axis */
		} view;

		/* these two would need to be moved in the router plugin.... There are two
		   reasons to keep them here:
		   - the original pcb and pcb-rnd file formats already have named/numbered flags for these, so io_pcb needs these
		   - more than one router plugin may share these */
		CFT_BOOLEAN enable_stroke;         /* Enable libstroke gesutres on middle mouse button when non-zero */
		CFT_BOOLEAN live_routing;          /* autorouter shows tracks in progress */

		/* Keep it here instead of the router plugin: more than one router plugin may share these */
		CFT_BOOLEAN beep_when_finished;    /* flag if a signal should be produced when searching of  connections is done */

		CFT_INTEGER undo_warning_size;     /* warn the user when undo list exceeds this amount of kilobytes in memory */
	} editor;

	const struct rc {
		CFT_INTEGER verbose;
		CFT_INTEGER backup_interval;       /* time between two backups in seconds */
		CFT_STRING font_command;           /* commands for file loading... */
		CFT_STRING file_command;
		CFT_STRING file_path;
		CFT_STRING library_shell;
		CFT_LIST library_search_paths;

		CFT_STRING emergency_name;         /* file name template for emergency save anonymous .pcb files (when pcb-rnd crashes); optional fields: %P --> pid */
		CFT_STRING backup_name;            /* file name template for periodic backup anonymous .pcb files; optional fields: %P --> pid */


		CFT_STRING save_command;
		CFT_LIST default_font_file;        /* name of default font file (list of names to search) */
		CFT_LIST default_pcb_file;

		CFT_STRING script_filename;        /* PCB Actions script to execute on startup */
		CFT_STRING action_string;          /* PCB Actions string to execute on startup */
		CFT_STRING rat_path;
		CFT_STRING rat_command;

		CFT_LIST preferred_gui;            /* if set, try GUI HIDs in this order when no GUI is explicitly selected */

		/***** automatically set (in postporc) *****/
		CFT_BOOLEAN have_regex;            /* whether we have regex compiled in */
		struct path {
			CFT_STRING prefix;               /* e.g. /usr/local */
			CFT_STRING lib;                  /* e.g. /usr/lib/pcb-rnd */
			CFT_STRING bin;                  /* e.g. /usr/bin */
			CFT_STRING share;                /* e.g. /usr/share/pcb-rnd */
			CFT_STRING home;                 /* user's home dir, determined run-time */
			
			CFT_STRING exec_prefix;          /* exec prefix path (extracted from argv[0]) */
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
		CFT_REAL poly_isle_area;  /* polygon min area */
		CFT_STRING default_layer_name[MAX_LAYER];
		CFT_STRING fab_author;                  /* Full name of author for FAB drawings */
		CFT_STRING initial_layer_stack;         /* If set, the initial layer stack is set to this */

		CFT_STRING groups;                 /* string with layergroups */
		CFT_STRING routes;                 /* string with route styles */
	} design;

/* @path appearance/color */
	const struct appearance {
		CFT_COORD rat_thickness;
		CFT_COORD mark_size;               /* relative marker size */
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
			CFT_COLOR rat;
			CFT_COLOR invisible_objects;
			CFT_COLOR invisible_mark;
			CFT_COLOR element_selected;
			CFT_COLOR rat_selected;
			CFT_COLOR connected;
			CFT_COLOR off_limit;
			CFT_COLOR grid;
			CFT_COLOR layer[MAX_LAYER];
			CFT_COLOR layer_selected[MAX_LAYER];
			CFT_COLOR warn;
			CFT_COLOR mask;
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
#endif
