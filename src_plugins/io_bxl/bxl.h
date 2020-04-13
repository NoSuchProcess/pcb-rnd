#ifndef PCB_BXL_H
#define PCB_BXL_H

#include "data.h"
#include "layer.h"
#include "obj_subc.h"
#include "obj_poly.h"
#include <genht/htsp.h>

typedef enum {
	PCB_BXL_JUST_TOP    = 1,
	PCB_BXL_JUST_LEFT   = 1,
	PCB_BXL_JUST_CENTER = 2,
	PCB_BXL_JUST_BOTTOM = 4,
	PCB_BXL_JUST_RIGHT  = 4
} pcb_bxl_just_t;

typedef struct {
	double width, height, char_width;
} pcb_bxl_test_style_t;

typedef struct pcb_bxl_ctx_s {
	pcb_board_t *pcb;
	pcb_subc_t *subc;
	char in_target_fp; /* 1 if we are parsing the target footprint; else skip */

	/* cache */
	htsp_t layer_name2ly;
	htsp_t text_name2style;

	struct {
		pcb_layer_t *layer;
		pcb_coord_t origin_x, origin_y, endp_x, endp_y, width, radius;
		pcb_poly_t *poly;
		double arc_start, arc_delta;
		double rot;
		pcb_bxl_just_t hjust, vjust;
		pcb_bxl_test_style_t *text_style;
		char *text_str;
		unsigned flipped:1;
		unsigned invis:1;
		unsigned plated:1;
		unsigned nopaste:1;
		unsigned surface:1;

		unsigned delayed_poly:1;
	} state;

	struct {
		long poly_broken;
		long property_null_obj;
		long property_nosep;
	} warn;
} pcb_bxl_ctx_t;

void pcb_bxl_pattern_begin(pcb_bxl_ctx_t *ctx, const char *name);
void pcb_bxl_pattern_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_reset(pcb_bxl_ctx_t *ctx);
void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name);
void pcb_bxl_set_justify(pcb_bxl_ctx_t *ctx, const char *str);
void pcb_bxl_set_text_str(pcb_bxl_ctx_t *ctx, char *str);
void pcb_bxl_set_text_style(pcb_bxl_ctx_t *ctx, const char *name);

void pcb_bxl_add_property(pcb_bxl_ctx_t *ctx, pcb_any_obj_t *obj, const char *keyval);

void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx);
void pcb_bxl_add_arc(pcb_bxl_ctx_t *ctx);
void pcb_bxl_add_text(pcb_bxl_ctx_t *ctx);

void pcb_bxl_poly_begin(pcb_bxl_ctx_t *ctx);
void pcb_bxl_poly_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_poly_add_vertex(pcb_bxl_ctx_t *ctx, pcb_coord_t x, pcb_coord_t y);

void pcb_bxl_text_style_begin(pcb_bxl_ctx_t *ctx, char *name);
void pcb_bxl_text_style_end(pcb_bxl_ctx_t *ctx);

#endif
