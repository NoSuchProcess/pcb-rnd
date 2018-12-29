#ifndef PCB_DIALOGS_CONF_H
#define PCB_DIALOGS_CONF_H

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
				const struct top {
					CFT_INTEGER x;
					CFT_INTEGER y;
					CFT_INTEGER width;
					CFT_INTEGER height;
				} top;
			} window_geometry;
		} dialogs;
	} plugins;
} conf_dialogs_t;

#endif
