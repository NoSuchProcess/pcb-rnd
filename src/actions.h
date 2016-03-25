#ifndef PCB_HID_ACTIONS_H
#define PCB_HID_ACTIONS_H

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

void print_actions(void);

const HID_Action *hid_find_action(const char *name);

#endif
