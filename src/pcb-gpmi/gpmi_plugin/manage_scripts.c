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

void gpmi_hid_manage_scripts(void)
{
	HID *h;
	HID_Attribute attr[10];
	HID_Attr_Val result[10];
	const char **scrl;
	script_info_t *i;
	int n;


	for(i = script_info, n = 0; i != NULL; i = i->next, n++) ;

	scrl = malloc(sizeof(char *) * (n+1));
	for(i = script_info, n = 0; i != NULL; i = i->next, n++) {
/*		int len;
		len = strlen(i->name);*/
		scrl[n] = strdup(i->name);
	}
	scrl[n] = NULL;

	memset(attr, 0, sizeof(attr));

	attr[0].name         = "" ;
	attr[0].help_text    = "HELP1";
	attr[0].type         = HID_Label;

	attr[1].name         = "" ;
	attr[1].help_text    = "HELP2";
	attr[1].type         = HID_Enum;
	attr[1].enumerations = scrl;
	attr[1].default_val.int_value = -1;

	result[0].str_value = NULL;
	result[1].str_value = NULL;

	gui->attribute_dialog(attr, 2, result, "GPMI manage scripts - select script", "Select one of the scripts already loaded");


	printf("res=%d\n", result[1].int_value);

	if (result[1].int_value != -1) {
		for(i = script_info, n = result[1].int_value; i != NULL && n != 0; i = i->next, n--);

		printf("name=%s\n", i->name);
	}

}

