#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct vendor {
			CFT_BOOLEAN enable;         /* Enable vendor mapping */
		} vendor;
	} plugins;
} conf_vendor_t;

#endif
