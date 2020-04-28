#ifndef PCB_ORDER_PCBWAY_CONF_H
#define PCB_ORDER_PCBWAY_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING api_key;
			RND_CFT_BOOLEAN verbose;             /* print log messages about the web traffic the plugin does */
			RND_CFT_INTEGER cache_update_sec;    /* re-download cached vendor files if they are older than the value specified here, in sec */
		} order_pcbway;
	} plugins;
} conf_order_pcbway_t;

#endif
