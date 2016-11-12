#ifndef PCB_HID_LOGGER_H
#define PCB_HID_LOGGER_H

#include "hid.h"

#include <stdio.h>

/*
 * Create a delegating HID that sends all calls to the
 * delegatee but also logs the calls.
 */
pcb_hid_t *create_log_hid(FILE *log_out, pcb_hid_t *delegatee);

#endif
