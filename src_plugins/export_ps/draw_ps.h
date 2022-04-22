typedef struct rnd_ps_s {
	FILE *outf;
} rnd_ps_t;

void rnd_ps_init(rnd_ps_t *pctx, FILE *f);

void ps_start_file(rnd_ps_t *pctx, const char *swver);
void ps_end_file(rnd_ps_t *pctx);


void rnd_ps_draw_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_ps_draw_line(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void rnd_ps_draw_arc(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle);
void rnd_ps_fill_circle(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius);
void rnd_ps_fill_polygon_offs(rnd_ps_t *pctx, rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void rnd_ps_fill_rect(rnd_ps_t *pctx, rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);





