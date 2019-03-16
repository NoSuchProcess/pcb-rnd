#ifndef PCB_ROUTE_DRAW_H
#define PCB_ROUTE_DRAW_H

#include "global_typedefs.h"
#include "hid.h"

void pcb_route_draw(pcb_route_t *p_route, pcb_hid_gc_t GC);
void pcb_route_draw_drc(pcb_route_t *p_route, pcb_hid_gc_t GC);

#endif
