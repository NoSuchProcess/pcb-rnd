#ifndef PCB_DRAW_FAB_CONF_H
#define PCB_DRAW_FAB_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN omit_date;         /* do not draw date (useful for testing) */
		} draw_fab;
	} plugins;
} conf_draw_fab_t;

#endif
