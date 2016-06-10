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

#ifndef PCB_CONF_H
#define PCB_CONF_H
#include "global.h"
#include "global_typedefs.h"
#include "pcb-printf.h"
#include <liblihata/lihata.h>
#include <liblihata/dom.h>
#include <genvector/vtp0.h>
#include "unit.h"

int conf_rev; /* icreased by one each time there's a change in any of the config binaries */

typedef enum {
	POL_PREPEND,
	POL_APPEND,
	POL_OVERWRITE,
	POL_DISABLE,
	POL_invalid
} conf_policy_t;

typedef union conflist_u conflist_t;

typedef char *      CFT_STRING;
typedef int         CFT_BOOLEAN;
typedef long        CFT_INTEGER;
typedef double      CFT_REAL;
typedef Coord       CFT_COORD;
typedef Unit *      CFT_UNIT;
typedef char *      CFT_COLOR;
typedef conflist_t  CFT_LIST;
typedef Increments  CFT_INCREMENTS;

typedef enum {
	CFN_STRING,
	CFN_BOOLEAN,
	CFN_INTEGER,     /* signed long */
	CFN_REAL,        /* double */
	CFN_COORD,
	CFN_UNIT,
	CFN_COLOR,
	CFN_LIST,
	CFN_INCREMENTS
} conf_native_type_t;

typedef union confitem_u {
	const char **string;
	int *boolean;
	long *integer;
	double *real;
	Coord *coord;
	const Unit **unit;
	const char **color;
	conflist_t *list;
	Increments *increments;
	void *any;
} confitem_t ;

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
	struct {
		unsigned io_pcb_no_attrib:1;
		unsigned read_only:1;         /* set by conf_core, has no lihata, should not be overwritten */
	} random_flags;  /* hack... persistent flags attached by various plugins */

	/* dynamic fields loaded from lihata */
	confitem_t val;   /* value is always an array (len 1 for the common case)  */
	confprop_t *prop; /* an array of properies allocated as big as val's array */
	int used;         /* number of items actually used in the arrays */
	int conf_rev;     /* last changed rev */

	/* dynamic fields for HIDs storing their data */
	vtp0_t hid_data;
} conf_native_t;

typedef struct conf_listitem_s conf_listitem_t;
struct conf_listitem_s {
	conf_native_type_t type;
	confitem_t val;   /* value is always an array (len 1 for the common case)  */
	confprop_t prop; /* an array of properies allocated as big as val's array */
	const char *payload;
	gdl_elem_t link;
};

#include "list_conf.h"

typedef enum {
	CFR_INTERNAL,
	CFR_SYSTEM,
	CFR_USER,
	CFR_ENV,       /* env vars */
	CFR_PROJECT,   /* project specific, from a local file */
	CFR_DESIGN,    /* from the design file */
	CFR_CLI,       /* from the command line */
	CFR_max,
	CFR_invalid = CFR_max
} conf_role_t;

void conf_init(void);

/* Load all config files from disk into memory-lht and run conf_update to
   get the binary representation updated */
void conf_load_all(void);

/* Update the binary representation from the memory-lht representation */
void conf_update(const char *path);

conf_native_t *conf_get_field(const char *path);
void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path, const char *desc);

void conf_unreg_fields(const char *prefix);

/* Set the value of path[arr_idx] in memory-lht role target to new_val using policy pol. Only lists should
   be indexed. Indexing can be a [n] suffix on path or a non-negative arr_idx. */
int conf_set(conf_role_t target, const char *path, int arr_idx, const char *new_val, conf_policy_t pol);

/* Same as conf_set, but doesn't look up where to set things: change the value of
   the lihata node backing the native field */
int conf_set_native(conf_native_t *field, int arr_idx, const char *new_val);

int conf_set_from_cli(const char *arg_, char **why);

#define conf_reg_field_array(globvar, field, type_name, path, desc) \
	conf_reg_field_((void *)&globvar.field, (sizeof(globvar.field) / sizeof(globvar.field[0])), type_name, path, desc)

#define conf_reg_field_scalar(globvar, field, type_name, path, desc) \
	conf_reg_field_((void *)&globvar.field, 1, type_name, path, desc)

