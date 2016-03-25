#ifndef PCB_HID_INIT_H
#define PCB_HID_INIT_H

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* A HID may use this if it does not need command line arguments in
   any special format; for example, the Lesstif HID needs to use the
   Xt parser, but the Postscript HID can use this function.  */
void hid_parse_command_line(int *argc, char ***argv);

/* Called by the init funcs, used to set up hid_list.  */
extern void hid_register_hid(HID * hid);
void hid_remove_hid(HID * hid);


/* NULL terminated list of all static HID structures.  Built by
   hid_register_hid, used by hid_find_*() and hid_enumerate().  The
   order in this list is the same as the order of hid_register_hid
   calls.  */
extern HID **hid_list;

/* Count of entries in the above.  */
extern int hid_num_hids;

#endif
