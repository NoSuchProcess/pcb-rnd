#ifndef PCB_LIB_HID_COMMON_CONF_H
#define PCB_LIB_HID_COMMON_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct lib_hid_common {
			const struct cli_history {
				CFT_STRING file;       /* Path to the history file (empty/unset means history is not preserved) */
				CFT_INTEGER slots;     /* Number of commands to store in the history */
			} cli_history;
		} lib_hid_common;
	} plugins;
} conf_lib_hid_common_t;

#endif
