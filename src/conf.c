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

#include <sys/stat.h>
#include <assert.h>
#include <genht/hash.h>
#include <liblihata/tree.h>
#include "conf.h"
#include "conf_core.h"
#include "conf_hid.h"
#include "hid_cfg.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "misc_util.h"
#include "error.h"
#include "paths.h"

/* conf list node's name */
const char *conf_list_name = "pcb-rnd-conf-v1";
static const char *conf_user_fn = "~/" DOT_PCB_RND "/pcb-conf.lht";

lht_doc_t *conf_root[CFR_max_alloc];
int conf_root_lock[CFR_max_alloc];
int conf_lht_dirty[CFR_max_alloc];

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
	if (conf_root[role] != NULL) {
		lht_dom_uninit(conf_root[role]);
		conf_root[role] = NULL;
	}
	if (fn_is_text)
		d = hid_cfg_load_str(fn);
	else
		d = hid_cfg_load_lht(fn);
	if (d == NULL) {
		FILE *f;
		char *efn;
		resolve_path(fn, &efn, 0);
		f = fopen(efn, "r");
		if (f != NULL) { /* warn only if the file is there - missing file is normal */
			Message("error: failed to load lht config: %s\n", efn);
			fclose(f);
		}
		free(efn);
		return -1;
	}

	if ((d->root->type == LHT_LIST) && (strcmp(d->root->name, "pcb-rnd-conf-v1") == 0)) {
		conf_root[role] = d;
		return 0;
	}

	if ((d->root->type == LHT_HASH) && (strcmp(d->root->name, "geda-project-v1") == 0)) {
		lht_node_t *confroot;
		confroot = lht_tree_path_(d, d->root, "pcb-rnd-conf-v1", 1, 0, NULL);
		if ((confroot != NULL)  && (confroot->type == LHT_LIST) && (strcmp(confroot->name, "li:pcb-rnd-conf-v1") == 0)) {
			conf_root[role] = d;
			return 0;
		}

		/* project file with no config root */
		confroot = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");
		lht_dom_hash_put(d->root, confroot);
		conf_root[role] = d;
		return 0;
	}

	hid_cfg_error(d->root, "Root node must be either li:pcb-rnd-conf-v1 or ha:geda-project-v1\n");

	if (d != NULL)
		lht_dom_uninit(d);
	return -1;
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

