void pcb_subc_xy_rot(pcb_subc_t *subc, rnd_coord_t *cx, rnd_coord_t *cy, double *theta, double *xray_theta, rnd_bool autodetect);

/* Automatic version of pcb_subc_xy_rot: inserts all the p&p attributes and creates the aux layer */
void pcb_subc_xy_rot_pnp(pcb_subc_t *subc, rnd_coord_t subc_ox, rnd_coord_t subc_oy, rnd_bool on_bottom);

