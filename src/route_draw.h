#ifndef PCB_ROUTE_DRAW_H
#define PCB_ROUTE_DRAW_H

#include <librnd/core/global_typedefs.h>
#include <librnd/core/hid.h>

void pcb_route_draw(pcb_route_t *p_route, rnd_hid_gc_t GC);
void pcb_route_draw_drc(pcb_route_t *p_route, rnd_hid_gc_t GC);

#endif
