#ifndef PCB_IMPORT_SCH_CONF_H
#define PCB_IMPORT_SCH_CONF_H

#include "globalconst.h"
#include "conf.h"

typedef struct {
	const struct plugins {
		const struct import_sch {
			CFT_STRING gnetlist_program;       /* gnetlist program name */
			CFT_STRING make_program;           /* make program name */
		} import_sch;
	} plugins;
} conf_import_sch_t;

#endif
