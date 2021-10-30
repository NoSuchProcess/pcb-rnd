#ifndef PCB_ADIALOGS_CONF_H
#define PCB_ADIALOGS_CONF_H

#include <librnd/core/conf.h>

/* pcb-rnd specific dialog config */

typedef struct {
	const struct {
		const struct {
			const struct {
				RND_CFT_INTEGER preview_refresh_timeout; /* how much time to wait (in milliseconds) after the last edit in filter before refreshing the preview, e.g. for parametric footprints */
				RND_CFT_BOOLEAN preview_vis_cpr;         /* whether copper layers are visible in preview by default */
				RND_CFT_BOOLEAN preview_vis_slk;         /* whether silk layers are visible in preview by default */
				RND_CFT_BOOLEAN preview_vis_mnp;         /* whether mask and paste layers are visible in preview by default */
				RND_CFT_BOOLEAN preview_vis_doc;         /* whether doc layers are visible in preview by default */
			} library;
		} dialogs;
	} plugins;
} conf_adialogs_t;

extern conf_adialogs_t adialogs_conf;

#endif
