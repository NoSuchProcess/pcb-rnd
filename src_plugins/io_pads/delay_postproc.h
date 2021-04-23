#include "thermal.h"

/* Set thermal on all terminals that are in the same net and within the poly */
void pcb_dlcr_post_poly_thermal_netname(pcb_board_t *pcb, pcb_poly_t *poly, const char *netname, pcb_thermal_t t, const char *(*obj_netname)(void *uctx, pcb_any_obj_t *obj), void *uctx);




