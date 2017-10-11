#ifndef PCB_HID_ATTRIB_H
#define PCB_HID_ATTRIB_H

#include "hid.h"

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses int_value, HA_enum sets int_value to the index and
   str_value to the enumeration string.  PCB_HATT_LABEL just shows the
   default str_value. */
struct pcb_hid_attr_val_s {
	int int_value;
	const char *str_value;
	double real_value;
	pcb_coord_t coord_value;
	void (*func)();
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
	PCB_HATT_STRING,              /* WARNING: string, must be malloc()'d, can't be a (const char *) */
	PCB_HATT_BOOL,
	PCB_HATT_ENUM,
	PCB_HATT_PATH,                /* WARNING: string, must be malloc()'d, can't be a (const char *) */
	PCB_HATT_UNIT,
	PCB_HATT_COORD,
	PCB_HATT_BUTTON,              /* push button; default value is the label */

	/* groups (e.g. boxes) */
	PCB_HATT_BEGIN_HBOX,          /* NOTE: PCB_HATT_IS_COMPOSITE() depends on it */
	PCB_HATT_BEGIN_VBOX,
	PCB_HATT_BEGIN_TABLE,         /* min_val is the number of columns */
	PCB_HATT_END          /* close one level of PCB_HATT_* */
} pcb_hids_t;

#define PCB_HATT_IS_COMPOSITE(type) \
	(((type) >= PCB_HATT_BEGIN_HBOX) && ((type) < PCB_HATT_END))

#define PCB_HAT_IS_STR(type) \
	((type == PCB_HATT_STRING) || (type == PCB_HATT_PATH))

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
	   the default_val.  */
	void *value;

	/* Advanced API: may not be available in all HIDs; supported by hid_gtk */
	int changed; /* 0 for initial values, 1 on user change */
	void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr); /* called upon value change by the user */
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

#endif
