#ifndef PCB_IMPORT_SCH_CONF_H
#define PCB_IMPORT_SCH_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_STRING gnetlist_program;       /* gnetlist program name */
			CFT_STRING make_program;           /* make program name */
			CFT_BOOLEAN verbose;                  /* verbose logging of the import code */
		} import_sch;
	} plugins;
} conf_import_sch_t;

#endif
