/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* definition of types */
#ifndef	PCB_GLOBAL_H
#define	PCB_GLOBAL_H

#include "config.h"

#include "const.h"
#include "macro.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include "global_typedefs.h"
#include "global_objs.h"
#include "attrib.h"
#include "rats_patch.h"
#include "list_common.h"
#include "list_line.h"
#include "list_arc.h"
#include "list_text.h"
#include "list_poly.h"
#include "list_pad.h"
#include "list_pin.h"
#include "list_rat.h"
#include "vtonpoint.h"
#include "hid.h"
#include "polyarea.h"
#include "vtroutestyle.h"


/* Internationalization support. */
#ifdef ENABLE_NLS
#	include <libintl.h>
#	define _(S) gettext(S)
#	if defined(gettext_noop)
#		define N_(S) gettext_noop(S)
#	else
#		define N_(S) (S)
#	endif
#else
#	define _(S) (S)
#	define N_(S) (S)
#	define textdomain(S) (S)
#	define gettext(S) (S)
#	define dgettext(D, S) (S)
#	define dcgettext(D, S, T) (S)
#	define bindtextdomain(D, Dir) (D)
#endif /* ENABLE_NLS */

/* This is used by the lexer/parser */
typedef struct {
	int ival;
	Coord bval;
	double dval;
	char has_units;
} PLMeasure;

/* ---------------------------------------------------------------------------
 * Macros to annotate branch-prediction information.
 * Taken from GLib 2.16.3 (LGPL 2).G_ / g_ prefixes have
 * been removed to avoid namespace clashes.
 */

/* The LIKELY and UNLIKELY macros let the programmer give hints to
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 *
 * The PCB_BOOLEAN_EXPR macro is intended to trigger a gcc warning when
 * putting assignments inside the test.
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#	define PCB_BOOLEAN_EXPR(expr)                   \
 __extension__ ({                             \
   int _boolean_var_;                         \
   if (expr)                                  \
      _boolean_var_ = 1;                      \
   else                                       \
      _boolean_var_ = 0;                      \
   _boolean_var_;                             \
})
#	define LIKELY(expr) (__builtin_expect (PCB_BOOLEAN_EXPR(expr), 1))
#	define UNLIKELY(expr) (__builtin_expect (PCB_BOOLEAN_EXPR(expr), 0))
#else
#	define LIKELY(expr) (expr)
#	define UNLIKELY(expr) (expr)
#endif

/* ---------------------------------------------------------------------------
 * Macros to annotate branch-prediction information.
 * Taken from GLib 2.42.1 (LGPL 2). PCB_ prefixes have
 * been added to avoid namespace clashes.
 */
#define PCB_CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define PCB_ABS(a)	   (((a) < 0) ? -(a) : (a))

/* ---------------------------------------------------------------------------
 * some useful values of our widgets
 */
typedef struct {								/* holds information about output window */
	hidGC bgGC,										/* background and foreground; */
	  fgGC,												/* changed from some routines */
	  pmGC;												/* depth 1 pixmap GC to store clip */
} OutputType, *OutputTypePtr;


/* ----------------------------------------------------------------------
 * layer group. A layer group identifies layers which are always switched
 * on/off together.
 */
typedef struct {
	pcb_cardinal_t Number[MAX_LAYER],		/* number of entries per groups */
	  Entries[MAX_LAYER][MAX_LAYER + 2];
} LayerGroupType, *LayerGroupTypePtr;

typedef struct {
	Coord x, y;
	Coord width, height;
} RectangleType, *RectangleTypePtr;

/* ---------------------------------------------------------------------------
 * the basic object types supported by PCB
 */

#include "global_element.h"

struct rtree {
	struct rtree_node *root;
	int size;											/* number of entries in tree */
};

struct layer_st {								/* holds information about one layer */
	const char *Name;		   				/* layer name */
	linelist_t Line;
	textlist_t Text;
	polylist_t Polygon;
	arclist_t Arc;
	rtree_t *line_tree, *text_tree, *polygon_tree, *arc_tree;
	pcb_bool On;											/* visible flag */
	const char *Color;						/* color */
	const char *SelectedColor;
	AttributeListType Attributes;
	int no_drc;										/* whether to ignore the layer when checking the design rules */
};

