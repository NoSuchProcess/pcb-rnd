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

#ifndef PCB_CONF_H
#define PCB_CONF_H
#include "config.h"
#include "global_typedefs.h"
#include "pcb-printf.h"
#include <liblihata/lihata.h>
#include <liblihata/dom.h>
#include <genvector/vtp0.h>
#include "unit.h"

typedef union confitem_u confitem_t ;
typedef struct conf_listitem_s conf_listitem_t;

#include "list_conf.h"

extern int conf_rev; /* increased by one each time there's a change in any of the config binaries */

typedef enum {
	POL_PREPEND,
	POL_APPEND,
	POL_OVERWRITE,
	POL_DISABLE,
	POL_invalid
} conf_policy_t;

typedef enum { /* bitfield */
	CFF_USAGE = 1    /* settings should be printed in usage help */
} conf_flag_t;


typedef const char *  CFT_STRING;
typedef int           CFT_BOOLEAN;
typedef long          CFT_INTEGER;
typedef double        CFT_REAL;
typedef pcb_coord_t   CFT_COORD;
typedef pcb_unit_t *  CFT_UNIT;
typedef pcb_color_t   CFT_COLOR;
typedef conflist_t    CFT_LIST;

typedef enum {
	CFN_STRING,
	CFN_BOOLEAN,
	CFN_INTEGER,     /* signed long */
	CFN_REAL,        /* double */
	CFN_COORD,
	CFN_UNIT,
	CFN_COLOR,
	CFN_LIST,
	CFN_max
} conf_native_type_t;

union confitem_u {
	const char **string;
	int *boolean;
	long *integer;
	double *real;
	pcb_coord_t *coord;
	const pcb_unit_t **unit;
	pcb_color_t *color;
	conflist_t *list;
	void *any;
};

typedef struct {
	int prio;
	lht_node_t *src;
} confprop_t;

typedef struct {
	/* static fields defined by the macros */
	const char *description;
	const char *hash_path;     /* points to the hash key once its added in the hash (else: NULL) */
	int array_size;
	conf_native_type_t type;
	conf_flag_t flags;
	struct {
		unsigned io_pcb_no_attrib:1;
		unsigned read_only:1;         /* set by conf_core, has no lihata, should not be overwritten */
	} random_flags;  /* hack... persistent flags attached by various plugins */

	/* dynamic fields loaded from lihata */
	confitem_t val;   /* value is always an array (len 1 for the common case)  */
	confprop_t *prop; /* an array of properties allocated as big as val's array */
	int used;         /* number of items actually used in the arrays */
	int conf_rev;     /* last changed rev */

	/* dynamic fields for HIDs storing their data */
	vtp0_t hid_data;
	vtp0_t hid_callbacks; /* vector of (const conf_hid_callbacks_t *) */
} conf_native_t;


struct conf_listitem_s {
	conf_native_type_t type;
	confitem_t val;   /* value is always an array (len 1 for the common case)  */
	confprop_t prop; /* an array of properties allocated as big as val's array */
	const char *name;
	const char *payload;
	gdl_elem_t link;
};


typedef enum {
	CFR_INTERNAL,
	CFR_SYSTEM,
	CFR_DEFAULTPCB, /* default.pcb */
	CFR_USER,
	CFR_ENV,        /* env vars */
	CFR_PROJECT,    /* project specific, from a local file */
	CFR_DESIGN,     /* from the design file */
	CFR_CLI,        /* from the command line */
	CFR_max_real,   /* all the above are real files and should be merged */
	/* these ones are not real roles and are used in internal communication in the GUI HIDs */
	CFR_file,       /* custom file */
	CFR_binary,     /* the in-memory binary representation */
	CFR_max_alloc,  /* all the above should have a root */

	CFR_invalid
} conf_role_t;

extern const int conf_default_prio[];

void conf_init(void);
void conf_uninit(void);

/* Load all config files from disk into memory-lht and run conf_update to
   get the binary representation updated. Called only once, from main.c.
   Switches the conf system into production mode - new plugins registering new
   internal files will get special treat after this. */
