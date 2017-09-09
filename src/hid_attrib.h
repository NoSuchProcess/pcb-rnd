#ifndef PCB_HID_ATTRIB_H
#define PCB_HID_ATTRIB_H

#include <assert.h>
#include "hid.h"
#include "compat_misc.h"

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses int_value, HA_enum sets int_value to the index and
   str_value to the enumeration string.  PCB_HATT_LABEL just shows the
   default str_value.  PCB_HATT_MIXED is a real_value followed by an enum,
   like 0.5in or 100mm. */
struct pcb_hid_attr_val_s {
	int int_value;
	const char *str_value;
	double real_value;
	pcb_coord_t coord_value;
};

typedef enum pcb_hatt_compflags_e {
	PCB_HATF_FRAME   = 1,  /* box/table has a visible frame around it */
	PCB_HATF_LABEL   = 2   /* direct children print label */
} pcb_hatt_compflags_t;

typedef enum pcb_hids_e {
	/* atomic entry types */
	PCB_HATT_LABEL,
	PCB_HATT_INTEGER,
	PCB_HATT_REAL,
	PCB_HATT_STRING,
	PCB_HATT_BOOL,
	PCB_HATT_ENUM,
	PCB_HATT_MIXED,
	PCB_HATT_PATH,
	PCB_HATT_UNIT,
	PCB_HATT_COORD,

	/* groups (e.g. boxes) */
	PCB_HATT_BEGIN_HBOX,          /* NOTE: PCB_HATT_IS_COMPOSITE() depends on it */
	PCB_HATT_BEGIN_VBOX,
	PCB_HATT_BEGIN_TABLE,         /* min_val is the number of columns */
	PCB_HATT_END          /* close one level of PCB_HATT_* */
} pcb_hids_t;

#define PCB_HATT_IS_COMPOSITE(type) \
	(((type) >= PCB_HATT_BEGIN_HBOX) && ((type) < PCB_HATT_END))

/* alternative field names in struct pcb_hid_attribute_s */
#define pcb_hatt_flags       max_val
#define pcb_hatt_table_cols  min_val

struct pcb_hid_attribute_s {
	const char *name;
	/* If the help_text is this, usage() won't show this option */
#define ATTR_UNDOCUMENTED ((char *)(1))
	const char *help_text;
	pcb_hids_t type;
	int min_val, max_val;				/* for integer and real */
	pcb_hid_attr_val_t default_val;		/* Also actual value for global attributes.  */
	const char **enumerations;
	/* If set, this is used for global attributes (i.e. those set
	   statically with REGISTER_ATTRIBUTES below) instead of changing
	   the default_val.  Note that a PCB_HATT_MIXED attribute must specify a
	   pointer to pcb_hid_attr_val_t here, and PCB_HATT_BOOL assumes this is
	   "char *" so the value should be initialized to zero, and may be
	   set to non-zero (not always one).  */
	void *value;

	/* Advanced API: may not be available in all HIDs; supported by hid_gtk */
	int changed; /* 0 for initial values, 1 on user change */
	void (*change_cb)(pcb_hid_attribute_t *attr); /* called upon value change by the user */
	void *user_data; /* ignored; the caller is free to use it */
};