struct data_st {								/* holds all objects */
	int LayerN;										/* number of layers in this board */
	pinlist_t Via;
	elementlist_t Element;
	ratlist_t Rat;
	rtree_t *via_tree, *element_tree, *pin_tree, *pad_tree, *name_tree[3],	/* for element names */
	 *rat_tree;
	struct PCBType *pcb;
	LayerType Layer[MAX_LAYER + 2];	/* add 2 silkscreen layers */
	plug_io_t *loader;
};

typedef struct {								/* holds drill information */
	Coord DrillSize;							/* this drill's diameter */
	pcb_cardinal_t ElementN,						/* the number of elements using this drill size */
	  ElementMax,									/* max number of elements from malloc() */
	  PinCount,										/* number of pins drilled this size */
	  ViaCount,										/* number of vias drilled this size */
	  UnplatedCount,							/* number of these holes that are unplated */
	  PinN,												/* number of drill coordinates in the list */
	  PinMax;											/* max number of coordinates from malloc() */
	PinTypePtr *Pin;							/* coordinates to drill */
	ElementTypePtr *Element;			/* a pointer to an array of element pointers */
} DrillType, *DrillTypePtr;

typedef struct {								/* holds a range of Drill Infos */
	pcb_cardinal_t DrillN,							/* number of drill sizes */
	  DrillMax;										/* max number from malloc() */
	DrillTypePtr Drill;						/* plated holes */
} DrillInfoType, *DrillInfoTypePtr;

typedef struct LibraryEntryTpye_s  LibraryEntryType, *LibraryEntryTypePtr;
typedef struct LibraryMenuType_s   LibraryMenuType, *LibraryMenuTypePtr;

/* ---------------------------------------------------------------------------
 * structure used by library routines
 */
struct LibraryEntryTpye_s {
	const char *ListEntry;				/* the string for the selection box */
	int ListEntry_dontfree;       /* do not free(ListEntry) if non-zero */
	/* This used to contain some char *AllocatedMemory, possibly with
	 * the intention of the following fields pointing into it.
	 * It was never used that way, so removing for now.
	 * TODO: re-introduce and actually use it for the following fields?
	 */
	const char *Package;	 				/* package */
	const char *Value;						/* the value field */
	const char *Description;			/* some descriptive text */
#if 0
	fp_type_t Type;
	void **Tags;									/* an array of void * tag IDs; last tag ID is NULL */
#endif
};

/* If the internal flag is set, the only field that is valid is Name,
   and the struct is allocated with malloc instead of
   CreateLibraryEntry.  These "internal" entries are used for
   electrical paths that aren't yet assigned to a real net.  */

struct LibraryMenuType_s {
	char *Name,										/* name of the menu entry */
	 *directory,									/* Directory name library elements are from */
	 *Style;											/* routing style */
	pcb_cardinal_t EntryN,							/* number of objects */
	  EntryMax;										/* number of reserved memory locations */
	LibraryEntryTypePtr Entry;		/* the entries */
	char flag;										/* used by the netlist window to enable/disable nets */
	char internal;								/* if set, this is an internal-only entry, not
																   part of the global netlist. */
};

typedef struct {
	pcb_cardinal_t MenuN;								/* number of objects */
	pcb_cardinal_t MenuMax;							/* number of reserved memory locations */
	LibraryMenuTypePtr Menu;			/* the entries */
} LibraryType, *LibraryTypePtr;


	/* The PCBType struct holds information about board layout most of which is
	   |  saved with the layout.  A new PCB layout struct is first initialized
	   |  with values from the user configurable Settings struct and then reset
	   |  to the saved layout values when a layout is loaded.
	   |  This struct is also used for the remove list and for buffer handling
	 */