void conf_load_all(const char *project_fn, const char *pcb_fn);

/* Set to 1 after conf_load_all() to indicate that the in-memory conf
   system is complete; any new plugin registering new conf will need to
   trigger a refresh */
extern int conf_in_production;

/* Load a file or a string as a role */
int conf_load_as(conf_role_t role, const char *fn, int fn_is_text);

/* copy root to be the new config of role; root must be a li:pcb-rnd-conf-v1
   return 0 on success and removes/invalidates root */
int conf_insert_tree_as(conf_role_t role, lht_node_t *root);

/* Load a project file into CFR_PROJECT. Both project_fn and pcb_fn can't be NULL.
   Leaves an initialized but empty CFR_PROJECT root if no project file was
   found. Runs conf_update(NULL); */
void conf_load_project(const char *project_fn, const char *pcb_fn);

void conf_load_extra(const char *project_fn, const char *pcb_fn);

/* Update the binary representation from the memory-lht representation */
void conf_update(const char *path, int arr_idx);

conf_native_t *conf_get_field(const char *path);
void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path, const char *desc, conf_flag_t flags);

void conf_unreg_fields(const char *prefix);

/* Set the value of path[arr_idx] in memory-lht role target to new_val using
   policy pol. Only lists should be indexed. Indexing can be a [n] suffix on
   path or a non-negative arr_idx. Updates the in-memory binary as well. If
   new_val is NULL, the selected subtree is removed from the lihata document. */
int conf_set(conf_role_t target, const char *path, int arr_idx, const char *new_val, conf_policy_t pol);

/* Remove the subtree of path[arr_idx] in memory-lht role target. Same
   considerations as in conf_set. */
int conf_del(conf_role_t target, const char *path, int arr_idx);

/* Same as conf_set, but without updating the binary - useful for multiple
   conf_set_dry calls and a single all-tree conf_udpate(NULL) for transactions.
   If mkdirp is non-zero, automatically create the policy subtree if it doesn't exist. */
int conf_set_dry(conf_role_t target, const char *path_, int arr_idx, const char *new_val, conf_policy_t pol, int mkdirp);

/* Same as conf_set, but doesn't look up where to set things: change the value of
   the lihata node backing the native field */
int conf_set_native(conf_native_t *field, int arr_idx, const char *new_val);

/* Process a command line argument arg_ (if val == NULL) or a pair of command line
   arguments arg_ and val. In the first case assume arg_ has both a config path
   and a value (may be implicit true for a boolean). In the second case val
   is always the value. If prefix is not NULL, the path is prefixed with it.
   On error always set *why to a const string reason.
   Returns 0 on success. */
int conf_set_from_cli(const char *prefix, const char *arg_, const char *val, const char **why);

/* Attempt to consume argv[] using conf_set_from_cli */
int conf_parse_arguments(const char *prefix, int *argc, char ***argv);

#define conf_reg_field_array(globvar, field, type_name, path, desc, flags) \
	conf_reg_field_((void *)&globvar.field, (sizeof(globvar.field) / sizeof(globvar.field[0])), type_name, path, desc, flags)

#define conf_reg_field_scalar(globvar, field, type_name, path, desc, flags) \
	conf_reg_field_((void *)&globvar.field, 1, type_name, path, desc, flags)

/* register a config field, array or scalar, selecting the right macro */
#define conf_reg_field(globvar,   field,isarray,type_name,cpath,cname, desc, flags) \
	conf_reg_field_ ## isarray(globvar, field,type_name,cpath "/" cname, desc, flags)

/* convert a policy text to policy value - return POL_invalid on error */
conf_policy_t conf_policy_parse(const char *s);

/* Return the name of the policy - always a static string, even for invalid roles */
const char *conf_policy_name(conf_policy_t p);

/* convert a role text to role value - return CFR_invalid on error */
conf_role_t conf_role_parse(const char *s);

