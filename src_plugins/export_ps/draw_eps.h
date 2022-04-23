typedef struct {
	/* public: config */
	FILE *outf;
	int in_mono, as_shown;
	rnd_box_t bounds;
	double scale;

	/* public: result */
	long drawn_objs;

	/* private: cache */
	rnd_coord_t linewidth;
	int lastcap;
	int lastcolor;
	rnd_composite_op_t drawing_mode;

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
} rnd_eps_t;

/* Set up context for a file */
void rnd_eps_init(rnd_eps_t *pctx, FILE *f, rnd_box_t bounds, double scale, int in_mono, int as_shown);

/* Set up output file and print header before export, footer after export */
void rnd_eps_print_header(rnd_eps_t *pctx, const char *outfn, int ymirror);
void rnd_eps_print_footer(rnd_eps_t *pctx);


/* standard HID API */
rnd_hid_gc_t rnd_eps_make_gc(rnd_hid_t *hid);
void rnd_eps_destroy_gc(rnd_hid_gc_t gc);
void rnd_eps_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style);
void rnd_eps_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width);
void rnd_eps_set_draw_xor(rnd_hid_gc_t gc, int xor_);
void rnd_eps_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int action);


/* standard HID API with extra pctx first arg */
void rnd_eps_set_drawing_mode(rnd_eps_t *pctx, rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);
void rnd_eps_set_color(rnd_eps_t *pctx, rnd_hid_gc_t gc, const rnd_color_t *color);
void rnd_eps_draw_rect(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_eps_draw_line(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_eps_draw_arc(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle);
void rnd_eps_fill_circle(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
void rnd_eps_fill_polygon_offs(rnd_eps_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void rnd_eps_fill_rect(rnd_eps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);

