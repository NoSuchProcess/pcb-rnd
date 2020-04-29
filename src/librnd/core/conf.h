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

#ifndef RND_CONF_H
#define RND_CONF_H
#include <librnd/config.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/pcb-printf.h>
#include <liblihata/lihata.h>
#include <liblihata/dom.h>
#include <genvector/vtp0.h>
#include <librnd/core/unit.h>

typedef union rnd_confitem_u rnd_confitem_t ;
typedef struct conf_listitem_s rnd_conf_listitem_t;

#include <librnd/core/list_conf.h>

extern int rnd_conf_rev; /* increased by one each time there's a change in any of the config binaries */

typedef enum {
	RND_POL_PREPEND,
	RND_POL_APPEND,
	RND_POL_OVERWRITE,
	RND_POL_DISABLE,
	RND_POL_invalid
} rnd_conf_policy_t;

typedef enum { /* bitfield */
	RND_CFF_USAGE = 1    /* settings should be printed in usage help */
} rnd_conf_flag_t;


typedef const char *      RND_CFT_STRING;
typedef int               RND_CFT_BOOLEAN;
typedef long              RND_CFT_INTEGER;
typedef double            RND_CFT_REAL;
typedef rnd_coord_t       RND_CFT_COORD;
typedef pcb_unit_t *      RND_CFT_UNIT;
typedef rnd_color_t       RND_CFT_COLOR;
typedef rnd_conflist_t    RND_CFT_LIST;
typedef rnd_conflist_t    RND_CFT_HLIST;

typedef enum {
	RND_CFN_STRING,
	RND_CFN_BOOLEAN,
	RND_CFN_INTEGER,     /* signed long */
	RND_CFN_REAL,        /* double */
	RND_CFN_COORD,
	RND_CFN_UNIT,
	RND_CFN_COLOR,
	RND_CFN_LIST,
	RND_CFN_HLIST,
	RND_CFN_max
} rnd_conf_native_type_t;

union rnd_confitem_u {
	const char **string;
	int *boolean;
	long *integer;
	double *real;
	rnd_coord_t *coord;
	const pcb_unit_t **unit;
	rnd_color_t *color;
	rnd_conflist_t *list;
	void *any;
};

typedef struct {
	int prio;
	lht_node_t *src;
} rnd_confprop_t;

typedef struct {
	/* static fields defined by the macros */
	const char *description;
	const char *hash_path;     /* points to the hash key once its added in the hash (else: NULL) */
	int array_size;
	rnd_conf_native_type_t type;
	rnd_conf_flag_t flags;
	struct {
		unsigned io_pcb_no_attrib:1;
		unsigned read_only:1;         /* set by conf_core, has no lihata, should not be overwritten */
		unsigned dyn_hash_path:1;     /* free(->hash_path) on node destroy */
		unsigned dyn_desc:1;          /* free(->description) on node destroy */
		unsigned dyn_val:1;           /* free(->val.any) on node destroy */
	} random_flags;  /* hack... persistent flags attached by various plugins */

	/* dynamic fields loaded from lihata */
	rnd_confitem_t val;   /* value is always an array (len 1 for the common case)  */
	rnd_confprop_t *prop; /* an array of properties allocated as big as val's array */
	int used;         /* number of items actually used in the arrays */
	int rnd_conf_rev;     /* last changed rev */

	/* dynamic fields for HIDs storing their data */
	vtp0_t hid_data;
	vtp0_t hid_callbacks; /* vector of (const rnd_conf_hid_callbacks_t *) */

	const char *gui_edit_act; /* if non-zero, run this action for GUI editing; args: role_name, full_list_conf_path, item_name */
} rnd_conf_native_t;


struct conf_listitem_s {
	rnd_conf_native_type_t type;
	rnd_confitem_t val;   /* value is always an array (len 1 for the common case)  */
	rnd_confprop_t prop; /* an array of properties allocated as big as val's array */
	const char *name;
	const char *payload;
	gdl_elem_t link;
};


