#include <gpmi.h>

typedef enum hid_attr_type_e {
	HIDA_Label,
	HIDA_Integer,
	HIDA_Real,
	HIDA_String,
	HIDA_Boolean,
	HIDA_Enum,
	HIDA_Mixed,
	HIDA_Path
} hid_attr_type_t;

gpmi_keyword *kw_hid_attr_type_e; /* of hid_attr_type_t */

#ifndef FROM_PKG
typedef void HID;
typedef void HID_Attribute;
typedef void* hidGC;
typedef char* HID_Attr_Val;
#endif

typedef struct hid_s {
	gpmi_module *module;
	int attr_num;
	HID_Attribute *attr;
	hid_attr_type_t *type;
	HID *hid;
	HID_Attr_Val *result;
	hidGC new_gc;
} hid_t;

/* Create a new hid context */
hid_t *hid_create(char *hid_name, char *description);

/* Add an attribute in the hid - returns unique ID of the attribute */
int hid_add_attribute(hid_t *hid, char *attr_name, char *help, hid_attr_type_t type, int min, int max, char *default_val);

/* query an attribute from the hid after dialog_attriutes ran */
dynamic char *hid_get_attribute(hid_t *hid, int attr_id);

/* Register the hid; call it after attributes and... are all set up */
int hid_register(hid_t *hid);

/* Mainly for internal use: */
void hid_gpmi_data_set(hid_t *h, void *data);
hid_t *hid_gpmi_data_get(HID *h);
nowrap HID_Attr_Val hid_string2val(const hid_attr_type_t type, const char *str);

