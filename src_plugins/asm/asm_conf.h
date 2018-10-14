#ifndef PCB_ASM_CONF_H
#define PCB_ASM_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct assm {
			CFT_STRING group_template;        /* asm template that determines grouping (parts resulting in the same string will be puit in the same group) */
			CFT_STRING sort_template;         /* asm template that determines order of groups and parts within groups */
		} assm;
	} plugins;
} conf_asm_t;

#endif
