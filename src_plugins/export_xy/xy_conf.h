#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct xy {
			CFT_LIST templates;
		} xy;
	} plugins;
} conf_xy_t;

#endif
