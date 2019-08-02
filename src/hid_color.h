#ifndef PCB_HID_COLOR_H
#define PCB_HID_COLOR_H

#include "hid.h"

/* Used to cache color lookups.  If set is zero, it looks up the name
   and if found sets val and returns nonzero.  If not found, it
   returns zero.  If set is nonzero, name/val is added to the
   cache.  */
int pcb_hid_cache_color(int set, const char *name, pcb_hidval_t * val, void **cache);

/* Call user_free() on each cache entry and then destroy the cache */
void pcb_hid_cache_color_destroy(void **vcache, void (*user_free)(void **vcache, const char *name, pcb_hidval_t *val));

#endif
