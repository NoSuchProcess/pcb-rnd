#ifndef PCB_IMPORT_SCH_CONF_H
#define PCB_IMPORT_SCH_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			CFT_STRING import_fmt;          /* name of the input format */
			CFT_LIST args;                  /* import_fmt arguments, typically file names */
			CFT_BOOLEAN verbose;            /* verbose logging of the import code */

			/* obsolete: temporary compatibility with import_sch for the transition period */
			CFT_STRING gnetlist_program;       /* DEPRECATED: gnetlist program name */
			CFT_STRING make_program;           /* DEPRECATED: make program name */
		} import_sch;
	} plugins;
} conf_import_sch_t;

#endif
