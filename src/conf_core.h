#ifndef PCB_CONF_CORE_H
#define PCB_CONF_CORE_H

#include "globalconst.h"
#include "conf.h"

/* NOTE: this struct has a strict format because a code generator needs to
   read it. Please always keep the format (preferrably even whitespace style).
   Use only the CFT_* prefixed types declared in conf.h.
   */

typedef struct {

	struct editor {
		CFT_UNIT grid_unit;
		CFT_COORD grid; /* grid in pcb-units */
#if 0
	const Increments increments;
#endif
		CFT_REAL zoom; /* default zoom */
		CFT_INTEGER mode;  /* currently active mode */
		CFT_INTEGER buffer_number; /* number of the current buffer */
		CFT_BOOLEAN clear_line;
		CFT_BOOLEAN full_poly;
		CFT_BOOLEAN unique_names;          /* force unique names */
		CFT_BOOLEAN snap_pin;              /* snap to pins and pads */
		CFT_BOOLEAN snap_offgrid_line;     /* Snap to certain off-grid points along a line. */
		CFT_BOOLEAN highlight_on_point;    /* Highlight if crosshair is on endpoints. */
		CFT_BOOLEAN show_solder_side;      /* mirror output */
		CFT_BOOLEAN save_last_command;     /* save the last command entered by user */
		CFT_BOOLEAN save_in_tmp;           /* always save data in /tmp */
		CFT_BOOLEAN draw_grid;             /* draw grid points */
		CFT_BOOLEAN rat_warn;              /* rats nest has set warnings */
		CFT_BOOLEAN stipple_polygons;      /* draw polygons with stipple */
		CFT_BOOLEAN all_direction_lines;   /* enable lines to all directions */
		CFT_BOOLEAN rubber_band_mode;      /* move, rotate use rubberband connections */
		CFT_BOOLEAN swap_start_direction;  /* change starting direction after each click */
		CFT_BOOLEAN show_drc;              /* show drc region on crosshair */
		CFT_BOOLEAN auto_drc;              /* */
		CFT_BOOLEAN show_number;           /* pinout shows number */
		CFT_BOOLEAN orthogonal_moves;      /* */
		CFT_BOOLEAN reset_after_element;   /* reset connections after each element */
		CFT_BOOLEAN auto_place;            /* flag which says we should force placement of the windows on startup */
#warning TODO: move this in the plugin
		CFT_BOOLEAN enable_mincut;         /* Enable calculating mincut on shorts (rats_mincut.c) when non-zero */
#warning TODO: move this in the plugin
		CFT_BOOLEAN enable_stroke;         /* Enable libstroke gesutres on middle mouse button when non-zero */
#warning TODO: move these to autorouter
		CFT_BOOLEAN live_routing;          /* autorouter shows tracks in progress */
		CFT_BOOLEAN beep_when_finished;    /* flag if a signal should be produced when searching of  connections is done */

	} editor;

	struct rc {
		CFT_INTEGER verbose;
		CFT_INTEGER backup_interval;       /* time between two backups in seconds */
		CFT_STRING font_command;           /* commands for file loading... */
		CFT_STRING file_command;
		CFT_STRING file_path;
#warning move this to printer?
		CFT_STRING print_file;
		CFT_STRING library_shell;
#warning TODO: this should be a list
/*		CFT_LIST library_search_paths; */
		CFT_STRING library_search_paths;

		CFT_STRING save_command;
		CFT_LIST default_font_file;  /* name of default font file */
		CFT_LIST default_pcb_file;

#warning move these two to design
		CFT_STRING groups;                 /* string with layergroups */
		CFT_STRING routes;                 /* string with route styles */

		CFT_STRING script_filename;        /* PCB Actions script to execute on startup */
		CFT_STRING action_string;          /* PCB Actions string to execute on startup */
		CFT_STRING rat_path;
		CFT_STRING rat_command;

#warning move to the import plugin:
		CFT_STRING gnetlist_program;       /* gnetlist program name */
		CFT_STRING make_program;           /* make program name */
	} rc;

	struct design { /* defaults of a new layout */
#warning maybe the first 5 and some others should be in editor?
		CFT_COORD via_thickness;
		CFT_COORD via_drilling_hole;
#warning appear?
		CFT_COORD line_thickness;
#warning move this to appearance
		CFT_COORD rat_thickness;
#warning rename this to clearance?
		CFT_COORD keepaway;

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
		CFT_STRING background_image;            /* PPM file for board background */
		CFT_STRING fab_author;                  /* Full name of author for FAB drawings */
		CFT_STRING initial_layer_stack;         /* If set, the initial layer stack is set to this */
	} design;

/* @path appearance/color */
	struct appearance {
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
			CFT_COORD offset_x;           /* offset of origin */
			CFT_COORD offset_y;
			CFT_COORD text_offset_x;      /* offset of text from pin center */
			CFT_COORD text_offset_y;
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

#endif