struct pcb_st_t {
	long ID;											/* see macro.h */
	char *Name,										/* name of board */
	 *Filename,										/* name of file (from load) */
	 *PrintFilename,							/* from print dialog */
	 *Netlistname,								/* name of netlist file */
	  ThermStyle;									/* type of thermal to place with thermal tool */
	pcb_bool Changed,									/* layout has been changed */
	  ViaOn,											/* visibility flags */
	  ElementOn, RatOn, InvisibleObjectsOn, PinOn, SilkActive,	/* active layer is actually silk */
	  RatDraw;										/* we're drawing rats */
	char *ViaColor,								/* some colors */
	 *ViaSelectedColor,
		*PinColor,
		*PinSelectedColor,
		*PinNameColor,
		*ElementColor,
		*ElementColor_nonetlist,
		*RatColor,
		*InvisibleObjectsColor,
		*InvisibleMarkColor, *ElementSelectedColor, *RatSelectedColor, *ConnectedColor, *WarnColor, *MaskColor;
	Coord CursorX,									/* cursor position as saved with layout */
	  CursorY;
	Coord Bloat,									/* drc sizes saved with layout */
	  Shrink, minWid, minSlk, minDrill, minRing;
	Coord GridOffsetX,						/* as saved with layout */
	  GridOffsetY, MaxWidth,			/* allowed size */
	  MaxHeight;

	Coord Grid;										/* used grid with offsets */
	double Zoom,									/* zoom factor */
	  IsleArea,										/* minimum poly island to retain */
	  ThermScale;									/* scale factor used with thermals */
	FontType Font;
	LayerGroupType LayerGroups;
	vtroutestyle_t RouteStyle;
	LibraryType NetlistLib[NUM_NETLISTS];
	rats_patch_line_t *NetlistPatches, *NetlistPatchLast;
	AttributeListType Attributes;
	DataTypePtr Data;							/* entire database */

	pcb_bool is_footprint;						/* If set, the user has loaded a footprint, not a pcb. */

/* netlist states */
	int netlist_frozen;                /* counter */
	unsigned netlist_needs_update:1;

};

typedef struct {								/* information about the paste buffer */
	Coord X, Y;										/* offset */
	BoxType BoundingBox;
	DataTypePtr Data;							/* data; not all members of PCBType */
	/* are used */
} BufferType, *BufferTypePtr;

/* ---------------------------------------------------------------------------
 * some types for cursor drawing, setting of block and lines
 * as well as for merging of elements
 */
typedef struct {								/* rubberband lines for element moves */
	LayerTypePtr Layer;						/* layer that holds the line */
	LineTypePtr Line;							/* the line itself */
	PointTypePtr MovedPoint;			/* and finally the point */
} RubberbandType, *RubberbandTypePtr;

typedef struct {								/* current marked line */
	PointType Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool draw;
} AttachedLineType, *AttachedLineTypePtr;

typedef struct {								/* currently marked block */
	PointType Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool otherway;
} AttachedBoxType, *AttachedBoxTypePtr;

typedef struct {								/* currently attached object */
	Coord X, Y;										/* saved position when PCB_MODE_MOVE */
	BoxType BoundingBox;
	long int Type,								/* object type */
	  State;
	void *Ptr1,										/* three pointers to data, see */
	 *Ptr2,												/* search.c */
	 *Ptr3;
	pcb_cardinal_t RubberbandN,					/* number of lines in array */
	  RubberbandMax;
	RubberbandTypePtr Rubberband;
} AttachedObjectType, *AttachedObjectTypePtr;

enum crosshair_shape {
	Basic_Crosshair_Shape = 0,		/*  4-ray */
	Union_Jack_Crosshair_Shape,		/*  8-ray */
	Dozen_Crosshair_Shape,				/* 12-ray */
	Crosshair_Shapes_Number
};

typedef struct {								/* holds cursor information */
	hidGC GC,											/* GC for cursor drawing */
	  AttachGC;										/* and for displaying buffer contents */
	Coord X, Y,										/* position in PCB coordinates */
	  MinX, MinY,									/* lowest and highest coordinates */
	  MaxX, MaxY;
	AttachedLineType AttachedLine;	/* data of new lines... */
	AttachedBoxType AttachedBox;
	PolygonType AttachedPolygon;
	AttachedObjectType AttachedObject;	/* data of attached objects */
	enum crosshair_shape shape;		/* shape of crosshair */
	vtop_t onpoint_objs;
	vtop_t old_onpoint_objs;

	/* list of object IDs that could have been dragged so that they can be cycled */
	long int *drags;
	int drags_len, drags_current;
	Coord dragx, dragy;						/* the point where drag started */
} CrosshairType, *CrosshairTypePtr;

