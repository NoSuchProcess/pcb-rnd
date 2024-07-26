#ifndef PCB_IO_EASYEDA_CONF_H
#define PCB_IO_EASYEDA_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	const struct {
		const struct {
			RND_CFT_STRING zip_list_cmd;           /* shell command that lists the content of a zip file to stdout; %s is replaced by path to the file; noise (headers and file sizes) is accepted as long as file names are not cut by newlines */
			RND_CFT_STRING zip_extract_cmd;        /* shell command that extracts a zip file in current working directory; %s is replaced by path to the file */
			RND_CFT_BOOLEAN load_color_copper;     /* load color of copper layers; when disabled pick random colors */
			RND_CFT_BOOLEAN load_color_noncopper;  /* load color of non-copper layers; when disabled use pcb-rnd standard layer colors */
			RND_CFT_REAL line_approx_seg_len;      /* path approximation line length in EasyEDA units (which is 10mil, so a value of 3 here means 30mil) */
			const struct {
				RND_CFT_BOOLEAN dump_dom;            /* print the DOM after expanding strings */
				RND_CFT_BOOLEAN unzip_static;        /* always unzip to /tmp/easypro and don't remove it - don't use in production (unsafe temp file creation, unzip blocking to ask for overwrite on console) */
			} debug;
		} io_easyeda;
	} plugins;
} conf_io_easyeda_t;

extern conf_io_easyeda_t conf_io_easyeda;

#endif
