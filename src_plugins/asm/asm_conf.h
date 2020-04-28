#ifndef PCB_ASM_CONF_H
#define PCB_ASM_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING group_template;        /* asm template that determines grouping (parts resulting in the same string will be put in the same group) */
			RND_CFT_STRING sort_template;         /* asm template that determines order of groups and parts within groups */
		} asm1;
	} plugins;
} conf_asm_t;

#endif
