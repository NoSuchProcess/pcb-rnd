#include <assert.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "conf.h"
#include "hid_cfg.h"
#include "misc.h"
#include "error.h"

#warning TODO: this should do settings_postproc too


typedef enum {
	POL_PREPEND,
	POL_APPEND,
	POL_OVERWRITE,
	POL_DISABLE
} policy_t;

static lht_doc_t *conf_root[CFR_max];
static htsp_t *conf_fields = NULL;

/*static lht_doc_t *conf_plugin;*/


int conf_load_as(conf_role_t role, const char *fn)
{
	lht_doc_t *d;
	if (conf_root[role] != NULL)
		lht_dom_uninit(conf_root[role]);
	d = hid_cfg_load_lht(fn);
	if (d->root->type != LHT_LIST) {
		hid_cfg_error(d->root, "Config root must be a list");
		conf_root[role] = NULL;
		return -1;
	}
	conf_root[role] = d;
	return 0;
}

static const char *get_hash_text(lht_node_t *parent, const char *name, lht_node_t **nout)
{
	lht_node_t *n;

	if ((parent == NULL) || (parent->type != LHT_HASH)) {
		if (nout != NULL)
			*nout = NULL;
		return NULL;
	}

	n = lht_dom_hash_get(parent, name);
	if (nout != NULL)
		*nout = n;

	if (n->type != LHT_TEXT)
		return NULL;

	return n->data.text.value;
}

static const int get_hash_int(long *out, lht_node_t *parent, const char *name)
{
	lht_node_t *n;
	const char *s;
	char *end;
	long l;

	s = get_hash_text(parent, name, &n);
	if (s == NULL)
		return -1;

	l = strtol(s, &end, 10);
	if (*end != '\0') {
		hid_cfg_error(parent, "%s should be an integer", s);
		return -1;
	}
	if (out != NULL)
		*out = l;
	return 0;
}

static const int get_hash_policy(policy_t *out, lht_node_t *parent, const char *name)
{
	lht_node_t *n;
	const char *s;
	char *end;
	policy_t p;

	s = get_hash_text(parent, name, &n);
	if (s == NULL)
		return -1;

	if (strcasecmp(s, "overwrite") == 0)
		p = POL_OVERWRITE;
	else if (strcasecmp(s, "prepend") == 0)
		p = POL_PREPEND;
	else if (strcasecmp(s, "append") == 0)
		p = POL_APPEND;
	else if (strcasecmp(s, "disable") == 0)
		p = POL_DISABLE;
	else {
		hid_cfg_error(parent, "invalid policy %s", s);
		return -1;
	}

	if (out != NULL)
		*out = p;
	return 0;
}

int conf_parse_text(confitem_t *dst, conf_native_type_t type, const char *text, lht_node_t *err_node)
{
	char *strue[]  = {"true",  "yes",  "on",   "1", NULL};
	char *sfalse[] = {"false", "no",   "off",  "0", NULL};
	char **s, *end;
	long l;
	int base = 10;
	double d;
	const Unit *u;

	switch(type) {
		case CFN_STRING:
			*dst->string = text;
			break;
		case CFN_BOOLEAN:
			for(s = strue; *s != NULL; s++)
				if (strcasecmp(*s, text) == 0) {
					*dst->boolean = 1;
					return 0;
				}
			for(s = sfalse; *s != NULL; s++)
				if (strcasecmp(*s, text) == 0) {
					*dst->boolean = 0;
					return 0;
				}
			hid_cfg_error(err_node, "Invalid boolean value: %s\n", text);
			return -1;
		case CFN_INTEGER:
			if ((text[0] == '0') && (text[1] == 'x')) {
				text += 2;
				base = 16;
			}
			l = strtol(text, &end, base);
			if (*end == '\0') {
				*dst->integer = l;
				return 0;
			}
			hid_cfg_error(err_node, "Invalid integer value: %s\n", text);
			return -1;
		case CFN_REAL:
			d = strtod(text, &end);
			if (*end == '\0') {
				*dst->real = d;
				return 0;
			}
			hid_cfg_error(err_node, "Invalid numeric value: %s\n", text);
			return -1;
		case CFN_COORD:
#warning TODO: write a new version of GetValue where absolute is optional and error is properly returned
			*dst->coord = GetValue(text, NULL, NULL);
			break;
		case CFN_UNIT:
			u = get_unit_struct(text);
			if (u == NULL)
				hid_cfg_error(err_node, "Invalid unit: %s\n", text);
			else
				*dst->unit = u;
			break;
		case CFN_COLOR:
#warning TODO: perhaps make some tests about validity?
			*dst->color = text;
			break;
		default:
			/* unknown field type registered in the fields hash: internal error */
			return -1;
	}
	return 0;
}

