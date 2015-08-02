HID_Attribute *gpmi_hid_get_export_options(int *num);
hidGC gpmi_hid_make_gc(void);
void gpmi_hid_destroy_gc(hidGC gc);
void gpmi_hid_do_export(HID_Attr_Val * options);
void gpmi_hid_parse_arguments(int *pcbargc, char ***pcbargv);
void gpmi_hid_set_crosshair(int x, int y, int cursor_action);
int gpmi_hid_set_layer(const char *name, int group, int _empty);
void gpmi_hid_set_color(hidGC gc, const char *name);
void gpmi_hid_set_line_cap(hidGC gc, EndCapStyle style);
void gpmi_hid_set_line_width(hidGC gc, Coord width);
void gpmi_hid_set_draw_xor(hidGC gc, int xor);
void gpmi_hid_set_draw_faded(hidGC gc, int faded);
void gpmi_hid_draw_line(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void gpmi_hid_draw_arc(hidGC gc, Coord cx, Coord cy, Coord xradius, Coord yradius, Angle start_angle, Angle delta_angle);
void gpmi_hid_draw_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void gpmi_hid_fill_circle(hidGC gc, Coord cx, Coord cy, Coord radius);
void gpmi_hid_fill_polygon(hidGC gc, int n_coords, Coord *x, Coord *y);
void gpmi_hid_fill_pcb_polygon(hidGC gc, PolygonType *poly, const BoxType *clip_box);
void gpmi_hid_fill_rect(hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
void gpmi_hid_use_mask(int use_it);
