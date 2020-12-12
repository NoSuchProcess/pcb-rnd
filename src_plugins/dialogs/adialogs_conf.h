#ifndef PCB_ADIALOGS_CONF_H
#define PCB_ADIALOGS_CONF_H

#include <librnd/core/conf.h>

/* pcb-rnd specific dialog config */

typedef struct {
	const struct {
		const struct {
			const struct {
				RND_CFT_INTEGER preview_refresh_timeout; /* how much time to wait (in milliseconds) after the last edit in filter before refreshing the preview, e.g. for parametric footprints */
			} library;
		} dialogs;
	} plugins;
} conf_adialogs_t;

extern conf_adialogs_t adialogs_conf;

#endif
