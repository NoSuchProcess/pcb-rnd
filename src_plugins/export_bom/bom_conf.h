#ifndef PCB_BOM_CONF_H
#define PCB_BOM_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_LIST templates;
		} export_bom;
	} plugins;
} conf_bom_t;

#endif
