#ifndef PCB_SHOW_NETNAMES_CONF_H
#define PCB_SHOW_NETNAMES_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN enable;         /* Enable displaying netnames */
			RND_CFT_COORD zoom_level;       /* Display netnames only if current zoom coords_per_pixel is smaller than this value */
			RND_CFT_BOOLEAN omit_nonets;    /* do not print 'nonet' labels on the objects not belonging to any network */
			RND_CFT_LIST omit_netnames;     /* list of regular expressions; if a netname matches any of these, it is not shown */
		} show_netnames;
	} plugins;
} conf_show_netnames_t;

#endif
