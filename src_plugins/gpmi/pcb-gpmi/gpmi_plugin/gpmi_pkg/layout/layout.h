#include <gpmi.h>
#include "src/global.h"
#include "src/rtree.h"
#include "src/data.h"
#include "src/pcb-printf.h"

#undef snprintf
#define snprintf pcb_snprintf


/* Object type search mask bits */
typedef enum layout_object_mask_e {
	OM_LINE     = 1,       /* lines (traces, silk lines, not font) */
	OM_TEXT     = 2,       /* text written using the font */
	OM_POLYGON  = 4,       /* polygons, including rectangles */
	OM_ARC      = 8,       /* arcs, circles */
	OM_VIA      = 16,      /* vias and holes which are not part of a footprint */
	OM_PIN      = 32,      /* pins/pads of a footprint */

	OM_ANY      = 0xffff   /* shorthand for "find anything" */
} layout_object_mask_t;
gpmi_keyword *kw_layout_object_mask_e; /* of layout_object_mask_t */

/* Which coordinate of the object is referenced */
typedef enum layout_object_coord_e {
/* X/Y coords */
	OC_BX1 = 1, /* bounding box X1 */
	OC_BX2 = 2, /* bounding box X2 */
	OC_BY1 = 3, /* bounding box Y1 */
	OC_BY2 = 4, /* bounding box Y2 */
	OC_P1X = 5, /* point 1 X */
	OC_P2X = 6, /* point 2 X */
	OC_P1Y = 7, /* point 1 Y */
	OC_P2Y = 8, /* point 2 Y */
	OC_OBJ = 9, /* the whole object */

/* point aliases for X/Y coords */
	OC_P1  = 5, /* point 1 is P1X*/
	OC_P2  = 6  /* point 2 is P2X */

} layout_object_coord_t;
gpmi_keyword *kw_layout_object_coord_e; /* of layout_object_coord_t */


typedef enum layout_flag_e {
	FL_NONE          = 0,
	FL_SHOWNUMBER    = 0x00000001,
	FL_LOCALREF      = 0x00000002,
	FL_CHECKPLANS    = 0x00000004,
	FL_SHOWDRC       = 0x00000008,
	FL_RUBBERBAND    = 0x00000010,
	FL_DESCRIPTION   = 0x00000020,
	FL_NAMEONPCB     = 0x00000040,
	FL_AUTODRC       = 0x00000080,
	FL_ALLDIRECTION  = 0x00000100,
	FL_SWAPSTARTDIR  = 0x00000200,
	FL_UNIQUENAME    = 0x00000400,
	FL_CLEARNEW      = 0x00000800,
	FL_SNAPPIN       = 0x00001000,
	FL_SHOWMASK      = 0x00002000,
	FL_THINDRAW      = 0x00004000,
	FL_ORTHOMOVE     = 0x00008000,
	FL_LIVEROUTE     = 0x00010000,
	FL_THINDRAWPOLY  = 0x00020000,
	FL_LOCKNAMES     = 0x00040000,
	FL_ONLYNAMES     = 0x00080000,
	FL_NEWFULLPOLY   = 0x00100000,
	FL_HIDENAMES     = 0x00200000,

	FL_THERMALSTYLE1 = 0x08000000,
	FL_THERMALSTYLE2 = 0x10000000,
	FL_THERMALSTYLE3 = 0x20000000,
	FL_THERMALSTYLE4 = 0x40000000,
	FL_THERMALSTYLE5 = 0x80000000
} layout_flag_t;
gpmi_keyword *kw_layout_flag_e; /* of layout_flag_t */



typedef struct layout_object_s {
	layout_object_mask_t type;
	union {
		LineType    *l;
		TextType    *t;
		PolygonType *p;
		ArcType     *a;
		PinType     *v;
		PinType     *pin;
	} obj;
	int layer;
} layout_object_t;


typedef struct layout_search_s {
	layout_object_mask_t searching; /* during the search, this field is used to communicate with the callback */
	int used, alloced;
	layout_object_t *objects;
	int layer;
} layout_search_t;

/* -- search -- (search.c) */
/* creates a new search and adds all objects that matches obj_types mask within the given rectangle on the current layer
   Arguments:
     search_ID: unique name of the search (overwrites existing search on the same name)
     obj_types: on or more object types
     x1, y1, x2, y2: box the search is done within (PCB coords)
   Returns the number of object on the search list. */
int layout_search_box(const char *search_ID, layout_object_mask_t obj_types, int x1, int y1, int x2, int y2);

/* creates a new search and adds all selected objects
   Arguments:
     search_ID: unique name of the search (overwrites existing search on the same name)
     obj_types: on or more object types
   Returns the number of object on the search list. */
int layout_search_selected(const char *search_ID, multiple layout_object_mask_t obj_types);

/* creates a new search and adds all found objects (the green highlight)
   Arguments:
     search_ID: unique name of the search (overwrites existing search on the same name)
     obj_types: on or more object types
   Returns the number of object on the search list. */
int layout_search_found(const char *search_ID, multiple layout_object_mask_t obj_types);

/* Returns the nth object from a search list (or NULL pointer if n is beyond the list) */
layout_object_t *layout_search_get(const char *search_ID, int n);

/* Frees all memory related to a search. Returns 0 on success.
   Argument:
     search_ID: unique name of the search (requires an existing search) */
int layout_search_free(const char *search_ID);

/* -- object accessors -- (object.c) */
/* Return the requested coord of an object; except for the bounding box
    coordinates, the meaning of coordinates are object-specific.
    Point 1 and point 2 are usually endpoints of the object (line, arc),
    "the whole object" coordinate is a central point. */
