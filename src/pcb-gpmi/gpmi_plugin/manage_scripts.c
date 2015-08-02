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

static script_info_t *choose_script(char **operations, int *operation)
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

	attr_make_enum(&attr[0],  "script", "Select an item from the list of scripts loaded", scrl, -1);

	if (operations != NULL)
		attr_make_enum(&attr[1],  "operation", "Choose what to do with the script", operations, *operation);


	if (gui->attribute_dialog(attr, 1 + (operations != NULL), result, "GPMI manage scripts - select script", "Select one of the scripts already loaded")) {
		if (operation != NULL)
			*operation = -1;
		return NULL;
	}

	if ((operations != NULL) && (operation != NULL))
		*operation = result[1].int_value;

/*	printf("res=%d\n", result[0].int_value);*/

	if (result[0].int_value != -1) {
		for(i = script_info, n = result[0].int_value; i != NULL && n != 0; i = i->next, n--);
/*		printf("name=%s\n", i->name);*/
		return i;
	}
	return NULL;
}

static script_info_t *load_script(void)
{
	return NULL;
}

void gpmi_hid_manage_scripts(void)
{
	script_info_t *i;
	char *operations[] = {"reload", "unload", "unload and remove from the config file", "load a new script", "load a new script and add it in the config", NULL};
	int op = 0;
	i = choose_script(operations, &op);
	switch(op) {
		case 0:
			if (i != NULL)
				gpmi_hid_script_reload(i);
			break;
		case 1:
			if (i != NULL)
				gpmi_hid_script_unload(i);
			break;
		case 2:
			if (i != NULL) {
				gpmi_hid_script_remove(i);
				gpmi_hid_script_unload(i);
			}
			break;
		case 3:
			load_script();
			break;
		case 4:
			i = load_script();
			if (i != NULL) {
				gpmi_hid_script_addcfg(i);
			}
			break;
	}
}
