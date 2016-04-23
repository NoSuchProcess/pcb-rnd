#ifndef PCB_CONF_H
#define PCB_CONF_H
#include "global.h"
#include "global_typedefs.h"
#include "pcb-printf.h"

typedef char *   CFT_STRING;
typedef int      CFT_BOOLEAN;
typedef long     CFT_INTEGER;
typedef double   CFT_REAL;
typedef Coord    CFT_COORD;
typedef Unit     CFT_UNIT;
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
		char **color;
	} native;
	const char *description;
} conf_native_t;
#endif

