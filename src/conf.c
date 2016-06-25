/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <assert.h>
#include <genht/hash.h>
#include <liblihata/tree.h>
#include "conf.h"
#include "conf_core.h"
#include "hid_cfg.h"
#include "hid_init.h"
#include "misc_util.h"
#include "error.h"

/* conf list node's name */
const char *conf_list_name = "pcb-rnd-conf-v1";

lht_doc_t *conf_root[CFR_max];
int conf_root_lock[CFR_max];
htsp_t *conf_fields = NULL;
static const int conf_default_prio[] = {
/*	CFR_INTERNAL */   100,
/*	CFR_SYSTEM */     200,
/*	CFR_DEFAULTPCB */ 300,
/*	CFR_USER */       400,
/*	CFR_ENV */        500,
/*	CFR_PROJECT */    600,
/*	CFR_DESIGN */     700,
/*	CFR_CLI */        800
};

extern const char *conf_internal;

/*static lht_doc_t *conf_plugin;*/

static lht_node_t *conf_lht_get_confroot(lht_node_t *cwd)
{
	if (cwd == NULL)
		return NULL;

	/* if it's a list with a matching name, we found it */
	if ((cwd->type == LHT_LIST) && (strcmp(cwd->name, conf_list_name) == 0))
		return cwd;

	/* else it may be the parent-hash of the list */
	if (cwd->type != LHT_HASH)
		return NULL;

	return lht_dom_hash_get(cwd, conf_list_name);
}


int conf_load_as(conf_role_t role, const char *fn, int fn_is_text)
{
	lht_doc_t *d;
	if (conf_root_lock[role])
		return -1;
	if (conf_root[role] != NULL)
		lht_dom_uninit(conf_root[role]);
	if (fn_is_text)
		d = hid_cfg_load_str(fn);
	else
		d = hid_cfg_load_lht(fn);
	if (d == NULL) {
		FILE *f;
		f = fopen(fn, "r");
		if (f != NULL) { /* warn only if the file is there - missing file is normal */
			Message("error: failed to load lht config: %s\n", fn);
			fclose(f);
		}
		conf_root[role] = NULL;
		return -1;
	}
	if (d->root->type != LHT_LIST) {
		hid_cfg_error(d->root, "Config root must be a list\n");
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

	if ((n == NULL) || (n->type != LHT_TEXT))
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
		hid_cfg_error(parent, "%s should be an integer\n", s);
		return -1;
	}
	if (out != NULL)
		*out = l;
	return 0;
}

conf_policy_t conf_policy_parse(const char *s)
{
	if (strcasecmp(s, "overwrite") == 0)  return POL_OVERWRITE;
	if (strcasecmp(s, "prepend") == 0)    return  POL_PREPEND;
	if (strcasecmp(s, "append") == 0)     return  POL_APPEND;
	if (strcasecmp(s, "disable") == 0)    return  POL_DISABLE;
	return POL_invalid;
}

conf_role_t conf_role_parse(const char *s)
{
	if (strcasecmp(s, "internal") == 0)   return CFR_INTERNAL;
	if (strcasecmp(s, "system") == 0)     return CFR_SYSTEM;
	if (strcasecmp(s, "defaultpcb") == 0) return CFR_DEFAULTPCB;
	if (strcasecmp(s, "user") == 0)       return CFR_USER;
	if (strcasecmp(s, "project") == 0)    return CFR_PROJECT;
	if (strcasecmp(s, "design") == 0)     return CFR_DESIGN;
	if (strcasecmp(s, "cli") == 0)        return CFR_CLI;
	return CFR_invalid;
}


static const int get_hash_policy(conf_policy_t *out, lht_node_t *parent, const char *name)
{
	lht_node_t *n;
	const char *s;
	char *end;
	conf_policy_t p;

	s = get_hash_text(parent, name, &n);
	if (s == NULL)
		return -1;

	p = conf_policy_parse(s);
	if (p == POL_invalid) {
		hid_cfg_error(parent, "invalid policy %s\n", s);
		return -1;
	}

	if (out != NULL)
		*out = p;
	return 0;
}

