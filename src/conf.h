#ifndef PCB_CONF_H
#define PCB_CONF_H
#include "global.h"
#include "global_typedefs.h"
#include "pcb-printf.h"
#include <liblihata/lihata.h>
#include <liblihata/dom.h>

typedef char *   CFT_STRING;
typedef int      CFT_BOOLEAN;
typedef long     CFT_INTEGER;
typedef double   CFT_REAL;
typedef Coord    CFT_COORD;
typedef Unit *   CFT_UNIT;
typedef char *   CFT_COLOR;

typedef enum {
	CFN_STRING,
	CFN_BOOLEAN,
	CFN_INTEGER,     /* signed long */
	CFN_REAL,        /* double */
	CFN_COORD,
	CFN_UNIT,
	CFN_COLOR,
} conf_native_type_t;

typedef struct {
	conf_native_type_t type;
	union {
		char **string;
		int *boolean;
		long *integer;
		double *real;
		Coord *coord;
		Unit *unit;
		char *color;
		void *any;
	} native;
	const char *description;
	int prio;
	int array_size;
	lht_node_t *src;
} conf_native_t;

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
	conf_reg_field_(&globvar.field, -1, type_name, path)

/* register a config field, array or scalar, selecting the right macro */
#define conf_reg_field(globvar,   field,isarray,type_name,cpath,cname) \
	conf_reg_field_ ## isarray(globvar, field,type_name,cpath cname)


#endif

