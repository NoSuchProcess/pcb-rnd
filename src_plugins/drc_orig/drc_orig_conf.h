#ifndef PCB_DRC_ORIG_CONF_H
#define PCB_DRC_ORIG_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN disable;        /* disable the whole engine */
		} drc_orig;
	} plugins;
} conf_drc_orig_t;

#endif