typedef enum {
	RND_CFR_INTERNAL,
	RND_CFR_SYSTEM,
	RND_CFR_DEFAULTPCB, /* default.pcb */
	RND_CFR_USER,
	RND_CFR_ENV,        /* env vars */
	RND_CFR_PROJECT,    /* project specific, from a local file */
	RND_CFR_DESIGN,     /* from the design file */
	RND_CFR_CLI,        /* from the command line */
	RND_CFR_max_real,   /* all the above are real files and should be merged */
	/* these ones are not real roles and are used in internal communication in the GUI HIDs */
	RND_CFR_file,       /* custom file */
	RND_CFR_binary,     /* the in-memory binary representation */
	RND_CFR_max_alloc,  /* all the above should have a root */

	RND_CFR_invalid
} rnd_conf_role_t;

extern const int rnd_conf_default_prio[];
extern long rnd_conf_main_root_replace_cnt[RND_CFR_max_alloc]; /* number of times the root has been replaced */

void rnd_conf_init(void);
void rnd_conf_uninit(void);

/* Load all config files from disk into memory-lht and run rnd_conf_update to
   get the binary representation updated. Called only once, from main.c.
   Switches the conf system into production mode - new plugins registering new
   internal files will get special treat after this. */
void rnd_conf_load_all(const char *project_fn, const char *design_fn);

/* Set to 1 after rnd_conf_load_all() to indicate that the in-memory conf
   system is complete; any new plugin registering new conf will need to
   trigger a refresh */
extern int rnd_conf_in_production;

/* Load a file or a string as a role */
int rnd_conf_load_as(rnd_conf_role_t role, const char *fn, int fn_is_text);

/* copy root to be the new config of role; root must be a li:pcb-rnd-conf-v1
   return 0 on success and removes/invalidates root */
int rnd_conf_insert_tree_as(rnd_conf_role_t role, lht_node_t *root);

/* Load a project file into RND_CFR_PROJECT. Both project_fn and design_fn can't be NULL.
   Leaves an initialized but empty RND_CFR_PROJECT root if no project file was
   found. Runs rnd_conf_update(NULL); */
void rnd_conf_load_project(const char *project_fn, const char *design_fn);

void rnd_conf_load_extra(const char *project_fn, const char *design_fn);

/* Update the binary representation from the memory-lht representation */
void rnd_conf_update(const char *path, int arr_idx);

rnd_conf_native_t *rnd_conf_get_field(const char *path);
rnd_conf_native_t *rnd_conf_reg_field_(void *value, int array_size, rnd_conf_native_type_t type, const char *path, const char *desc, rnd_conf_flag_t flags);

void rnd_conf_unreg_field(rnd_conf_native_t *field);
void rnd_conf_unreg_fields(const char *prefix);

/* Set the value of path[arr_idx] in memory-lht role target to new_val using
   policy pol. Only lists should be indexed. Indexing can be a [n] suffix on
   path or a non-negative arr_idx. Updates the in-memory binary as well. If
   new_val is NULL, the selected subtree is removed from the lihata document. */
int rnd_conf_set(rnd_conf_role_t target, const char *path, int arr_idx, const char *new_val, rnd_conf_policy_t pol);

/* Remove the subtree of path[arr_idx] in memory-lht role target. Same
   considerations as in rnd_conf_set. */
int rnd_conf_del(rnd_conf_role_t target, const char *path, int arr_idx);

/* Increase the size of a list (array) to new_size; returns 0 on success */
int rnd_conf_grow(const char *path, int new_size);

/* Same as rnd_conf_set, but without updating the binary - useful for multiple
   rnd_conf_set_dry calls and a single all-tree rnd_conf_udpate(NULL) for transactions.
   If mkdirp is non-zero, automatically create the policy subtree if it doesn't exist. */
int rnd_conf_set_dry(rnd_conf_role_t target, const char *path_, int arr_idx, const char *new_val, rnd_conf_policy_t pol, int mkdirp);

