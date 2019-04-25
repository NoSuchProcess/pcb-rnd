#ifndef PCB_HID_FLAGS_H
#define PCB_HID_FLAGS_H


/* Looks up an integer (usually boolean) value by conf path or by running
   an action (if name has a parenthesis). When an action is run, it has 0
   or 1 argument only and the return value of the action is returned.
   On error, returns -1. */
int pcb_hid_get_flag(const char *name);

#endif
