#include "bxl.h"

rnd_coord_t pcb_bxl_coord_x(rnd_coord_t c) { return c; }
rnd_coord_t pcb_bxl_coord_y(rnd_coord_t c) { return -c; }

void pcb_bxl_pattern_begin(pcb_bxl_ctx_t *ctx, const char *name) {}
void pcb_bxl_pattern_end(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_reset(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_reset_pattern(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name) {}
void pcb_bxl_set_justify(pcb_bxl_ctx_t *ctx, const char *str) {}
void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_add_arc(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_add_text(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_set_text_str(pcb_bxl_ctx_t *ctx, char *str) { free(str); }
void pcb_bxl_poly_begin(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_poly_end(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_poly_add_vertex(pcb_bxl_ctx_t *ctx, rnd_coord_t x, rnd_coord_t y) {}
void pcb_bxl_add_property(pcb_bxl_ctx_t *ctx, pcb_any_obj_t *obj, const char *keyval) {}
void pcb_bxl_set_attr_val(pcb_bxl_ctx_t *ctx, char *key, char *val) { free(key); free(val); }

static pcb_bxl_test_style_t dummy_ts;
void pcb_bxl_text_style_begin(pcb_bxl_ctx_t *ctx, char *name) { free(name); ctx->state.text_style = &dummy_ts; }
void pcb_bxl_text_style_end(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_set_text_style(pcb_bxl_ctx_t *ctx, const char *name) {}


void pcb_bxl_padstack_begin(pcb_bxl_ctx_t *ctx, char *name) { free(name); }
void pcb_bxl_padstack_end(pcb_bxl_ctx_t *ctx) { }
void pcb_bxl_padstack_begin_shape(pcb_bxl_ctx_t *ctx, const char *name) {}
void pcb_bxl_padstack_end_shape(pcb_bxl_ctx_t *ctx) {}


void pcb_bxl_pad_begin(pcb_bxl_ctx_t *ctx) {}
void pcb_bxl_pad_end(pcb_bxl_ctx_t *ctx) { free(ctx->state.pin_name); }
void pcb_bxl_pad_set_style(pcb_bxl_ctx_t *ctx, const char *pstkname) { }
