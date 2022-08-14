#ifndef PCB_IMPORT_SCH_RND_CONF_H
#define PCB_IMPORT_SCH_RND_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING sch_rnd_program;       /* sch-rnd program name */
		} import_sch_rnd;
	} plugins;
} conf_import_sch_rnd_t;

#endif
