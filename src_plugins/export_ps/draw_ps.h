#include <librnd/core/hidlib.h>
typedef struct rnd_ps_s {

	/* public: config */
	rnd_hidlib_t *hidlib;
	FILE *outf;
	double calibration_x, calibration_y;
	double fade_ratio;
	rnd_bool invert;
	rnd_bool align_marks;
	rnd_bool fillpage;
	rnd_bool incolor;
	int media_idx;
	rnd_bool legend;
	rnd_bool single_page;

	/* public: result */
	int pagecount;
	long drawn_objs;

	/* private: cache */
	rnd_coord_t linewidth;
	rnd_coord_t ps_width, ps_height;
	double scale_factor;
	rnd_coord_t media_width, media_height;
	rnd_composite_op_t drawing_mode;
	int lastcap, lastcolor, lastgroup;
	rnd_bool doing_toc;

	/* private: pcb-rnd leftover */
	rnd_bool is_mask;
	rnd_bool is_drill;
} rnd_ps_t;

void rnd_ps_init(rnd_ps_t *pctx, rnd_hidlib_t *hidlib, FILE *f, int media_idx, int fillpage, double scale_factor);

void rnd_ps_start_file(rnd_ps_t *pctx, const char *swver);
void rnd_ps_end_file(rnd_ps_t *pctx);

void rnd_ps_begin_toc(rnd_ps_t *pctx);
void rnd_ps_end_toc(rnd_ps_t *pctx);
void rnd_ps_begin_pages(rnd_ps_t *pctx);
void rnd_ps_end_pages(rnd_ps_t *pctx);

int rnd_ps_printed_toc(rnd_ps_t *pctx, int group, const char *name);
int rnd_ps_is_new_page(rnd_ps_t *pctx, int group);
int rnd_ps_new_file(rnd_ps_t *pctx, FILE *new_f, const char *fn);
double rnd_ps_page_frame(rnd_ps_t *pctx, int mirror_this, const char *layer_fn, int noscale);
void rnd_ps_page_background(rnd_ps_t *pctx, int has_outline, int page_is_route, rnd_coord_t min_wid);

rnd_hid_gc_t rnd_ps_make_gc(rnd_hid_t *hid);
void rnd_ps_destroy_gc(rnd_hid_gc_t gc);
void rnd_ps_use_gc(rnd_ps_t *pctx, rnd_hid_gc_t gc);

void rnd_ps_set_drawing_mode(rnd_ps_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);
void rnd_ps_set_color(rnd_ps_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color);
void rnd_ps_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style);
void rnd_ps_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width);
void rnd_ps_set_draw_xor(rnd_hid_gc_t gc, int xor_);
void rnd_ps_set_draw_faded(rnd_hid_gc_t gc, int faded);
void rnd_ps_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action);

void rnd_ps_draw_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_ps_draw_line(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_ps_draw_arc(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle);
void rnd_ps_fill_circle(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
void rnd_ps_fill_polygon_offs(rnd_ps_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void rnd_ps_fill_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);


/*** for pcb-rnd only ***/
int rnd_ps_gc_get_erase(rnd_hid_gc_t gc);
