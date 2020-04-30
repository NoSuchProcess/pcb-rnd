/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <genht/htsi.h>
#include <genht/hash.h>
#include <liblihata/tree.h>
#include <stdarg.h>
#include <librnd/config.h>
#include <librnd/core/globalconst.h>
#include <librnd/core/color.h>
#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/error.h>
#include <librnd/core/paths.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/file_loaded.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/fptr_cast.h>

/* conf list node's name */
const char *pcb_conf_list_name = "pcb-rnd-conf-v1";
static const char *flcat = "conf";

/* plugin config files and interns */
static htsi_t conf_files;
static htsi_t conf_interns;
static int conf_files_inited = 0;

void (*rnd_conf_core_postproc)(void) = NULL;
extern void pcbhl_conf_postproc(void);

int rnd_conf_in_production = 0;

/* The main conf: monolithic config files affecting all parts of the conf tree;
   By default every operation is done on these trees. */
lht_doc_t *pcb_conf_main_root[RND_CFR_max_alloc];
long rnd_conf_main_root_replace_cnt[RND_CFR_max_alloc]; /* number of times the root has been replaced */
int pcb_conf_main_root_lock[RND_CFR_max_alloc];
int pcb_conf_lht_dirty[RND_CFR_max_alloc];

/* Plugin config: only plugin configuration is accepted; never edited, only
   merged in. Merge takes two steps: first all files per role are merged into
   a single pcb_conf_plug_root[R] (lihata level merge), then pcb_conf_plug_root[R]
   is merged in using the normal conf merge mechanism. Plug roots are
   merged before main roots so main root overwrite are stronger. */
lht_doc_t *pcb_conf_plug_root[RND_CFR_max_alloc];


htsp_t *rnd_conf_fields = NULL;
const int rnd_conf_default_prio[] = {
/*	RND_CFR_INTERNAL */   100,
/*	RND_CFR_SYSTEM */     200,
/*	RND_CFR_DEFAULTPCB */ 300,
/*	RND_CFR_USER */       400,
/*	RND_CFR_ENV */        500,
/*	RND_CFR_PROJECT */    600,
/*	RND_CFR_DESIGN */     700,
/*	RND_CFR_CLI */        800,
0, 0, 0, 0, 0, 0, 0, 0, 0 /* make sure the array is addressable until RND_CFR_max_alloc */
};

extern const char *pcb_conf_internal;


static lht_node_t *conf_lht_get_confroot(lht_node_t *cwd)
{
	if (cwd == NULL)
		return NULL;

	/* if it's a list with a matching name, we found it */
	if ((cwd->type == LHT_LIST) && (strcmp(cwd->name, pcb_conf_list_name) == 0))
		return cwd;

	/* else it may be the parent-hash of the list */
	if (cwd->type != LHT_HASH)
		return NULL;

	return lht_dom_hash_get(cwd, pcb_conf_list_name);
}

int rnd_conf_insert_tree_as(rnd_conf_role_t role, lht_node_t *root)
{
	lht_doc_t *d;

	if ((root->type != LHT_LIST) || (strcmp(root->name, "pcb-rnd-conf-v1") != 0))
		return -1;

	if (pcb_conf_main_root[role] != NULL) {
		lht_dom_uninit(pcb_conf_main_root[role]);
		pcb_conf_main_root[role] = NULL;
	}

	d = lht_dom_init();
	d->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");
	d->root->doc = d;
	lht_tree_merge(d->root, root);
	pcb_conf_main_root[role] = d;
	rnd_conf_main_root_replace_cnt[role]++;
	return 0;
}

static lht_doc_t *conf_load_plug_file(const char *fn, int fn_is_text)
{
	lht_doc_t *d;
	const char *ifn;

	if (fn_is_text) {
		d = pcb_hid_cfg_load_str(fn);
		ifn = "<string>";
	}
	else {
		d = pcb_hid_cfg_load_lht(NULL, fn);
		ifn = fn;
	}

	if (d == NULL) {
		rnd_message(RND_MSG_ERROR, "error: failed to load lht plugin config: %s (can't open the file or syntax error)\n", ifn);
		return NULL;
	}

	if (d->root == NULL) {
		rnd_message(RND_MSG_ERROR, "error: failed to load lht plugin config: %s (no root)\n", ifn);
		lht_dom_uninit(d);
		return NULL;
	}

	if ((d->root->type != LHT_LIST) || (strcmp(d->root->name, "pcb-rnd-conf-v1") != 0)) {
		rnd_message(RND_MSG_ERROR, "error: failed to load lht plugin config: %s (not a pcb-rnd-conf-v1)\n", ifn);
		lht_dom_uninit(d);
		return NULL;
	}

	return d;
}

int rnd_conf_load_as(rnd_conf_role_t role, const char *fn, int fn_is_text)
{
	lht_doc_t *d;
	const char *ifn, *role_name = rnd_conf_role_name(role);

	if (pcb_conf_main_root_lock[role])
		return -1;
	if (pcb_conf_main_root[role] != NULL) {
		lht_dom_uninit(pcb_conf_main_root[role]);
		pcb_conf_main_root[role] = NULL;
		if (role_name != NULL)
			rnd_file_loaded_del_at(flcat, role_name);
	}
	if (fn_is_text) {
		d = pcb_hid_cfg_load_str(fn);
		ifn = "<string>";
	}
	else {
		d = pcb_hid_cfg_load_lht(NULL, fn);
		ifn = fn;
	}

	if (d == NULL) {
		FILE *f;
		char *efn;

		f = pcb_fopen_fn(NULL, fn, "r", &efn);
		if (f != NULL) { /* warn only if the file is there - missing file is normal */
			rnd_message(RND_MSG_ERROR, "error: failed to load lht config: %s (%s)\n", fn, efn);
			fclose(f);
		}
		free(efn);
		return -1;
	}

	if (d->root == NULL) {
		lht_node_t *prjroot, *confroot;
		prjroot = lht_dom_node_alloc(LHT_HASH, "coraleda-project-v1");
		confroot = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");
		lht_dom_hash_put(prjroot, confroot);
		prjroot->doc = d;
		confroot->doc = d;
		d->root = prjroot;
		pcb_conf_main_root[role] = d;
		if (role_name != NULL)
			rnd_file_loaded_set_at(flcat, role_name, ifn, "project/conf");
		return 0;
	}

	if ((d->root->type == LHT_LIST) && (strcmp(d->root->name, "pcb-rnd-conf-v1") == 0)) {
		pcb_conf_main_root[role] = d;
		if (role_name != NULL)
			rnd_file_loaded_set_at(flcat, role_name, ifn, "conf");
		return 0;
	}

	if ((d->root->type == LHT_HASH) && ((strcmp(d->root->name, "geda-project-v1") == 0) || (strcmp(d->root->name, "coraleda-project-v1") == 0))) {
		lht_node_t *confroot;
		confroot = lht_tree_path_(d, d->root, "pcb-rnd-conf-v1", 1, 0, NULL);

		if (role_name != NULL)
			rnd_file_loaded_set_at(flcat, role_name, ifn, "project/conf");

		if ((confroot != NULL)  && (confroot->type == LHT_LIST) && (strcmp(confroot->name, "li:pcb-rnd-conf-v1") == 0)) {
			pcb_conf_main_root[role] = d;
			return 0;
		}

		/* project file with no config root */
		confroot = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");
		lht_dom_hash_put(d->root, confroot);
		pcb_conf_main_root[role] = d;
		return 0;
	}

	pcb_hid_cfg_error(d->root, "Root node must be one of li:pcb-rnd-conf-v1, ha:coraleda-project-v1 or ha:geda-project-v1\n");

	if (d != NULL)
		lht_dom_uninit(d);
	return -1;
}


static int conf_merge_plug(lht_doc_t *d, rnd_conf_role_t role, const char *path)
{
	lht_err_t err;

	if (pcb_conf_plug_root[role] == NULL) {
		pcb_conf_plug_root[role] = lht_dom_init();
		pcb_conf_plug_root[role]->root = lht_dom_node_alloc(LHT_LIST, "pcb-rnd-conf-v1");
		pcb_conf_plug_root[role]->root->doc = pcb_conf_plug_root[role];
	}
	err = lht_tree_merge(pcb_conf_plug_root[role]->root, d->root);
	lht_dom_uninit(d);
	if (err != 0) {
		rnd_message(RND_MSG_ERROR, "Failed to lihata-merge plugin config %s: %s\n", path, lht_err_str(err));
		return 0;
	}
	return 1;
}

