#ifndef PCB_FP_WGET_CONF_H
#define PCB_FP_WGET_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct fp_wget {
			CFT_BOOLEAN auto_update_gedasymbols;       /* update the index of gedasymbols on startup automatically */
		} fp_wget;
	} plugins;
} conf_fp_wget_t;

#endif
