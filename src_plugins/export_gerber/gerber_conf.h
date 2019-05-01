#ifndef PCB_GERBER_CONF_H
#define PCB_GERBER_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_BOOLEAN plated_g85_slot;         /* use G85 (drill cycle instead of route) for plated slots - only affects direct gerber export, DO NOT USE, check excellon's config instead */
			CFT_BOOLEAN unplated_g85_slot;       /* use G85 (drill cycle instead of route) for unplated slots - only affects direct gerber export, DO NOT USE, check excellon's config instead */
		} export_gerber;
	} plugins;
} conf_gerber_t;

#endif
