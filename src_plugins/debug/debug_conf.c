#include "src/conf.h"

static void conf_dump_(FILE *f, const char *prefix, int verbose, confitem_t *val, conf_native_type_t type, confprop_t *prop, int idx)
{
	switch(type) {
		case CFN_STRING:  fprintf(f, "%s", val->string[idx] == NULL ? "<NULL>" : val->string[idx]); break;
		case CFN_BOOLEAN: fprintf(f, "%d", val->boolean[idx]); break;
		case CFN_INTEGER: fprintf(f, "%ld", val->integer[idx]); break;
		case CFN_REAL:    fprintf(f, "%f", val->real[idx]); break;
		case CFN_COORD:   pcb_fprintf(f, "%$mS", val->coord[idx]); break;
		case CFN_UNIT:    pcb_fprintf(f, "%s", val->unit[idx] == NULL ? "<NULL>" : val->unit[idx]->suffix); break;
		case CFN_COLOR:   fprintf(f, "%s", val->color[idx] == NULL ? "<NULL>" : val->color[idx]); break;
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
				else
					fprintf(f, "<empty list>");
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
}

void conf_dump(FILE *f, const char *prefix, int verbose)
{
	htsp_entry_t *e;
	for (e = htsp_first(conf_fields); e; e = htsp_next(conf_fields, e)) {
		conf_native_t *node = (conf_native_t *)e->value;
		if (node->array_size > 1) {
			int n;
			for(n = 0; n < node->used; n++) {
				fprintf(f, "%s I %s[%d] = ", prefix, e->key, n);
				conf_dump_(f, prefix, verbose, &node->val, node->type, node->prop, n);
				fprintf(f, "\n");
			}
			if (node->used == 0)
				fprintf(f, "%s I %s[] = <empty>\n", prefix, e->key);
		}
		else {
			fprintf(f, "%s I %s = ", prefix, e->key);
			conf_dump_(f, prefix, verbose, &node->val, node->type, node->prop, 0);
			fprintf(f, "\n");
		}
	}
}