/* Same as rnd_conf_set, but doesn't look up where to set things: change the value of
   the lihata node backing the native field */
int rnd_conf_set_native(rnd_conf_native_t *field, int arr_idx, const char *new_val);

/* Process a command line argument arg_ (if val == NULL) or a pair of command line
   arguments arg_ and val. In the first case assume arg_ has both a config path
   and a value (may be implicit true for a boolean). In the second case val
   is always the value. If prefix is not NULL, the path is prefixed with it.
   On error always set *why to a const string reason.
   Returns 0 on success. */
int rnd_conf_set_from_cli(const char *prefix, const char *arg_, const char *val, const char **why);

/* Attempt to consume argv[] using rnd_conf_set_from_cli */
int rnd_conf_parse_arguments(const char *prefix, int *argc, char ***argv);

#define rnd_conf_reg_field_array(globvar, field, type_name, path, desc, flags) \
	rnd_conf_reg_field_((void *)&globvar.field, (sizeof(globvar.field) / sizeof(globvar.field[0])), type_name, path, desc, flags)

#define rnd_conf_reg_field_scalar(globvar, field, type_name, path, desc, flags) \
	rnd_conf_reg_field_((void *)&globvar.field, 1, type_name, path, desc, flags)

/* register a config field, array or scalar, selecting the right macro */
#define rnd_conf_reg_field(globvar,   field,isarray,type_name,cpath,cname, desc, flags) \
	rnd_conf_reg_field_ ## isarray(globvar, field,type_name,cpath "/" cname, desc, flags)

/* convert type name t type - return RND_CFN_max on error */
rnd_conf_native_type_t rnd_conf_native_type_parse(const char *s);

/* convert a policy text to policy value - return RND_POL_invalid on error */
rnd_conf_policy_t rnd_conf_policy_parse(const char *s);

/* Return the name of the policy - always a static string, even for invalid roles */
const char *rnd_conf_policy_name(rnd_conf_policy_t p);

/* convert a role text to role value - return RND_CFR_invalid on error */
rnd_conf_role_t rnd_conf_role_parse(const char *s);

/* Return the name of the role - always a static string, even for invalid roles */
const char *rnd_conf_role_name(rnd_conf_role_t r);

/* Lock/unlock the structure of a role. In a locked role value of existing
   fields may be modified but the structure of the tree is static (can't
   create or remove nodes). This is useful when an io_ file format supports
   only a subset of settings: it can build the RND_CFR_DESIGN tree, lock it so
   settings that it wouldn't know how to save won't appear. NOTE: io_pcb
   supports all settings via attributes so does not lock. */
void rnd_conf_lock(rnd_conf_role_t target);
void rnd_conf_unlock(rnd_conf_role_t target);


/* replace dst_role:dst_path with a copy of src_role:src_path */
int rnd_conf_replace_subtree(rnd_conf_role_t dst_role, const char *dst_path, rnd_conf_role_t src_role, const char *src_path);

/* Throw out a subtree (remove all nodes from the lihata representation).
   Useful for io_ plugins, on RND_CFR_DESIGN, before loading a new file. */
void rnd_conf_reset(rnd_conf_role_t target, const char *source_fn);

/* Save an in-memory lihata representation to the disk */
int rnd_conf_save_file(rnd_hidlib_t *hidlib, const char *project_fn, const char *design_fn, rnd_conf_role_t role, const char *fn);

/* Returns whether a given lihata tree is locked */
int rnd_conf_islocked(rnd_conf_role_t target);

/* Returns whether a given lihata tree has changed since load or last save */
int rnd_conf_isdirty(rnd_conf_role_t target);
void rnd_conf_makedirty(rnd_conf_role_t target);

/* all configuration fields ever seen */
extern htsp_t *rnd_conf_fields;

/***** print fields from binary to lihata/text *****/

typedef int (*rnd_conf_pfn)(void *ctx, const char *fmt, ...);

