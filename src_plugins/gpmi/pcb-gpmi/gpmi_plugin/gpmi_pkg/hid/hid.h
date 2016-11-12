#include <gpmi.h>
#include "src/pcb-printf.h"

#undef snprintf
#define snprintf pcb_snprintf

/* Type of an HID attribute (usually a widget on an attribute dialog box) */
typedef enum hid_attr_type_e {
	HIDA_Label,    /* non-editable label displayed on the GUI */
	HIDA_Integer,  /* a sugned integer value */
	HIDA_Real,     /* a floating point value */
	HIDA_String,   /* one line textual input */
	HIDA_Boolean,  /* true/false boolean value */
	HIDA_Enum,     /* select an item of a predefined list */
	HIDA_Mixed,    /* TODO */
	HIDA_Path,     /* path to a file or directory */
	HIDA_Unit,     /* select a dimension unit */
	HIDA_Coord     /* enter a coordinate */
} hid_attr_type_t;

gpmi_keyword *kw_hid_attr_type_e; /* of hid_attr_type_t */

/* TODO: these should not be here; GPMI needs to switch over to c99tree! */
#ifndef FROM_PKG
typedef void pcb_hid_t;
typedef void pcb_hid_attribute_t;
typedef void* pcb_hid_gc_t;
typedef char* pcb_hid_attr_val_t;

/* Line or arc ending style */
typedef enum pcb_cap_style_t_e {
 Trace_Cap,    /* filled circle (trace drawing) */
 Square_Cap,   /* rectangular lines (square pad) */
 Round_Cap,    /* round pins or round-ended pads, thermals */
 Beveled_Cap   /* octagon pins or bevel-cornered pads */
} pcb_cap_style_t;

typedef void *pcb_polygon_t;
typedef void *pcb_box_t;
#endif

typedef struct hid_s {
	gpmi_module *module;
	int attr_num;
	pcb_hid_attribute_t *attr;
	hid_attr_type_t *type;
	pcb_hid_t *hid;
	pcb_hid_attr_val_t *result;
	pcb_hid_gc_t new_gc;
} hid_t;

/* Creates a new hid context. Name and description matters only if the hid is
registered as an exporter later. */
hid_t *hid_create(char *hid_name, char *description);

/* Append an attribute in a hid previously created using hid_create().
   Arguments:
     hid: hid_t previously created using hid_create()
     attr_name: name of the attribute
     help: help text for the attribute
     type: type of the attribute (input widget type)
     min: minimum value of the attribute, if type is integer or real)
     max: maximum value of the attribute, if type is integer or real)
     default_val: default value of the attribute
  Returns an unique ID of the attribute the caller should store for
  later reference. For example this ID is used when retrieving the
  value of the attribute after the user finished entering data in
  the dialog. */
int hid_add_attribute(hid_t *hid, char *attr_name, char *help, hid_attr_type_t type, int min, int max, char *default_val);

/* Query an attribute from the hid after dialog_attributes() returned.
   Arguments:
     hid: hid_t previously created using hid_create()
     attr_id: the unique ID of the attribute (returned by hid_add_attribute())
   Returns the value (converted to string) set by the user. */
dynamic char *hid_get_attribute(hid_t *hid, int attr_id);

/* Register the hid; call it after a hid is created and its attributes
   are all set up */
int hid_register(hid_t *hid);

/* For internal use */
void hid_gpmi_data_set(hid_t *h, void *data);

/* For internal use */
hid_t *hid_gpmi_data_get(pcb_hid_t *h);

/* For internal use */
nowrap pcb_hid_attr_val_t hid_string2val(const hid_attr_type_t type, const char *str);

