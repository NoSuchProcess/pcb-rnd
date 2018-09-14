#ifndef PCB_GERBER_CONF_H
#define PCB_GERBER_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct export_gerber {
			CFT_BOOLEAN plated_g85_slot;         /* use G85 (drill cycle instead of route) for plated slots */
			CFT_BOOLEAN unplated_g85_slot;       /* use G85 (drill cycle instead of route) for unplated slots */
		} export_gerber;
	} plugins;
} conf_gerber_t;

#endif
