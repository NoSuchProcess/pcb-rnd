#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include "../src/conf.h"

typedef struct {
	const struct utils {
		const struct gsch2pcb_rnd {
			CFT_BOOLEAN remove_unfound_elements; /*  = TRUE */
			CFT_BOOLEAN quiet_mode; /*  = FALSE */
			CFT_BOOLEAN verbose;
			CFT_BOOLEAN preserve;
			CFT_BOOLEAN fix_elements;
			CFT_STRING element_search_path; /* = NULL; */
			CFT_STRING element_shell; /* = PCB_LIBRARY_SHELL; */
			CFT_STRING DefaultPcbFile; /* = PCB_DEFAULT_PCB_FILE; */
			CFT_STRING sch_basename;
		} gsch2pcb_rnd;
	} utils;
} conf_gsch2pcb_rnd_t;

extern conf_gsch2pcb_rnd_t conf_gsch2pcb_rnd;
#endif
