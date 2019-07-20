extern const char *ps_cookie;
extern pcb_hid_t ps_hid;
extern void ps_hid_export_to_file(FILE *, pcb_hid_attr_val_t *);
extern void ps_start_file(FILE *);
extern void ps_calibrate_1(pcb_hid_t *hid, double, double, int);
extern void hid_eps_init();
void ps_ps_init(pcb_hid_t * hid);
