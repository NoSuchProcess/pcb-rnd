#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct cam {
			CFT_LIST jobs;         /* named cam scripts */
		} cam;
	} plugins;
} conf_cam_t;

#endif
