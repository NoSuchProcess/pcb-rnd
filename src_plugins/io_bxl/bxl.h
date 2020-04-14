#ifndef PCB_BXL_H
#define PCB_BXL_H

#include "data.h"
#include "layer.h"
#include "obj_subc.h"
#include "obj_poly.h"
#include "obj_pstk.h"
#include <genht/htsp.h>
#include <genht/htsi.h>

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
	htsi_t proto_name2id;
	int proto_id;

	struct {
		pcb_layer_t *layer;
		pcb_coord_t origin_x, origin_y, endp_x, endp_y, width, height, radius;
		pcb_coord_t hole;
		pcb_poly_t *poly;
		double arc_start, arc_delta;
		double rot;
		int num_shapes, pad_type, shape_type, pin_number;
		long pstk_proto_id;
		char *pin_name;
		pcb_bxl_just_t hjust, vjust;
		pcb_bxl_test_style_t *text_style;
		char *text_str, *attr_key, *attr_val;
		pcb_pstk_proto_t proto;
		unsigned flipped:1;
		unsigned is_visible:1;
		unsigned plated:1;
		unsigned nopaste:1;
		unsigned surface:1;

		unsigned delayed_poly:1;
		unsigned is_text:1;
	} state;

	struct {
		long poly_broken;
		long property_null_obj;
		long property_nosep;
	} warn;
} pcb_bxl_ctx_t;

pcb_coord_t pcb_bxl_coord_x(pcb_coord_t c);
pcb_coord_t pcb_bxl_coord_y(pcb_coord_t c);

void pcb_bxl_pattern_begin(pcb_bxl_ctx_t *ctx, const char *name);
void pcb_bxl_pattern_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_reset(pcb_bxl_ctx_t *ctx);
void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name);
void pcb_bxl_set_justify(pcb_bxl_ctx_t *ctx, const char *str);
void pcb_bxl_set_text_str(pcb_bxl_ctx_t *ctx, char *str);
void pcb_bxl_set_text_style(pcb_bxl_ctx_t *ctx, const char *name);
void pcb_bxl_set_attr_val(pcb_bxl_ctx_t *ctx, char *key, char *val);

void pcb_bxl_add_property(pcb_bxl_ctx_t *ctx, pcb_any_obj_t *obj, const char *keyval);

void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx);
void pcb_bxl_add_arc(pcb_bxl_ctx_t *ctx);
void pcb_bxl_add_text(pcb_bxl_ctx_t *ctx);

void pcb_bxl_poly_begin(pcb_bxl_ctx_t *ctx);
void pcb_bxl_poly_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_poly_add_vertex(pcb_bxl_ctx_t *ctx, pcb_coord_t x, pcb_coord_t y);

void pcb_bxl_text_style_begin(pcb_bxl_ctx_t *ctx, char *name);
void pcb_bxl_text_style_end(pcb_bxl_ctx_t *ctx);

void pcb_bxl_padstack_begin(pcb_bxl_ctx_t *ctx, char *name);
void pcb_bxl_padstack_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_padstack_begin_shape(pcb_bxl_ctx_t *ctx, const char *name);
void pcb_bxl_padstack_end_shape(pcb_bxl_ctx_t *ctx);

void pcb_bxl_pad_begin(pcb_bxl_ctx_t *ctx);
void pcb_bxl_pad_end(pcb_bxl_ctx_t *ctx);
void pcb_bxl_pad_set_style(pcb_bxl_ctx_t *ctx, const char *pstkname);
#endif
