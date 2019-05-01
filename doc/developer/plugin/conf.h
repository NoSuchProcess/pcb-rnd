!!! replace FOOBAR and foobar with your plugin name !!!

#ifndef PCB_FOOBAR_CONF_H
#define PCB_FOOBAR_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_BOOLEAN enable;         /* Enable the plugin */
			CFT_INTEGER snow;           /* intensity of snowing, 0..100 */
		} foobar;
	} plugins;
} conf_foobar_t;

#endif
