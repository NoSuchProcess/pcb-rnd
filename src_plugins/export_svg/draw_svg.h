#include <librnd/core/hidlib.h>

typedef struct {
	/* public: config */
	rnd_hidlib_t *hidlib;
	FILE *outf;
	gds_t sbright, sdark, snormal, sclip; /* accumulators for various groups generated parallel */
	int opacity;
	int flip;
	int true_size;

	/* public: result */
	long drawn_objs;

	/* private: cache */
	int group_open, comp_cnt;
	rnd_composite_op_t drawing_mode;

	/* private: pcb-rnd leftover */
	int photo_mode, photo_noise, drawing_mask, drawing_hole;
} rnd_svg_t;


void rnd_svg_init(rnd_svg_t *pctx, rnd_hidlib_t *hidlib, FILE *f, int opacity, int flip, int true_size);
void rnd_svg_uninit(rnd_svg_t *pctx);

/* Write header or footer. Footer also writes cached groups and closes them. */
void rnd_svg_header(rnd_svg_t *pctx);
void rnd_svg_footer(rnd_svg_t *pctx);

int rnd_svg_new_file(rnd_svg_t *pctx, FILE *f, const char *fn);
void rnd_svg_layer_group_begin(rnd_svg_t *pctx, long group, const char *name, int is_our_mask);

void rnd_svg_background(rnd_svg_t *pctx);


rnd_hid_gc_t rnd_svg_make_gc(rnd_hid_t *hid);
void rnd_svg_destroy_gc(rnd_hid_gc_t gc);
void rnd_svg_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style);
void rnd_svg_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width);
void rnd_svg_set_draw_xor(rnd_hid_gc_t gc, int xor_);
void rnd_svg_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a);



void rnd_svg_set_drawing_mode(rnd_svg_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);
void rnd_svg_set_color(rnd_svg_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color);
void rnd_svg_draw_rect(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_svg_fill_rect(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_svg_draw_line(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_svg_draw_arc(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle);
void rnd_svg_fill_circle(rnd_svg_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
void rnd_svg_fill_polygon_offs(rnd_svg_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);

/* leftover for pcb-rnd */
typedef enum {
	RND_SVG_PHOTO_MASK,
	RND_SVG_PHOTO_SILK,
	RND_SVG_PHOTO_COPPER,
	RND_SVG_PHOTO_INNER
} rnd_svg_photo_color_t;

extern rnd_svg_photo_color_t rnd_svg_photo_color;