/* Load plugin config files */
static int conf_load_plug_files(rnd_conf_role_t role, const char *dir)
{
	char path[RND_PATH_MAX], *fn;
	int dlen, cnt = 0;
	htsi_entry_t *e;

	if (pcb_conf_plug_root[role] != NULL) {
		lht_dom_uninit(pcb_conf_plug_root[role]);
		pcb_conf_plug_root[role] = NULL;
	}

	if (!conf_files_inited) return 0;

	dlen = strlen(dir);
	memcpy(path, dir, dlen);
	path[dlen] = '/';
	fn = path+dlen+1;

	for (e = htsi_first(&conf_files); e; e = htsi_next(&conf_files, e)) {
		strcpy(fn, e->key);
		if (rnd_file_readable(path)) {
			lht_doc_t *d = conf_load_plug_file(path, 0);
			if (d != NULL) {
				int res = conf_merge_plug(d, role, path);
				if (res) {
					char id[256];
					pcb_snprintf(id, sizeof(id), "%s:%s", rnd_conf_role_name(role), fn);
					rnd_file_loaded_set_at("conf/plugin", id, path, NULL);
				}
				cnt += res;
			}
		}
	}

	return cnt;
}

static int conf_load_plug_interns(rnd_conf_role_t role)
{
	int cnt = 0;
	htsi_entry_t *e;

	if (pcb_conf_plug_root[role] != NULL) {
		lht_dom_uninit(pcb_conf_plug_root[role]);
		pcb_conf_plug_root[role] = NULL;
	}

	if (!conf_files_inited) return 0;
	for (e = htsi_first(&conf_interns); e; e = htsi_next(&conf_interns, e)) {
		lht_doc_t *d = conf_load_plug_file(e->key, 1);
		if (d != NULL) {
			int res = conf_merge_plug(d, role, NULL);
			cnt += res;
		}
	}

	return cnt;
}

rnd_conf_native_type_t rnd_conf_native_type_parse(const char *s)
{
	if (rnd_strcasecmp(s, "string") == 0)     return  RND_CFN_STRING;
	if (rnd_strcasecmp(s, "str") == 0)        return  RND_CFN_STRING;
	if (rnd_strcasecmp(s, "boolean") == 0)    return  RND_CFN_BOOLEAN;
	if (rnd_strcasecmp(s, "bool") == 0)       return  RND_CFN_BOOLEAN;
	if (rnd_strcasecmp(s, "integer") == 0)    return  RND_CFN_INTEGER;
	if (rnd_strcasecmp(s, "real") == 0)       return  RND_CFN_REAL;
	if (rnd_strcasecmp(s, "coord") == 0)      return  RND_CFN_COORD;
	if (rnd_strcasecmp(s, "unit") == 0)       return  RND_CFN_UNIT;
	if (rnd_strcasecmp(s, "color") == 0)      return  RND_CFN_COLOR;
	if (rnd_strcasecmp(s, "list") == 0)       return  RND_CFN_LIST;
	if (rnd_strcasecmp(s, "hlist") == 0)      return  RND_CFN_HLIST;

	return RND_CFN_max;
}


rnd_conf_policy_t rnd_conf_policy_parse(const char *s)
{
	if (rnd_strcasecmp(s, "overwrite") == 0)  return  RND_POL_OVERWRITE;
	if (rnd_strcasecmp(s, "prepend") == 0)    return  RND_POL_PREPEND;
	if (rnd_strcasecmp(s, "append") == 0)     return  RND_POL_APPEND;
	if (rnd_strcasecmp(s, "disable") == 0)    return  RND_POL_DISABLE;
	return RND_POL_invalid;
}

const char *rnd_conf_policy_name(rnd_conf_policy_t p)
{
	switch(p) {
		case RND_POL_OVERWRITE: return "overwrite";
		case RND_POL_PREPEND:   return "prepend";
		case RND_POL_APPEND:    return "append";
		case RND_POL_DISABLE:   return "disable";
		case RND_POL_invalid:   return "(invalid)";
	}
	return "(unknown)";
}

rnd_conf_role_t rnd_conf_role_parse(const char *s)
{
	if (rnd_strcasecmp(s, "internal") == 0)   return RND_CFR_INTERNAL;
	if (rnd_strcasecmp(s, "system") == 0)     return RND_CFR_SYSTEM;
	if (rnd_strcasecmp(s, "defaultpcb") == 0) return RND_CFR_DEFAULTPCB;
	if (rnd_strcasecmp(s, "user") == 0)       return RND_CFR_USER;
	if (rnd_strcasecmp(s, "env") == 0)        return RND_CFR_ENV;
	if (rnd_strcasecmp(s, "project") == 0)    return RND_CFR_PROJECT;
	if (rnd_strcasecmp(s, "design") == 0)     return RND_CFR_DESIGN;
	if (rnd_strcasecmp(s, "cli") == 0)        return RND_CFR_CLI;
	return RND_CFR_invalid;
}

const char *rnd_conf_role_name(rnd_conf_role_t r)
{
	switch(r) {
		case RND_CFR_INTERNAL:    return "internal";
		case RND_CFR_SYSTEM:      return "system";
		case RND_CFR_DEFAULTPCB:  return "defaultpcb";
		case RND_CFR_USER:        return "user";
		case RND_CFR_ENV:         return "env";
		case RND_CFR_PROJECT:     return "project";
		case RND_CFR_DESIGN:      return "design";
		case RND_CFR_CLI:         return "cli";
		case RND_CFR_file:        return "(file)";
		case RND_CFR_binary:      return "(binary)";

		case RND_CFR_max_alloc:
		case RND_CFR_max_real:
		case RND_CFR_invalid:     return "(invalid role)";
	}
	return "(unknown role)";
}


void pcb_conf_extract_poliprio(lht_node_t *root, rnd_conf_policy_t *gpolicy, long *gprio)
{
	long len = strlen(root->name), p = -1;
	char tmp[128];
	char *sprio, *end;
	rnd_conf_policy_t pol;

	if (len >= sizeof(tmp)) {
		pcb_hid_cfg_error(root, "Invalid policy-prio '%s', subtree is ignored\n", root->name);
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
			pcb_hid_cfg_error(root, "Invalid prio in '%s', subtree is ignored\n", root->name);
			return;
		}
	}

	/* convert policy */
	pol = rnd_conf_policy_parse(tmp);
	if (pol == RND_POL_invalid) {
		pcb_hid_cfg_error(root, "Invalid policy in '%s', subtree is ignored\n", root->name);
		return;
	}

	/* by now the syntax is checked; save the output */
	*gpolicy = pol;
	if (p >= 0)
		*gprio = p;
}


int rnd_conf_get_policy_prio(lht_node_t *node, rnd_conf_policy_t *gpolicy, long *gprio)
{
	for(;;node = node->parent) {
		if (node->parent == node->doc->root) {
			pcb_conf_extract_poliprio(node, gpolicy, gprio);
			return 0;
		}
		if (node->parent == NULL)
			return -1;
	}
}

const char *rnd_conf_get_user_conf_name()
{
	return pcphl_conf_user_path;
}

const char *rnd_conf_get_project_conf_name(const char *project_fn, const char *pcb_fn, const char **try)
{
	static char res[RND_PATH_MAX+1];
	static const char *project_name = "project.lht";
	FILE *f;
	char *efn;

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
		end = strrchr(res, RND_DIR_SEPARATOR_C);
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
	f = pcb_fopen_fn(NULL, res, "r", &efn);
	if (f != NULL) {
		fclose(f);
		strncpy(res, efn, sizeof(res)-1);
		res[sizeof(res)-1] = '\0';
		free(efn);
		return res;
	}

	return NULL;
}

int rnd_conf_parse_text(rnd_confitem_t *dst, int idx, rnd_conf_native_type_t type, const char *text, lht_node_t *err_node)
{
	const char *strue[]  = {"true",  "yes",  "on",   "1", NULL};
	const char *sfalse[] = {"false", "no",   "off",  "0", NULL};
	const char **s;
	char *end;
	long l;
	int base = 10;
	double d;
	const rnd_unit_t *u;

	switch(type) {
		case RND_CFN_STRING:
			dst->string[idx] = text;
			break;
		case RND_CFN_BOOLEAN:
			while(isspace(*text)) text++;
			for(s = strue; *s != NULL; s++)
				if (rnd_strncasecmp(*s, text, strlen(*s)) == 0) {
					dst->boolean[idx] = 1;
					return 0;
				}
			for(s = sfalse; *s != NULL; s++)
				if (rnd_strncasecmp(*s, text, strlen(*s)) == 0) {
					dst->boolean[idx] = 0;
					return 0;
				}
			pcb_hid_cfg_error(err_node, "Invalid boolean value: %s\n", text);
			return -1;
		case RND_CFN_INTEGER:
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
			pcb_hid_cfg_error(err_node, "Invalid integer value: %s\n", text);
			return -1;
		case RND_CFN_REAL:
			d = strtod(text, &end);
			if (*end == '\0') {
				dst->real[idx] = d;
				return 0;
			}
			pcb_hid_cfg_error(err_node, "Invalid numeric value: %s\n", text);
			return -1;
		case RND_CFN_COORD:
			{
				rnd_bool succ;
				dst->coord[idx] = pcb_get_value(text, NULL, NULL, &succ);
				if (!succ)
					pcb_hid_cfg_error(err_node, "Invalid numeric value (coordinate): %s\n", text);
			}
			break;
		case RND_CFN_UNIT:
			u = get_unit_struct(text);
			if (u == NULL)
				pcb_hid_cfg_error(err_node, "Invalid unit: %s\n", text);
			else
				dst->unit[idx] = u;
			break;
		case RND_CFN_COLOR:
			if (rnd_color_load_str(&dst->color[idx], text) != 0) {
				pcb_hid_cfg_error(err_node, "Invalid color value: '%s'\n", text);
				return -1;
			}
			break;
		default:
			/* unknown field type registered in the fields hash: internal error */
			return -1;
	}
	return 0;
}

