#ifndef PCB_DJOPT_CONF_H
#define PCB_DJOPT_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct djopt {
			CFT_BOOLEAN auto_only;         /* Operate on autorouted tracks only */
		} djopt;
	} plugins;
} conf_djopt_t;

#endif
