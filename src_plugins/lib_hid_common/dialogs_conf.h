#ifndef PCB_DIALOGS_CONF_H
#define PCB_DIALOGS_CONF_H

#include "conf.h"

/* dialog config is here because of hidlib: each hidlib user shall have its
   own dialogs plugin */

typedef struct {
	const struct plugins {
		const struct dialogs {
			const struct auto_save_window_geometry {
				CFT_BOOLEAN to_design;
				CFT_BOOLEAN to_project;
				CFT_BOOLEAN to_user;
			} auto_save_window_geometry;
			const struct window_geometry {
				const struct example_template {
					CFT_INTEGER x;
					CFT_INTEGER y;
					CFT_INTEGER width;
					CFT_INTEGER height;
				} example_template;
			} window_geometry;
		} dialogs;

		const struct lib_hid_common {
			const struct cli_history {
				CFT_STRING file;       /* Path to the history file (empty/unset means history is not preserved) */
				CFT_INTEGER slots;     /* Number of commands to store in the history */
			} cli_history;
		} lib_hid_common;

	} plugins;
} conf_dialogs_t;

#endif
