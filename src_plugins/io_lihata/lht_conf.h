#ifndef PCB_IO_LIHATA_CONF_H
#define PCB_IO_LIHATA_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING aux_pcb_pattern;  /* [obsolete] file name pattern to use when generating the .pcb backup */
			RND_CFT_BOOLEAN omit_font;       /* [dangerous] do not save the font subtree in board files */
			RND_CFT_BOOLEAN omit_config;     /* [dangerous] do not save the config subtree in board files */
			RND_CFT_BOOLEAN omit_styles;     /* do not save the routing styles subtree in board files */
		} io_lihata;
	} plugins;
} conf_io_lihata_t;

#endif
