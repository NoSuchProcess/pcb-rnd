#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include "layer.h"
#include "obj_common.h"

typedef struct {
	char *name;
	pcb_layer_type_t lyt;
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
	htsp_t layers;       /* key=->name, val=pcb_dlcr_layer_t */
	gdl_list_t drawing;  /* of pcb_dlcr_draw_t; */
} pcb_dlcr_t;
