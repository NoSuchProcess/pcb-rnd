#ifndef PCB_IO_EASYEDA_CONF_H
#define PCB_IO_EASYEDA_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN dummy;
		} io_easyeda;
	} plugins;
} conf_io_easyeda_t;

#endif
