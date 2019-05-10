#ifndef PCB_HID_DRAW_HELPERS_H
#define PCB_HID_DRAW_HELPERS_H
void pcb_dhlp_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t * poly, const pcb_box_t * clip_box);
void pcb_dhlp_thindraw_pcb_polygon(pcb_hid_gc_t gc, pcb_poly_t * poly, const pcb_box_t * clip_box);
#endif