int layout_obj_coord(layout_object_t *obj, layout_object_coord_t coord);

/* Return the type of an object (always a single bit) */
layout_object_mask_t layout_obj_type(layout_object_t *obj);

/* Change location of an object or parts of the object (like move endpoint of a line);
   Arguments:
     obj: the object
     coord: which coordinate to drag (e.g. move only the endpoint)
     dx, dy: relative x and y coordinates the selected coordinate is displaced by
   Returns 0 on success */
int layout_obj_move(layout_object_t *obj, layout_object_coord_t coord, int dx, int dy);

/* change angles of an arc; start and delate are relative if relative is non-zero; returns 0 on success */
int layout_arc_angles(layout_object_t *obj, int relative, int start, int delta);

/* -- create new objects -- (create.c) */
/* create a line */
int layout_create_line(int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags);

/* same as layout_create_line(), but appends the result to a list and
   returns the index of the new object on the list (can be used as n for
   layour_search_get)
int layout_lcreate_line(const char *search_ID, int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags);*/

/* create a named via */
int layout_create_via(int x, int y, int thickness, int clearance, int mask, int hole, const char *name, multiple layout_flag_t flags);

/* create a new arc; sa is start angle, dir is delta angle */
int layout_create_arc(int x, int y, int width, int height, int sa, int dir, int thickness, int clearance, multiple layout_flag_t flags);

/* -- layer manipulation -- (layers.c) */
/* Field name of the layer structure */
typedef enum layer_field_e {
	LFLD_NUM_LINES,   /* number of lines on the layer */
	LFLD_NUM_TEXTS,   /* number of texts on the layer */
	LFLD_NUM_POLYS,   /* number of polygons on the layer */
	LFLD_NUM_ARCS,    /* number of arcs on the layer */
	LFLD_VISIBLE,     /* non-zero if the layer is visible */
	LFLD_NODRC        /* non-zero if the layer doesn't use DRC */
} layer_field_t;

/* switch to layer (further layer-specific actions will take place there) */
void layout_switch_to_layer(int layer);

/* returns the number of the current layer */
int layout_get_current_layer();

/* resolve layer number by name (case sensitive); returns negative number if not found */
int layout_resolve_layer(const char *name);

/* return the theoretical number of layers supported by PCB */
int layout_get_max_possible_layer();

/* return the actual number of copper layers on the current design */
int layout_get_max_copper_layer();

/* return the actual number of layers on the current design */
int layout_get_max_layer();

/* return the name of a layer */
const char *layout_layer_name(int layer);

/* return the color of a layer */
const char *layout_layer_color(int layer);

/* return an integer field of a layer */
int layout_layer_field(int layer, layer_field_t fld);


/* -- page manipulation -- (page.c) */
/* query or set width and height of the drawing */
int layout_get_page_width();
int layout_get_page_height();
void layout_set_page_size(int width, int height);

/* -- coordinate system -- (coord.c) */
double mil2pcb_multiplier();
double mm2pcb_multiplier();
const char *current_grid_unit();

typedef struct dctx_s {
	void *hid;
	void *gc;
} dctx_t;

/* -- debug draw GC -- */
/* Initialize debug drawing; returns 1 if worked, 0 if denied */
int debug_draw_request(void);

/* Flush the drawing */
void debug_draw_flush(void);

/* Finish (close) drawing */
void debug_draw_finish(dctx_t *ctx);

/* Get the draw context of debug draw */
dctx_t *debug_draw_dctx(void);

/* -- draw on a GC -- */

/* Debug draw style: set drawing color */
void draw_set_color(dctx_t *ctx, const char *name);
/*void set_line_cap(dctx_t *ctx, EndCapStyle style_);*/

/* Debug draw style: set line width */
void draw_set_line_width(dctx_t *ctx, int width);

/* Debug draw style: set whether drawing should happen in xor */
void draw_set_draw_xor(dctx_t *ctx, int xor);

/* Debug draw style: set whether drawing should happen in faded mode  */
void draw_set_draw_faded(dctx_t *ctx, int faded);

/* Debug draw: draw a line using the current style settings */
void draw_line(dctx_t *ctx, int x1_, int y1_, int x2_, int y2_);

/*
void draw_arc(dctx_t *ctx, int cx_, int cy_, int xradius_, int yradius_, double start_angle_, double delta_angle_);
void draw_rect(dctx_t *ctx, int x1_, int y1_, int x2_, int y2_);
void fill_circle(dctx_t *ctx, int cx_, int cy_, int radius_);
void fill_polygon(dctx_t *ctx, int n_ints_, int *x_, int *y_);
void fill_pcb_polygon(dctx_t *ctx, PolygonType *poly, const BoxType *clip_box);
void thindraw_pcb_polygon(dctx_t *ctx, PolygonType *poly, const BoxType *clip_box);
void fill_pcb_pad(dctx_t *ctx, PadType *pad, pcb_bool clip, pcb_bool mask);
void thindraw_pcb_pad(dctx_t *ctx, PadType *pad, pcb_bool clip, pcb_bool mask);
void fill_pcb_pv(hidGC fg_gc, hidGC bg_gc, PinType *pv, pcb_bool drawHole, pcb_bool mask);
void thindraw_pcb_pv(hidGC fg_gc, hidGC bg_gc, PinType *pv, pcb_bool drawHole, pcb_bool mask);
void fill_rect(dctx_t *ctx, int x1_, int y1_, int x2_, int y2_);
*/