extern void pcb_hid_register_attributes(pcb_hid_attribute_t *, int, const char *cookie, int copy);
#define PCB_REGISTER_ATTRIBUTES(a, cookie) PCB_HIDCONCAT(void register_,a) ()\
{ pcb_hid_register_attributes(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* Remove all attributes registered with the given cookie */
void pcb_hid_remove_attributes_by_cookie(const char *cookie);

/* remove all attributes and free the list */
void pcb_hid_attributes_uninit(void);

typedef struct pcb_hid_attr_node_s {
	struct pcb_hid_attr_node_s *next;
	pcb_hid_attribute_t *attributes;
	int n;
	const char *cookie;
} pcb_hid_attr_node_t;

extern pcb_hid_attr_node_t *hid_attr_nodes;

void pcb_hid_usage(pcb_hid_attribute_t * a, int numa);
void pcb_hid_usage_option(const char *name, const char *help);

/* Count the number of direct children, start_from the first children */
int pcb_hid_atrdlg_num_children(pcb_hid_attribute_t *attrs, int start_from, int n_attrs);

/*** Helpers for building dynamic attribute dialogs (DAD) ***/
#define PCB_DAD_DECL(table) \
	pcb_hid_attribute_t *table = NULL; \
	pcb_hid_attr_val_t *table ## _result = NULL; \
	int table ## _len = 0; \
	int table ## _alloced = 0;

/* Free all resources allocated by DAD macros for table */
#define PCB_DAD_FREE(table) \
do { \
	int __n__; \
	for(__n__ = 0; __n__ < table ## _len; __n__++) { \
		PCB_DAD_FREE_FIELD(table, __n__); \
	} \
	free(table); \
	free(table ## _result); \
} while(0)

#define PCB_DAD_RUN(table, title, descr) \
do { \
	if (table ## _result == NULL) \
		PCB_DAD_ALLOC_RESULT(table); \
	pcb_gui->attribute_dialog(table, table ## _len, table ## _result, title, descr); \
} while(0)

/* Return the index of the item currenty being edited */
#define PCB_DAD_CURRENT(table) (table ## _len - 1)

/* Begin a new box or table */
#define PCB_DAD_BEGIN(table, item_type) \
	PCB_DAD_ALLOC(table, item_type);

#define PCB_DAD_BEGIN_TABLE(table, cols) \
do { \
	PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_TABLE); \
	PCB_DAD_SET(table, pcb_hatt_table_cols, cols); \
} while(0)

#define PCB_DAD_BEGIN_HBOX(table)      PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_HBOX)
#define PCB_DAD_BEGIN_VBOX(table)      PCB_DAD_BEGIN(table, PCB_HATT_BEGIN_VBOX)
#define PCB_DAD_END(table)             PCB_DAD_BEGIN(table, PCB_HATT_END)
#define PCB_DAD_COMPFLAG(table, val)   PCB_DAD_SET(table, pcb_hatt_flags, val)

#define PCB_DAD_LABEL(table, text) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_LABEL); \
	PCB_DAD_SET(table, name, pcb_strdup(text)); \
} while(0)

/* Add label usign printf formatting syntax: PCB_DAD_LABELF(tbl, ("a=%d", 12)); */
#define PCB_DAD_LABELF(table, printf_args) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_LABEL); \
	PCB_DAD_SET(table, name, pcb_strdup_printf printf_args); \
} while(0)

#define PCB_DAD_ENUM(table, choices) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_ENUM); \
	PCB_DAD_SET(table, enumerations, choices); \
} while(0)

#define PCB_DAD_INTEGER(table, label) \
do { \
	PCB_DAD_ALLOC(table, PCB_HATT_INTEGER); \
	PCB_DAD_SET(table, name, label); \
} while(0)

/* Set properties of the current item */
#define PCB_DAD_MINVAL(table, val)       PCB_DAD_SET(table, min_val, val)
#define PCB_DAD_MAXVAL(table, val)       PCB_DAD_SET(table, max_val, val)
#define PCB_DAD_DEFAULT(table, val)      PCB_DAD_SET_VAL(table, default_val, val)
#define PCB_DAD_MINMAX(table, min, max)  (PCB_DAD_SET(table, min_val, min),PCB_DAD_SET(table, max_val, max))


/*** DAD internals (do not use directly) ***/
#define PCB_DAD_ALLOC(table, item_type) \
	do { \
		if (table ## _len >= table ## _alloced) { \
			table ## _alloced += 16; \
			table = realloc(table, sizeof(table[0]) * table ## _alloced); \
		} \
		memset(&table[table ## _len], 0, sizeof(table[0])); \
		table[table ## _len].type = item_type; \
		table ## _len++; \
	} while(0)

#define PCB_DAD_SET(table, field, value) \
	table[table ## _len - 1].field = (value)

#define PCB_DAD_SET_VAL(table, field, val) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			assert(0); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_MIXED: \
		case PCB_HATT_COORD: \
		case PCB_HATT_UNIT: \
			table[table ## _len - 1].field.int_value = (int)val; \
			break; \
		case PCB_HATT_REAL: \
			table[table ## _len - 1].field.real_value = (double)val; \
			break; \
		case PCB_HATT_STRING: \
		case PCB_HATT_PATH: \
			table[table ## _len - 1].field.str_value = (char *)val; \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_END: \
			assert(0); \
	} \
} while(0)

#define PCB_DAD_FREE_FIELD(table, field) \
do { \
	switch(table[table ## _len - 1].type) { \
		case PCB_HATT_LABEL: \
			free(table[table ## _len - 1].name); \
			break; \
		case PCB_HATT_INTEGER: \
		case PCB_HATT_BOOL: \
		case PCB_HATT_ENUM: \
		case PCB_HATT_MIXED: \
		case PCB_HATT_COORD: \
		case PCB_HATT_UNIT: \
		case PCB_HATT_REAL: \
		case PCB_HATT_STRING: \
		case PCB_HATT_PATH: \
			break; \
		case PCB_HATT_BEGIN_HBOX: \
		case PCB_HATT_BEGIN_VBOX: \
		case PCB_HATT_BEGIN_TABLE: \
		case PCB_HATT_END: \
			break; \
	} \
} while(0)

#define PCB_DAD_ALLOC_RESULT(table) \
do { \
	free(table ## _result); \
	table ## _result = calloc(PCB_DAD_CURRENT(table)+1, sizeof(pcb_hid_attr_val_t)); \
} while(0)

#endif
