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
		const CFT_UNIT  grid_unit;
		CFT_COORD grid; /* grid in pcb-units */
#if 0
TODO
	const Increments increments;
#endif
		CFT_REAL zoom; /* default zoom */
		CFT_INTEGER mode;  /* currently active mode */
		CFT_INTEGER buffer_number; /* number of the current buffer */
		CFT_BOOLEAN ClearLine;
		CFT_BOOLEAN FullPoly;
		CFT_BOOLEAN UniqueNames;           /* force unique names */
		CFT_BOOLEAN SnapPin;               /* snap to pins and pads */
		CFT_BOOLEAN SnapOffGridLine;       /* Snap to certain off-grid points along a line. */
		CFT_BOOLEAN HighlightOnPoint;      /* Highlight if crosshair is on endpoints. */
		CFT_BOOLEAN ShowSolderSide;        /* mirror output */
		CFT_BOOLEAN SaveLastCommand;       /* save the last command entered by user */
		CFT_BOOLEAN SaveInTMP;             /* always save data in /tmp */
		CFT_BOOLEAN DrawGrid;              /* draw grid points */
		CFT_BOOLEAN RatWarn;               /* rats nest has set warnings */
		CFT_BOOLEAN StipplePolygons;       /* draw polygons with stipple */
		CFT_BOOLEAN AllDirectionLines;     /* enable lines to all directions */
		CFT_BOOLEAN RubberBandMode;        /* move, rotate use rubberband connections */
		CFT_BOOLEAN SwapStartDirection;    /* change starting direction after each click */
		CFT_BOOLEAN ShowDRC;               /* show drc region on crosshair */
		CFT_BOOLEAN AutoDRC;               /* */
		CFT_BOOLEAN ShowNumber;            /* pinout shows number */
		CFT_BOOLEAN OrthogonalMoves;       /* */
		CFT_BOOLEAN ResetAfterElement;     /* reset connections after each element */
		CFT_BOOLEAN liveRouting;           /* autorouter shows tracks in progress */
		CFT_BOOLEAN RingBellWhenFinished;  /* flag if a signal should be produced when searching of  connections is done */
		CFT_BOOLEAN AutoPlace;             /* flag which says we should force placement of the windows on startup */
		CFT_BOOLEAN EnableMincut;          /* Enable calculating mincut on shorts (rats_mincut.c) when non-zero */
		CFT_BOOLEAN EnableStroke;          /* Enable libstroke gesutres on middle mouse button when non-zero */
	} editor;

	struct rc {
		CFT_INTEGER verbose;
		CFT_INTEGER backup_interval;       /* time between two backups in seconds */
		CFT_STRING FontCommand;            /* commands for file loading... */
		CFT_STRING FileCommand;
		CFT_STRING FilePath;
		CFT_STRING PrintFile;
		CFT_STRING LibraryShell;
		CFT_STRING LibrarySearchPaths;
		CFT_STRING SaveCommand;
		CFT_STRING FontFile;               /* name of default font file */
		CFT_STRING DefaultPcbFile;
		CFT_STRING Groups;                 /* string with layergroups */
		CFT_STRING Routes;                 /* string with route styles */
		CFT_STRING ScriptFilename;         /* PCB Actions script to execute on startup */
		CFT_STRING ActionString;           /* PCB Actions string to execute on startup */
		CFT_STRING RatPath;
		CFT_STRING RatCommand;
		CFT_STRING FontPath;
	} rc;

	struct design { /* defaults of a new layout */
		CFT_COORD ViaThickness;
		CFT_COORD ViaDrillingHole;
		CFT_COORD LineThickness;
		CFT_COORD RatThickness;
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
		CFT_STRING DefaultLayerName[MAX_LAYER];
		CFT_STRING BackgroundImage;             /* PPM file for board background */
		CFT_STRING FabAuthor;                   /* Full name of author for FAB drawings */
		CFT_STRING InitialLayerStack;           /* If set, the initial layer stack is set to this */
	} design;

/* @path appearance/color */
	struct apperance {
		struct color {
			CFT_COLOR black;
			CFT_COLOR white;
			CFT_COLOR background;	/* background and cursor color ... */
			CFT_COLOR crosshair;							/* different object colors */
			CFT_COLOR Cross;
			CFT_COLOR Via;
			CFT_COLOR ViaSelected;
			CFT_COLOR Pin;
			CFT_COLOR PinSelected;
			CFT_COLOR PinName;
			CFT_COLOR Element;
			CFT_COLOR Element_nonetlist;
			CFT_COLOR Rat;
			CFT_COLOR InvisibleObjects;
			CFT_COLOR InvisibleMark;
			CFT_COLOR ElementSelected;
			CFT_COLOR RatSelected;
			CFT_COLOR Connected;
			CFT_COLOR OffLimit;
			CFT_COLOR Grid;
			CFT_COLOR Layer[MAX_LAYER];
			CFT_COLOR LayerSelected[MAX_LAYER];
			CFT_COLOR Warn;
			CFT_COLOR Mask;
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
			CFT_INTEGER Volume;          /* the speakers volume -100..100 */
		} misc;
	} apperance;

#if 0
cache:
	RouteStyleType RouteStyle[NUM_STYLES];	/* default routing styles */
	LayerGroupType LayerGroups;		/* default layer groups */

to the import plugin:
	*GnetlistProgram,						/* gnetlist program name */
	*MakeProgram,								/* make program name */
#endif

} conf_core_t;

extern conf_core_t conf_core;

#endif
