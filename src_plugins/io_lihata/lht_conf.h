#ifndef PCB_IO_LIHATA_CONF_H
#define PCB_IO_LIHATA_CONF_H

#include "conf.h"

typedef struct {
	const struct plugins {
		const struct io_lihata {
			CFT_STRING aux_pcb_pattern;  /* file name pattern to use when generating the .pcb backup */
		} io_lihata;
	} plugins;
} conf_io_lihata_t;

#endif
