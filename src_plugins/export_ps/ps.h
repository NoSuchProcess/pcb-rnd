/* required by lpr */
extern rnd_hid_t ps_hid;
extern void ps_hid_export_to_file(FILE *, rnd_hid_attr_val_t *, rnd_xform_t *);
void ps_ps_init(rnd_hid_t * hid);

/* required for ps<->eps cross call */
extern void hid_eps_init();
extern void hid_eps_uninit();
extern const char *ps_cookie;

/* will be the low level */
typedef struct rnd_ps_s {
	FILE *outf;
} rnd_ps_t;

extern void ps_start_file(rnd_ps_t *, const char *swver);
