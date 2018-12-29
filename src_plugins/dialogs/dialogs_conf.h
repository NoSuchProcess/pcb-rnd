#ifndef PCB_DIALOG_CONF_H
#define PCB_DIALOG_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct dialogs {
			const struct auto_save_window_geometry {
				CFT_BOOLEAN to_design;
				CFT_BOOLEAN to_project;
				CFT_BOOLEAN to_user;
			} auto_save_window_geometry;
			const struct window_geometry {
			} window_geometry;
		} dialogs;
	} plugins;
} conf_dialogs_t;

#endif
