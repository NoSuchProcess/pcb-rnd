#ifndef PCB_BXL_H
#define PCB_BXL_H

#include "data.h"
#include "layer.h"
#include "obj_subc.h"
#include "obj_poly.h"
#include <genht/htsp.h>

enum {
	BXL_ORIGIN_X,
	BXL_ORIGIN_Y,
	BXL_ENDP_X,
	BXL_ENDP_Y,
	BXL_WIDTH,
	BXL_coord_max
};

typedef struct pcb_bxl_ctx_s {
	pcb_subc_t *subc;
	char in_target_fp; /* 1 if we are parsing the target footprint; else skip */

	/* cache */
	htsp_t layer_name2ly;

	struct {
		pcb_layer_t *layer;
		pcb_coord_t coord[BXL_coord_max];
		pcb_poly_t *poly;
		unsigned delayed_poly:1;
	} state;

	struct {
		long property_null_obj;
		long property_nosep;
	} warn;
} pcb_bxl_ctx_t;

void pcb_bxl_pattern_begin(pcb_bxl_ctx_t *ctx, const char *name);
void pcb_bxl_pattern_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_reset(pcb_bxl_ctx_t *ctx);
void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name);
void pcb_bxl_set_coord(pcb_bxl_ctx_t *ctx, int idx, pcb_coord_t val);

void pcb_bxl_add_property(pcb_bxl_ctx_t *ctx, pcb_any_obj_t *obj, const char *keyval);

void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx);

void pcb_bxl_poly_begin(pcb_bxl_ctx_t *ctx);
void pcb_bxl_poly_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_poly_add_vertex(pcb_bxl_ctx_t *ctx, pcb_coord_t x, pcb_coord_t y);

#endif
