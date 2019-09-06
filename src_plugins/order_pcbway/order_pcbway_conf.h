#ifndef PCB_ORDER_PCBWAY_CONF_H
#define PCB_ORDER_PCBWAY_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_STRING api_key;
		} order_pcbway;
	} plugins;
} conf_order_pcbway_t;

#endif
