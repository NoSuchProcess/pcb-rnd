#include "conf.h"

#define print_str_or_null(pfn, ctx, verbose, chk, out) \
	do { \
			if (chk == NULL) { \
				if (verbose) \
					pfn(ctx, "<NULL>");\
				return 0; \
			} \
			pfn(ctx, "%s", out); \
	} while(0)

/* Prints the value of a node in a form that is suitable for lihata. Returns 1
   if field is printed, 0 if not (e.g. the field was empty). */
static int conf_dump_(FILE *f, const char *prefix, int verbose, confitem_t *val, conf_native_type_t type, confprop_t *prop, int idx)
{
	switch(type) {
		case CFN_STRING:  print_str_or_null(fprintf, f, verbose, val->string[idx], val->string[idx]); break;
		case CFN_BOOLEAN: fprintf(f, "%d", val->boolean[idx]); break;
		case CFN_INTEGER: fprintf(f, "%ld", val->integer[idx]); break;
		case CFN_REAL:    fprintf(f, "%f", val->real[idx]); break;
		case CFN_COORD:   pcb_fprintf(f, "%$mS", val->coord[idx]); break;
		case CFN_UNIT:    print_str_or_null(fprintf, f, verbose, val->unit[idx], val->unit[idx]->suffix); break;
		case CFN_COLOR:   print_str_or_null(fprintf, f, verbose, val->color[idx], val->color[idx]); break;
		case CFN_INCREMENTS:
			{
				Increments *i = &val->increments[idx];
				pcb_fprintf(f, "{ grid=%$mS/%$mS/%$mS size=%$mS/%$mS/%$mS line=%$mS/%$mS/%$mS clear=%$mS/%$mS/%$mS}",
				i->grid, i->grid_min, i->grid_max,
				i->size, i->size_min, i->size_max,
				i->line, i->line_min, i->line_max,
				i->clear, i->clear_min, i->clear_max);
			}
			break;
		case CFN_LIST:
			{
				conf_listitem_t *n;
				if (conflist_length(val->list) > 0) {
					fprintf(f, "{");
					for(n = conflist_first(val->list); n != NULL; n = conflist_next(n)) {
						fprintf(f, "{");
						conf_dump_(f, prefix, verbose, &n->val, n->type, &n->prop, 0);
						fprintf(f, "};");
					}
					fprintf(f, "}");
				}
				else {
					if (verbose)
						fprintf(f, "<empty list>");
					else
						fprintf(f, "{}");
					return 0;
				}
			}
			break;
	}
	if (verbose) {
		fprintf(f, " <<prio=%d", prop[idx].prio);
		if (prop[idx].src != NULL) {
			fprintf(f, " from=%s:%d", prop[idx].src->file_name, prop[idx].src->line);
		}
		fprintf(f, ">>");
	}
	return 1;
}

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
		if (node->array_size > 1) {
			int n;
			for(n = 0; n < node->used; n++) {
				fprintf(f, "%s I %s[%d] = ", prefix, e->key, n);
				conf_dump_(f, prefix, verbose, &node->val, node->type, node->prop, n);
				fprintf(f, " conf_rev=%d\n", node->conf_rev);
			}
			if (node->used == 0)
				fprintf(f, "%s I %s[] = <empty>\n", prefix, e->key);
		}
		else {
			fprintf(f, "%s I %s = ", prefix, e->key);
			conf_dump_(f, prefix, verbose, &node->val, node->type, node->prop, 0);
			fprintf(f, " conf_rev=%d\n", node->conf_rev);
		}
	}
}
