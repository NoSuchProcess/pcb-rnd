#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include "board.h"

#include "data.h"
#include "layer.h"
#include "obj_common.h"
#include "obj_subc.h"

typedef struct {
	char *name;
	long id;
	pcb_layer_type_t lyt;
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


#define PCB_DLCR_INVALID_LAYER_ID -32768

typedef enum {
	DLCR_OBJ,
	DLCR_SUBC_BEGIN,
	DLCR_SUBC_END,
	DLCR_SUBC_FROM_LIB    /* place a subc from local lib by a list of names */
/*	DLCR_SEQPOLY_BEGIN,
	DLCR_SEQPOLY_END*/
} pcb_dlcr_type_t;


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
	} val;
	long loc_line; /* for debug */
	gdl_elem_t link;
} pcb_dlcr_draw_t;

typedef struct {
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

	/* config */
	unsigned flip_y:1;   /* if 1, mirror y coordinates over the X axis */
} pcb_dlcr_t;

void pcb_dlcr_init(pcb_dlcr_t *dlcr);
void pcb_dlcr_uninit(pcb_dlcr_t *dlcr);

void pcb_dlcr_layer_reg(pcb_dlcr_t *dlcr, pcb_dlcr_layer_t *layer);
void pcb_dlcr_layer_free(pcb_dlcr_layer_t *layer);

/* allocate a new named subcircuit within the local dlcr lib */
pcb_subc_t *pcb_dlcr_subc_new_in_lib(pcb_dlcr_t *dlcr, const char *name);

pcb_dlcr_draw_t *pcb_dlcr_line_new(pcb_dlcr_t *dlcr, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t width, rnd_coord_t clearance);
pcb_dlcr_draw_t *pcb_dlcr_arc_new(pcb_dlcr_t *dlcr, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, double start_deg, double delta_deg, rnd_coord_t width, rnd_coord_t clearance);
pcb_dlcr_draw_t *pcb_dlcr_text_new(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, double rot, int scale, rnd_coord_t thickness, const char *str);
pcb_dlcr_draw_t *pcb_dlcr_via_new(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, rnd_coord_t clearance, long id, const char *name);

/* delayed place a subcricuit from the local dlcr lib by name; names is either
   a single string or a \0 separated list of strings with an empty string at
   the end */
pcb_dlcr_draw_t *pcb_dlcr_subc_new_from_lib(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, double rot, int on_bottom, const char *names, long names_len);

pcb_pstk_proto_t *pcb_dlcr_pstk_proto_new(pcb_dlcr_t *dlcr, long int *pid_out);

void pcb_dlcr_subc_begin(pcb_dlcr_t *dlcr, pcb_subc_t *subc);
void pcb_dlcr_subc_end(pcb_dlcr_t *dlcr);


/* Create all objects; call this only after layers are finalized */
void pcb_dlcr_create(pcb_board_t *pcb, pcb_dlcr_t *dlcr);

