#ifndef PCB_AR_EXTERN_CONF_H
#define PCB_AR_EXTERN_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			const struct route_rnd {
				RND_CFT_STRING exe;
				RND_CFT_BOOLEAN debug;
			} route_rnd;
			const struct freerouting_cli {
				RND_CFT_STRING exe;
				RND_CFT_BOOLEAN debug;
			} freerouting_cli;
			const struct freerouting_net {
				RND_CFT_STRING exe;
				RND_CFT_BOOLEAN debug;
			} freerouting_net;
		} ar_extern;
	} plugins;
} conf_ar_extern_t;

#endif
