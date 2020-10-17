#ifndef PCB_FP_FS_CONF_H
#define PCB_FP_FS_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_LIST ignore_prefix; /* ignore file names starting with these prefixes */
			RND_CFT_LIST ignore_suffix; /* ignore file names ending with these suffixes */
			RND_CFT_LIST remove_regex;  /* regex's used to remove 'file extension', e.g. .fp or .lht to match footprint file name to footprint sch name */
		} fp_fs;
	} plugins;
} conf_fp_fs_t;

#endif
