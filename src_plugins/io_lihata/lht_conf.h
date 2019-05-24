#ifndef PCB_IO_LIHATA_CONF_H
#define PCB_IO_LIHATA_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_STRING aux_pcb_pattern;  /* [obsolete] file name pattern to use when generating the .pcb backup */
			CFT_BOOLEAN omit_font;       /* do not save the font subtree in board files */
			CFT_BOOLEAN omit_config;     /* do not save the config subtree in board files */
		} io_lihata;
	} plugins;
} conf_io_lihata_t;

#endif