/* Prints the value of a node in a form that is suitable for lihata. Prints
   a single element of an array, but prints lists as lists. Returns
   the sum of rnd_conf_pfn call return values - this is usually the number of
   bytes printed. */
int rnd_conf_print_native_field(rnd_conf_pfn pfn, void *ctx, int verbose, rnd_confitem_t *val, rnd_conf_native_type_t type, rnd_confprop_t *prop, int idx);

/* Prints the value of a node in a form that is suitable for lihata. Prints
   full arrays. Returns the sum of rnd_conf_pfn call return values - this is
   usually the number of bytes printed. */
int rnd_conf_print_native(rnd_conf_pfn pfn, void *ctx, const char * prefix, int verbose, rnd_conf_native_t *node);

/* Mark a path read-only */
void rnd_conf_ro(const char *path);


/****** utility ******/

#define rnd_conf_is_read_only(role) \
	(((role) == RND_CFR_INTERNAL) || ((role) == RND_CFR_SYSTEM) || ((role) == RND_CFR_DEFAULTPCB))

void rnd_conf_setf(rnd_conf_role_t role, const char *path, int idx, const char *fmt, ...);

#define rnd_conf_list_foreach_path_first(hidlib, res, conf_list, call) \
do { \
	rnd_conf_listitem_t *__n__; \
	const rnd_conflist_t *__lst1__ = (conf_list); \
	rnd_conflist_t *__lst__ = (rnd_conflist_t *)(__lst1__); \
	if (__lst__ != NULL) { \
		for(__n__ = rnd_conflist_first(__lst__); __n__ != NULL; __n__ = rnd_conflist_next(__n__)) { \
			char *__path__; \
			const char **__in__ = __n__->val.string; \
			if (__in__ == NULL) \
				continue; \
			pcb_path_resolve(hidlib, *__in__, &__path__, 0, pcb_false); \
			res = call; \
			free(__path__); \
			if (res == 0) \
				break; \
		} \
	} \
} while(0)

/*	printf("conf_list_foreach_path_first: %s using %s\n", # call, # conf_list); \*/

/* htsp_entry_t *e; */
#define rnd_conf_fields_foreach(e) \
	for (e = htsp_first(rnd_conf_fields); e; e = htsp_next(rnd_conf_fields, e))

/* helpers to make the code shorter */
#define rnd_conf_set_design(path, fmt, new_val) \
	rnd_conf_setf(RND_CFR_DESIGN, path, -1, fmt, new_val)