int pcb_conf_merge_patch_text(rnd_conf_native_t *dest, lht_node_t *src, int prio, rnd_conf_policy_t pol)
{
	if ((pol == RND_POL_DISABLE) || (dest->prop[0].prio > prio))
		return 0;

	if (dest->random_flags.read_only) {
		pcb_hid_cfg_error(src, "WARNING: not going to overwrite read-only value %s from a config file\n", dest->hash_path);
		return 0;
	}

	if (rnd_conf_parse_text(&dest->val, 0, dest->type, src->data.text.value, src) == 0) {
		dest->prop[0].prio = prio;
		dest->prop[0].src  = src;
		dest->used         = 1;
		dest->rnd_conf_rev     = rnd_conf_rev;
	}
	return 0;
}

static void conf_insert_arr(rnd_conf_native_t *dest, int offs)
{
#define CASE_PCB_MOVE(typ, fld) \
	case typ: memmove(dest->val.fld+offs, dest->val.fld, dest->used * sizeof(dest->val.fld[0])); return

	memmove(dest->prop+offs, dest->prop, dest->used * sizeof(dest->prop[0]));
	if (dest->used > 0) {
		switch(dest->type) {
			CASE_PCB_MOVE(RND_CFN_STRING, string);
			CASE_PCB_MOVE(RND_CFN_BOOLEAN, boolean);
			CASE_PCB_MOVE(RND_CFN_INTEGER, integer);
			CASE_PCB_MOVE(RND_CFN_REAL, real);
			CASE_PCB_MOVE(RND_CFN_COORD, coord);
			CASE_PCB_MOVE(RND_CFN_UNIT, unit);
			CASE_PCB_MOVE(RND_CFN_COLOR, color);
			CASE_PCB_MOVE(RND_CFN_LIST, list);
			CASE_PCB_MOVE(RND_CFN_HLIST, list);
			case RND_CFN_max: break;
		}
	}
#undef CASE_MOVE
	abort(); /* unhandled type */
}

int pcb_conf_merge_patch_array(rnd_conf_native_t *dest, lht_node_t *src_lst, int prio, rnd_conf_policy_t pol)
{
	lht_node_t *s;
	int res, idx, didx, maxpr;

	if ((pol == RND_POL_DISABLE) || (pol == RND_POL_invalid))
		return 0;

	if (pol == RND_POL_PREPEND) {
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
				case RND_POL_PREPEND:
					if (idx < maxpr)
						didx = idx;
					else
						didx = dest->array_size+1; /* indicate array full */
					break;
				case RND_POL_APPEND:
					didx = dest->used++;
					break;
				case RND_POL_OVERWRITE:
					didx = idx;
				break;
				case RND_POL_DISABLE: case RND_POL_invalid: return 0; /* compiler warning */
			}
/*printf("   didx: %d / %d '%s'\n", didx, dest->array_size, s->data.text.value);*/
			if (didx >= dest->array_size) {
				pcb_hid_cfg_error(s, "Array is already full [%d] of [%d] ignored value: '%s' policy=%d\n", dest->used, dest->array_size, s->data.text.value, pol);
				res = -1;
				break;
			}

			if ((s->data.text.value != NULL) && (rnd_conf_parse_text(&dest->val, didx, dest->type, s->data.text.value, s) == 0)) {
				dest->prop[didx].prio = prio;
				dest->prop[didx].src  = s;
				if (didx >= dest->used)
					dest->used = didx+1;
			}
		}
		else {
			pcb_hid_cfg_error(s, "List item (in an array) must be text\n");
			res = -1;
		}
	}

	return res;
}

int pcb_conf_merge_patch_list(rnd_conf_native_t *dest, lht_node_t *src_lst, int prio, rnd_conf_policy_t pol)
{
	lht_node_t *s, *prev;
	int res = 0;
	rnd_conf_listitem_t *i;

	switch(pol) {
		case RND_POL_DISABLE:
		case RND_POL_invalid:
			return 0;

		case RND_POL_PREPEND:
			for(s = src_lst->data.list.last; s != NULL; s = prev) {
				/* because lihata lists are single linked, need to look up the previous node manually; O(N^2) */
				if (s != src_lst->data.list.first)
					for(prev = src_lst->data.list.first; prev->next != s; prev = prev->next);
				else
					prev = NULL;

				if ((s->type == LHT_TEXT) || ((s->type == LHT_HASH) && (dest->type == RND_CFN_HLIST))) {
					i = calloc(sizeof(rnd_conf_listitem_t), 1);
					i->name = s->name;
					i->val.string = &i->payload;
					i->prop.prio = prio;
					i->prop.src  = s;
				}

				if (s->type == LHT_TEXT) {
					if (rnd_conf_parse_text(&i->val, 0, RND_CFN_STRING, s->data.text.value, s) != 0) {
						free(i);
						continue;
					}
					rnd_conflist_insert(dest->val.list, i);
					dest->used |= 1;
				}
				else if ((s->type == LHT_HASH) && (dest->type == RND_CFN_HLIST)) {
					i->val.any = NULL;
					rnd_conflist_insert(dest->val.list, i);
					dest->used |= 1;
					if ((dest->type == RND_CFN_HLIST) || (s->user_data == NULL)) {
						rnd_conf_hid_global_cb_ptr(dest, i, new_hlist_item_post);
						s->user_data = rnd_cast_f2d((rnd_fptr_t)pcb_conf_merge_patch_list);
					}
				}
				else {
					pcb_hid_cfg_error(s, "List item (on a list, prepend) must be text\n");
					res = -1;
				}
			}
			break;
		case RND_POL_OVERWRITE:
			/* overwrite the whole list: make it empty then append new elements */
			while((i = rnd_conflist_first(dest->val.list)) != NULL) {
				rnd_conflist_remove(i);
				free(i);
			}
			/* fall through */
		case RND_POL_APPEND:
			for(s = src_lst->data.list.first; s != NULL; s = s->next) {
				if ((s->type == LHT_TEXT) || ((s->type == LHT_HASH) && (dest->type == RND_CFN_HLIST))) {
					i = calloc(sizeof(rnd_conf_listitem_t), 1);
					i->name = s->name;
					i->val.string = &i->payload;
					i->prop.prio = prio;
					i->prop.src  = s;
				}
				if (s->type == LHT_TEXT) {
					if (rnd_conf_parse_text(&i->val, 0, RND_CFN_STRING, s->data.text.value, s) != 0) {
						free(i);
						continue;
					}
					rnd_conflist_append(dest->val.list, i);
					dest->used |= 1;
				}
				else if ((s->type == LHT_HASH) && (dest->type == RND_CFN_HLIST)) {
					rnd_conflist_append(dest->val.list, i);
					i->val.any = NULL;
					dest->used |= 1;
					if ((dest->type == RND_CFN_HLIST) && (s->user_data == NULL)) {
						rnd_conf_hid_global_cb_ptr(dest, i, new_hlist_item_post);
						s->user_data = rnd_cast_f2d((rnd_fptr_t)pcb_conf_merge_patch_list);
					}
				}
				else {
					pcb_hid_cfg_error(s, "List item (on a list) must be text\n");
					res = -1;
				}
			}
			break;
	}
	return res;
}

int pcb_conf_merge_patch_recurse(lht_node_t *sect, rnd_conf_role_t role, int default_prio, rnd_conf_policy_t default_policy, const char *path_prefix);

typedef struct conf_ignore_s {
	const char *name;
	int len;
	int warned;
} conf_ignore_t;

