#ifndef PCB_DIAG_CONF_H
#define PCB_DIAG_CONF_H

#include "conf.h"

typedef struct {
	const struct {
		const struct {
			CFT_BOOLEAN auto_integrity;         /* Enable (expensive) automatic integrity check after each user action */
		} diag;
	} plugins;
} conf_diag_t;

extern conf_diag_t conf_diag;

/* Print all configuration items to f, prefixing each line with prefix
   If match_prefix is not NULL, print only items with matching path prefix */
void conf_dump(FILE *f, const char *prefix, int verbose, const char *match_prefix);

#endif
