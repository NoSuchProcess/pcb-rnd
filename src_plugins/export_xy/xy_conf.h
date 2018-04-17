#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct export_xy {
			CFT_LIST templates;
		} export_xy;
	} plugins;
} conf_xy_t;

#endif
