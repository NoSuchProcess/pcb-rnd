#ifndef PCB_ORDER_CONF_H
#define PCB_ORDER_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_STRING cache;        /* path to the cache directory where order related vendor data are saved */
		} order;
	} plugins;
} conf_order_t;

#endif
