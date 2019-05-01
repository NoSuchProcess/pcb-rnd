#ifndef PCB_DJOPT_CONF_H
#define PCB_DJOPT_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_BOOLEAN auto_only;         /* Operate on autorouted tracks only */
		} djopt;
	} plugins;
} conf_djopt_t;

#endif
