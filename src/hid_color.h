#ifndef PCB_HID_COLOR_H
#define PCB_HID_COLOR_H

#include "hid.h"

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* Used to cache color lookups.  If set is zero, it looks up the name
   and if found sets val and returns nonzero.  If not found, it
   returns zero.  If set is nonzero, name/val is added to the
   cache.  */
int hid_cache_color(int set, const char *name, pcb_hidval_t * val, void **cache);

#endif