static conf_ignore_t conf_ignores[] = {
	/* it is normal to have configuration for plugins and utils not loaded - ignore these */
	{"plugins/", 8, 1},
	{"utils/", 6, 1},

	/* old config paths - warn once and move on */
	{"editor/show_mask", 16, 0},
	{"editor/show_paste", 17, 0},
	{"editor/increments", 17, 0},
	{"design/max_width", 16, 0},
	{"design/max_height", 17, 0},
	{"design/groups", 13, 1},
	{"design/default_layer_name", 25, 1},

	{NULL, 0, 0}
};

/* for security reasons ignore these nodes when coming from a board or project file */
static conf_ignore_t conf_board_ignores[] = {
	{"rc/action_string", 16, 0},
	{"rc/library_shell", 16, 0},
	{"rc/file_command", 15, 0},
	{"rc/font_command", 15, 0},
	{"rc/save_command", 15, 0},
	{"rc/rat_command", 14, 0},
	{NULL, 0, 0}
};

static void conf_warn_unknown_paths(const char *path, lht_node_t *n)
{
	conf_ignore_t *i;

	for(i = conf_ignores; i->name != NULL; i++) {
		if (strncmp(path, i->name, i->len) == 0) {
			if (i->warned)
				return; /* do not warn again */
			i->warned = 1;
			break;
		}
	}
	pcb_hid_cfg_error(n, "conf error: lht->bin conversion: can't find path '%s'\n(it may be an obsolete setting, check your lht)\n", path);
}

/* returns 1 if the config node should be ignored */
static int conf_board_ignore(const char *path, lht_node_t *n)
{
	conf_ignore_t *i;

	for(i = conf_board_ignores; i->name != NULL; i++) {
		if (strncmp(path, i->name, i->len) == 0) {
			if (!i->warned) {
				i->warned = 1;
				pcb_hid_cfg_error(n, "conf error: lht->bin conversion: path '%s' from board or project file\nis ignored (probably due to security considerations)\n", path);
			}
			return 1;
		}
	}
	return 0;
}


int pcb_conf_merge_patch_item(const char *path, lht_node_t *n, rnd_conf_role_t role, int default_prio, rnd_conf_policy_t default_policy)
{
	rnd_conf_native_t *target = rnd_conf_get_field(path);
	int res = 0;

	if ((role == RND_CFR_DESIGN) || (role == RND_CFR_PROJECT)) {
		if (conf_board_ignore(path, n))
			return 0;
	}

	switch(n->type) {
		case LHT_TEXT:
			if (target == NULL)
				conf_warn_unknown_paths(path, n);
			else
				pcb_conf_merge_patch_text(target, n, default_prio, default_policy);
			break;
		case LHT_HASH:
			if (target == NULL) /* no leaf: another level of structs */
				res |= pcb_conf_merge_patch_recurse(n, role, default_prio, default_policy, path);
			else /* leaf: pretend it's text so it gets parsed */
				pcb_conf_merge_patch_text(target, n, default_prio, default_policy);
			break;
		case LHT_LIST:
			if (target == NULL)
				conf_warn_unknown_paths(path, n);
			else if ((target->type == RND_CFN_LIST) || (target->type == RND_CFN_HLIST))
				res |= pcb_conf_merge_patch_list(target, n, default_prio, default_policy);
			else if (target->array_size > 1)
				res |= pcb_conf_merge_patch_array(target, n, default_prio, default_policy);
			else
				pcb_hid_cfg_error(n, "Attempt to initialize a scalar with a list - this node should be a text node\n");
			break;
		case LHT_SYMLINK:
TODO("TODO")
			break;
		case LHT_INVALID_TYPE:
		case LHT_TABLE:
			/* intentionally unhandled types */
			break;
	}
	return res;
}


/* merge main subtree of a patch */
int pcb_conf_merge_patch_recurse(lht_node_t *sect, rnd_conf_role_t role, int default_prio, rnd_conf_policy_t default_policy, const char *path_prefix)
{
	lht_dom_iterator_t it;
	lht_node_t *n;
	char path[256], *pathe;
	char *namee;
	int nl, ppl = strlen(path_prefix), res = 0;

	if (ppl >= sizeof(path)) {
		rnd_message(RND_MSG_ERROR, "Can not merge conf patch: name too long: '%s'\n", path_prefix);
		return -1;
	}

	memcpy(path, path_prefix, ppl);
	path[ppl] = '/';
	pathe = path + ppl + 1;

	for(n = lht_dom_first(&it, sect); n != NULL; n = lht_dom_next(&it)) {
		nl = strlen(n->name);
		if (nl >= (sizeof(path) - (pathe - path))) {
			rnd_message(RND_MSG_ERROR, "Can not merge conf patch: name too long: '%s'\n", n->name);
			return -1;
		}
		memcpy(pathe, n->name, nl);
		namee = pathe+nl;
		*namee = '\0';
		res |= pcb_conf_merge_patch_item(path, n, role, default_prio, default_policy);
	}
	return res;
}

int pcb_conf_merge_patch(lht_node_t *root, rnd_conf_role_t role, long gprio)
{
	rnd_conf_policy_t gpolicy;
	lht_node_t *n;
	lht_dom_iterator_t it;

	if (root->type != LHT_HASH) {
		pcb_hid_cfg_error(root, "patch root should be a hash\n");
		return -1;
	}

	pcb_conf_extract_poliprio(root, &gpolicy, &gprio);

	/* iterate over all hashes and insert them recursively */
	for(n = lht_dom_first(&it, root); n != NULL; n = lht_dom_next(&it))
		if (n->type == LHT_HASH)
			pcb_conf_merge_patch_recurse(n, role, gprio, gpolicy, n->name);

	return 0;
}

typedef struct {
	rnd_conf_role_t role;
	long prio;
	rnd_conf_policy_t policy;
	lht_node_t *subtree;
} merge_subtree_t;

#define GVT(x) vmst_ ## x
#define GVT_ELEM_TYPE merge_subtree_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 64
#define GVT_START_SIZE 32
#define GVT_FUNC
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_impl.h>
#include <genvector/genvector_impl.c>
#include <genvector/genvector_undef.h>

vmst_t merge_subtree; /* automatically initialized to all-zero */

static void add_subtree(rnd_conf_role_t role, lht_node_t *subtree_parent_root, lht_node_t *subtree_root)
{
	merge_subtree_t *m;

	m = vmst_alloc_append(&merge_subtree, 1);
	m->role = role;
	m->prio = rnd_conf_default_prio[role];
	m->policy = RND_POL_invalid;

	pcb_conf_extract_poliprio(subtree_parent_root, &m->policy, &m->prio);
	m->subtree = subtree_root;
}

static int mst_prio_cmp(const void *a, const void *b)
{
	const merge_subtree_t *s1 = a, *s2 = b;
	if (s1->prio > s2->prio)
		return 1;
	return -1;
}

static void conf_merge_all_top(rnd_conf_role_t role, const char *path, lht_node_t *cr)
{
	lht_node_t *r, *r2;
	for(r = cr->data.list.first; r != NULL; r = r->next) {
		if (path != NULL) {
			r2 = lht_tree_path_(r->doc, r, path, 1, 0, NULL);
			if (r2 != NULL)
				add_subtree(role, r, r2);
		}
		else
			add_subtree(role, r, r);
	}
}

int pcb_conf_merge_all(const char *path)
{
	int n, ret = 0;
	vmst_truncate(&merge_subtree, 0);

	for(n = 0; n < RND_CFR_max_real; n++) {
		lht_node_t *cr;
		if (pcb_conf_main_root[n] != NULL) {
			cr = conf_lht_get_confroot(pcb_conf_main_root[n]->root);
			if (cr != NULL)
				conf_merge_all_top(n, path, cr);
		}
		if (pcb_conf_plug_root[n] != NULL) {
			cr = conf_lht_get_confroot(pcb_conf_plug_root[n]->root);
			if (cr != NULL)
				conf_merge_all_top(n, path, cr);
		}
	}

	qsort(merge_subtree.array, vmst_len(&merge_subtree), sizeof(merge_subtree_t), mst_prio_cmp);
	for(n = 0; n < vmst_len(&merge_subtree); n++) {
		if (path != NULL)
			ret |= pcb_conf_merge_patch_item(path, merge_subtree.array[n].subtree, merge_subtree.array[n].role, merge_subtree.array[n].prio, merge_subtree.array[n].policy);
		else
			ret |= pcb_conf_merge_patch(merge_subtree.array[n].subtree, merge_subtree.array[n].role, merge_subtree.array[n].prio);
	}
	return ret;
}

