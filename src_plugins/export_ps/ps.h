/* required by lpr */
extern rnd_hid_t ps_hid;
extern void ps_hid_export_to_file(rnd_design_t *dsg, FILE *, rnd_hid_attr_val_t *, rnd_xform_t *, void *);
void ps_ps_init(rnd_hid_t * hid);

/* required for ps<->eps cross call */
extern void hid_eps_init();
extern void hid_eps_uninit();
extern const char *ps_cookie;

