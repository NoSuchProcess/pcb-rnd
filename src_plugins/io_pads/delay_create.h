#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include <librnd/core/vtc0.h>
#include "board.h"

#include "data.h"
#include "layer.h"
#include "obj_common.h"
#include "obj_subc.h"

/* When creating padstacks save integer layer ID in padstack shp->dlcr_psh_layer_id */
#define dlcr_psh_layer_id layer_mask

#define PCB_DLCR_INVALID_LAYER_ID -32768

typedef struct {
	char *name;
	long id;
	pcb_layer_type_t lyt;
	pcb_layer_combining_t comb;
	const char *purpose;
	int can_have_components;


	pcb_layer_t *ly; /* the final assignment */

	void *user_data;
	union {
		char start;
		void *bulkp[32];
		double bulkd[32];
	} local_user_data;
} pcb_dlcr_layer_t;



typedef enum {
	DLCR_OBJ,
	DLCR_CALL,            /* call user function on previous or next object */
	DLCR_ATTR,            /* set attribute on previous object */
	DLCR_SUBC_BEGIN,
	DLCR_SUBC_END,
	DLCR_SUBC_FROM_LIB    /* place a subc from local lib by a list of names */
/*	DLCR_SEQPOLY_BEGIN,
	DLCR_SEQPOLY_END*/
} pcb_dlcr_type_t;


#define PCB_OBJ_DLCR_POLY           0x0000801
#define PCB_OBJ_DLCR_TEXT_BY_BBOX   0x0000802

typedef struct {
	PCB_ANY_PRIMITIVE_FIELDS;
	rnd_font_t *font;
	rnd_coord_t x, y, bbw, bbh, anchx, anchy;
	double scxy, rot;
	pcb_text_mirror_t mirror;
	rnd_coord_t thickness;
	char *str;
	long flags;
} pcb_dlcr_text_by_bbox_t;

typedef struct {
	pcb_dlcr_type_t type;
	union {
		struct {
			union {
				pcb_any_obj_t any;
				pcb_line_t line;
				pcb_arc_t arc;
				pcb_text_t text;
				pcb_pstk_t pstk;
				struct {
					PCB_ANY_PRIMITIVE_FIELDS;
					vtc0_t xy;
				} poly;
				pcb_dlcr_text_by_bbox_t text_by_bbox;
			} obj;
			long layer_id;
			pcb_layer_type_t lyt;
			char *layer_name;
		} obj;
		struct {
			pcb_subc_t *subc;
		} subc_begin; /* in DLCR_SUBC_BEGIN */
		struct {
			rnd_coord_t x;
			rnd_coord_t y;
			double rot;
			int on_bottom;
			char *names; /* \0 separated list with an empty string at the end (double \0) */
		} subc_from_lib;
		struct {
			void (*cb)(void *rctx, pcb_any_obj_t *obj, void *callctx);
			void *rctx, *callctx;
			int on_next; /* or on prev if 0 */
		} call;
		struct {
			char *key, *val;
		} attr;
	} val;
	char *name;    /* for pstk: terminal name */
	char *netname;
	long loc_line; /* for debug */
	gdl_elem_t link;
	unsigned in_last_subc:1; /* create object within the last created subc instead of on the board */
	unsigned subc_relative:1;/* coords are relative to the parent subc */
} pcb_dlcr_draw_t;

typedef struct pcb_dlcr_s pcb_dlcr_t;

struct pcb_dlcr_s {
	/* caller provided config/callbacks */
	int (*proto_layer_lookup)(pcb_dlcr_t *dlcr, pcb_pstk_shape_t *shp); /* optional: set shp's layer on special cases and return 0; return 1 for executing the standard layer lookup; required only if there are layer ID special cases, e.g. for -1; layer id is in shp->dlcr_psh_layer_id */

	/* layers */
	htsp_t name2layer;
	vtp0_t id2layer;     /* key=->id, val=(pcb_dlcr_layer_t *) */

	/* local footprint lib, if any */
	htsp_t name2subc;    /* val=(pcb_subc_t *) */

	/* board global */
	gdl_list_t drawing;  /* of pcb_dlcr_draw_t; */
	rnd_box_t board_bbox;
	pcb_data_t pstks;    /* wrapper for padstack prototypes, otherwise empty */