static void conf_field_clear(rnd_conf_native_t *f)
{
	if (strncmp(f->hash_path, "temp", 4) == 0)
		return;
	if (f->used > 0) {
#define clr(field) memset(f->val.field, 0, sizeof(*(f->val.field)) * f->used)
		switch(f->type) {
			case RND_CFN_STRING:      clr(string); break;
			case RND_CFN_BOOLEAN:     clr(boolean); break;
			case RND_CFN_INTEGER:     clr(integer); break;
			case RND_CFN_REAL:        clr(real); break;
			case RND_CFN_COORD:       clr(coord); break;
			case RND_CFN_UNIT:        clr(unit); break;
			case RND_CFN_COLOR:       clr(color); break;
			case RND_CFN_LIST:
			case RND_CFN_HLIST:
				while(rnd_conflist_first(f->val.list)) { /* need to free all items of a list before clearing the main struct */
					rnd_conf_listitem_t *i = rnd_conflist_first(f->val.list);
					rnd_conflist_remove(i);
					free(i);
				}
				clr(list);
				break;
			case RND_CFN_max: break;
		}
		memset(f->prop, 0, sizeof(rnd_confprop_t) * f->used);
#undef clr
	}

	f->used     = 0;
	f->rnd_conf_rev = rnd_conf_rev;
}

int rnd_conf_rev = 0;
void rnd_conf_update(const char *path, int arr_idx)
{
	rnd_conf_native_t *n;

	/* clear memory-bin data first */
	if (path == NULL) {
		htsp_entry_t *e;
		rnd_conf_fields_foreach(e) {
			rnd_conf_hid_global_cb((rnd_conf_native_t *)e->value, -1, val_change_pre);
			rnd_conf_hid_local_cb((rnd_conf_native_t *)e->value, -1, val_change_pre);
			conf_field_clear(e->value);
		}
	}
	else {
		n = rnd_conf_get_field(path);
		if (n == NULL) {
			char *path_, *field;
			path_ = rnd_strdup(path);

			/* It might be an array element - truncate */
			field = strrchr(path_, '[');
			if (field != NULL) {
				arr_idx = atoi(field+1);
				*field = '\0';
				n = rnd_conf_get_field(path_);
			}

			/* if a valid node is found, update it */
			if (n != NULL)
				rnd_conf_update(path_, arr_idx);

			free(path_);
			return;
		}
		conf_field_clear(n);
		rnd_conf_hid_global_cb(n, arr_idx, val_change_pre);
		rnd_conf_hid_local_cb(n, arr_idx, val_change_pre);
	}

	/* merge all memory-lht data to memory-bin */
	pcb_conf_merge_all(path);
	if (rnd_conf_core_postproc != NULL)
		rnd_conf_core_postproc();
	pcbhl_conf_postproc();

	if (path == NULL) {
		htsp_entry_t *e;
		rnd_conf_fields_foreach(e) {
			rnd_conf_hid_local_cb((rnd_conf_native_t *)e->value, -1, val_change_post);
			rnd_conf_hid_global_cb((rnd_conf_native_t *)e->value, -1, val_change_post);
		}
	}
	else {
		rnd_conf_hid_local_cb(n, arr_idx, val_change_post);
		rnd_conf_hid_global_cb(n, arr_idx, val_change_post);
	}
	rnd_conf_rev++;
}

#define POL_ANY 63

static lht_node_t *conf_lht_get_first_(lht_node_t *cwd, rnd_conf_policy_t pol, int create)
{
	lht_node_t *ov; /* normally the "overwrite" node */

	/* assume root is a li and add to the first hash */
	cwd = conf_lht_get_confroot(cwd);
	if (cwd == NULL)
		return NULL;


	ov = cwd->data.list.first;
	if (pol != POL_ANY) {
		for(; ov != NULL; ov = ov->next)
			if (rnd_conf_policy_parse(ov->name) == pol)
				break;
	}
	else
		pol = RND_POL_OVERWRITE; /* this is what is created by default */

	if (ov == NULL) {
		lht_node_t *new_ov;
		if (!create)
			return NULL;
		new_ov = lht_dom_node_alloc(LHT_HASH, rnd_conf_policy_name(pol));
		lht_dom_list_append(cwd, new_ov);
		ov = new_ov;
	}
	else if (ov->type != LHT_HASH)
		return NULL;

	return ov;
}

lht_node_t *rnd_conf_lht_get_first(rnd_conf_role_t target, int create)
{
	assert(target != RND_CFR_invalid);
	assert(target >= 0);
	assert(target < RND_CFR_max_alloc);
	if (pcb_conf_main_root[target] == NULL)
		return NULL;
	return conf_lht_get_first_(pcb_conf_main_root[target]->root, POL_ANY, create);
}

lht_node_t *pcb_conf_lht_get_first_plug(rnd_conf_role_t target, int create)
{
	assert(target != RND_CFR_invalid);
	assert(target >= 0);
	assert(target < RND_CFR_max_alloc);
	if (pcb_conf_plug_root[target] == NULL)
		return NULL;
	return conf_lht_get_first_(pcb_conf_plug_root[target]->root, POL_ANY, create);
}

lht_node_t *rnd_conf_lht_get_first_pol(rnd_conf_role_t target, rnd_conf_policy_t pol, int create)
{
	assert(target != RND_CFR_invalid);
	assert(target >= 0);
	assert(target < RND_CFR_max_alloc);
	if (pcb_conf_main_root[target] == NULL)
		return NULL;
	return conf_lht_get_first_(pcb_conf_main_root[target]->root, pol, create);
}

static lht_node_t *conf_lht_get_at_(rnd_conf_role_t target, const char *conf_path, const char *lht_path, int allow_plug, int create)
{
	lht_node_t *n, *r = NULL;

	n = rnd_conf_lht_get_first(target, create);
	if (n == NULL)
		return NULL;

	r = lht_tree_path_(n->doc, n, lht_path, 1, 0, NULL);
	if ((r == NULL) && (create)) {
		rnd_conf_set_dry(target, conf_path, -1, "", RND_POL_OVERWRITE, 0);
		r = lht_tree_path_(n->doc, n, lht_path, 1, 0, NULL);
	}

	if ((r == NULL) && (allow_plug) && (pcb_conf_plug_root[target] != NULL)) {
		for(n = pcb_conf_plug_root[target]->root->data.list.first; n != NULL; n = n->next) {
			r = lht_tree_path_(n->doc, n, lht_path, 1, 0, NULL);
			if (r != NULL)
				return r;
		}
	}

	return r;
}

lht_node_t *rnd_conf_lht_get_at_mainplug(rnd_conf_role_t target, const char *path, int allow_plug, int create)
{
	lht_node_t *r;
	char *pc, *end;
	if (pcb_conf_main_root[target] == NULL) {
		if (!create)
			return NULL;
		rnd_conf_reset(target, "<pcb_conf_lht_get_at>");
	}
	end = strchr(path, '[');
	if (end == NULL) {
		r = conf_lht_get_at_(target, path, path, allow_plug, create);
	}
	else {
		/* lihata syntax differs from conf syntax in array indexing */
		pc = rnd_strdup(path);
		pc[end-path] = '/';
		end = strchr(pc+(end-path), ']');
		if (end != NULL)
			*end = '\0';
		r = conf_lht_get_at_(target, path, pc, allow_plug, create);
		free(pc);
	}
	return r;
}

lht_node_t *rnd_conf_lht_get_at(rnd_conf_role_t target, const char *path, int create)
{
	return rnd_conf_lht_get_at_mainplug(target, path, 0, create);
}


void rnd_conf_load_all(const char *project_fn, const char *pcb_fn)
{
	const char *pc, *try;

	/* the ultimate fallback: all essential values are built in the executable
	   on a low priority */
	rnd_conf_load_as(RND_CFR_INTERNAL, pcb_conf_internal, 1);

	/* load config files */
	rnd_conf_load_as(RND_CFR_SYSTEM, pcbhl_conf_sys_path, 0);
	rnd_conf_load_as(RND_CFR_USER, pcphl_conf_user_path, 0);
	pc = rnd_conf_get_project_conf_name(project_fn, pcb_fn, &try);
	if (pc != NULL)
		rnd_conf_load_as(RND_CFR_PROJECT, pc, 0);
	pcb_conf_merge_all(NULL);

	/* create the user config (in-memory-lht) if it does not exist on disk;
	   this is needed so if the user makes config changes from the GUI things
	   get saved. */
	if (pcb_conf_main_root[RND_CFR_USER] == NULL)
		rnd_conf_reset(RND_CFR_USER, pcphl_conf_user_path);

	rnd_conf_in_production = 1;
}

void rnd_conf_load_extra(const char *project_fn, const char *pcb_fn)
{
	int cnt;
	cnt = conf_load_plug_files(RND_CFR_SYSTEM, pcbhl_conf_sysdir_path);
	cnt += conf_load_plug_files(RND_CFR_USER, pcbhl_conf_userdir_path);
	cnt += conf_load_plug_interns(RND_CFR_INTERNAL);
	if (cnt > 0)
		pcb_conf_merge_all(NULL);
}


