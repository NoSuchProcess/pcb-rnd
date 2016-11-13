#ifndef PCB_HID_INIT_H
#define PCB_HID_INIT_H

#include "hid.h"

/* NULL terminated list of all static HID structures.  Built by
   hid_register_hid, used by hid_find_*() and pcb_hid_enumerate().  The
   order in this list is the same as the order of hid_register_hid
   calls.  */
extern pcb_hid_t **hid_list;

/* Count of entries in the above.  */
extern int hid_num_hids;

/* Call this as soon as possible from main().  No other HID calls are
   valid until this is called.  */
void pcb_hid_init(void);

/* Call this at exit */
void pcb_hid_uninit(void);

/* When PCB runs in interactive mode, this is called to instantiate
   one GUI HID which happens to be the GUI.  This HID is the one that
   interacts with the mouse and keyboard.  */
pcb_hid_t *pcb_hid_find_gui(const char *preference);

/* Finds the one printer HID and instantiates it.  */
pcb_hid_t *pcb_hid_find_printer(void);

/* Finds the indicated exporter HID and instantiates it.  */
pcb_hid_t *pcb_hid_find_exporter(const char *);

/* This returns a NULL-terminated array of available HIDs.  The only
   real reason to use this is to locate all the export-style HIDs. */
pcb_hid_t **pcb_hid_enumerate(void);

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* A HID may use this if it does not need command line arguments in
   any special format; for example, the Lesstif HID needs to use the
   Xt parser, but the Postscript HID can use this function.  */
void pcb_hid_parse_command_line(int *argc, char ***argv);

/* Called by the init funcs, used to set up hid_list.  */
extern void pcb_hid_register_hid(pcb_hid_t * hid);
void pcb_hid_remove_hid(pcb_hid_t * hid);

#endif
