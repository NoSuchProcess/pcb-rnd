#ifndef PCB_MINCUT_CONF_H
#define PCB_MINCUT_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_BOOLEAN enable;         /* Enable calculating mincut on shorts (rats_mincut.c) when non-zero */
		} mincut;
	} plugins;
} conf_mincut_t;

#endif
