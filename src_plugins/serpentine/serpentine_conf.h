#ifndef PCB_SERPENTINE_CONF_H
#define PCB_SERPENTINE_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct plugins {
		const struct serpentine {
			RND_CFT_REAL pitch;						/* This value multiplied by the line width gives the serpentine pitch */
			RND_CFT_BOOLEAN show_length;	/* If true, show the length gained by adding the serpentine to an existing line */
		} serpentine;
	} plugins;
} conf_serpentine_t;

extern conf_serpentine_t conf_serpentine;

#endif
