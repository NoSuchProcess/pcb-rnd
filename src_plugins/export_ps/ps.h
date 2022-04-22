extern const char *ps_cookie;
extern rnd_hid_t ps_hid;
extern void ps_hid_export_to_file(FILE *, rnd_hid_attr_val_t *, rnd_xform_t *);
extern void hid_eps_init();
extern void hid_eps_uninit();
void ps_ps_init(rnd_hid_t * hid);


typedef struct rnd_ps_s {
	FILE *outf;
} rnd_ps_t;

extern void ps_start_file(rnd_ps_t *, const char *swver);
