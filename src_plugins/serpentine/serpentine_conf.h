#ifndef PCB_SERPENTINE_CONF_H
#define PCB_SERPENTINE_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct serpentine {
			CFT_REAL pitch;         /* @usage this value multiplied by the line width gives the serpentine pitch */
		} serpentine;
	} plugins;
} conf_serpentine_t;

extern conf_serpentine_t conf_serpentine;

#endif
