#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include "board.h"

#include "layer.h"
#include "obj_common.h"

typedef struct {
	char *name;
	long id;
	pcb_layer_type_t lyt;
	const char *purpose;
	int can_have_components;

	rnd_box_t board_bbox;

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
	DLCR_OBJ
/*	DLCR_SEQPOLY_BEGIN,
	DLCR_SEQPOLY_END*/
} pcb_dlcr_type_t;


typedef struct {
	pcb_dlcr_type_t type;
	union {
		struct {
			pcb_any_line_t obj;
			long layer_id;
			char *layer_name;
		} obj;
	} val;
	gdl_elem_t link;
} pcb_dlcr_draw_t;

typedef struct {
	htsp_t name2layer;   /* key=->id, val=pcb_dlcr_layer_t */
	vtp0_t id2layer;
	gdl_list_t drawing;  /* of pcb_dlcr_draw_t; */
} pcb_dlcr_t;

void pcb_dlcr_init(pcb_dlcr_t *dlcr);
void pcb_dlcr_uninit(pcb_dlcr_t *dlcr);

void pcb_dlcr_layer_reg(pcb_dlcr_t *dlcr, pcb_dlcr_layer_t *layer);
void pcb_dlcr_layer_free(pcb_dlcr_layer_t *layer);

pcb_dlcr_draw_t *pcb_dlcr_line_new(pcb_dlcr_t *dlcr, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t width, rnd_coord_t clearance);

/* Create all objects; call this only after layers are finalized */
void pcb_dlcr_create(pcb_board_t *pcb, pcb_dlcr_t *dlcr);