/* Return the name of the role - always a static string, even for invalid roles */
const char *conf_role_name(conf_role_t r);

/* Lock/unlock the structure of a role. In a locked role value of existing
   fields may be modified but the structure of the tree is static (can't
   create or remove nodes). This is useful when an io_ file format supports
   only a subset of settings: it can build the CFR_DESIGN tree, lock it so
   settings that it wouldn't know how to save won't appear. NOTE: io_pcb
   supports all settings via attributes so does not lock. */
void conf_lock(conf_role_t target);
void conf_unlock(conf_role_t target);


/* replace dst_role:dst_path with a copy of src_role:src_path */
int conf_replace_subtree(conf_role_t dst_role, const char *dst_path, conf_role_t src_role, const char *src_path);

/* Throw out a subtree (remove all nodes from the lihata representation).
   Useful for io_ plugins, on CFR_DESIGN, before loading a new file. */
void conf_reset(conf_role_t target, const char *source_fn);

/* Save an in-memory lihata representation to the disk */
int conf_save_file(const char *project_fn, const char *pcb_fn, conf_role_t role, const char *fn);

/* Returns whether a given lihata tree is locked */
int conf_islocked(conf_role_t target);

/* Returns whether a given lihata tree has changed since load or last save */
int conf_isdirty(conf_role_t target);
void conf_makedirty(conf_role_t target);

/* all configuration fields ever seen */
extern htsp_t *conf_fields;

/***** print fields from binary to lihata/text *****/

typedef int (*conf_pfn)(void *ctx, const char *fmt, ...);

/* Prints the value of a node in a form that is suitable for lihata. Prints
   a single element of an array, but prints lists as lists. Returns
   the sum of conf_pfn call return values - this is usually the number of
   bytes printed. */
int conf_print_native_field(conf_pfn pfn, void *ctx, int verbose, confitem_t *val, conf_native_type_t type, confprop_t *prop, int idx);

/* Prints the value of a node in a form that is suitable for lihata. Prints
   full arrays. Returns the sum of conf_pfn call return values - this is
   usually the number of bytes printed. */
int conf_print_native(conf_pfn pfn, void *ctx, const char * prefix, int verbose, conf_native_t *node);


/****** utility ******/

void conf_setf(conf_role_t role, const char *path, int idx, const char *fmt, ...);

#define conf_list_foreach_path_first(res, conf_list, call) \
do { \
	conf_listitem_t *__n__; \
	const conflist_t *__lst1__ = (conf_list); \
	conflist_t *__lst__ = (conflist_t *)(__lst1__); \
	if (__lst__ != NULL) { \
		for(__n__ = conflist_first(__lst__); __n__ != NULL; __n__ = conflist_next(__n__)) { \
			char *__path__; \
			const char **__in__ = __n__->val.string; \
			if (__in__ == NULL) \
				continue; \
			pcb_path_resolve(*__in__, &__path__, 0, pcb_false); \
			res = call; \
			free(__path__); \
			if (res == 0) \
				break; \
		} \
	} \
} while(0)

/*	printf("conf_list_foreach_path_first: %s using %s\n", # call, # conf_list); \*/

/* htsp_entry_t *e; */
#define conf_fields_foreach(e) \
	for (e = htsp_first(conf_fields); e; e = htsp_next(conf_fields, e))

/* helpers to make the code shorter */
#define conf_set_design(path, fmt, new_val) \
	conf_setf(CFR_DESIGN, path, -1, fmt, new_val)

