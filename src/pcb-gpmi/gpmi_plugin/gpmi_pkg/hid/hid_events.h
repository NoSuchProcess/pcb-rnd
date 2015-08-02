/*********** Exporter events ************/

/* Called before get_exporter_options returns the option list to the GUI hid */
gpmi_define_event(HIDE_get_export_options)(void *hid);
/* Called before export redraw starts */
gpmi_define_event(HIDE_do_export_start)(void *hid);
/* Called after export redraw finihsed */
gpmi_define_event(HIDE_do_export_finish)(void *hid);


/* PCB callback events for drawing */
gpmi_define_event(HIDE_set_layer)(void *hid, const char *name, int group, int empty);
gpmi_define_event(HIDE_set_color)(void *hid, void *gc, const char *name);
gpmi_define_event(HIDE_set_line_cap)(void *hid, void *gc, EndCapStyle style);
gpmi_define_event(HIDE_set_line_width)(void *hid, void *gc, int width);
gpmi_define_event(HIDE_set_draw_xor)(void *hid, void *gc, int xor);
gpmi_define_event(HIDE_set_draw_faded)(void *hid, void *gc, int faded);
gpmi_define_event(HIDE_draw_line)(void *hid, void *gc, int x1, int y1, int x2, int y2);
gpmi_define_event(HIDE_draw_arc)(void *hid, void *gc, int cx, int cy, int xradius, int yradius, double start_angle, double delta_angle);
gpmi_define_event(HIDE_draw_rect)(void *hid, void *gc, int x1, int y1, int x2, int y2);
gpmi_define_event(HIDE_fill_circle)(void *hid, void *gc, int cx, int cy, int radius);
gpmi_define_event(HIDE_fill_polygon)(void *hid, void *gc, int n_coords, int *x, int *y);
gpmi_define_event(HIDE_fill_rect)(void *hid, void *gc, int x1, int y1, int x2, int y2);
gpmi_define_event(HIDE_use_mask)(void *hid, int use_it);
gpmi_define_event(HIDE_make_gc)(void *hid, void *gc);
gpmi_define_event(HIDE_destroy_gc)(void *hid, void *gc);

