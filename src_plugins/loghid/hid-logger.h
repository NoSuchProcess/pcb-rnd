#ifndef PCB_HID_LOGGER_H
#define PCB_HID_LOGGER_H

#include <librnd/core/hid.h>

#include <stdio.h>

/* Set up the delegating loghid that sends all calls to the delegatee but also
   logs the calls. */
void create_log_hid(FILE *log_out, pcb_hid_t *loghid, pcb_hid_t *delegatee);

#endif
