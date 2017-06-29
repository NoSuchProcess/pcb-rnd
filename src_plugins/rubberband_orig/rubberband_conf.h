#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct rubberband_orig {
			CFT_BOOLEAN enable_rubberband_arcs;   /* TODO: Enable to allow attached arcs to rubberband. */
		} rubberband_orig;
	} plugins;
} conf_rubberband_orig_t;

#endif
