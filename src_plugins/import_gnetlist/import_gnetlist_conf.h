#ifndef PCB_IMPORT_GNETLIST_CONF_H
#define PCB_IMPORT_GNETLIST_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING gnetlist_program;       /* gnetlist program name */
		} import_gnetlist;
	} plugins;
} conf_import_gnetlist_t;

#endif
