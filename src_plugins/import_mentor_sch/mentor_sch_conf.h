#ifndef PCB_MENTOR_SCH_CONF_H
#define PCB_MENTOR_SCH_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct import_mentor_sch {
			CFT_LIST map_search_paths; /* parts map file search paths */
		} import_mentor_sch;
	} plugins;
} conf_mentor_sch_t;

#endif
