#ifndef PCB_IO_PADS_CONF_H
#define PCB_IO_PADS_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN load_teardrops;    /* Enable loading teardrops - creates many extended objects */
			RND_CFT_BOOLEAN load_polygons;     /* Enable loading polygon ''pours'' - major slowdown */
			RND_CFT_BOOLEAN save_trace_indep;  /* Save traces as independent objects instead of routed signals */
			RND_CFT_BOOLEAN save_abuse_copcls; /* Save non-copper polygons as COPCLS (which normally should be only copper); when off, non-copper polygons are not saved */
		} io_pads;
	} plugins;
} conf_io_pads_t;

#endif
