#ifndef PCB_SHOW_NETNAMES_CONF_H
#define PCB_SHOW_NETNAMES_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN enable;         /* Enable displaying netnames */
			RND_CFT_COORD zoom_level;       /* Display netnames only if current zoom coords_per_pixel is smaller than this value */
		} show_netnames;
	} plugins;
} conf_show_netnames_t;

#endif
