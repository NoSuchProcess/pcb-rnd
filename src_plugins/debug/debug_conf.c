#include "conf.h"

void conf_dump(FILE *f, const char *prefix, int verbose, const char *match_prefix)
{
	htsp_entry_t *e;
	int pl;

	if (match_prefix != NULL)
		pl = strlen(match_prefix);

	for (e = htsp_first(conf_fields); e; e = htsp_next(conf_fields, e)) {
		conf_native_t *node = (conf_native_t *)e->value;
		if (match_prefix != NULL) {
			if (strncmp(node->hash_path, match_prefix, pl) != 0)
				continue;
		}
		conf_print_native((conf_pfn)pcb_fprintf, f, prefix, verbose, node);
	}
}
