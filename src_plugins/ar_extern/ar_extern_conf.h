#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			const struct route_rnd {
				RND_CFT_STRING exe;
				RND_CFT_BOOLEAN debug;
			} route_rnd;
		} ar_extern;
	} plugins;
} conf_ar_extern_t;

#endif