void extract_poliprio(lht_node_t *root, conf_policy_t *gpolicy, long *gprio)
{
	long len = strlen(root->name), p = -1;
	char tmp[128];
	char *sprio, *end;
	conf_policy_t pol;

	if (len >= sizeof(tmp)) {
		hid_cfg_error(root, "Invalid policy-prio '%s', subtree is ignored\n", root->name);
		return;
	}

	memcpy(tmp, root->name, len+1);

	/* split and convert prio */
	sprio = strchr(tmp, '-');
	if (sprio != NULL) {
		*sprio = '\0';
		sprio++;
		p = strtol(sprio, &end, 10);
		if ((*end != '\0') || (p < 0)) {
			hid_cfg_error(root, "Invalid prio in '%s', subtree is ignored\n", root->name);
			return;
		}
	}

	/* convert policy */
	pol = conf_policy_parse(tmp);
	if (pol == POL_invalid) {
		hid_cfg_error(root, "Invalid policy in '%s', subtree is ignored\n", root->name);
		return;
	}

	/* by now the syntax is checked; save the output */
	*gpolicy = pol;
	if (p >= 0)
		*gprio = p;
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
				hid_cfg_error(node, "invalid numeric value in increment field " #field ": %s\n", val->data.text.value); \
		} \
		else\
			hid_cfg_error(node, "increment field " #field " needs to be a text node\n"); \
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
	return 0;
}

static const char *get_project_conf_name(const char *project_fn, const char *pcb_fn, const char **try)
{
	static char res[MAXPATHLEN];
	static const char *project_name = "project.lht";
	FILE *f;

	if (project_fn != NULL) {
		strncpy(res, project_fn, sizeof(res)-1);
		res[sizeof(res)-1] = '\0';
		goto check;
	}

	if (pcb_fn != NULL) {
		char *end;
		/* replace pcb name with project_name and test */
		strncpy(res, pcb_fn, sizeof(res)-1);
		res[sizeof(res)-1] = '\0';
		end = strrchr(res, PCB_DIR_SEPARATOR_C);
		if (end != NULL) {
			if (end+1+sizeof(project_name) >= res + sizeof(res))
				return NULL;
			strcpy(end+1, project_name);
			goto check;
		}
		/* no path in pcb, try cwd */
		strcpy(res, project_name);
		goto check;
	}

	/* no pcb - no project */
	*try = "<no pcb file, no path to try>";
	return NULL;

	check:;
	*try = res;
	f = fopen(res, "r");
	if (f != NULL) {
		fclose(f);
		return res;
	}

	return NULL;
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
			while(isspace(*text)) text++;
			for(s = strue; *s != NULL; s++)
				if (strncasecmp(*s, text, strlen(*s)) == 0) {
					dst->boolean[idx] = 1;
					return 0;
				}
			for(s = sfalse; *s != NULL; s++)
				if (strncasecmp(*s, text, strlen(*s)) == 0) {
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
			while(isspace(*end))
				end++;
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

static void conf_insert_arr(conf_native_t *dest, int offs)
{
#define CASE_MOVE(typ, fld) \
	case typ: memmove(dest->val.fld+offs, dest->val.fld, dest->used * sizeof(dest->val.fld[0])); return

	memmove(dest->prop+offs, dest->prop, dest->used * sizeof(dest->prop[0]));
	if (dest->used > 0) {
		switch(dest->type) {
			CASE_MOVE(CFN_STRING, string);
			CASE_MOVE(CFN_BOOLEAN, boolean);
			CASE_MOVE(CFN_INTEGER, integer);
			CASE_MOVE(CFN_REAL, real);
			CASE_MOVE(CFN_COORD, coord);
			CASE_MOVE(CFN_UNIT, unit);
			CASE_MOVE(CFN_COLOR, color);
			CASE_MOVE(CFN_LIST, list);
			CASE_MOVE(CFN_INCREMENTS, increments);
		}
	}
#undef CASE_MOVE
	abort(); /* unhandled type */
}

int conf_merge_patch_array(conf_native_t *dest, lht_node_t *src_lst, int prio, conf_policy_t pol)
{
	lht_node_t *s;
	int res, idx, didx, maxpr;

	if ((pol == POL_DISABLE) || (pol == POL_invalid))
		return 0;

	if (pol == POL_PREPEND) {
		for(s = src_lst->data.list.first, maxpr = 0; s != NULL; s = s->next, maxpr++) ;

		if (dest->used+maxpr >= dest->array_size)
			maxpr = dest->array_size - dest->used - 1;

		conf_insert_arr(dest, maxpr);
		dest->used += maxpr;
	}
/*printf("arr merge prio: %d\n", prio);*/
	for(s = src_lst->data.list.first, idx = 0; s != NULL; s = s->next, idx++) {
		if (s->type == LHT_TEXT) {
			switch(pol) {
				case POL_PREPEND:
					if (idx < maxpr)
						didx = idx;
					else
						didx = dest->array_size+1; /* indicate array full */
					break;
				case POL_APPEND:
					didx = dest->used++;
					break;
				case POL_OVERWRITE:
					didx = idx;
				break;
				case POL_DISABLE: case POL_invalid: return 0; /* compiler warning */
			}
/*printf("   didx: %d / %d '%s'\n", didx, dest->array_size, s->data.text.value);*/
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

	if ((pol == POL_DISABLE) || (pol == POL_invalid))
		return 0;

	if (pol == POL_OVERWRITE) {
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
					dest->used |= 1;
					break;
				case POL_APPEND:
				case POL_OVERWRITE:
					conflist_append(dest->val.list, i);
					dest->used |= 1;
					break;
				case POL_DISABLE: case POL_invalid: return 0; /* compiler warning */
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
		case LHT_SYMLINK:
#warning TODO
			break;
		case LHT_INVALID_TYPE:
		case LHT_TABLE:
			/* intentionally unhandled types */
			break;
	}
	return res;
}


/* merge main subtree of a patch */
int conf_merge_patch_recurse(lht_node_t *sect, int default_prio, conf_policy_t default_policy, const char *path_prefix)
{
	lht_dom_iterator_t it;
	lht_node_t *n;
	char path[256], *pathe;
	char *namee;
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
	conf_policy_t gpolicy;
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (root->type != LHT_HASH) {
		hid_cfg_error(root, "patch root should be a hash\n");
		return -1;
	}

	extract_poliprio(root, &gpolicy, &gprio);

	/* iterate over all hashes and insert them recursively */
	for(n = lht_dom_first(&it, root); n != NULL; n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			conf_merge_patch_recurse(n, gprio, gpolicy, n->name);

	return 0;
}

typedef struct {
	long prio;
	conf_policy_t policy;
	lht_node_t *subtree;
} merge_subtree_t;

#define GVT(x) vmst_ ## x
#define GVT_ELEM_TYPE merge_subtree_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 64
#define GVT_START_SIZE 32
/*#define GVT_FUNC static*/
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.h>
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

vmst_t merge_subtree; /* automatically initialized to all-zero */

static void add_subtree(int role, lht_node_t *subtree_parent_root, lht_node_t *subtree_root)
{
	merge_subtree_t *m;

	m = vmst_alloc_append(&merge_subtree, 1);
	m->prio = conf_default_prio[role];

	extract_poliprio(subtree_parent_root, &m->policy, &m->prio);
	m->subtree = subtree_root;
}

static int mst_prio_cmp(const void *a, const void *b)
{
	const merge_subtree_t *s1 = a, *s2 = b;
	return s1->prio > s2->prio;
}

int conf_merge_all(const char *path)
{
	int n, ret = 0;

	vmst_truncate(&merge_subtree, 0);

	for(n = 0; n < CFR_max_real; n++) {
		lht_node_t *cr, *r, *r2;
		if (conf_root[n] == NULL)
			continue;
		cr = conf_lht_get_confroot(conf_root[n]->root);
		if (cr == NULL)
			continue;
		for(r = cr->data.list.first; r != NULL; r = r->next) {
			if (path != NULL) {
				r2 = lht_tree_path_(r->doc, r, path, 1, 0, NULL);
				if (r2 != NULL)
					add_subtree(n, r, r2);
			}
			else
				add_subtree(n, r, r);
		}
	}

	qsort(merge_subtree.array, vmst_len(&merge_subtree), sizeof(merge_subtree_t), mst_prio_cmp);
	for(n = 0; n < vmst_len(&merge_subtree); n++) {
		if (path != NULL)
			ret |= conf_merge_patch_item(path, merge_subtree.array[n].subtree, merge_subtree.array[n].prio, merge_subtree.array[n].policy);
		else
			ret |= conf_merge_patch(merge_subtree.array[n].subtree, merge_subtree.array[n].prio);
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
			case CFN_INCREMENTS:  clr(increments); break;
			case CFN_LIST:
				while(conflist_first(f->val.list)) { /* need to free all items of a list before clearing the main struct */
					conf_listitem_t *i = conflist_first(f->val.list);
					conflist_remove(i);
					free(i);
				}
				clr(list);
				break;
		}
		memset(f->prop, 0, sizeof(confprop_t) * f->used);
#undef clr
	}

	f->used     = 0;
	f->conf_rev = conf_rev;
}

int conf_rev = 0;
void conf_update(const char *path)
{
	conf_native_t *n;

	/* clear memory-bin data first */
	if (path == NULL) {
		htsp_entry_t *e;
		conf_fields_foreach(e) {
			conf_hid_global_cb((conf_native_t *)e->value, val_change_pre);
			conf_hid_local_cb((conf_native_t *)e->value, val_change_pre);
			conf_field_clear(e->value);
		}
	}
	else {
		n = conf_get_field(path);
		if (n == NULL) {
			char *path_, *field;
			path_ = strdup(path);

			/* It might be an array element - truncate */
			field = strrchr(path_, '[');
			if (field != NULL) {
				*field = '\0';
				n = conf_get_field(path_);
			}

			/* It may be a field of an increment, truncate last portion */
			if (n == NULL) {
				field = strrchr(path_, '/');
				if (field == NULL) {
					free(path_);
					return;
				}
				*field = '\0';
				n = conf_get_field(path_);
				if (n->type != CFN_INCREMENTS)
					n = NULL;
			}

			/* if a valid node is found, update it */
			if (n != NULL)
				conf_update(path_);

			free(path_);
			return;
		}
		conf_field_clear(n);
		conf_hid_global_cb(n, val_change_pre);
		conf_hid_local_cb(n, val_change_pre);
	}

	/* merge all memory-lht data to memory-bin */
	conf_merge_all(path);
	conf_core_postproc();

	if (path == NULL) {
		htsp_entry_t *e;
		conf_fields_foreach(e) {
			conf_hid_local_cb((conf_native_t *)e->value, val_change_post);
			conf_hid_global_cb((conf_native_t *)e->value, val_change_post);
		}
	}
	else {
		conf_hid_local_cb(n, val_change_post);
		conf_hid_global_cb(n, val_change_post);
	}
	conf_rev++;
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
	assert(target != CFR_invalid);
	assert(target >= 0);
	assert(target < CFR_max_alloc);
	if (conf_root[target] == NULL)
		return NULL;
	return conf_lht_get_first_(conf_root[target]->root);
}

static lht_node_t *conf_lht_get_at_(conf_role_t target, const char *conf_path, const char *lht_path, int create)
{
	lht_node_t *n, *r;
	n = conf_lht_get_first(target);
	if (n == NULL)
		return NULL;
	r = lht_tree_path_(n->doc, n, lht_path, 1, 0, NULL);
	if ((r == NULL) && (create)) {
		conf_set_dry(target, conf_path, -1, "", POL_OVERWRITE);
		r = lht_tree_path_(n->doc, n, lht_path, 1, 0, NULL);
	}
	return r;
}

lht_node_t *conf_lht_get_at(conf_role_t target, const char *path, int create)
{
	lht_node_t *r;
	char *pc, *end;
	if (conf_root[target] == NULL) {
		if (!create)
			return NULL;
		conf_reset(target, "<conf_lht_get_at>");
	}
	end = strchr(path, '[');
	if (end == NULL) {
		r = conf_lht_get_at_(target, path, path, create);
	}
	else {
		/* lihata syntax differs from conf syntax in array indexing */
		pc = strdup(path);
		pc[end-path] = '/';
		end = strchr(pc+(end-path), ']');
		if (end != NULL)
			*end = '\0';
		r = conf_lht_get_at_(target, path, pc, create);
		free(pc);
	}
	return r;
}


void conf_load_all(const char *project_fn, const char *pcb_fn)
{
	int i;
	lht_node_t *dln;
	const char *pc, *try;

	/* get the lihata node for design/default_layer_name */
	conf_load_as(CFR_INTERNAL, conf_internal, 1);
	dln = conf_lht_get_at(CFR_INTERNAL, "design/default_layer_name", 1);
	assert(dln != NULL);
	assert(dln->type == LHT_LIST);
	dln = dln->data.list.first;

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
	pc = get_project_conf_name(project_fn, pcb_fn, &try);
	if (pc != NULL)
		conf_load_as(CFR_PROJECT, pc, 0);
	conf_merge_all(NULL);

	/* create the user config (in-memory-lht) if it does not exist on disk;
	   this is needed so if the user makes config changes from the GUI things
	   get saved. */
	if (conf_root[CFR_USER] == NULL)
		conf_reset(CFR_USER, conf_user_fn);
}

void conf_load_project(const char *project_fn, const char *pcb_fn)
{
	const char *pc, *try;

	assert((project_fn != NULL) || (pcb_fn != NULL));

	pc = get_project_conf_name(project_fn, pcb_fn, &try);
	if (pc != NULL)
		if (conf_load_as(CFR_PROJECT, pc, 0) != 0)
			pc = NULL;
	if (pc == NULL)
		conf_reset(CFR_PROJECT, "<conf_load_project>");
	conf_update(NULL);
}

void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path, const char *desc, conf_flag_t flags)
{
	conf_native_t *node;

	if (conf_fields == NULL) {
		conf_fields = htsp_alloc(strhash, strkeyeq);
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
	node->flags       = flags;
	vtp0_init(&(node->hid_data));

	htsp_set(conf_fields, (char *)path, node);
}

void conf_free_native(conf_native_t *node)
{
	if (node->type == CFN_LIST) {
		while(conflist_first(node->val.list) != NULL) {
			conf_listitem_t *first = conflist_first(node->val.list);
			conflist_remove(first);
			free(first);
		}
	}

	vtp0_uninit(&(node->hid_data));
	free(node->prop);
	free(node);
}

void conf_unreg_fields(const char *prefix)
{
	int len = strlen(prefix);
	htsp_entry_t *e;

	assert(prefix[len-1] == '/');

	conf_fields_foreach(e) {
		if (strncmp(e->key, prefix, len) == 0) {
			conf_free_native(e->value);
			htsp_delentry(conf_fields, e);
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
		next = strchr(last, '/');
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
		lht_err_t err = 0;
		nn = lht_dom_node_alloc(LHT_TEXT, "");
		if (pol == POL_OVERWRITE) {
			if (idx == -1) {
				/* empty the list so that we insert to an empty list which is overwriting the list */
				while(cwd->data.list.first != NULL)
					lht_tree_del(cwd->data.list.first);
				lht_dom_list_append(cwd, nn);
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
		if (idx > 0) {
			free(path);
			return -1; /* only lists/array path should have index larger than 0 */
		}
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

	conf_lht_dirty[target]++;

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
	const char *sc;
	conf_policy_t pol = POL_OVERWRITE;
	int ret;

	if (prefix != NULL) {
		for(sc = arg_; (*sc != '=') && (*sc != '\0'); sc++)
			if (*sc == '/')
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

	/* replace any - with _ in the path part; cli accepts dash but the backing C
	   struct field names don't */
	for(s = arg, op = NULL; (*s != '\0') && (op == NULL); s++) {
		if (*s == '=')
			op = s;
		else if (*s == '-')
			*s = '_';
	}

	/* extract val, if we need to */
	if (val == NULL) {
		*why = "";
		if (op == arg) {
			free(arg);
			*why = "value not specified; syntax is path=val";
			return -1;
		}
		if (op == NULL) {
			conf_native_t *n = conf_get_field(arg);
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

	/* now that we have a clean path (arg) and a value, try to set the config */
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

		if (a[0] == '-') a++;
		if (a[0] == '-') a++;

		try_again = conf_set_from_cli(prefix, a, NULL, &why);
		if (try_again && (n < (*argc) - 1)) {
			try_again = conf_set_from_cli(prefix, a, (*argv)[n+1], &why);
			if (!try_again)
				n++;
		}
		if (try_again)
			(*argv)[dst] = (*argv)[n];
		else
			dst--;
	}
	*argc = dst;
}

void conf_usage(char *prefix, void (*print)(const char *name, const char *help))
{
	htsp_entry_t *e;
	int pl = (prefix == NULL ? 0 : strlen(prefix));

	conf_fields_foreach(e) {
		if ((prefix == NULL) || (strncmp(prefix, e->key, pl) == 0)) {
			conf_native_t *n = e->value;
			if (n->flags & CFF_USAGE) {
				int kl = strlen(n->hash_path);
				char *s, *name = malloc(kl+32);
				const char *sc;
				for(sc = n->hash_path + pl; *sc == '/'; sc++) ;
				name[0] = '-';
				name[1] = '-';
				strcpy(name+2, s);
				for(s = name; *s != '\0'; s++)
					if (*s == '_')
						*s = '-';
				switch(n->type) {
					case CFN_BOOLEAN: /* implicit true */ break;
					case CFN_STRING:  strcpy(s, " str"); break;
					case CFN_INTEGER: strcpy(s, " int"); break;
					case CFN_REAL:    strcpy(s, " num"); break;
					case CFN_COORD:   strcpy(s, " coord"); break;
					case CFN_UNIT:    strcpy(s, " unit"); break;
					case CFN_COLOR:   strcpy(s, " color"); break;
					case CFN_LIST:
					case CFN_INCREMENTS:
						strcpy(s, " ???");
						break;
				}
				print(name, n->description);
				free(name);
			}
		}
	}
}

int conf_replace_subtree(conf_role_t dst_role, const char *dst_path, conf_role_t src_role, const char *src_path)
{
	lht_node_t *dst = conf_lht_get_at(dst_role, dst_path, 1);
	lht_node_t *src, *new_src = NULL;

	if (src_role == CFR_binary) {
		char *name;
		lht_node_t *ch = NULL;
		int isarr, i;
		conf_native_t *n = conf_get_field(src_path);

		if (n == NULL)
			return -1;
		name = strrchr(n->hash_path, '/');
		if (name == NULL)
			return -1;

		isarr = n->array_size > 1;
		if (isarr)
			src = lht_dom_node_alloc(LHT_LIST, name+1);

		for(i = 0; i < n->array_size; i++) {
			gds_t s;

			gds_init(&s);
			conf_print_native_field((conf_pfn)pcb_append_printf, &s, 0, &n->val, n->type, n->prop+i, i);
fprintf(stderr, "ly1 '%s'\n", s.array);
			ch = lht_dom_node_alloc(LHT_TEXT, isarr ? "" : name+1);
			ch->data.text.value = s.array;
			if (isarr) {
				lht_dom_list_append(src, ch);
			}
			s.array = NULL;
			gds_uninit(&s);
		}

		if (!isarr)
			src = ch;
	}
	else
		src = conf_lht_get_at(src_role, src_path, 0);

	if ((src == NULL) && (dst != NULL)) {
		lht_tree_del(dst);
		return 0;
	}

	if (dst == NULL)
		goto err;

	new_src = lht_dom_duptree(src);
	if (new_src == NULL)
		goto err;

	if (lht_tree_replace(dst, new_src) != LHTE_SUCCESS)
		goto err;

	conf_lht_dirty[dst_role]++;

	lht_tree_del(dst);
	if (src_role == CFR_binary)
		lht_dom_node_free(src);
	return 0;

	err:;
	if (src_role == CFR_binary)
		lht_dom_node_free(src);
	if (new_src != NULL)
		lht_dom_node_free(new_src);
	return -1;
}


int conf_save_file(const char *project_fn, const char *pcb_fn, conf_role_t role, const char *fn)
{
	int fail = 1;
	lht_node_t *r = conf_lht_get_first(role);
	const char *try;
	char *efn;

	/* do not save if there's no change */
	if (conf_lht_dirty[role] == 0)
		return 0;

	if (fn == NULL) {
		switch(role) {
			case CFR_USER:
				fn = conf_user_fn;
				break;
			case CFR_PROJECT:
				fn = get_project_conf_name(project_fn, pcb_fn, &try);
				if (fn == NULL) {
					Message("Error: can not save config to project file: %s does not exist - please create an empty file there first\n", try);
					return -1;
				}
				break;
			default: return -1;
		}
	}

	resolve_path(fn, &efn, 0);

	if (r != NULL) {
		FILE *f;
		f = fopen(efn, "w");

		if ((f == NULL) && (role == CFR_USER)) {
			/* create the directory and try again */
			char *path = strdup(efn), *end;
			end = strrchr(path, '/');
			if (end != NULL) {
				*end = '\0';
#warning TODO: more portable mkdir
				if (mkdir(path, 0755) == 0) {
					Message("Created directory %s for saving %s\n", path, fn);
					f = fopen(efn, "w");
				}
				else
					Message("Error: failed to creat directory %s for saving %s\n", path, efn);
			}
			free(path);
		}

		if (f != NULL) {
#warning CONF TODO: a project file needs to be loaded, merged, then written (to preserve non-config nodes)
			lht_dom_export(r->doc->root, f, "");
			fail = 0;
			conf_lht_dirty[role] = 0;
			fclose(f);
		}
		else
			Message("Error: can't save config to %s - can't open the file for write\n", fn);
	}

	free(efn);
	return fail;
}

int conf_export_to_file(const char *fn, conf_role_t role, const char *conf_path)
{
	lht_node_t *at = conf_lht_get_at(role, conf_path, 0);
	lht_err_t r;
	FILE *f;

	if (at == NULL)
		return -1;

	f = fopen(fn, "w");
	if (f == NULL)
		return -1;

	r = lht_dom_export(at, f, "");
	fclose(f);
	return r;
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

int conf_isdirty(conf_role_t target)
{
	return conf_lht_dirty[target];
}

void conf_makedirty(conf_role_t target)
{
	conf_lht_dirty[target]++;
}
conf_role_t conf_lookup_role(const lht_node_t *nd)
{
	conf_role_t r;
	for(r = 0; r < CFR_max_real; r++)
		if (conf_root[r] == nd->doc)
			return r;

	return CFR_invalid;
}

void conf_reset(conf_role_t target, const char *source_fn)
{
	if (conf_root[target] != NULL)
		lht_dom_uninit(conf_root[target]);
	lht_node_t *n;
	conf_root[target] = lht_dom_init();
	lht_dom_loc_newfile(conf_root[target], source_fn);
	conf_root[target]->root = lht_dom_node_alloc(LHT_LIST, conf_list_name);
	conf_root[target]->root->doc = conf_root[target];
	n = lht_dom_node_alloc(LHT_HASH, "overwrite");
	lht_dom_list_insert(conf_root[target]->root, n);
}

/*****************/
static int needs_braces(const char *s)
{
	for(; *s != '\0'; s++)
		if (!isalnum(*s) && (*s != '_') && (*s != '-') && (*s != '+') && (*s != '/') && (*s != ':') && (*s != '.') && (*s != ',') && (*s != '$') && (*s != '(') && (*s != ')') && (*s != '~'))
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

int conf_print_native_field(conf_pfn pfn, void *ctx, int verbose, confitem_t *val, conf_native_type_t type, confprop_t *prop, int idx)
{
	int ret = 0;
	switch(type) {
		case CFN_STRING:  print_str_or_null(pfn, ctx, verbose, val->string[idx], val->string[idx]); break;
		case CFN_BOOLEAN: ret += pfn(ctx, "%d", val->boolean[idx]); break;
		case CFN_INTEGER: ret += pfn(ctx, "%ld", val->integer[idx]); break;
		case CFN_REAL:    ret += pfn(ctx, "%f", val->real[idx]); break;
		case CFN_COORD:   ret += pfn(ctx, "%$mS", val->coord[idx]); break;
		case CFN_UNIT:    print_str_or_null(pfn, ctx, verbose, val->unit[idx], val->unit[idx]->suffix); break;
		case CFN_COLOR:   print_str_or_null(pfn, ctx, verbose, val->color[idx], val->color[idx]); break;
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

int conf_print_native(conf_pfn pfn, void *ctx, const char * prefix, int verbose, conf_native_t *node)
{
	int ret = 0;
	if ((node->used <= 0) && (!verbose))
		return 0;
	if (node->array_size > 1) {
		int n;
		if (!verbose)
			ret += pfn(ctx, "{");
		for(n = 0; n < node->used; n++) {
			if (verbose)
				ret += pfn(ctx, "%s I %s[%d] = ", prefix, node->hash_path, n);
			ret += conf_print_native_field(pfn, ctx, verbose, &node->val, node->type, node->prop, n);
			if (verbose)
				ret += pfn(ctx, " conf_rev=%d\n", node->conf_rev);
			else
				ret += pfn(ctx, ";");
		}
		if (verbose) {
			if (node->used == 0)
				ret += pfn(ctx, "%s I %s[] = <empty>\n", prefix, node->hash_path);
		}
		else
			ret += pfn(ctx, "}");
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


/****************/

void conf_init(void)
{
	conf_reset(CFR_ENV, "<environment-variables>");
	conf_reset(CFR_CLI, "<commandline>");
	conf_reset(CFR_DESIGN, "<null-design>");

	conf_reset(CFR_file, "<conf_init>");
	conf_reset(CFR_binary, "<conf_init>");
}

void conf_uninit(void)
{
	int n;
	htsp_entry_t *e;

	conf_hid_uninit();

	for(n = 0; n < CFR_max_alloc; n++)
		if (conf_root[n] != NULL)
			lht_dom_uninit(conf_root[n]);

	conf_fields_foreach(e) {
		conf_free_native(e->value);
		htsp_delentry(conf_fields, e);
	}
	htsp_free(conf_fields);

	vmst_uninit(&merge_subtree);
}