int conf_merge_patch_text(conf_native_t *dest, lht_node_t *src, int prio, policy_t pol)
{
	if ((pol == POL_DISABLE) || (dest->prop[0].prio > prio))
		return 0;

	if (conf_parse_text(&dest->val, dest->type, src->data.text.value, src) == 0) {
		dest->prop[0].prio = prio;
		dest->prop[0].src  = src;
		dest->used         = 1;
	}
	return 0;
}

/* merge main subtree of a patch */
int conf_merge_patch_recurse(lht_node_t *sect, int default_prio, policy_t default_policy, const char *path_prefix)
{
	lht_dom_iterator_t it;
	lht_node_t *n, *h;
	char path[256], *pathe;
	char name[256], *namee;
	int nl, ppl = strlen(path_prefix), res = 0;
	conf_native_t *target;

#warning TODO: use gds with static length

	memcpy(path, path_prefix, ppl);
	path[ppl] = '/';
	pathe = path + ppl + 1;

	for(n = lht_dom_first(&it, sect); n != NULL; n = lht_dom_next(&it)) {
		nl = strlen(n->name);
		memcpy(pathe, n->name, nl);
		namee = pathe+nl;
		*namee = '\0';
		target = conf_get_field(path);

		if (target == NULL) {
			hid_cfg_error(n, "ignoring unrecognized field: %s", path);
			continue;
		}

		switch(n->type) {
			case LHT_TEXT:
				conf_merge_patch_text(target, n, default_prio, default_policy);
				break;
			case LHT_HASH:
				res |= conf_merge_patch_recurse(n, default_prio, default_policy, path);
		}
	}
	return res;
}

int conf_merge_patch(lht_node_t *root)
{
	long gprio = 0;
	policy_t gpolicy = POL_OVERWRITE;
	const char *ps;
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (root->type != LHT_HASH) {
		hid_cfg_error(root, "patch root should be a hash");
		return -1;
	}

	/* get global settings */
	get_hash_int(&gprio, root, "priority");
	get_hash_policy(&gpolicy, root, "policy");

	/* iterate over all hashes and insert them recursively */
	for(n = lht_dom_first(&it, root); n != NULL; n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			conf_merge_patch_recurse(n, gprio, gpolicy, "/");

	return 0;
}

int conf_merge_all()
{
	int n, ret = 0;
	for(n = 0; n < CFR_max; n++) {
		lht_node_t *r;
		if (conf_root[n] == NULL)
			continue;
		for(r = conf_root[n]->root->data.list.first; r != NULL; r = r->next)
			if (conf_merge_patch(r) != 0)
				ret = -1;
	}
	return ret;
}

void conf_update(void)
{
	conf_load_as(CFR_SYSTEM, "./pcb-conf.lht");
	conf_merge_all();
}

static int keyeq(char *a, char *b)
{
	return !strcmp(a, b);
}

void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path)
{
	conf_native_t *node;

	if (conf_fields == NULL) {
		conf_fields = htsp_alloc(strhash, keyeq);
		assert(conf_fields != NULL);
	}
	assert(array_size >= 1);

	assert(htsp_get(conf_fields, (char *)path) == NULL);

	node = malloc(sizeof(conf_native_t));
	node->description = "n/a";
	node->array_size  = array_size;
	node->type        = type;
	node->val.any     = value;
	node->prop        = calloc(sizeof(confprop_t), array_size);
	node->used        = 0;
	htsp_set(conf_fields, path, node);
}

conf_native_t *conf_get_field(const char *path)
{
	return htsp_get(conf_fields, path);
}
