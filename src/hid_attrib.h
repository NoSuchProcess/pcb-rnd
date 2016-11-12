#ifndef PCB_HID_ATTRIB_H
#define PCB_HID_ATTRIB_H

#include "hid.h"

/* Used for HID attributes (exporting and printing, mostly).
   HA_boolean uses int_value, HA_enum sets int_value to the index and
   str_value to the enumeration string.  HID_Label just shows the
   default str_value.  HID_Mixed is a real_value followed by an enum,
   like 0.5in or 100mm. */
struct pcb_hid_attr_val_s {
	int int_value;
	const char *str_value;
	double real_value;
	pcb_coord_t coord_value;
};

enum pcb_hids_t { HID_Label, HID_Integer, HID_Real, HID_String,
	HID_Boolean, HID_Enum, HID_Mixed, HID_Path,
	HID_Unit, HID_Coord
};

struct pcb_hid_attribute_s {
	const char *name;
	/* If the help_text is this, usage() won't show this option */
#define ATTR_UNDOCUMENTED ((char *)(1))
	const char *help_text;
	enum pcb_hids_t type;
	int min_val, max_val;				/* for integer and real */
	pcb_hid_attr_val_t default_val;		/* Also actual value for global attributes.  */
	const char **enumerations;
	/* If set, this is used for global attributes (i.e. those set
	   statically with REGISTER_ATTRIBUTES below) instead of changing
	   the default_val.  Note that a HID_Mixed attribute must specify a
	   pointer to pcb_hid_attr_val_t here, and HID_Boolean assumes this is
	   "char *" so the value should be initialized to zero, and may be
	   set to non-zero (not always one).  */
	void *value;
	int hash;										/* for detecting changes. */
};

extern void hid_register_attributes(pcb_hid_attribute_t *, int, const char *cookie, int copy);
#define REGISTER_ATTRIBUTES(a, cookie) HIDCONCAT(void register_,a) ()\
{ hid_register_attributes(a, sizeof(a)/sizeof(a[0]), cookie, 0); }

/* Remove all attributes registered with the given cookie */
void hid_remove_attributes_by_cookie(const char *cookie);

/* remove all attributes and free the list */
void hid_attributes_uninit(void);

typedef struct pcb_hid_attr_node_s {
	struct pcb_hid_attr_node_s *next;
	pcb_hid_attribute_t *attributes;
	int n;
	const char *cookie;
} pcb_hid_attr_node_t;

extern pcb_hid_attr_node_t *hid_attr_nodes;

void hid_usage(pcb_hid_attribute_t * a, int numa);
void hid_usage_option(const char *name, const char *help);
#endif
