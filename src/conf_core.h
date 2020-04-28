#ifndef PCB_CONF_CORE_H
#define PCB_CONF_CORE_H

#include <librnd/core/conf.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/color.h>

/* NOTE: this struct has a strict format because a code generator needs to
   read it. Please always keep the format (preferably even whitespace style).
   Use only the CFT_* prefixed types declared in conf.h.
   */

typedef struct {

	struct {
		RND_CFT_BOOLEAN rat_warn;              /* rats nest has set warnings */
		RND_CFT_BOOLEAN clip_inhibit_chg;      /* dummy node to inform the menu about clip inhibit change */
	} temp;

	const struct {                       /* editor */
		RND_CFT_REAL zoom;                     /* default zoom */
		RND_CFT_INTEGER buffer_number;         /* number of the current buffer */
		RND_CFT_BOOLEAN clear_line;            /* new lines/arc clear polygons. */
		RND_CFT_BOOLEAN clear_polypoly;        /* new polygons clear polygons. */
		RND_CFT_BOOLEAN full_poly;             /* new polygons are full polygons. */
		RND_CFT_BOOLEAN unique_names;          /* OBSOLETE: force unique names */
		RND_CFT_BOOLEAN snap_pin;              /* snap to pins and pads */
		RND_CFT_BOOLEAN snap_offgrid_line;     /* Snap to certain off-grid points along a line. */
		RND_CFT_BOOLEAN marker_snaps;          /* marker snaps to grid or snap points, as any other click */
		RND_CFT_BOOLEAN highlight_on_point;    /* Highlight if crosshair is on endpoints. */
		RND_CFT_BOOLEAN show_solder_side;      /* mirror output */
		RND_CFT_BOOLEAN save_last_command;     /* OBSOLETE: use the session-persistent command line history instead (press the up arrow) */
		RND_CFT_INTEGER line_refraction;       /* value for line lookahead setting */
		RND_CFT_BOOLEAN save_in_tmp;           /* emergency save unsaved PCB data (despite the user clicks don't save) when: user starts a new PCB; user quits pcb-rnd. Does not affect the on-crash emergency save. */
		RND_CFT_BOOLEAN all_direction_lines;   /* enable lines to all directions */
		RND_CFT_BOOLEAN rubber_band_mode;      /* move, rotate use rubberband connections */
		RND_CFT_BOOLEAN rubber_band_keep_midlinedir; /* keep line direction when a middle line is moved */
		RND_CFT_BOOLEAN swap_start_direction;  /* change starting direction after each click */
		RND_CFT_BOOLEAN show_drc;              /* show drc region on crosshair */
		RND_CFT_BOOLEAN auto_drc;              /* when set, PCB doesn't let you place copper that violates DRC. */
		RND_CFT_BOOLEAN conn_find_rat;         /* connection find includes rats; when off, only existing galvanic connections are mapped */
		RND_CFT_BOOLEAN show_number;           /* OBSOLETE: pinout shows number */
		RND_CFT_BOOLEAN orthogonal_moves;      /* move items orthogonally. */
		RND_CFT_BOOLEAN reset_after_element;   /* OBSOLETE: reset connections after each element while saving all connections */
		RND_CFT_BOOLEAN lock_names;            /* lock down text so they can not be moved or selected */
		RND_CFT_BOOLEAN only_names;            /* lock down everything else but text so only text objects can be moved or selected */
		RND_CFT_BOOLEAN thin_draw;             /* if set, objects on the screen are drawn as outlines (lines are drawn as center-lines).  This lets you see line endpoints hidden under pins, for example. */
		RND_CFT_BOOLEAN thin_draw_poly;        /* if set, polygons on the screen are drawn as outlines. */
		RND_CFT_BOOLEAN as_drawn_poly;         /* if set, also draw the as-drawn outline of polygons */
		RND_CFT_BOOLEAN wireframe_draw;        /* if set, lines and arcs on the screen are drawn as outlines. */
		RND_CFT_BOOLEAN local_ref;             /* use local reference for moves, by setting the mark at the beginning of each move. */
		RND_CFT_BOOLEAN check_planes;          /* when set, only polygons and their clearances are drawn, to see if polygons have isolated regions. */
		RND_CFT_BOOLEAN hide_names;            /* when set, subc floater text objects (typical use case: refdes text) are not drawn. */
		RND_CFT_BOOLEAN description;           /* obsolete - DO NOT USE - kept for compatibility */
		RND_CFT_BOOLEAN name_on_pcb;           /* obsolete - DO NOT USE - kept for compatibility */
		RND_CFT_STRING  subc_id;               /* subcircuit ID template for diplaying the subcircuit label on the subcircuit layer; default to displaying the refes, if empty; syntax if the same as for DYNTEXT */
		RND_CFT_STRING  term_id;               /* terminal ID template for diplaying the subcircuit label on the subcircuit layer; default to displaying termid[intconn], if empty; syntax if the same as for DYNTEXT */
		RND_CFT_BOOLEAN move_linepoint_uses_route;	/* Moving a line point calculates a new line route. This allows 45/90 line modes when editing lines. */
		RND_CFT_BOOLEAN auto_via;              /* when drawing traces and switching layers or when moving an object from one layer to another, try to keep connections by automatically inserting vias. */
		RND_CFT_REAL route_radius;             /* temporary: route draw helper's arc radius at corners (factor of the trace thickness) */

		RND_CFT_BOOLEAN io_incomp_popup;       /* wether to enable popping up the io incompatibility list dialog on save incompatibility errors */
		RND_CFT_STRING io_incomp_style;        /* view listing style (list or simple), when io_incomp_popup is true */

		RND_CFT_INTEGER click_time;            /* default time for click expiration, in ms */

		struct {
			RND_CFT_BOOLEAN disable_negative;    /* selection box behaviour: disable the negative-direction selection - any selection box will select only what's fully within the box */
			RND_CFT_BOOLEAN symmetric_negative;  /* selection box behaviour: when set, the selection direction is considered negative only if the box has negative size in the X direction */
		} selection;


		/* this would need to be moved in the router plugin.... There are two
		   reasons to keep it here:
		   - the original pcb and pcb-rnd file formats already have named/numbered flags for it, so io_pcb needs it
		   - more than one router plugins may share it */
		RND_CFT_BOOLEAN live_routing;          /* autorouter shows tracks in progress */

		/* Keep it here instead of the router plugin: more than one router plugin may share these */
		RND_CFT_BOOLEAN beep_when_finished;    /* flag if a signal should be produced when searching of  connections is done */

		RND_CFT_INTEGER undo_warning_size;     /* warn the user when undo list exceeds this amount of kilobytes in memory */

		RND_CFT_STRING subc_conv_refdes;       /* automatic refdes value assigned to new subcircuits on conversion from objects - if empty, no refdes text or attribute is added; if the value is <?>, the refdes text object is added but no refdes attribute is created */
	} editor;

	struct {
		struct {
			RND_CFT_STRING method;             /* method/strategy of placement; one of: disperse, frame, fit */
			RND_CFT_STRING location;           /* placement coordinate for methods that require it; if empty, use fixed coordinates at x,y; if non-empty, it may be: mark, center */
			RND_CFT_COORD x;                   /* for some methods if location is empty, X coordinate of placement */
			RND_CFT_COORD y;                   /* for some methods if location is empty, Y coordinate of placement */
			RND_CFT_COORD disperse;            /* dispersion distance for the disperse method */
		} footprint_placement;

		struct {
			RND_CFT_STRING method;             /* method/strategy of removal; one of:  */
		} footprint_removal;
	} import;

	const struct {                       /* rc */
		RND_CFT_REAL file_changed_interval;    /* how often to check if the file has changed on the disk (in seconds); 0 or negative means no check at all */
		RND_CFT_INTEGER backup_interval;       /* time between two backups in seconds; 0 means disabled (no backups) */
		RND_CFT_STRING brave;                  /* brave mode flags: when non-empty, enable various experimental (unstable) features - useful for testers */
		RND_CFT_STRING font_command;           /* file name template; if not empty, run this command and read its output for loading the font; %f is the file name  */
		RND_CFT_STRING file_command;           /* file name template; if not empty, run this command and read its output for loading a pcb file; %f is the file name, %p is the conf setting rc.file_path */
		RND_CFT_STRING file_path;
		RND_CFT_STRING library_shell;
		RND_CFT_LIST library_search_paths;

		RND_CFT_STRING emergency_name;         /* file name template for emergency save anonymous .pcb files (when pcb-rnd crashes); optional field: %ld --> pid; must be shorter than 240 characters. Don't do emergency save if this item is empty. */
		RND_CFT_STRING emergency_format;       /* if set, use this format for the backups; if unset, use the default format */
		RND_CFT_STRING backup_name;            /* file name template for periodic backup of board files; optional fields (the usual % substitutions work) */
		RND_CFT_STRING backup_format;          /* if set, use this format for the backups; if unset or set to 'original', use the original format */

		RND_CFT_STRING save_command;           /* command to pipe the pcb, footprint or buffer file into, when saving (makes lihata persist impossible) */
		RND_CFT_BOOLEAN keep_save_backups;     /* a copy is made before a save operation overwrites an existing file; if this setting is true, keep the copy even after a successful save */
		RND_CFT_LIST default_font_file;        /* name of default font file (list of names to search) */
		RND_CFT_LIST default_pcb_file;

		RND_CFT_BOOLEAN silently_create_on_load; /* do not generate an error message if the board does not exist on load from command line argument, silently create it in memory */

		RND_CFT_STRING script_filename;        /* PCB Actions script to execute on startup */
		RND_CFT_STRING action_string;          /* PCB Actions string to execute on startup */
		RND_CFT_STRING rat_path;
		RND_CFT_STRING rat_command;            /* file name template; if not empty, run this command and read its output for loading a rats; %f is the file name, %p is the rc.rat_path conf setting */

		RND_CFT_STRING save_final_fallback_fmt;/* when a new file is created (by running pcb-rnd with the file name) there won't be a known format; pcb-rnd will guess from the file name (extension) but eventhat may fail. This format is the final fallback that'll be used if no other guessing mechanism worked. The user can override this by save as. */
		RND_CFT_STRING save_fp_fmt;            /* when saving a buffer element/subcircuit, prefer this format by default */

		/***** automatically set (in postproc) *****/
		RND_CFT_BOOLEAN have_regex;            /* whether we have regex compiled in */
		struct {
			RND_CFT_STRING prefix;               /* e.g. /usr/local */
			RND_CFT_STRING lib;                  /* e.g. /usr/lib/pcb-rnd */
			RND_CFT_STRING bin;                  /* e.g. /usr/bin */
			RND_CFT_STRING share;                /* e.g. /usr/share/pcb-rnd */

			RND_CFT_STRING design;               /* directory path of the current design, or <invalid> if the current design doesn't have a file name yet */
		} path;
	} rc;

	const struct {                       /* design: defaults of a new layout */
		RND_CFT_COORD via_thickness;
		RND_CFT_COORD via_drilling_hole;
		RND_CFT_COORD line_thickness;
		RND_CFT_COORD clearance;

		/* old drc settings */
		RND_CFT_COORD bloat;                   /* minimum space between copper features on different networks */
		RND_CFT_COORD shrink;                  /* minimum overlap between connected copper features */
		RND_CFT_COORD min_wid;                 /* minimum copper width */
		RND_CFT_COORD min_slk;                 /* minimum silk width */
		RND_CFT_COORD min_drill;               /* minimum drill diameter */
		RND_CFT_COORD min_ring;                /* minimum annular ring */

		struct {                           /* drc: new, flexible drc settings */
			RND_CFT_COORD min_copper_clearance;  /* minimum space between copper features on different networks (default when the net attributes or other settings don't specify a different value) */
			RND_CFT_COORD min_copper_overlap;    /* minimum overlap between connected copper features (default when the net attributes or other settings don't specify a different value)*/
		} drc;

		struct {                           /* disable drc checks on a per rule basis */
			RND_CFT_BOOLEAN dummy;               /* placeholder, not used */
		} drc_disable;

		RND_CFT_INTEGER text_scale;            /* text scaling in % */
		RND_CFT_COORD text_thickness;          /* override stroke font text thickness */
		RND_CFT_INTEGER text_font_id;
		RND_CFT_REAL poly_isle_area;           /* polygon min area */
		RND_CFT_STRING fab_author;             /* Full name of author for FAB drawings */
		RND_CFT_STRING initial_layer_stack;    /* deprecated. */

		RND_CFT_COORD paste_adjust;            /* Adjust paste thickness */
	} design;

/* @path appearance/color */
	const struct {                       /* appearance */
		RND_CFT_BOOLEAN compact;               /* when set: optimize GUI widget arrangement for small screen; may be wasting some screen space on large screen */
		RND_CFT_COORD rat_thickness;
		RND_CFT_COORD mark_size;               /* relative marker size */
		RND_CFT_BOOLEAN text_host_bbox;        /* when moving a text object, the outline thin-draw should also include the bounding box */
		RND_CFT_REAL term_label_size;          /* size of terminal labels, in pcb font scale (100 is for the normal size) */
		RND_CFT_BOOLEAN subc_layer_per_side;   /* hide top or bottom placed subcircuit annotations if the view is showing the other side */
		RND_CFT_BOOLEAN invis_other_groups;    /* render non-current group layers with the inivisble color */
		RND_CFT_BOOLEAN black_current_group;   /* render all layers of the current group black, for maximum contrast */

		struct {                           /* color */
			RND_CFT_COLOR crosshair;             /* obsolete - DO NOT USE - kept for compatibility (use appearance/color/cross instead) */
			RND_CFT_COLOR attached;              /* color of the non-layer corsshair-attached objects drawn in XOR (e.g. when padstack or buffer outline before placed; layer-objects like lines and arcs typically onherit the layer color) */
			RND_CFT_COLOR drc;                   /* color of the on-screen drc clearance indication while routing */
			RND_CFT_COLOR mark;                  /* color of the on-screen marks (user placed mark and automatic 'grabbed' mark) */
			RND_CFT_COLOR selected;              /* generic object selection color */
			RND_CFT_COLOR via;                   /* non-terminal padstack shape on current layer */
			RND_CFT_COLOR via_far;               /* non-terminal padstack shape on non-current ('far side') layer */
			RND_CFT_COLOR pin;                   /* terminal padstack shape on current layer */
			RND_CFT_COLOR pin_far;               /* terminal padstack shape on non-current ('far side') layer */
			RND_CFT_COLOR pin_name;              /* on-screen terminal number/name labels */
			RND_CFT_COLOR subc;                  /* on-screen subcircuit marks */
			RND_CFT_COLOR subc_nonetlist;        /* on-screen subcircuit marks for subcircuits with the nonetlist flag */
			RND_CFT_COLOR extobj;                /* on-screen marks of extended objects */
			RND_CFT_COLOR padstackmark;          /* on-screen center mark cross for padstacks */
			RND_CFT_COLOR rat;                   /* on-screen rat lines */
			RND_CFT_COLOR invisible_objects;     /* other-side objects and padstack shapes on non-current layer */
			RND_CFT_COLOR connected;             /* 'connected' highlight (galvanic connections found) */
			RND_CFT_COLOR warn;                  /* warning highlight (e.g. object found to cause a short) */
			RND_CFT_COLOR layer[PCB_MAX_LAYER];  /* default layer colors; when a new layer is created, a color from this list is assigned initially */
			RND_CFT_COLOR mask;                  /* default mask layer color (when a new mask layer is created) */
			RND_CFT_COLOR paste;                 /* default paste layer color (when a new paste layer is created) */
			RND_CFT_COLOR element;               /* default silk layer color (when a new silk layer is created) */
		} color;
		struct {
			RND_CFT_INTEGER cross_thick;      /* cross thickness in pixels - 0 means disable crosses */
			RND_CFT_COORD cross_size;         /* cross size in word coords - size of one arm of the cross (minus the hole radius) */
		} padstack;
		struct {
			RND_CFT_INTEGER dash_freq;        /* how dense the dashed outline should be; -1 means do not display the dashed outline; 0 means solid outline; 1..32 means dashed outline */
		} subc;
		struct {
			RND_CFT_INTEGER char_per_line;   /* width of an output line in characters (used by separator drawing in find.c) */		
		} messages;
		struct {
			RND_CFT_INTEGER volume;          /* the speakers volume -100..100 */
		} misc;
	} appearance;

} conf_core_t;

extern conf_core_t conf_core;
void conf_core_init(void);
void conf_core_uninit(void);
void conf_core_uninit_pre(void);
void pcb_conf_legacy(const char *dst_path, const char *legacy_path);

#endif