#define conf_set_editor(field, val) \
	conf_set(CFR_DESIGN, "editor/" #field, -1, val ? "1" : "0", POL_OVERWRITE)

#define conf_set_editor_(sfield, val) \
	conf_set(CFR_DESIGN, sfield, -1, val ? "1" : "0", POL_OVERWRITE)

#define conf_toggle_editor(field) \
	conf_set_editor(field, !conf_core.editor.field)

#define conf_toggle_editor_(sfield, field) \
	conf_set_editor_("editor/" sfield, !conf_core.editor.field)

/* For temporary modification/restoration of variables (hack) */
#define conf_force_set_bool(var, val) *((CFT_BOOLEAN *)(&var)) = val
#define conf_force_set_str(var, val) *((CFT_STRING *)(&var)) = val

/* get the first config subtree node (it's a hash and its children
   are "design", "rc", ...); if create is 1, and the subtree doesn't
   exist, create an "overwrite". */
lht_node_t *conf_lht_get_first(conf_role_t target, int create);

/* loop helper */
conf_listitem_t *conf_list_first_str(conflist_t *list, const char **item_str, int *idx);
conf_listitem_t *conf_list_next_str(conf_listitem_t *item_li, const char **item_str, int *idx);

/*conf_listitem_t *item;*/
#define conf_loop_list(list, item, idx) \
	for (idx = 0, item = conflist_first((conflist_t *)list); item != NULL; item = conflist_next(item), idx++)

/*conf_listitem_t *item; const char *item_str; */
#define conf_loop_list_str(list, item_li, item_str, idx) \
	for (idx = 0, item_li = conf_list_first_str((conflist_t *)list, &item_str, &idx); \
		item_li != NULL;\
		item_li = conf_list_next_str(item_li, &item_str, &idx))

const char *conf_concat_strlist(const conflist_t *lst, gds_t *buff, int *inited, char sep);

/* Print usage help for all nodes that have the CFF_USAGE flag and whose
   path starts with prefix (if prefix != NULL) */
void conf_usage(const char *prefix, void (*print)(const char *name, const char *help));

/* Determine under which role a node is */
conf_role_t conf_lookup_role(const lht_node_t *nd);

/* Return the lihata node of a path in target, optionally creating it with the right type */
lht_node_t *conf_lht_get_at(conf_role_t target, const char *path, int create);

/* Write an existing conf subtree to a file */
int conf_export_to_file(const char *fn, conf_role_t role, const char *conf_path);

/* Determine the policy and priority of a config lihata node;
   returns 0 on success but may not fill in both values, caller is
   responsible for initializing them before the call. */
int conf_get_policy_prio(lht_node_t *node, conf_policy_t *gpolicy, long *gprio);

/* Parse text and convert the value into native form and store in one of dst
   fields depending on type */
int conf_parse_text(confitem_t *dst, int idx, conf_native_type_t type, const char *text, lht_node_t *err_node);

/* Returns the user configuration file name */
const char *conf_get_user_conf_name();

/* Determine the file name of the project file - project_fn and pcb_fn can be NULL */
const char *conf_get_project_conf_name(const char *project_fn, const char *pcb_fn, const char **out_project_fn);

/* Get the first subtree that matches pol within target; allocate new
   subtree if needed */
lht_node_t *conf_lht_get_first_pol(conf_role_t target, conf_policy_t pol, int create);

/* (un)register a custom config file name (not path, just file name);
   if intern is not NULL, it is the internal (executable-embedded)
   version; it's not strdup'd, the caller needs to keep the string available
   until conf_unreg_file(). path is strdup'd */
void conf_reg_file(const char *path, const char *intern);
void conf_unreg_file(const char *path, const char *intern);

void pcb_conf_files_uninit(void);

extern void (*pcb_conf_core_postproc)(void); /* if not NULL, called after conf updates to give conf_core a chance to update dynamic entries */


/*** mass resolve (useful to avoid conf_core dep) ***/
typedef struct {
	const char *path;           /* full conf path to look up */
	conf_native_type_t type;    /* accept only if type matches */
	int allow_array;            /* if 0, refuse arrays */
	conf_native_t *nat;         /* NULL if refused or not found */
} pcb_conf_resolve_t;

/* Resolve a single nat or an array of them (until a terminaltor where
   path==NULL). Return the number of succesful resolves */
int pcb_conf_resolve(pcb_conf_resolve_t *res);
int pcb_conf_resolve_all(pcb_conf_resolve_t *res);


#endif
