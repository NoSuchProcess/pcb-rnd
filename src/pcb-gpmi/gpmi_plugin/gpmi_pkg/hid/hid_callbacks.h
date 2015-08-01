HID_Attribute *gpmi_hid_get_export_options(int *num);
hidGC gpmi_hid_make_gc(void);
void gpmi_hid_destroy_gc(hidGC gc);
void gpmi_hid_do_export(HID_Attr_Val * options);
void gpmi_hid_parse_arguments(int *pcbargc, char ***pcbargv);
void gpmi_hid_set_crosshair(int x, int y);
int gpmi_hid_set_layer(const char *name, int group);
void gpmi_hid_set_color(hidGC gc, const char *name);
void gpmi_hid_set_line_cap(hidGC gc, EndCapStyle style);
void gpmi_hid_set_line_width(hidGC gc, int width);
void gpmi_hid_set_draw_xor(hidGC gc, int xor);
void gpmi_hid_set_draw_faded(hidGC gc, int faded);
void gpmi_hid_draw_line(hidGC gc, int x1, int y1, int x2, int y2);
void gpmi_hid_draw_arc(hidGC gc, int cx, int cy, int xradius, int yradius, int start_angle, int delta_angle);
void gpmi_hid_draw_rect(hidGC gc, int x1, int y1, int x2, int y2);
void gpmi_hid_fill_circle(hidGC gc, int cx, int cy, int radius);
void gpmi_hid_fill_polygon(hidGC gc, int n_coords, int *x, int *y);
void gpmi_hid_fill_pcb_polygon(hidGC gc, PolygonType *poly, const BoxType *clip_box);
void gpmi_hid_fill_rect(hidGC gc, int x1, int y1, int x2, int y2);
void gpmi_hid_use_mask(int use_it);
