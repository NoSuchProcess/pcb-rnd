#ifndef PCB_HID_EXTENTS_H
#define PCB_HID_EXTENTS_H

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* Convenience function that calls the expose callback for the item,
   and returns the extents of what was drawn.  */
BoxType *hid_get_extents(void *item);


#endif
