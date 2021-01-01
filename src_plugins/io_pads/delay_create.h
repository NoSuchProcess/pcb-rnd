#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include "layer.h"
#include "obj_common.h"

typedef struct {
	char *name;
	long id;
	pcb_layer_type_t lyt;
	const char *purpose;
	int can_have_components;
	void *user_ctx;
	union {
		char start;
		void *bulkp[32];
		double bulkd[32];
	} local_user_data;
} pcb_dlcr_layer_t;


typedef struct {
	pcb_any_obj_t obj;
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


