#ifndef PCB_HID_EXTENTS_H
#define PCB_HID_EXTENTS_H

/* DO NOT USE: will be removed soom. Use a normal bbox calculation */

/* Convenience function that calls the expose callback for the item,
   and returns the extents of what was drawn.  */
pcb_box_t *pcb_hid_get_extents_pinout(void *item);


#endif
