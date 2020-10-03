#ifndef PCB_CH_EDITPOINT_CONF_H
#define PCB_CH_EDITPOINT_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN enable;        /* indicate edit points when the cursor is over an object */
		} ch_editpoint;
	} plugins;
} conf_ch_editpoint_t;

#endif
