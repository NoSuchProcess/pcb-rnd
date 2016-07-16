#include "conf.h"

static int needs_braces(const char *s)
{
	for(; *s != '\0'; s++)
		if (!isalnum(*s) && (*s != '_') && (*s != '-') && (*s != '+') && (*s != '/') && (*s != ':') && (*s != '.') && (*s != ','))
			return 1;
	return 0;
}

#define print_str_or_null(pfn, ctx, verbose, chk, out) \
	do { \
			if (chk == NULL) { \
				if (verbose) \
					ret += pfn(ctx, "<NULL>");\
			} \
			else {\
				if (needs_braces(out)) \
					ret += pfn(ctx, "{%s}", out); \
				else \
					ret += pfn(ctx, "%s", out); \
			} \
	} while(0)

typedef int (*conf_pfn)(void *ctx, const char *fmt, ...);

/* Prints the value of a node in a form that is suitable for lihata. Returns
   the sum of conf_pfn call return values - this is usually the number of
   bytes printed. */
static int conf_print_native_field(conf_pfn pfn, void *ctx, int verbose, confitem_t *val, conf_native_type_t type, confprop_t *prop, int idx)
{
	int ret = 0;
	switch(type) {
		case CFN_STRING:  print_str_or_null(fprintf, ctx, verbose, val->string[idx], val->string[idx]); break;
		case CFN_BOOLEAN: ret += pfn(ctx, "%d", val->boolean[idx]); break;
		case CFN_INTEGER: ret += pfn(ctx, "%ld", val->integer[idx]); break;
		case CFN_REAL:    ret += pfn(ctx, "%f", val->real[idx]); break;
		case CFN_COORD:   ret += pfn(ctx, "%$mS", val->coord[idx]); break;
		case CFN_UNIT:    print_str_or_null(fprintf, ctx, verbose, val->unit[idx], val->unit[idx]->suffix); break;
		case CFN_COLOR:   print_str_or_null(fprintf, ctx, verbose, val->color[idx], val->color[idx]); break;
		case CFN_INCREMENTS:
			{
				Increments *i = &val->increments[idx];
				ret += pfn(ctx, "{ grid=%$mS/%$mS/%$mS size=%$mS/%$mS/%$mS line=%$mS/%$mS/%$mS clear=%$mS/%$mS/%$mS}",
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
					ret += pfn(ctx, "{");
					for(n = conflist_first(val->list); n != NULL; n = conflist_next(n)) {
						conf_print_native_field(pfn, ctx, verbose, &n->val, n->type, &n->prop, 0);
						ret += pfn(ctx, ";");
					}
					ret += pfn(ctx, "}");
				}
				else {
					if (verbose)
						ret += pfn(ctx, "<empty list>");
					else
						ret += pfn(ctx, "{}");
				}
			}
			break;
	}
	if (verbose) {
		ret += pfn(ctx, " <<prio=%d", prop[idx].prio);
		if (prop[idx].src != NULL) {
			ret += pfn(ctx, " from=%s:%d", prop[idx].src->file_name, prop[idx].src->line);
		}
		ret += pfn(ctx, ">>");
	}
	return ret;
}

static int conf_print_native(conf_pfn pfn, void *ctx, const char * prefix, int verbose, conf_native_t *node)
{
	int ret = 0;
	if (node->array_size > 1) {
		int n;
		for(n = 0; n < node->used; n++) {
			if (verbose)
				ret += pfn(ctx, "%s I %s[%d] = ", prefix, node->hash_path, n);
			ret += conf_print_native_field(pfn, ctx, verbose, &node->val, node->type, node->prop, n);
			if (verbose)
				ret += pfn(ctx, " conf_rev=%d\n", node->conf_rev);
		}
		if ((node->used == 0) && (verbose))
			ret += pfn(ctx, "%s I %s[] = <empty>\n", prefix, node->hash_path);
	}
	else {
		if (verbose)
			ret += pfn(ctx, "%s I %s = ", prefix, node->hash_path);
		ret += conf_print_native_field(pfn, ctx, verbose, &node->val, node->type, node->prop, 0);
		if (verbose)
			ret += pfn(ctx, " conf_rev=%d\n", node->conf_rev);
	}
	return ret;
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
		conf_print_native((conf_pfn)pcb_fprintf, f, prefix, verbose, node);
	}
}