void rnd_conf_load_project(const char *project_fn, const char *pcb_fn)
{
	const char *pc, *try;

	assert((project_fn != NULL) || (pcb_fn != NULL));

	pc = rnd_conf_get_project_conf_name(project_fn, pcb_fn, &try);
	if (pc != NULL)
		if (rnd_conf_load_as(RND_CFR_PROJECT, pc, 0) != 0)
			pc = NULL;
	if (pc == NULL)
		rnd_conf_reset(RND_CFR_PROJECT, "<pcb_conf_load_project>");
	rnd_conf_update(NULL, -1);
}

rnd_conf_native_t *rnd_conf_reg_field_(void *value, int array_size, rnd_conf_native_type_t type, const char *path, const char *desc, rnd_conf_flag_t flags)
{
	rnd_conf_native_t *node;

	if (rnd_conf_fields == NULL) {
		rnd_conf_fields = htsp_alloc(strhash, strkeyeq);
		assert(rnd_conf_fields != NULL);
	}
	assert(array_size >= 1);

	assert(htsp_get(rnd_conf_fields, path) == NULL);

	node = calloc(sizeof(rnd_conf_native_t), 1);
	node->array_size  = array_size;
	node->type        = type;
	node->val.any     = value;
	node->prop        = calloc(sizeof(rnd_confprop_t), array_size);
	node->description = desc;
	node->hash_path   = path;
	node->flags       = flags;
	vtp0_init(&(node->hid_data));
	vtp0_init(&(node->hid_callbacks));

	htsp_set(rnd_conf_fields, (char *)path, node);
	rnd_conf_hid_global_cb(node, -1, new_item_post);

	return node;
}

int pcb_conf_grow_list_(rnd_conf_native_t *node, int new_size)
{
	void *v;
	int old_size = node->array_size;

	if (new_size == old_size)
		return 0;
	if (new_size < old_size)
		return -1;

	v = realloc(node->prop, sizeof(rnd_confprop_t) * new_size);
	if (v == NULL)
		return -1;

	node->array_size = new_size;
	node->prop = v;
	memset(node->prop + old_size, 0, sizeof(rnd_confprop_t) * (new_size - old_size));
	return 0;
}

void pcb_conf_free_native(rnd_conf_native_t *node)
{
	if ((node->type == RND_CFN_LIST) || (node->type == RND_CFN_HLIST)) {
		while(rnd_conflist_first(node->val.list) != NULL) {
			rnd_conf_listitem_t *first = rnd_conflist_first(node->val.list);
			rnd_conflist_remove(first);
			free(first);
		}
	}

	if (node->random_flags.dyn_hash_path)
		free((char *)node->hash_path);

	if (node->random_flags.dyn_desc)
		free((char *)node->description);

	if (node->random_flags.dyn_val)
		free(node->val.any);

	vtp0_uninit(&(node->hid_data));
	vtp0_uninit(&(node->hid_callbacks));
	free(node->prop);
	free(node);
}

void rnd_conf_unreg_fields(const char *prefix)
{
	int len = strlen(prefix);
	htsp_entry_t *e;

	assert(prefix[len-1] == '/');

	rnd_conf_fields_foreach(e) {
		if (strncmp(e->key, prefix, len) == 0) {
			pcb_conf_free_native(e->value);
			htsp_delentry(rnd_conf_fields, e);
		}
	}
}

void rnd_conf_unreg_field(rnd_conf_native_t *field)
{
	htsp_pop(rnd_conf_fields, field->hash_path);
	pcb_conf_free_native(field);
}


rnd_conf_native_t *rnd_conf_get_field(const char *path)
{
	return htsp_get(rnd_conf_fields, path);
}

