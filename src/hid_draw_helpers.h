#ifndef PCB_HID_DRAW_HELPERS_H
#define PCB_HID_DRAW_HELPERS_H
void common_fill_pcb_polygon(pcb_hid_gc_t gc, pcb_polygon_t * poly, const pcb_box_t * clip_box);
void common_thindraw_pcb_polygon(pcb_hid_gc_t gc, pcb_polygon_t * poly, const pcb_box_t * clip_box);
void common_fill_pcb_pad(pcb_hid_gc_t gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask);
void common_thindraw_pcb_pad(pcb_hid_gc_t gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask);
void common_fill_pcb_pv(pcb_hid_gc_t gc, pcb_pin_t * pv, pcb_bool drawHole, pcb_bool mask);
void common_thindraw_pcb_pv(pcb_hid_gc_t fg_gc, pcb_hid_gc_t bg_gc, pcb_pin_t * pv, pcb_bool drawHole, pcb_bool mask);
void common_draw_helpers_init(pcb_hid_t * hid);
#endif
