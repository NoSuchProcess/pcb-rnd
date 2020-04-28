#ifndef PCB_FP_WGET_CONF_H
#define PCB_FP_WGET_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_BOOLEAN auto_update_gedasymbols;       /* update the index of gedasymbols on startup automatically */
			RND_CFT_BOOLEAN auto_update_edakrill;          /* update the index of edakrill on startup automatically */
			RND_CFT_STRING cache_dir;                      /* where to build the cache */
		} fp_wget;
	} plugins;
} conf_fp_wget_t;

extern conf_fp_wget_t conf_fp_wget;

#endif
