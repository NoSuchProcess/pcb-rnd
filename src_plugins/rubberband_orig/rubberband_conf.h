#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN enable_rubberband_arcs;   /* TODO: Enable to allow attached arcs to rubberband. */
		} rubberband_orig;
	} plugins;
} conf_rubberband_orig_t;

#endif
