/*********** Exporter events ************/

/* Called before get_exporter_options returns the option list to the GUI hid */
gpmi_define_event(HIDE_get_export_options)(void *hid);

/* Called before export redraw starts */
gpmi_define_event(HIDE_do_export_start)(void *hid);

/* Called after export redraw finihsed */
gpmi_define_event(HIDE_do_export_finish)(void *hid);

/* DRAWING */

/* PCB callback events for drawing: change layer */
gpmi_define_event(HIDE_set_layer)(void *hid, const char *name, int group, int empty);

/* PCB callback events for drawing: change drawing color */
gpmi_define_event(HIDE_set_color)(void *hid, void *gc, const char *name);

/* PCB callback events for drawing: change drawing line cap style*/
gpmi_define_event(HIDE_set_line_cap)(void *hid, void *gc, pcb_cap_style_t style);

/* PCB callback events for drawing: change drawing line width */
gpmi_define_event(HIDE_set_line_width)(void *hid, void *gc, int width);

/* PCB callback events for drawing: toggle xor drawing method */
gpmi_define_event(HIDE_set_draw_xor)(void *hid, void *gc, int xor);

/* PCB callback events for drawing: toggle faded drawing method */
gpmi_define_event(HIDE_set_draw_faded)(void *hid, void *gc, int faded);

/* PCB callback events for drawing: draw a line */
gpmi_define_event(HIDE_draw_line)(void *hid, void *gc, int x1, int y1, int x2, int y2);

/* PCB callback events for drawing: draw an arc from center cx;cy */
gpmi_define_event(HIDE_draw_arc)(void *hid, void *gc, int cx, int cy, int xradius, int yradius, double start_angle, double delta_angle);

/* PCB callback events for drawing: draw a rectangle */
gpmi_define_event(HIDE_draw_rect)(void *hid, void *gc, int x1, int y1, int x2, int y2);

/* PCB callback events for drawing: draw a filled circle */
gpmi_define_event(HIDE_fill_circle)(void *hid, void *gc, int cx, int cy, int radius);

/* PCB callback events for drawing: draw a filled ploygon */
gpmi_define_event(HIDE_fill_polygon)(void *hid, void *gc, int n_coords, int *x, int *y);

/* PCB callback events for drawing: draw a filled rectangle */
gpmi_define_event(HIDE_fill_rect)(void *hid, void *gc, int x1, int y1, int x2, int y2);

/* PCB callback events for drawing: TODO */
gpmi_define_event(HIDE_use_mask)(void *hid, int use_it);

/* PCB callback events for drawing: create a new graphical context */
gpmi_define_event(HIDE_make_gc)(void *hid, void *gc);

/* PCB callback events for drawing: destroy a graphical context */
gpmi_define_event(HIDE_destroy_gc)(void *hid, void *gc);

/* PCB callback events for drawing: TODO */
gpmi_define_event(HIDE_fill_pcb_pv)(void *hid, void *fg_gc, void *bg_gc, void *pad, int drawHole, int mask);

/* PCB callback events for drawing: TODO */
gpmi_define_event(HIDE_fill_pcb_pad)(void *hid, void *pad, int clear, int mask);