	/* current context/state */
	pcb_dlcr_draw_t *subc_begin;    /* NULL when drawing on the board, pointing to the SUBC_BEGIN node in between DLCR_SUBC_BEGIN and DLCR_SUBC_END */
	pcb_any_obj_t *prev_obj;        /* for CALL prev */
	pcb_dlcr_draw_t *call_next;     /* for CALL next */
	pcb_subc_t *last_subc_placed;   /* for adding objects in the last subc instead of board */

	/* aux output */
	vtp0_t netname_objs;            /* (pcb_any_obj_t *):(char *netname) pairs; filled in only if ->save_netname_objs is 1 */

	/* config */
	unsigned flip_y:1;              /* if 1, mirror y coordinates over the X axis */
	unsigned save_netname_objs:1;   /* if 1, save each object:netname pair in ->netname_objs */
};

void pcb_dlcr_init(pcb_dlcr_t *dlcr);
void pcb_dlcr_uninit(pcb_dlcr_t *dlcr);

void pcb_dlcr_layer_reg(pcb_dlcr_t *dlcr, pcb_dlcr_layer_t *layer);
void pcb_dlcr_layer_free(pcb_dlcr_layer_t *layer);

/* allocate a new named subcircuit within the local dlcr lib */
pcb_subc_t *pcb_dlcr_subc_new_in_lib(pcb_dlcr_t *dlcr, const char *name);

pcb_dlcr_draw_t *pcb_dlcr_line_new(pcb_dlcr_t *dlcr, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t width, rnd_coord_t clearance);
pcb_dlcr_draw_t *pcb_dlcr_arc_new(pcb_dlcr_t *dlcr, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, double start_deg, double delta_deg, rnd_coord_t width, rnd_coord_t clearance);
pcb_dlcr_draw_t *pcb_dlcr_text_new(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, double rot, int scale, rnd_coord_t thickness, const char *str, long flags);
pcb_dlcr_draw_t *pcb_dlcr_text_by_bbox_new(pcb_dlcr_t *dlcr, rnd_font_t *font, rnd_coord_t x, rnd_coord_t y, rnd_coord_t bbw, rnd_coord_t bbh, rnd_coord_t anchx, rnd_coord_t anchy, double scxy, pcb_text_mirror_t mirror, double rot, rnd_coord_t thickness, const char *str, long flags);
pcb_dlcr_draw_t *pcb_dlcr_via_new(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, rnd_coord_t clearance, long proto_id, const char *proto_name, const char *term);
pcb_dlcr_draw_t *pcb_dlcr_poly_new(pcb_dlcr_t *dlcr, int hole, long prealloc_len);
pcb_dlcr_draw_t *pcb_dlcr_poly_lineto(pcb_dlcr_t *dlcr, pcb_dlcr_draw_t *poly, rnd_coord_t x, rnd_coord_t y);

/* attach a netname to an existing to-be-created object */
void pcb_dlcr_set_net(pcb_dlcr_draw_t *obj, const char *netname);

/* Call back cb() on the previous (last created) or next object after the
   object is created */
pcb_dlcr_draw_t *pcb_dlcr_call_on(pcb_dlcr_t *dlcr, void (*cb)(void *rctx, pcb_any_obj_t *obj, void *callctx), void *rctx, void *callctx, int on_next);
pcb_dlcr_draw_t *pcb_dlcr_call_prev(pcb_dlcr_t *dlcr, void (*cb)(void *rctx, pcb_any_obj_t *obj, void *callctx), void *rctx, void *callctx);
pcb_dlcr_draw_t *pcb_dlcr_call_next(pcb_dlcr_t *dlcr, void (*cb)(void *rctx, pcb_any_obj_t *obj, void *callctx), void *rctx, void *callctx);

/* Set an attribute on the previous (last created) object */
pcb_dlcr_draw_t *pcb_dlcr_attrib_set_prev(pcb_dlcr_t *dlcr, const char *key, const char *val);


/* delayed place a subcricuit from the local dlcr lib by name; names is either
   a single string or a \0 separated list of strings with an empty string at
   the end */
pcb_dlcr_draw_t *pcb_dlcr_subc_new_from_lib(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, double rot, int on_bottom, const char *names, long names_len);

pcb_pstk_proto_t *pcb_dlcr_pstk_proto_new(pcb_dlcr_t *dlcr, long int *pid_out);

void pcb_dlcr_subc_begin(pcb_dlcr_t *dlcr, pcb_subc_t *subc);
void pcb_dlcr_subc_end(pcb_dlcr_t *dlcr);


/* Create all objects; call this only after layers are finalized */
void pcb_dlcr_create(pcb_board_t *pcb, pcb_dlcr_t *dlcr);