int rnd_conf_set_dry(rnd_conf_role_t target, const char *path_, int arr_idx, const char *new_val, rnd_conf_policy_t pol, int mkdirp)
{
	char *path, *basename, *next, *last, *sidx;
	rnd_conf_native_t *nat;
	lht_node_t *cwd, *nn;
	lht_node_type_t ty;
	int idx = -1;

	/* Remove in overwrite only */
	if ((new_val == NULL) && (pol != RND_POL_OVERWRITE))
		return -1;

	if (mkdirp)
		rnd_conf_lht_get_first_pol(target, pol, 1);

	path = rnd_strdup(path_);
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

	nat = rnd_conf_get_field(path);
	if (nat == NULL) {
		free(path);
		return -1;
	}


	if (pcb_conf_main_root[target] == NULL) {
		free(path);
		return -1;
	}
	if (pol == RND_POL_DISABLE) {
		free(path);
		return 0;
	}
	if ((pol != RND_POL_OVERWRITE) && (idx >= 0)) {
		free(path);
		return -1;
	}

	cwd = rnd_conf_lht_get_first_pol(target, pol, 0);
	if (cwd == NULL) {
		free(path);
		return -1;
	}

	if (idx >= nat->array_size) {
		rnd_message(RND_MSG_ERROR, "Error: can't rnd_conf_set() %s[%d]: %d is beyond the end of the array (%d)\n", path, idx, idx, nat->array_size);
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

		nn = lht_tree_path_(pcb_conf_main_root[target], cwd, last, 1, 0, NULL);
		if (nn == NULL) {
			if (new_val == NULL) {
				free(path);
				return 0;
			}
			if (pcb_conf_main_root_lock[target]) {
				rnd_message(RND_MSG_WARNING, "WARNING: can't set config item %s because target in-memory lihata does not have the node and is tree-locked\n", path_);
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
	if ((nat->array_size > 1) || (nat->type == RND_CFN_LIST) || (nat->type == RND_CFN_HLIST))
		ty = LHT_LIST;
	else
		ty = LHT_TEXT;

	nn = lht_tree_path_(pcb_conf_main_root[target], cwd, basename, 1, 0, NULL);
	if (nn == NULL) {
		if (pcb_conf_main_root_lock[target]) {
			free(path);
			return -1;
		}
		nn = lht_dom_node_alloc(ty, basename);
		if (cwd->type != LHT_HASH) {
			rnd_message(RND_MSG_ERROR, "Expected HASH conf subtree '%s' (in path '%s'); cleaning up broken conf, check your config sources!\n", cwd->name, path);
			lht_tree_del(cwd);
			free(path);
			return -1;
		}
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

		if (new_val != NULL)
			nn = lht_dom_node_alloc(LHT_TEXT, "");

		if (pol == RND_POL_OVERWRITE) {
			if (idx == -1) {
				/* empty the list so that we insert to an empty list which is overwriting the list */
				while(cwd->data.list.first != NULL)
					lht_tree_del(cwd->data.list.first);
				if (new_val != NULL)
					lht_dom_list_append(cwd, nn);
			}
			else {
				lht_node_t *old = lht_tree_list_nth(cwd, idx);
				if (old != NULL) {
					/* the list is large enough already: overwrite the element at idx */
					if (new_val == NULL) {
						err = lht_tree_del(old);
						free(path);
						return 0;
					}
					else
						err = lht_tree_list_replace_child(cwd, old, nn);
				}
				else if (new_val == NULL) {
					free(path);
					return 0;
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
		else if ((pol == RND_POL_PREPEND) || (pol == RND_POL_OVERWRITE))
			err = lht_dom_list_insert(cwd, nn);
		else if (pol == RND_POL_APPEND)
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

	if (new_val != NULL) {
		if (cwd->data.text.value != NULL)
			free(cwd->data.text.value);

		cwd->data.text.value = rnd_strdup(new_val);
		cwd->file_name = pcb_conf_main_root[target]->active_file;
	}
	else
		lht_tree_del(cwd);

	pcb_conf_lht_dirty[target]++;

	free(path);
	return 0;
}

int rnd_conf_set(rnd_conf_role_t target, const char *path, int arr_idx, const char *new_val, rnd_conf_policy_t pol)
{
	int res;
	res = rnd_conf_set_dry(target, path, arr_idx, new_val, pol, 1);
	if (res < 0)
		return res;
	rnd_conf_update(path, arr_idx);
	return 0;
}

int rnd_conf_del(rnd_conf_role_t target, const char *path, int arr_idx)
{
	int res;
	res = rnd_conf_set_dry(target, path, arr_idx, NULL, RND_POL_OVERWRITE, 0);
	if (res < 0)
		return res;
	rnd_conf_update(path, arr_idx);
	return 0;
}

int rnd_conf_grow(const char *path, int new_size)
{
	rnd_conf_native_t *nat = rnd_conf_get_field(path);
	if (nat == NULL)
		return -1;
	return pcb_conf_grow_list_(nat, new_size);
}

int rnd_conf_set_native(rnd_conf_native_t *field, int arr_idx, const char *new_val)
{
	lht_node_t *node;

	if (arr_idx >= field->used)
		return -1;

	node = field->prop[arr_idx].src;

	if (node->data.text.value != NULL)
		free(node->data.text.value);

	node->data.text.value = rnd_strdup(new_val);
	return 0;
}


int rnd_conf_set_from_cli(const char *prefix, const char *arg_, const char *val, const char **why)
{
	char *arg = NULL, *op, *s;
	const char *sc;
	rnd_conf_policy_t pol = RND_POL_OVERWRITE;
	int ret;

	if (prefix != NULL) {
		for(sc = arg_; (*sc != '=') && (*sc != '\0'); sc++)
			if (*sc == '/')
				arg = rnd_strdup(arg_); /* full path, don't use prefix */

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
		arg = rnd_strdup(arg_);

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
			rnd_conf_native_t *n = rnd_conf_get_field(arg);
			if ((n == NULL) || (n->type != RND_CFN_BOOLEAN)) {
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
				case '+': pol = RND_POL_APPEND; *op = '\0'; break;
				case '^': pol = RND_POL_PREPEND; *op = '\0'; break;
			}
		}
	}

	/* now that we have a clean path (arg) and a value, try to set the config */
	rnd_conf_lht_get_first_pol(RND_CFR_CLI, pol, 1); /* make sure the root for the given policy is created */
	ret = rnd_conf_set(RND_CFR_CLI, arg, -1, val, pol);
	if (ret != 0)
		*why = "invalid config path";

	free(arg);
	return ret;
}

int rnd_conf_parse_arguments(const char *prefix, int *argc, char ***argv)
{
	int n, dst;

	for(n = 0, dst = 0; n < *argc; n++,dst++) {
		char *a = (*argv)[n];
		const char *why;
		int try_again = -1;

		if (a[0] == '-') a++;
		if (a[0] == '-') a++;

		try_again = rnd_conf_set_from_cli(prefix, a, NULL, &why);
		if (try_again && (n < (*argc) - 1)) {
			try_again = rnd_conf_set_from_cli(prefix, a, (*argv)[n+1], &why);
			if (!try_again)
				n++;
		}
		if (try_again)
			(*argv)[dst] = (*argv)[n];
		else
			dst--;
	}
	*argc = dst;
	return 0;
}

void rnd_conf_usage(const char *prefix, void (*print)(const char *name, const char *help))
{
	htsp_entry_t *e;
	int pl = (prefix == NULL ? 0 : strlen(prefix));

	rnd_conf_fields_foreach(e) {
		if ((prefix == NULL) || (strncmp(prefix, e->key, pl) == 0)) {
			rnd_conf_native_t *n = e->value;
			if (n->flags & RND_CFF_USAGE) {
				int kl = strlen(n->hash_path);
				char *s, *name = malloc(kl+32);
				const char *sc;
				for(sc = n->hash_path + pl; *sc == '/'; sc++) ;
				strcpy(name, sc);
				for(s = name; *s != '\0'; s++)
					if (*s == '_')
						*s = '-';
				switch(n->type) {
					case RND_CFN_BOOLEAN: /* implicit true */ break;
					case RND_CFN_STRING:  strcpy(s, " str"); break;
					case RND_CFN_INTEGER: strcpy(s, " int"); break;
					case RND_CFN_REAL:    strcpy(s, " num"); break;
					case RND_CFN_COORD:   strcpy(s, " coord"); break;
					case RND_CFN_UNIT:    strcpy(s, " unit"); break;
					case RND_CFN_COLOR:   strcpy(s, " color"); break;
					case RND_CFN_LIST:    strcpy(s, " <list>"); break;
					case RND_CFN_HLIST:   strcpy(s, " <hlist>"); break;
					case RND_CFN_max:     break;
				}
				print(name, n->description);
				free(name);
			}
		}
	}
}

int rnd_conf_replace_subtree(rnd_conf_role_t dst_role, const char *dst_path, rnd_conf_role_t src_role, const char *src_path)
{
	lht_node_t *dst = rnd_conf_lht_get_at(dst_role, dst_path, 1);
	lht_node_t *src, *new_src = NULL;

	if (src_role == RND_CFR_binary) {
		char *name;
		lht_node_t *ch = NULL;
		int isarr, i;
		rnd_conf_native_t *n = rnd_conf_get_field(src_path);

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
			if ((n->type == RND_CFN_LIST) || (n->type == RND_CFN_HLIST)) {
				rnd_conf_listitem_t *it;
				ch = lht_dom_node_alloc(LHT_LIST, name+1);
				for(it = rnd_conflist_first(n->val.list); it != NULL; it = rnd_conflist_next(it)) {
					lht_node_t *txt;
					txt = lht_dom_node_alloc(LHT_TEXT, "");
					txt->data.text.value = rnd_strdup(it->payload);
					lht_dom_list_append(ch, txt);
				}
			}
			else {
				rnd_conf_print_native_field((rnd_conf_pfn)pcb_append_printf, &s, 0, &n->val, n->type, n->prop+i, i);
				ch = lht_dom_node_alloc(LHT_TEXT, isarr ? "" : name+1);
				ch->data.text.value = s.array;
				if (isarr)
					lht_dom_list_append(src, ch);
				s.array = NULL;
				gds_uninit(&s);
			}
		}

		if (!isarr)
			src = ch;
	}
	else
		src = rnd_conf_lht_get_at(src_role, src_path, 0);

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

	pcb_conf_lht_dirty[dst_role]++;

	lht_tree_del(dst);
	if (src_role == RND_CFR_binary)
		lht_dom_node_free(src);
	return 0;

	err:;
	if (src_role == RND_CFR_binary)
		lht_dom_node_free(src);
	if (new_src != NULL)
		lht_dom_node_free(new_src);
	return -1;
}


int rnd_conf_save_file(rnd_hidlib_t *hidlib, const char *project_fn, const char *pcb_fn, rnd_conf_role_t role, const char *fn)
{
	int fail = 1;
	lht_node_t *r = rnd_conf_lht_get_first(role, 0);
	const char *try;
	char *efn;

	/* do not save if there's no change */
	if (pcb_conf_lht_dirty[role] == 0)
		return 0;

	if (fn == NULL) {
		switch(role) {
			case RND_CFR_USER:
				fn = pcphl_conf_user_path;
				break;
			case RND_CFR_PROJECT:
				fn = rnd_conf_get_project_conf_name(project_fn, pcb_fn, &try);
				if (fn == NULL) {
					rnd_message(RND_MSG_ERROR, "Error: can not save config to project file: %s does not exist - please create an empty file there first\n", try);
					return -1;
				}
				break;
			default: return -1;
		}
	}

	if (r != NULL) {
		FILE *f;

		f = pcb_fopen_fn(hidlib, fn, "w", &efn);
		if ((f == NULL) && (role == RND_CFR_USER)) {
			/* create the directory and try again */
			char *path = NULL, *end;
			
			if (efn != NULL)
				path = rnd_strdup(efn);
			else
				path = pcb_build_fn(hidlib, fn);

			if (path == NULL) {
				rnd_message(RND_MSG_ERROR, "Error: failed to calculate the project file name (board file name or allocation error)\n");
				fclose(f);
				return -1;
			}

			end = strrchr(path, '/');
			if (end != NULL) {
				*end = '\0';
				if (pcb_mkdir(NULL, path, 0755) == 0) {
					rnd_message(RND_MSG_INFO, "Created directory %s for saving %s\n", path, fn);
					*end = '/';
					f = pcb_fopen(hidlib, path, "w");
				}
				else
					rnd_message(RND_MSG_ERROR, "Error: failed to create directory %s for saving %s\n", path, efn);
			}
			free(path);
		}

		if (f != NULL) {
TODO("CONF: a project file needs to be loaded, merged, then written (to preserve non-config nodes)")
			lht_dom_export(r->doc->root, f, "");
			fail = 0;
			pcb_conf_lht_dirty[role] = 0;
			fclose(f);
		}
		else
			rnd_message(RND_MSG_ERROR, "Error: can't save config to %s - can't open the file for write\n", fn);
	}

	free(efn);
	return fail;
}

int rnd_conf_export_to_file(rnd_hidlib_t *hidlib, const char *fn, rnd_conf_role_t role, const char *conf_path)
{
	lht_node_t *at = rnd_conf_lht_get_at(role, conf_path, 0);
	lht_err_t r;
	FILE *f;

	if (at == NULL)
		return -1;

	f = pcb_fopen(hidlib, fn, "w");
	if (f == NULL)
		return -1;

	r = lht_dom_export(at, f, "");
	fclose(f);
	return r;
}


rnd_conf_listitem_t *rnd_conf_list_first_str(rnd_conflist_t *list, const char **item_str, int *idx)
{
	rnd_conf_listitem_t *item_li;
	item_li = rnd_conflist_first(list);
	if (item_li == NULL)
		return NULL;
	if (item_li->type == RND_CFN_STRING) {
		*item_str = item_li->val.string[0];
		return item_li;
	}
	return rnd_conf_list_next_str(item_li, item_str, idx);
}

rnd_conf_listitem_t *rnd_conf_list_next_str(rnd_conf_listitem_t *item_li, const char **item_str, int *idx)
{
	while((item_li = rnd_conflist_next(item_li)) != NULL) {
		(*idx)++;
		if (item_li->type != RND_CFN_STRING)
			continue;
		/* found next string */
		*item_str = item_li->val.string[0];
		return item_li;
	}
	/* found end of the list */
	(*idx)++;
	*item_str = NULL;
	return item_li;
}

const char *rnd_conf_concat_strlist(const rnd_conflist_t *lst, gds_t *buff, int *inited, char sep)
{
	int n;
	rnd_conf_listitem_t *ci;

	if ((inited == NULL) || (!*inited)) {
		gds_init(buff);
		if (inited != NULL)
			*inited = 1;
	}
	else
		gds_truncate(buff, 0);

	for (n = 0, ci = rnd_conflist_first((rnd_conflist_t *)lst); ci != NULL; ci = rnd_conflist_next(ci), n++) {
		const char *p = ci->val.string[0];
		if (ci->type != RND_CFN_STRING)
			continue;
		if (n > 0)
			gds_append(buff, sep);
		gds_append_str(buff, p);
	}
	return buff->array;
}

void rnd_conf_lock(rnd_conf_role_t target)
{
	pcb_conf_main_root_lock[target] = 1;
}

void rnd_conf_unlock(rnd_conf_role_t target)
{
	pcb_conf_main_root_lock[target] = 0;
}

int rnd_conf_islocked(rnd_conf_role_t target)
{
	return pcb_conf_main_root_lock[target];
}

int rnd_conf_isdirty(rnd_conf_role_t target)
{
	return pcb_conf_lht_dirty[target];
}

void rnd_conf_makedirty(rnd_conf_role_t target)
{
	pcb_conf_lht_dirty[target]++;
}
rnd_conf_role_t rnd_conf_lookup_role(const lht_node_t *nd)
{
	rnd_conf_role_t r;
	for(r = 0; r < RND_CFR_max_real; r++)
		if (pcb_conf_main_root[r] == nd->doc)
			return r;

	for(r = 0; r < RND_CFR_max_real; r++)
		if (pcb_conf_plug_root[r] == nd->doc)
			return r;

	return RND_CFR_invalid;
}

void rnd_conf_reset(rnd_conf_role_t target, const char *source_fn)
{
	lht_node_t *n;

	if (pcb_conf_main_root[target] != NULL)
		lht_dom_uninit(pcb_conf_main_root[target]);

	pcb_conf_main_root[target] = lht_dom_init();
	lht_dom_loc_newfile(pcb_conf_main_root[target], source_fn);
	pcb_conf_main_root[target]->root = lht_dom_node_alloc(LHT_LIST, pcb_conf_list_name);
	pcb_conf_main_root[target]->root->doc = pcb_conf_main_root[target];
	n = lht_dom_node_alloc(LHT_HASH, "overwrite");
	lht_dom_list_insert(pcb_conf_main_root[target]->root, n);
}

/*****************/
static int needs_braces(const char *s)
{
	for(; *s != '\0'; s++)
		if (!isalnum(*s) && (*s != '_') && (*s != '-') && (*s != '+') && (*s != '/')
										 && (*s != ':') && (*s != '.') && (*s != ',') && (*s != '$')
										 && (*s != '(') && (*s != ')') && (*s != '~') && (*s != '#'))
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

int rnd_conf_print_native_field(rnd_conf_pfn pfn, void *ctx, int verbose, rnd_confitem_t *val, rnd_conf_native_type_t type, rnd_confprop_t *prop, int idx)
{
	int ret = 0;
	switch(type) {
		case RND_CFN_STRING:  print_str_or_null(pfn, ctx, verbose, val->string[idx], val->string[idx]); break;
		case RND_CFN_BOOLEAN: ret += pfn(ctx, "%d", val->boolean[idx]); break;
		case RND_CFN_INTEGER: ret += pfn(ctx, "%ld", val->integer[idx]); break;
		case RND_CFN_REAL:    ret += pfn(ctx, "%f", val->real[idx]); break;
		case RND_CFN_COORD:   ret += pfn(ctx, "%$mS", val->coord[idx]); break;
		case RND_CFN_UNIT:    print_str_or_null(pfn, ctx, verbose, val->unit[idx], val->unit[idx]->suffix); break;
		case RND_CFN_COLOR:   print_str_or_null(pfn, ctx, verbose, val->color[idx].str, val->color[idx].str); break;
		case RND_CFN_LIST:
		case RND_CFN_HLIST:
			{
				rnd_conf_listitem_t *n;
				if (rnd_conflist_length(val->list) > 0) {
					ret += pfn(ctx, "{");
					for(n = rnd_conflist_first(val->list); n != NULL; n = rnd_conflist_next(n)) {
						rnd_conf_print_native_field(pfn, ctx, verbose, &n->val, n->type, &n->prop, 0);
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
			case RND_CFN_max: break;
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

int rnd_conf_print_native(rnd_conf_pfn pfn, void *ctx, const char * prefix, int verbose, rnd_conf_native_t *node)
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
			ret += rnd_conf_print_native_field(pfn, ctx, verbose, &node->val, node->type, node->prop, n);
			if (verbose)
				ret += pfn(ctx, " conf_rev=%d\n", node->rnd_conf_rev);
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
		ret += rnd_conf_print_native_field(pfn, ctx, verbose, &node->val, node->type, node->prop, 0);
		if (verbose)
			ret += pfn(ctx, " conf_rev=%d\n", node->rnd_conf_rev);
	}
	return ret;
}


/****************/

void rnd_conf_init(void)
{
	rnd_conf_reset(RND_CFR_ENV, "<environment-variables>");
	rnd_conf_reset(RND_CFR_CLI, "<commandline>");
	rnd_conf_reset(RND_CFR_DESIGN, "<null-design>");

	rnd_conf_reset(RND_CFR_file, "<pcb_conf_init>");
	rnd_conf_reset(RND_CFR_binary, "<pcb_conf_init>");
}

void rnd_conf_uninit(void)
{
	int n;
	htsp_entry_t *e;

	rnd_conf_fields_foreach(e) {
		if (strncmp(e->key, "plugins/", 8) == 0)
			fprintf(stderr, "pcb-rnd conf ERROR: conf node '%s' is not unregistered\n", e->key);
	}

	rnd_conf_pcb_hid_uninit();

	for(n = 0; n < RND_CFR_max_alloc; n++) {
		if (pcb_conf_main_root[n] != NULL)
			lht_dom_uninit(pcb_conf_main_root[n]);
		if (pcb_conf_plug_root[n] != NULL)
			lht_dom_uninit(pcb_conf_plug_root[n]);
	}

	rnd_conf_fields_foreach(e) {
		pcb_conf_free_native(e->value);
		htsp_delentry(rnd_conf_fields, e);
	}
	htsp_free(rnd_conf_fields);

	vmst_uninit(&merge_subtree);
	rnd_conf_files_uninit();
}

void rnd_conf_setf(rnd_conf_role_t role, const char *path, int idx, const char *fmt, ...)
{
	char *tmp;
	va_list ap;

	va_start(ap, fmt);
	tmp = pcb_strdup_vprintf(fmt, ap);
	va_end(ap);

	rnd_conf_set(role, path, idx, tmp, RND_POL_OVERWRITE);

	free(tmp);
}

int rnd_conf_resolve(rnd_conf_resolve_t *res)
{
	res->nat = rnd_conf_get_field(res->path);
	if (res->nat == NULL)
		return 0;
	if ((res->type != res->nat->type) || (!res->allow_array && (res->nat->array_size > 1))) {
		res->nat = NULL;
		return 0;
	}
	return 1;
}

int rnd_conf_resolve_all(rnd_conf_resolve_t *res)
{
	int cnt = 0;
	for(; res->path != NULL; res++)
		cnt += rnd_conf_resolve(res);
	return cnt;
}

void rnd_conf_ro(const char *path)
{
	rnd_conf_native_t *n = rnd_conf_get_field(path);
	if (n != NULL) {
		n->used = 1;
		n->random_flags.read_only = 1;
	}
}

#include "conf_regfile.c"