typedef struct {
	pcb_bool status;
	Coord X, Y;
} MarkType, *MarkTypePtr;

/* ----------------------------------------------------------------------
 * pointer to low-level copy, move and rotate functions
 */
typedef struct {
	void *(*Line) (LayerTypePtr, LineTypePtr);
	void *(*Text) (LayerTypePtr, TextTypePtr);
	void *(*Polygon) (LayerTypePtr, PolygonTypePtr);
	void *(*Via) (PinTypePtr);
	void *(*Element) (ElementTypePtr);
	void *(*ElementName) (ElementTypePtr);
	void *(*Pin) (ElementTypePtr, PinTypePtr);
	void *(*Pad) (ElementTypePtr, PadTypePtr);
	void *(*LinePoint) (LayerTypePtr, LineTypePtr, PointTypePtr);
	void *(*Point) (LayerTypePtr, PolygonTypePtr, PointTypePtr);
	void *(*Arc) (LayerTypePtr, ArcTypePtr);
	void *(*Rat) (RatTypePtr);
} ObjectFunctionType, *ObjectFunctionTypePtr;

/* ---------------------------------------------------------------------------
 * structure used by device drivers
 */

typedef struct {								/* holds a connection */
	Coord X, Y;										/* coordinate of connection */
	long int type;								/* type of object in ptr1 - 3 */
	void *ptr1, *ptr2;						/* the object of the connection */
	pcb_cardinal_t group;								/* the layer group of the connection */
	LibraryMenuType *menu;				/* the netmenu this *SHOULD* belong too */
} ConnectionType, *ConnectionTypePtr;

struct net_st {								/* holds a net of connections */
	pcb_cardinal_t ConnectionN,					/* the number of connections contained */
	  ConnectionMax;							/* max connections from malloc */
	ConnectionTypePtr Connection;
	RouteStyleTypePtr Style;
};

typedef struct {								/* holds a list of nets */
	pcb_cardinal_t NetN,								/* the number of subnets contained */
	  NetMax;											/* max subnets from malloc */
	NetTypePtr Net;
} NetListType, *NetListTypePtr;

typedef struct {								/* holds a list of net lists */
	pcb_cardinal_t NetListN,						/* the number of net lists contained */
	  NetListMax;									/* max net lists from malloc */
	NetListTypePtr NetList;
} NetListListType, *NetListListTypePtr;

typedef struct {								/* holds a generic list of pointers */
	pcb_cardinal_t PtrN,								/* the number of pointers contained */
	  PtrMax;											/* max subnets from malloc */
	void **Ptr;
} PointerListType, *PointerListTypePtr;

typedef struct {
	pcb_cardinal_t BoxN,								/* the number of boxes contained */
	  BoxMax;											/* max boxes from malloc */
	BoxTypePtr Box;

} BoxListType, *BoxListTypePtr;

/* ---------------------------------------------------------------------------
 * add a macro for wrapping RCS ID's in so that ident will still work
 * but we won't get as many compiler warnings
 */

#ifndef GCC_VERSION
#	define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#endif /* GCC_VERSION */

#if GCC_VERSION > 2007
#	define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#	define ATTRIBUTE_UNUSED
#endif

/* ---------------------------------------------------------------------------
 * Macros called by various action routines to show usage or to report
 * a syntax error and fail
 */
#define AUSAGE(x) Message (PCB_MSG_INFO, "Usage:\n%s\n", (x##_syntax))
#define AFAIL(x) { Message (PCB_MSG_ERROR, "Syntax error.  Usage:\n%s\n", (x##_syntax)); return 1; }

/* Make sure to catch usage of non-portable functions in debug mode */
#ifndef NDEBUG
#	undef strdup
#	undef strndup
#	undef snprintf
#	undef round
#	define strdup      never_use_strdup__use_pcb_strdup
#	define strndup     never_use_strndup__use_pcb_strndup
#	define snprintf    never_use_snprintf__use_pcb_snprintf
#	define round       never_use_round__use_pcb_round
#endif

#endif /* PCB_GLOBAL_H  */
