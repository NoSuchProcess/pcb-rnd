#ifndef PCB_CONF_H
#define PCB_CONF_H
#include "global.h"
#include "global_typedefs.h"
#include "pcb-printf.h"
#include <liblihata/lihata.h>
#include <liblihata/dom.h>

typedef union conflist_u conflist_t;

typedef char *      CFT_STRING;
typedef int         CFT_BOOLEAN;
typedef long        CFT_INTEGER;
typedef double      CFT_REAL;
typedef Coord       CFT_COORD;
typedef Unit *      CFT_UNIT;
typedef char *      CFT_COLOR;
typedef conflist_t  CFT_LIST;

typedef enum {
	CFN_STRING,
	CFN_BOOLEAN,
	CFN_INTEGER,     /* signed long */
	CFN_REAL,        /* double */
	CFN_COORD,
	CFN_UNIT,
	CFN_COLOR,
	CFN_LIST,
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
	void *any;
} confitem_t ;

typedef struct {
	int prio;
	lht_node_t *src;
} confprop_t;

typedef struct {
	/* static fields defined by the macros */
	const char *description;
	int array_size;
	conf_native_type_t type;

	/* dynamic fields loaded from lihata */
	confitem_t val;   /* value is always an array (len 1 for the common case)  */
	confprop_t *prop; /* an array of properies allocated as big as val's array */
	int used;         /* number of items actually used in the arrays */
} conf_native_t;

typedef struct conf_listitem_s conf_listitem_t;
struct conf_listitem_s {
	conf_native_type_t type;
	confitem_t val;   /* value is always an array (len 1 for the common case)  */
	confprop_t prop; /* an array of properies allocated as big as val's array */
	char *payload;
	gdl_elem_t link;
};

#include "list_conf.h"

typedef enum {
	CFR_SYSTEM,
	CFR_max
} conf_role_t;

void conf_update(void);

conf_native_t *conf_get_field(const char *path);
void conf_reg_field_(void *value, int array_size, conf_native_type_t type, const char *path);

#define conf_reg_field_array(globvar, field, type_name, path) \
	conf_reg_field_(&globvar.field, (sizeof(globvar.field) / sizeof(globvar.field[0])), type_name, path)

#define conf_reg_field_scalar(globvar, field, type_name, path) \
	conf_reg_field_(&globvar.field, 1, type_name, path)

/* register a config field, array or scalar, selecting the right macro */
#define conf_reg_field(globvar,   field,isarray,type_name,cpath,cname) \
	conf_reg_field_ ## isarray(globvar, field,type_name,cpath "/" cname)


/****** utility ******/

#define conf_list_foreach_path_first(res, conf_list, call) \
do { \
	conf_listitem_t *__n__; \
	conflist_t *__lst__ = (conf_list); \
	for(__n__ = conflist_first(__lst__); __n__ != NULL; __n__ = conflist_next(__n__)) { \
		char *__path__; \
		const char **__in__ = __n__->val.string; \
		if (__in__ == NULL) \
			continue; \
		resolve_path(*__in__, &__path__, 0); \
		res = call; \
		free(__path__); \
		if (res == 0) \
			break; \
	} \
} while(0)



#endif