#define rnd_conf_set_editor(field, val) \
	rnd_conf_set(RND_CFR_DESIGN, "editor/" #field, -1, val ? "1" : "0", RND_POL_OVERWRITE)

#define rnd_conf_set_editor_(sfield, val) \
	rnd_conf_set(RND_CFR_DESIGN, sfield, -1, val ? "1" : "0", RND_POL_OVERWRITE)

#define rnd_conf_toggle_editor(field) \
	rnd_conf_set_editor(field, !conf_core.editor.field)

#define rnd_conf_toggle_heditor(field) \
	rnd_conf_set_editor(field, !pcbhl_conf.editor.field)

#define rnd_conf_toggle_editor_(sfield, field) \
	rnd_conf_set_editor_("editor/" sfield, !conf_core.editor.field)

#define rnd_conf_toggle_heditor_(sfield, field) \
	rnd_conf_set_editor_("editor/" sfield, !pcbhl_conf.editor.field)

/* For temporary modification/restoration of variables (hack) */
#define rnd_conf_force_set_bool(var, val) *((RND_CFT_BOOLEAN *)(&var)) = val
#define rnd_conf_force_set_str(var, val) *((RND_CFT_STRING *)(&var)) = val

/* get the first config subtree node (it's a hash and its children
   are "design", "rc", ...); if create is 1, and the subtree doesn't
   exist, create an "overwrite". */
lht_node_t *rnd_conf_lht_get_first(rnd_conf_role_t target, int create);

/* loop helper */
rnd_conf_listitem_t *rnd_conf_list_first_str(rnd_conflist_t *list, const char **item_str, int *idx);
rnd_conf_listitem_t *rnd_conf_list_next_str(rnd_conf_listitem_t *item_li, const char **item_str, int *idx);

/*rnd_conf_listitem_t *item;*/
#define rnd_conf_loop_list(list, item, idx) \
	for (idx = 0, item = rnd_conflist_first((rnd_conflist_t *)list); item != NULL; item = rnd_conflist_next(item), idx++)

/*rnd_conf_listitem_t *item; const char *item_str; */
#define rnd_conf_loop_list_str(list, item_li, item_str, idx) \
	for (idx = 0, item_li = rnd_conf_list_first_str((rnd_conflist_t *)list, &item_str, &idx); \
		item_li != NULL;\
		item_li = rnd_conf_list_next_str(item_li, &item_str, &idx))

const char *rnd_conf_concat_strlist(const rnd_conflist_t *lst, gds_t *buff, int *inited, char sep);

/* Print usage help for all nodes that have the RND_CFF_USAGE flag and whose
   path starts with prefix (if prefix != NULL) */
void rnd_conf_usage(const char *prefix, void (*print)(const char *name, const char *help));

/* Determine under which role a node is */
rnd_conf_role_t rnd_conf_lookup_role(const lht_node_t *nd);

/* Return the lihata node of a path in target, optionally creating it with the right type;
   if allow_plug is 1, fall back returning node from plugin conf */
lht_node_t *rnd_conf_lht_get_at(rnd_conf_role_t target, const char *path, int create);
lht_node_t *rnd_conf_lht_get_at_mainplug(rnd_conf_role_t target, const char *path, int allow_plug, int create);

/* Write an existing conf subtree to a file */
int rnd_conf_export_to_file(rnd_hidlib_t *hidlib, const char *fn, rnd_conf_role_t role, const char *conf_path);

/* Determine the policy and priority of a config lihata node;
   returns 0 on success but may not fill in both values, caller is
   responsible for initializing them before the call. */
int rnd_conf_get_policy_prio(lht_node_t *node, rnd_conf_policy_t *gpolicy, long *gprio);

/* Parse text and convert the value into native form and store in one of dst
   fields depending on type */
int rnd_conf_parse_text(rnd_confitem_t *dst, int idx, rnd_conf_native_type_t type, const char *text, lht_node_t *err_node);

/* Returns the user configuration file name */
const char *rnd_conf_get_user_conf_name();

/* Determine the file name of the project file - project_fn and design_fn can be NULL */
const char *rnd_conf_get_project_conf_name(const char *project_fn, const char *design_fn, const char **out_project_fn);

/* Get the first subtree that matches pol within target; allocate new
   subtree if needed */
lht_node_t *rnd_conf_lht_get_first_pol(rnd_conf_role_t target, rnd_conf_policy_t pol, int create);

/* (un)register a custom config file name (not path, just file name);
   if intern is not NULL, it is the internal (executable-embedded)
   version; it's not strdup'd, the caller needs to keep the string available
   until rnd_conf_unreg_file(). path is strdup'd */
void rnd_conf_reg_file(const char *path, const char *intern);
void rnd_conf_unreg_file(const char *path, const char *intern);

void rnd_conf_files_uninit(void);

extern void (*rnd_conf_core_postproc)(void); /* if not NULL, called after conf updates to give conf_core a chance to update dynamic entries */


/*** mass resolve (useful to avoid conf_core dep) ***/
typedef struct {
	const char *path;           /* full conf path to look up */
	rnd_conf_native_type_t type;    /* accept only if type matches */
	int allow_array;            /* if 0, refuse arrays */
	rnd_conf_native_t *nat;         /* NULL if refused or not found */
} rnd_conf_resolve_t;

/* Resolve a single nat or an array of them (until a terminaltor where
   path==NULL). Return the number of succesful resolves */
int rnd_conf_resolve(rnd_conf_resolve_t *res);
int rnd_conf_resolve_all(rnd_conf_resolve_t *res);


#endif
