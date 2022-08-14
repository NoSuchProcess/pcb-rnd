#ifndef PCB_IMPORT_SCH_CONF_H
#define PCB_IMPORT_SCH_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING import_fmt;          /* name of the input format */
			RND_CFT_LIST args;                  /* import_fmt arguments, typically file names */
			RND_CFT_BOOLEAN verbose;            /* verbose logging of the import code */
			RND_CFT_BOOLEAN design_relative;    /* generate design-relative paths using $(rc.path.design) when GUI-selecting input file names */

			/* obsolete: temporary compatibility with import_sch for the transition period */
			RND_CFT_STRING gnetlist_program;       /* DEPRECATED: gnetlist program name */
			RND_CFT_STRING make_program;           /* DEPRECATED: make program name */
		} import_sch;
	} plugins;
} conf_import_sch_t;

#endif
