#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_LIST gestures;
			CFT_BOOLEAN warn4unknown; /* Warn for unknown sequences */
		} stroke;
	} plugins;
} conf_stroke_t;

#endif
