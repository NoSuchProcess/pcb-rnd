#ifndef PCB_DRC_QUERY_CONF_H
#define PCB_DRC_QUERY_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN disable;     /* disable the whole engine */
			RND_CFT_HLIST definitions;   /* DRC constant definitions */
			RND_CFT_HLIST rules;         /* inline rules */
		} drc_query;
	} plugins;
} conf_drc_query_t;

#endif
