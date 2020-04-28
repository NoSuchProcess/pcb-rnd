#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN enable;         /* Enable vendor mapping */
		} vendor;
	} plugins;
} conf_vendor_t;

#endif
