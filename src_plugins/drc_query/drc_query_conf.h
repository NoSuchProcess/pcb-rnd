#ifndef PCB_VENDOR_CONF_H
#define PCB_VENDOR_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			CFT_LIST rules;         /* inline rules */
		} drc_query;
	} plugins;
} conf_drc_query_t;

#endif
