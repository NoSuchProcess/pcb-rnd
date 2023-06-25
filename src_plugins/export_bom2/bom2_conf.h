#ifndef PCB_BOM2_CONF_H
#define PCB_BOM2_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_LIST templates;
		} export_bom2;
	} plugins;
} conf_bom2_t;

#endif
