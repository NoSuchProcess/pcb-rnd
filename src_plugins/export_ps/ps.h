extern const char *ps_cookie;
extern rnd_hid_t ps_hid;
extern void ps_hid_export_to_file(FILE *, rnd_hid_attr_val_t *, rnd_xform_t *);
extern void ps_start_file(FILE *);
extern void ps_calibrate_1(rnd_hid_t *hid, double, double, int);
extern void hid_eps_init();
void ps_ps_init(rnd_hid_t * hid);