static int conf_parse_increments(Increments *inc, lht_node_t *node)
{
	lht_node_t *val;
	
	if (node->type != LHT_HASH) {
		hid_cfg_error(node, "Increments need to be a hash\n");
		return -1;
	}

#define incload(field) \
	val = lht_dom_hash_get(node, #field); \
	if (val != NULL) {\
		if (val->type == LHT_TEXT) {\
			bool succ; \
			inc->field = GetValue(val->data.text.value, NULL, NULL, &succ); \
			if (!succ) \
				hid_cfg_error(node, "invalid numeric value in increment field " #field "\n", val); \
		} \
		else\
			hid_cfg_error(node, "increment field " #field " needs to be a text node\n", val); \
	}

	incload(grid);
	incload(grid_min);
	incload(grid_max);
	incload(size);
	incload(size_min);
	incload(size_max);
	incload(line);
	incload(line_min);
	incload(line_max);
	incload(clear);
	incload(clear_min);
	incload(clear_max);

#undef incload
}

int conf_parse_text(confitem_t *dst, int idx, conf_native_type_t type, const char *text, lht_node_t *err_node)
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
			dst->string[idx] = text;
			break;
		case CFN_BOOLEAN:
			for(s = strue; *s != NULL; s++)
				if (strcasecmp(*s, text) == 0) {
					dst->boolean[idx] = 1;
					return 0;
				}
			for(s = sfalse; *s != NULL; s++)
				if (strcasecmp(*s, text) == 0) {
					dst->boolean[idx] = 0;
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
				dst->integer[idx] = l;
				return 0;
			}
			hid_cfg_error(err_node, "Invalid integer value: %s\n", text);
			return -1;
		case CFN_REAL:
			d = strtod(text, &end);
			if (*end == '\0') {
				dst->real[idx] = d;
				return 0;
			}
			hid_cfg_error(err_node, "Invalid numeric value: %s\n", text);
			return -1;
		case CFN_COORD:
			{
				bool succ;
				dst->coord[idx] = GetValue(text, NULL, NULL, &succ);
				if (!succ)
					hid_cfg_error(err_node, "Invalid numeric value (coordinate): %s\n", text);
			}
			break;
		case CFN_UNIT:
			u = get_unit_struct(text);
			if (u == NULL)
				hid_cfg_error(err_node, "Invalid unit: %s\n", text);
			else
				dst->unit[idx] = u;
			break;
		case CFN_INCREMENTS:
			return conf_parse_increments(&(dst->increments[idx]), err_node);
		case CFN_COLOR:
			dst->color[idx] = text; /* let the HID check validity to support flexibility of the format */
			break;
		default:
			/* unknown field type registered in the fields hash: internal error */
			return -1;
	}
	return 0;
}

int conf_merge_patch_text(conf_native_t *dest, lht_node_t *src, int prio, conf_policy_t pol)
{
	if ((pol == POL_DISABLE) || (dest->prop[0].prio > prio))
		return 0;

	if (dest->random_flags.read_only) {
		hid_cfg_error(src, "WARNING: not going to overwrite read-only value %s from a config file\n", dest->hash_path);
		return 0;
	}

	if (conf_parse_text(&dest->val, 0, dest->type, src->data.text.value, src) == 0) {
		dest->prop[0].prio = prio;
		dest->prop[0].src  = src;
		dest->used         = 1;
		dest->conf_rev     = conf_rev;
	}
	return 0;
}

int conf_merge_patch_array(conf_native_t *dest, lht_node_t *src_lst, int prio, conf_policy_t pol)
{
	lht_node_t *s;
	int res, idx, didx;

	if (pol == POL_DISABLE)
		return 0;

	for(s = src_lst->data.list.first, idx = 0; s != NULL; s = s->next, idx++) {
		if (s->type == LHT_TEXT) {

			if ((dest->prop[dest->used].prio > prio))
				continue;

			switch(pol) {
				case POL_PREPEND:
#warning TODO
					abort();
				case POL_APPEND:
					didx = dest->used++;
					break;
				case POL_OVERWRITE:
					didx = idx;
				break;
			}

			if (didx >= dest->array_size) {
				hid_cfg_error(s, "Array is already full [%d] of [%d] ingored value: '%s' policy=%d\n", dest->used, dest->array_size, s->data.text.value, pol);
				res = -1;
				break;
			}

			if (conf_parse_text(&dest->val, didx, dest->type, s->data.text.value, s) == 0) {
				dest->prop[didx].prio = prio;
				dest->prop[didx].src  = s;
				if (didx >= dest->used)
					dest->used = didx+1;
			}
		}
		else {
			hid_cfg_error(s, "List item must be text\n");
			res = -1;
		}
	}

	return res;
}

int conf_merge_patch_list(conf_native_t *dest, lht_node_t *src_lst, int prio, conf_policy_t pol)
{
	lht_node_t *s;
	int res = 0;
	conf_listitem_t *i;

	if (pol == POL_DISABLE)
		return 0;

	if (pol = POL_OVERWRITE) {
		/* overwrite the whole list: make it empty then append new elements */
		while((i = conflist_first(dest->val.list)) != NULL)
			conflist_remove(i);
	}

	for(s = src_lst->data.list.first; s != NULL; s = s->next) {
		if (s->type == LHT_TEXT) {
			i = calloc(sizeof(conf_listitem_t), 1);
			i->val.string = &i->payload;
			i->prop.prio = prio;
			i->prop.src  = s;
			if (conf_parse_text(&i->val, 0, CFN_STRING, s->data.text.value, s) != 0) {
				free(i);
				continue;
			}
			switch(pol) {
				case POL_PREPEND:
					conflist_insert(dest->val.list, i);
					break;
				case POL_APPEND:
				case POL_OVERWRITE:
					conflist_append(dest->val.list, i);
					break;
			}
		}
		else {
			hid_cfg_error(s, "List item must be text\n");
			res = -1;
		}
	}

	return res;
}

int conf_merge_patch_recurse(lht_node_t *sect, int default_prio, conf_policy_t default_policy, const char *path_prefix);

int conf_merge_patch_item(const char *path, lht_node_t *n, int default_prio, conf_policy_t default_policy)
{
	conf_native_t *target = conf_get_field(path);
	int res = 0;
	switch(n->type) {
		case LHT_TEXT:
			if (target == NULL) {
				if ((strncmp(path, "plugins/", 8) != 0) && (strncmp(path, "utils/", 6) != 0))/* it is normal to have configuration for plugins and utils not loaded - ignore these */
					hid_cfg_error(n, "conf error: lht->bin conversion: can't find path '%s' - check your lht!\n", path);
				break;
			}
			conf_merge_patch_text(target, n, default_prio, default_policy);
			break;
		case LHT_HASH:
			if (target == NULL) /* no leaf: another level of structs */
				res |= conf_merge_patch_recurse(n, default_prio, default_policy, path);
			else /* leaf: pretend it's text so it gets parsed */
				conf_merge_patch_text(target, n, default_prio, default_policy);
			break;
		case LHT_LIST:
			if (target == NULL)
				hid_cfg_error(n, "conf error: lht->bin conversion: can't find path '%s' - check your lht; may it be that it should be a hash instead of a list?\n", path);
			else if (target->type == CFN_LIST)
				res |= conf_merge_patch_list(target, n, default_prio, default_policy);
			else if (target->array_size > 1)
				res |= conf_merge_patch_array(target, n, default_prio, default_policy);
			else
				hid_cfg_error(n, "Attempt to initialize a scalar with a list - this node should be a text node\n");
			break;
	}
	return res;
}


/* merge main subtree of a patch */
int conf_merge_patch_recurse(lht_node_t *sect, int default_prio, conf_policy_t default_policy, const char *path_prefix)
{
	lht_dom_iterator_t it;
	lht_node_t *n, *h;
	char path[256], *pathe;
	char name[256], *namee;
	int nl, ppl = strlen(path_prefix), res = 0;

	memcpy(path, path_prefix, ppl);
	path[ppl] = '/';
	pathe = path + ppl + 1;

	for(n = lht_dom_first(&it, sect); n != NULL; n = lht_dom_next(&it)) {
		nl = strlen(n->name);
		memcpy(pathe, n->name, nl);
		namee = pathe+nl;
		*namee = '\0';
		res |= conf_merge_patch_item(path, n, default_prio, default_policy);
	}
	return res;
}

int conf_merge_patch(lht_node_t *root, long gprio)
{
	conf_policy_t gpolicy = POL_OVERWRITE;
	const char *ps;
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (root->type != LHT_HASH) {
		hid_cfg_error(root, "patch root should be a hash\n");
		return -1;
	}

	/* get global settings */
	get_hash_int(&gprio, root, "priority");
	get_hash_policy(&gpolicy, root, "policy");

	/* iterate over all hashes and insert them recursively */
	for(n = lht_dom_first(&it, root); n != NULL; n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			conf_merge_patch_recurse(n, gprio, gpolicy, n->name);

	return 0;
}

int conf_merge_all(const char *path)
{
	int n, ret = 0;
	for(n = 0; n < CFR_max; n++) {
		lht_node_t *cr, *r, *r2;
		if (conf_root[n] == NULL)
			continue;
		cr = conf_lht_get_confroot(conf_root[n]->root);
		if (cr == NULL)
			continue;
		for(r = cr->data.list.first; r != NULL; r = r->next) {
			if (path != NULL) {
				conf_policy_t gpolicy = POL_OVERWRITE;
				r2 = lht_tree_path_(r->doc, r, path, 1, 0, NULL);
				get_hash_policy(&gpolicy, r, "policy");
				if ((r2 == NULL) || (conf_merge_patch_item(path, r2, conf_default_prio[n], gpolicy) != 0))
					ret = -1;
			}
			else
				if (conf_merge_patch(r, conf_default_prio[n]) != 0)
					ret = -1;
		}
	}
	return ret;
}

static void conf_field_clear(conf_native_t *f)
{
	if (strncmp(f->hash_path, "temp", 4) == 0)
		return;
	if (f->used > 0) {
#define clr(field) memset(f->val.field, 0, sizeof(*(f->val.field)) * f->used)
		switch(f->type) {
			case CFN_STRING:      clr(string); break;
			case CFN_BOOLEAN:     clr(boolean); break;
			case CFN_INTEGER:     clr(integer); break;
			case CFN_REAL:        clr(real); break;
			case CFN_COORD:       clr(coord); break;
			case CFN_UNIT:        clr(unit); break;
			case CFN_COLOR:       clr(color); break;
			case CFN_LIST:        clr(list); break;
			case CFN_INCREMENTS:  clr(increments); break;
		}
		memset(f->prop, 0, sizeof(confprop_t) * f->used);
#undef clr
	}

	f->used     = 0;
	f->conf_rev = conf_rev;
}

int conf_rev = 0;
static void conf_notify_hids()
{
	conf_rev++;
	hid_notify_conf_changed();
}

void conf_update(const char *path)
{
	/* clear memory-bin data first */
	if (path == NULL) {
		htsp_entry_t *e;
		conf_fields_foreach(e)
			conf_field_clear(e->value);
	}
	else {
		conf_native_t *n;
		n = conf_get_field(path);
		if (n == NULL) {
			char *path_, *field;
			path_ = strdup(path);
			field = strrchr(path_, '/');
			if (field == NULL) {
				free(path_);
				return;
			}
			*field = '\0';
			n = conf_get_field(path_);
			if ((n != NULL) && (n->type == CFN_INCREMENTS))
				conf_update(path_);
			free(path_);
			return;
		}
		conf_field_clear(n);
	}

	/* merge all memory-lht data to memory-bin */
	conf_merge_all(path);
	conf_core_postproc();
	conf_notify_hids();
}

static lht_node_t *conf_lht_get_first_(lht_node_t *cwd)
{
	/* assume root is a li and add to the first hash */
	cwd = conf_lht_get_confroot(cwd);
	if (cwd == NULL)
		return NULL;
	cwd = cwd->data.list.first;
	if ((cwd == NULL) || (cwd->type != LHT_HASH))
		return NULL;
	return cwd;
}

lht_node_t *conf_lht_get_first(conf_role_t target)
{
	return conf_lht_get_first_(conf_root[target]->root);
}

void conf_load_all(void)
{
	int i;
	lht_node_t *dln;
	const char *conf_user_fn = "~/" DOT_PCB_RND "/pcb-conf.lht";

	/* get the lihata node for design/default_layer_name */
	conf_load_as(CFR_INTERNAL, conf_internal, 1);
	dln = conf_lht_get_first(CFR_INTERNAL);
	assert(dln != NULL);
	dln = lht_tree_path_(conf_root[CFR_INTERNAL], dln, "design/default_layer_name", 1, 0, NULL);
	if (dln != NULL) {
		assert(dln->type == LHT_LIST);
		dln = dln->data.list.first;
	}

	/* Set up default layer names - make sure there are enough layers (over the hardwired ones, if any) */
	for (i = 0; i < MAX_LAYER; i++) {
		char buf[20];
		if (dln == NULL) {
			sprintf(buf, "signal%d", i + 1);
			if (conf_set_dry(CFR_INTERNAL, "design/default_layer_name", i, buf, POL_OVERWRITE) != 0)
				printf("Can't set layer name\n");
		}
		else
			dln = dln->next;
	}

	/* load config files */
	conf_load_as(CFR_SYSTEM, PCBSHAREDIR "/pcb-conf.lht", 0);
	conf_load_as(CFR_USER, conf_user_fn, 0);
	conf_load_as(CFR_PROJECT, "./project.lht", 0);
	conf_merge_all(NULL);

	/* create the user config (in-memory-lht) if it does not exist on disk;
	   this is needed so if the user makes config changes from the GUI things
	   get saved. */
	if (conf_root[CFR_USER] == NULL)
		conf_reset(CFR_USER, conf_user_fn);
}

static int keyeq(char *a, char *b)
{
	return !strcmp(a, b);
}

void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path, const char *desc)
{
	conf_native_t *node;

	if (conf_fields == NULL) {
		conf_fields = htsp_alloc(strhash, keyeq);
		assert(conf_fields != NULL);
	}
	assert(array_size >= 1);

	assert(htsp_get(conf_fields, (char *)path) == NULL);

	node = calloc(sizeof(conf_native_t), 1);
	node->array_size  = array_size;
	node->type        = type;
	node->val.any     = value;
	node->prop        = calloc(sizeof(confprop_t), array_size);
	node->description = desc;
	node->hash_path   = path;
	vtp0_init(&(node->hid_data));

	htsp_set(conf_fields, (char *)path, node);
}

void conf_unreg_fields(const char *prefix)
{
	int len = strlen(prefix);
	htsp_entry_t *e;

	assert(prefix[len-1] == '/');

	conf_fields_foreach(e) {
		if (strncmp(e->key, prefix, len) == 0) {
			conf_native_t *node = e->value;
			htsp_pop(conf_fields, e->value);
			vtp0_uninit(&(node->hid_data));
			free(node->prop);
			free(node);
		}
	}
}

conf_native_t *conf_get_field(const char *path)
{
	return htsp_get(conf_fields, (char *)path);
}

int conf_set_dry(conf_role_t target, const char *path_, int arr_idx, const char *new_val, conf_policy_t pol)
{
	char *path, *basename, *next, *last, *sidx, *increment_field = NULL;
	conf_native_t *nat;
	lht_node_t *cwd, *nn;
	lht_node_type_t ty;
	int idx = -1;

	path = strdup(path_);
	sidx = strrchr(path, '[');
	if (sidx != NULL) {
		char *end;
		*sidx = '\0';
		sidx++;
		idx = strtol(sidx, &end, 10);
		if ((*end != ']') || (strchr(sidx, '/') != NULL)) {
			free(path);
			return -1;
		}
		if ((arr_idx >= 0) && (arr_idx != idx)) {
			free(path);
			return -1;
		}
	}
	else if (arr_idx >= 0)
		idx = arr_idx;

	nat = conf_get_field(path);
	if (nat == NULL) {
		/* might be increments */
		increment_field = strrchr(path, '/');
		if (increment_field == NULL) {
			free(path);
			return -1;
		}
		*increment_field = '\0';
		nat = conf_get_field(path);
		if ((nat == NULL) || (nat->type != CFN_INCREMENTS)) {
			free(path);
			return -1;
		}
		*increment_field = '/';
		increment_field++;
	}


	if (conf_root[target] == NULL) {
		free(path);
		return -1;
	}
	if (pol == POL_DISABLE) {
		free(path);
		return 0;
	}
	if ((pol != POL_OVERWRITE) && (idx >= 0)) {
		free(path);
		return -1;
	}

	cwd = conf_lht_get_first(target);
	if (cwd == NULL) {
		free(path);
		return -1;
	}

	if (idx >= nat->array_size) {
		Message("Error: can't conf_set() %s[%d]: %d is beyond the end of the array (%d)\n", path, idx, idx, nat->array_size);
		free(path);
		return -1;
	}

	basename = strrchr(path, '/');
	if (basename == NULL) {
		free(path);
		return -1;
	}
	*basename = '\0';
	basename++;

	/* create parents if they do not exist */
	last = next = path;
	do {
		next = strchr(next, '/');
		if (next != NULL)
			*next = '\0';

		nn = lht_tree_path_(conf_root[target], cwd, last, 1, 0, NULL);
		if (nn == NULL) {
			if (conf_root_lock[target]) {
				Message("WARNING: can't set config item %s because target in-memory lihata does not have the node and is tree-locked\n", path_);
				free(path);
				return -1;
			}
			/* create a new hash node */
			nn = lht_dom_node_alloc(LHT_HASH, last);
			if (lht_dom_hash_put(cwd, nn) != LHTE_SUCCESS) {
				lht_dom_node_free(nn);
				free(path);
				return -1;
			}
		}
		cwd = nn;
		if (next != NULL)
			last = next+1;
	} while(next != NULL);

	/* add the last part of the path, which is either a list or a text node */
	if ((nat->array_size > 1) || (nat->type == CFN_LIST))
		ty = LHT_LIST;
	else
		ty = LHT_TEXT;

	nn = lht_tree_path_(conf_root[target], cwd, basename, 1, 0, NULL);
	if (nn == NULL) {
		if (conf_root_lock[target]) {
			free(path);
			return -1;
		}
		nn = lht_dom_node_alloc(ty, basename);
		if (lht_dom_hash_put(cwd, nn) != LHTE_SUCCESS) {
			lht_dom_node_free(nn);
			free(path);
			return -1;
		}
	}
	cwd = nn;

	/* set value */
	if (ty == LHT_LIST) {
		lht_err_t err;
		nn = lht_dom_node_alloc(LHT_TEXT, "");
		if (pol == POL_OVERWRITE) {
			if (idx == -1) {
				/* empty the list so that we insert to an empty list which is overwriting the list */
				while(cwd->data.list.first != NULL)
					lht_tree_del(cwd->data.list.first);
			}
			else {
				lht_node_t *old = lht_tree_list_nth(cwd, idx);
				if (old != NULL) {
					/* the list is large enough already: overwrite the element at idx */
					err = lht_tree_list_replace_child(cwd, old, nn);
				}
				else {
					int n;
					lht_node_t *i;
					/* count members */
					for (n = 0, i = cwd->data.list.first; i != NULL; i = i->next) n++;
					/* append just enough elements to get one less than needed */
					err = 0;
					for(n = idx - n; n > 0; n--) {
						lht_node_t *dummy = lht_dom_node_alloc(LHT_TEXT, "");
						err |= lht_dom_list_append(cwd, dummy);
					}
					/* append the new node */
					err |= lht_dom_list_append(cwd, nn);
				}
			}
		}
		else if ((pol == POL_PREPEND) || (pol == POL_OVERWRITE))
			err = lht_dom_list_insert(cwd, nn);
		else if (pol == POL_APPEND)
			err = lht_dom_list_append(cwd, nn);
		if (err != LHTE_SUCCESS) {
			lht_dom_node_free(nn);
			free(path);
			return -1;
		}
		cwd = nn;
	}
	else {
		if (idx > 0)
			return -1; /* only lists/array path should have index larger than 0 */
		cwd = nn;
	}

	/* by now cwd is the text node we need to load with the new value; it is
	   either a text config value under a hash or a list item already allocated */
	if (cwd->type != LHT_TEXT) {
		free(path);
		return -1;
	}

	if (cwd->data.text.value != NULL)
		free(cwd->data.text.value);

	cwd->data.text.value = strdup(new_val);
	cwd->file_name = conf_root[target]->active_file;

	free(path);
	return 0;
}

int conf_set(conf_role_t target, const char *path, int arr_idx, const char *new_val, conf_policy_t pol)
{
	int res;
	res = conf_set_dry(target, path, arr_idx, new_val, pol);
	if (res < 0)
		return res;
	conf_update(path);
	return 0;
}

int conf_set_native(conf_native_t *field, int arr_idx, const char *new_val)
{
	lht_node_t *node;

	if (arr_idx >= field->used)
		return -1;

	node = field->prop[arr_idx].src;

	if (node->data.text.value != NULL)
		free(node->data.text.value);

	node->data.text.value = strdup(new_val);
	return 0;
}


int conf_set_from_cli(const char *prefix, const char *arg_, const char *val, const char **why)
{
	char *arg = NULL, *op, *s;
	conf_policy_t pol = POL_OVERWRITE;
	int ret;

	if (prefix != NULL) {
		for(s = arg_; (*s != '=') && (*s != '\0'); s++)
			if (*s == '/')
				arg = strdup(arg_); /* full path, don't use prefix */

		if (arg == NULL) { /* insert prefix */
			int pl = strlen(prefix), al = strlen(arg_);
			while((pl > 0) && (prefix[pl-1] == '/')) pl--;
			arg = malloc(pl+al+2);
			memcpy(arg, prefix, pl);
			arg[pl] = '/';
			strcpy(arg+pl+1, arg_);
		}
	}
	else
		arg = strdup(arg_);

	if (val == NULL) {
		*op = strchr(arg, '=');
		*why = "";
		if (op == val) {
			free(arg);
			*why = "value not specified; syntax is path=val";
			return -1;
		}
		if (op == NULL) {
			conf_native_t *n = conf_get_field(arg_);
			if ((n == NULL) || (n->type != CFN_BOOLEAN)) {
				free(arg);
				*why = "value not specified for non-boolean variable; syntax is path=val";
				return -1;
			}
			val = "1"; /* auto-value for boolean */
		}
		else {
			*op = '\0';
			val = op+1;
			op--;
			switch(*op) {
				case '+': pol = POL_APPEND; *op = '\0'; break;
				case '^': pol = POL_PREPEND; *op = '\0'; break;
			}
		}
	}

	ret = conf_set(CFR_CLI, arg, -1, val, pol);
	if (ret != 0)
		*why = "invalid config path";

	free(arg);
	return ret;
}

void conf_parse_arguments(const char *prefix, int *argc, char ***argv)
{
	int n, dst;

	for(n = 0, dst = 0; n < *argc; n++,dst++) {
		char *a = (*argv)[n];
		const char *why;
		int try_again = -1;

		if (strchr(a, '=') != NULL) {
			try_again = conf_set_from_cli(prefix, a, NULL, &why);
			dst--;
		}
		if (try_again && (n < (*argv) - 1)) {
			try_again = conf_set_from_cli(prefix, a, (*argv)[n+1], &why);
			dst -= 2;
		}
		if (try_again)
			(*argv)[dst] = (*argv)[n];
	}
	*argc = dst-1;
}

void conf_save_file(conf_role_t role)
{
#warning TODO
}

conf_listitem_t *conf_list_first_str(conflist_t *list, const char **item_str, int *idx)
{
	conf_listitem_t *item_li;
	item_li = conflist_first(list);
	if (item_li == NULL)
		return NULL;
	if (item_li->type == CFN_STRING) {
		*item_str = item_li->val.string[0];
		return item_li;
	}
	return conf_list_next_str(item_li, item_str, idx);
}

conf_listitem_t *conf_list_next_str(conf_listitem_t *item_li, const char **item_str, int *idx)
{
	while((item_li = conflist_next(item_li)) != NULL) {
		if (item_li->type != CFN_STRING)
			continue;
		/* found next string */
		*item_str = item_li->val.string[0];
		return item_li;
	}
	/* found end of the list */
	*item_str = NULL;
	return item_li;
}

const char *conf_concat_strlist(const conflist_t *lst, gds_t *buff, int *inited, char sep)
{
	int n;
	conf_listitem_t *ci;

	if ((inited == NULL) || (!*inited)) {
		gds_init(buff);
		if (inited != NULL)
			*inited = 1;
	}
	else
		gds_truncate(buff, 0);

	for (n = 0, ci = conflist_first((conflist_t *)lst); ci != NULL; ci = conflist_next(ci), n++) {
		const char *p = ci->val.string[0];
		if (ci->type != CFN_STRING)
			continue;
		if (n > 0)
			gds_append(buff, sep);
		gds_append_str(buff, p);
	}
	return buff->array;
}

void conf_lock(conf_role_t target)
{
	conf_root_lock[target] = 1;
}

void conf_unlock(conf_role_t target)
{
	conf_root_lock[target] = 0;
}

int conf_islocked(conf_role_t target)
{
	return conf_root_lock[target];
}

void conf_reset(conf_role_t target, const char *source_fn)
{
	if (conf_root[target] != NULL)
		lht_dom_uninit(conf_root[target]);
	lht_node_t *n, *p;
	conf_root[target] = lht_dom_init();
	lht_dom_loc_newfile(conf_root[target], source_fn);
	conf_root[target]->root = lht_dom_node_alloc(LHT_LIST, conf_list_name);
	n = lht_dom_node_alloc(LHT_HASH, "main");
	lht_dom_list_insert(conf_root[target]->root, n);
	p = lht_dom_node_alloc(LHT_TEXT, "priority");
	p->data.text.value = strdup("500");
	lht_dom_hash_put(n, p);
}

void conf_init(void)
{
	conf_reset(CFR_ENV, "<environment-variables>");
	conf_reset(CFR_CLI, "<commandline>");
	conf_reset(CFR_DESIGN, "<null-design>");
}
