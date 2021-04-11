/*** This API should be used only by io_pcb and io_lihata ***/

/* For IO code: attempt to convert via padstack proto of a route style into old
   geda/pcb route style description. Generates IO errors on failure. */
int pcb_compat_route_style_via_save(pcb_data_t *data, const pcb_route_style_t *rst, rnd_coord_t *drill_dia, rnd_coord_t *pad_dia, rnd_coord_t *mask);

/* For IO code: look up or create a via pstk prototype for a route style that
   specified diameters (old style via description). */
int pcb_compat_route_style_via_load(pcb_data_t *data, pcb_route_style_t *rst, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t mask);
