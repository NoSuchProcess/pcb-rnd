#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include "../src/conf.h"

typedef struct {
	const struct utils {
		const struct gsch2pcb_rnd {
			CFT_BOOLEAN remove_unfound_elements; /*  = TRUE */
			CFT_BOOLEAN quiet_mode; /*  = FALSE */
			CFT_INTEGER verbose;
			CFT_BOOLEAN preserve;
			CFT_BOOLEAN fix_elements;
			CFT_STRING sch_basename;
			CFT_STRING default_pcb;            /* override default pcb with a given file */
			CFT_STRING empty_footprint_name;
		} gsch2pcb_rnd;
	} utils;
} conf_gsch2pcb_rnd_t;

extern conf_gsch2pcb_rnd_t conf_g2pr;
#endif
