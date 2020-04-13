#include "bxl.h"

void pcb_bxl_pattern_begin(pcb_bxl_ctx_t *ctx, const char *name) {}
void pcb_bxl_pattern_end(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_reset(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name) {}
void pcb_bxl_set_coord(pcb_bxl_ctx_t *ctx, int idx, pcb_coord_t val) {}
void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_add_arc(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_poly_begin(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_poly_end(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_poly_add_vertex(pcb_bxl_ctx_t *ctx, pcb_coord_t x, pcb_coord_t y) {}
void pcb_bxl_add_property(pcb_bxl_ctx_t *ctx, pcb_any_obj_t *obj, const char *keyval) {}

