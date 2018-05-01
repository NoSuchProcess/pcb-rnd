#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct stroke {
			CFT_LIST gestures;
		} stroke;
	} plugins;
} conf_stroke_t;

#endif
