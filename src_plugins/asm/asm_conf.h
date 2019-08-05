#ifndef PCB_ASM_CONF_H
#define PCB_ASM_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_STRING group_template;        /* asm template that determines grouping (parts resulting in the same string will be put in the same group) */
			CFT_STRING sort_template;         /* asm template that determines order of groups and parts within groups */
		} asm1;
	} plugins;
} conf_asm_t;

#endif
