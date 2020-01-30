#ifndef PCB_IMPORT_SCH2_CONF_H
#define PCB_IMPORT_SCH2_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			CFT_STRING netlist_cmd;         /* netlist generator program (command line) */
			CFT_STRING netlist_act;         /* netlist import action to execute */
			CFT_BOOLEAN verbose;            /* verbose logging of the import code */
		} import_sch;
	} plugins;
} conf_import_sch_t;

#endif
