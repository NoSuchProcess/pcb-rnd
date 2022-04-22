typedef struct rnd_ps_s {
	/* public: config */
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

} rnd_ps_t;

void rnd_ps_init(rnd_ps_t *pctx, FILE *f);

void rnd_ps_start_file(rnd_ps_t *pctx, const char *swver);
void rnd_ps_end_file(rnd_ps_t *pctx);


void rnd_ps_set_color(rnd_ps_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color);
void rnd_ps_draw_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_ps_draw_line(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_ps_draw_arc(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle);
void rnd_ps_fill_circle(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
void rnd_ps_fill_polygon_offs(rnd_ps_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void rnd_ps_fill_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);




