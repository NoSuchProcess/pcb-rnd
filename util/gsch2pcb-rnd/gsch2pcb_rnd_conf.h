#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct utils {
		const struct gsch2pcb_rnd {
			RND_CFT_BOOLEAN remove_unfound_elements; /*  = TRUE */
			RND_CFT_BOOLEAN quiet_mode; /*  = FALSE */
			RND_CFT_INTEGER verbose;
			RND_CFT_BOOLEAN preserve;
			RND_CFT_BOOLEAN fix_elements;
			RND_CFT_STRING sch_basename;
			RND_CFT_STRING default_pcb;            /* override default pcb with a given file */
			RND_CFT_STRING empty_footprint_name;
			RND_CFT_STRING method;
			RND_CFT_LIST schematics;               /* use these schematics as input */
		} gsch2pcb_rnd;
	} utils;
} conf_gsch2pcb_rnd_t;

extern conf_gsch2pcb_rnd_t conf_g2pr;
#endif
