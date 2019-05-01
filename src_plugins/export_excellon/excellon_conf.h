#ifndef PCB_EXCELLON_CONF_H
#define PCB_EXCELLON_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_BOOLEAN plated_g85_slot;         /* use G85 (drill cycle instead of route) for plated slots */
			CFT_BOOLEAN unplated_g85_slot;       /* use G85 (drill cycle instead of route) for unplated slots */
		} export_excellon;
	} plugins;
} conf_excellon_t;

#endif
