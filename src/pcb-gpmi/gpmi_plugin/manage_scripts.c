#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gpmi.h>
#include "src/misc.h"
#include "src/event.h"
#include "gpmi_plugin.h"
#include "scripts.h"

extern HID *gui;

#define attr_make_label(attr, name_, help_) \
do { \
	memset((attr), 0, sizeof(HID_Attribute)); \
	(attr)->name         = name_; \
	(attr)->help_text    = help_; \
	(attr)->type         = HID_Label; \
} while(0)

#define attr_make_enum(attr, name_, help_, enum_vals, default_item) \
do { \
	memset((attr), 0, sizeof(HID_Attribute)); \
	(attr)->name         = name_; \
	(attr)->help_text    = help_; \
	(attr)->type         = HID_Enum; \
	(attr)->enumerations = enum_vals; \
	(attr)->default_val.int_value = default_item; \
} while(0)

static script_info_t *choose_script(void)
{
	HID_Attribute attr[10];
	HID_Attr_Val result[10];
	const char **scrl;
	script_info_t *i;
	int n;

	n = gpmi_hid_scripts_count();

	scrl = malloc(sizeof(char *) * (n+1));
	for(i = script_info, n = 0; i != NULL; i = i->next, n++) {
/*		int len;
		len = strlen(i->name);*/
		scrl[n] = strdup(i->name);
	}
	scrl[n] = NULL;

	attr_make_enum(&attr[0],  "", "Select an item from the list of scripts loaded", scrl, -1);

	if (gui->attribute_dialog(attr, 1, result, "GPMI manage scripts - select script", "Select one of the scripts already loaded"))
		return NULL;


/*	printf("res=%d\n", result[0].int_value);*/

	if (result[0].int_value != -1) {
		for(i = script_info, n = result[0].int_value; i != NULL && n != 0; i = i->next, n--);
/*		printf("name=%s\n", i->name);*/
		return i;
	}
	return NULL;
}

void gpmi_hid_manage_scripts(void)
{
	script_info_t *i;
	i = choose_script();
}
