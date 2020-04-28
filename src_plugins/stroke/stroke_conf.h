#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_LIST gestures;
			RND_CFT_BOOLEAN warn4unknown; /* Warn for unknown sequences */
		} stroke;
	} plugins;
} conf_stroke_t;

#endif
