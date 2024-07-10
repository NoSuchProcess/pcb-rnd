#ifndef PCB_IO_EASYEDA_CONF_H
#define PCB_IO_EASYEDA_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING zip_list_cmd;           /* shell command that lists the content of a zip file to stdout; %s is replaced by path to the file; noise (headers and file sizes) is accepted as long as file names are not cut by newlines */
			RND_CFT_STRING zip_extract_cmd;        /* shell command that extracts a zip file in current working directory; %s is replaced by path to the file */
			const struct {
				RND_CFT_BOOLEAN dump_dom;            /* print the DOM after expanding strings */
			} debug;
		} io_easyeda;
	} plugins;
} conf_io_easyeda_t;

extern conf_io_easyeda_t conf_io_easyeda;

#endif