/* register a config field, array or scalar, selecting the right macro */
#define conf_reg_field(globvar,   field,isarray,type_name,cpath,cname, desc) \
	conf_reg_field_ ## isarray(globvar, field,type_name,cpath "/" cname, desc)

/* convert a policy text to policy value - return POL_invalid on error */
conf_policy_t conf_policy_parse(const char *s);

/* convert a role text to role value - return CFR_invalid on error */
conf_role_t conf_role_parse(const char *s);

/* Lock/unlock the structure of a role. In a locked role value of existing
   fields may be modified but the structure of the tree is static (can't
   create or remove nodes). This is useful when an io_ file format supports
   only a subset of settings: it can build the CFR_DESIGN tree, lock it so
   settings that it wouldn't know how to save won't appear. NOTE: io_pcb
   supports all settings via attributes so does not lock. */
void conf_lock(conf_role_t target);
void conf_unlock(conf_role_t target);

/* Throw out a subtree (remove all nodes from the lihata representation).
   Useful for io_ plugins, on CFR_DESIGN, before loading a new file. */
void conf_reset(conf_role_t target, const char *source_fn);

/* Returns whether a given lihata tree is locked */
int conf_islocked(conf_role_t target);

/* all configuration fields ever seen */
extern htsp_t *conf_fields;

/****** utility ******/

#define conf_list_foreach_path_first(res, conf_list, call) \
do { \
	conf_listitem_t *__n__; \
	const conflist_t *__lst1__ = (conf_list); \
	conflist_t *__lst__ = (conflist_t *)(__lst1__); \
	printf("conf_list_foreach_path_first: %s using %s\n", # call, # conf_list); \
	for(__n__ = conflist_first(__lst__); __n__ != NULL; __n__ = conflist_next(__n__)) { \
		char *__path__; \
		const char **__in__ = __n__->val.string; \
		if (__in__ == NULL) \
			continue; \
		resolve_path(*__in__, &__path__, 0); \
		res = call; \
		printf("  %s -> %d\n", __path__, res); \
		free(__path__); \
		if (res == 0) \
			break; \
	} \
} while(0)


/* htsp_entry_t *e; */
#define conf_fields_foreach(e) \
	for (e = htsp_first(conf_fields); e; e = htsp_next(conf_fields, e))


/* helpers to make the code shorter */
#define conf_set_design(path, fmt, new_val) \
do { \
	char *__tmp__ = pcb_strdup_printf(fmt, new_val); \
	conf_set(CFR_DESIGN, path, -1, __tmp__, POL_OVERWRITE); \
	free(__tmp__); \
	conf_update(path); \
} while(0)

#define conf_set_editor(field, val) \
do { \
	conf_set(CFR_DESIGN, "editor/" #field, -1, val ? "1" : "0", POL_OVERWRITE); \
	conf_update("editor/" #field); \
} while(0)

#define conf_toggle_editor(field) \
	conf_set_editor(field, !conf_core.editor.field)

#define conf_toggle_editor_(sfield, field) \
	conf_set_editor(sfield, !conf_core.editor.field)

/* For temporary modification/restoration of variables (hack) */
#define conf_force_set_bool(var, val) *((CFT_BOOLEAN *)(&var)) = val
#define conf_force_set_str(var, val) *((CFT_STRING *)(&var)) = val

/* get the main node of a configuration (it's a hash and its children
   are "design", "rc", ...) */
lht_node_t *conf_lht_get_main(conf_role_t target);

/* loop helper */
conf_listitem_t *conf_list_first_str(conflist_t *list, const char **item_str, int *idx);
conf_listitem_t *conf_list_next_str(conf_listitem_t *item_li, const char **item_str, int *idx);

/*conf_listitem_t *item;*/
#define conf_loop_list(list, item, idx) \
	for (idx = 0, item = conflist_first((conflist_t *)cl); item != NULL; item = conflist_next(item), idx++)

/*conf_listitem_t *item; const char *item_str; */
#define conf_loop_list_str(list, item_li, item_str, idx) \
	for (idx = 0, item_li = conf_list_first_str((conflist_t *)list, &item_str, &idx); \
		item_li != NULL;\
		item_li = conf_list_next_str(item_li, &item_str, &idx))

const char *conf_concat_strlist(const conflist_t *lst, gds_t *buff, int *inited, char sep);

#endif

