#include <gpmi.h>
#include "src/global.h"
#include "src/rtree.h"
#include "src/create.h"
#include "src/data.h"

typedef enum layout_object_mask_e {
	OM_LINE     = 1,
	OM_TEXT     = 2,
	OM_POLYGON  = 4,
	OM_ARC      = 8,
	OM_VIA      = 16,
	OM_PIN      = 32
} layout_object_mask_t;
gpmi_keyword *kw_layout_object_mask_e; /* of layout_object_mask_t */

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
} layout_object_t;


typedef struct layout_search_s {
	layout_object_mask_t searching; /* during the search, this field is used to communicate with the callback */
	int used, alloced;
	layout_object_t *objects;
} layout_search_t;

/* -- search -- (search.c) */
/* creates a new search and adds all objects that matches obj_types mask within the given rectangle on the current layer */
int layout_search_box(const char *search_ID, multiple layout_object_mask_t obj_types, int x1, int y1, int x2, int y2);

/* creates a new search and adds all selected objects */
int layout_search_selected(const char *search_ID, multiple layout_object_mask_t obj_types);

/* creates a new search and adds all found objects */
int layout_search_found(const char *search_ID, multiple layout_object_mask_t obj_types);

/* returns the nth object of a search */
layout_object_t *layout_search_get(const char *search_ID, int n);

/* returns 0 on success */
int layout_search_free(const char *search_ID);

/* -- object accessors -- (object.c) */
/* return the requested coord of an object */
int layout_obj_coord(layout_object_t *obj, layout_object_coord_t coord);

/* return the type of an object (always a single bit) */
layout_object_mask_t layout_obj_type(layout_object_t *obj);

/* change location of an object or parts of the object (like move endpoint of a line); returns 0 on success */
int layout_obj_move(layout_object_t *obj, layout_object_coord_t coord, int dx, int dy);

/* change angles of an arc; start and delate are relative if relative is non-zero; returns 0 on success */
int layout_arc_angles(layout_object_t *obj, int relative, int start, int delta);

/* -- create new objects -- (create.c) */
/* create a line */
int layout_create_line(int x1, int y1, int x2, int y2, int thickness, int clearance, multiple layout_flag_t flags);

/* create a named via */
int layout_create_via(int x, int y, int thickness, int clearance, int mask, int hole, const char *name, multiple layout_flag_t flags);

/* create a new arc */
int layout_create_arc(int x, int y, int width, int height, int sa, int dir, int thickness, int clearance, multiple layout_flag_t flags);

/* -- layer manipulation -- (layers.c) */
/* switch to layer (further actions will take place there) */
void layout_switch_to_layer(int layer);

/* returns the number of the current layer */
int layout_get_current_layer();

/* resolve layer number by name (case sensitive); returns negative number if not found */
int layout_resolve_layer(const char *name);

/* -- page manipulation -- (page.c) */
/* query or set width and height of the drawing */
int layout_get_page_width();
int layout_get_page_height();
void layout_set_page_size(int width, int height);

/* -- coordinate system -- (coord.c) */
double mil2pcb_multiplier();
double mm2pcb_multiplier();


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
void draw_set_color(dctx_t *ctx, const char *name);
/*void set_line_cap(dctx_t *ctx, EndCapStyle style_);*/
void draw_set_line_width(dctx_t *ctx, int width);
void draw_set_draw_xor(dctx_t *ctx, int xor);
void draw_set_draw_faded(dctx_t *ctx, int faded);
void draw_line(dctx_t *ctx, int x1_, int y1_, int x2_, int y2_);

/*
void draw_arc(dctx_t *ctx, int cx_, int cy_, int xradius_, int yradius_, double start_angle_, double delta_angle_);
void draw_rect(dctx_t *ctx, int x1_, int y1_, int x2_, int y2_);
void fill_circle(dctx_t *ctx, int cx_, int cy_, int radius_);
void fill_polygon(dctx_t *ctx, int n_ints_, int *x_, int *y_);
void fill_pcb_polygon(dctx_t *ctx, PolygonType *poly, const BoxType *clip_box);
void thindraw_pcb_polygon(dctx_t *ctx, PolygonType *poly, const BoxType *clip_box);
void fill_pcb_pad(dctx_t *ctx, PadType *pad, bool clip, bool mask);
void thindraw_pcb_pad(dctx_t *ctx, PadType *pad, bool clip, bool mask);
void fill_pcb_pv(hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
void thindraw_pcb_pv(hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
void fill_rect(dctx_t *ctx, int x1_, int y1_, int x2_, int y2_);
*/
