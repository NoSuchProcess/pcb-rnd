#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <gpmi.h>
#include "src/global_typedefs.h"
#include "src/misc_util.h"
#include "src/event.h"
#include "gpmi_plugin.h"
#include "scripts.h"
#include "src/hid_attrib.h"


extern pcb_hid_t *gui;

#define attr_make_label(attr, name_, help_) \
do { \
	memset((attr), 0, sizeof(pcb_hid_attribute_t)); \
	(attr)->name         = name_; \
	(attr)->help_text    = help_; \
	(attr)->type         = HID_Label; \
} while(0)

#define attr_make_label_str(attr, name1, name2, help_) \
do { \
	char *__names__; \
	__names__ = Concat(name1, name2, NULL); \
	attr_make_label(attr, __names__, help_); \
} while(0)


#define attr_make_enum(attr, name_, help_, enum_vals, default_item) \
do { \
	memset((attr), 0, sizeof(pcb_hid_attribute_t)); \
	(attr)->name         = name_; \
	(attr)->help_text    = help_; \
	(attr)->type         = HID_Enum; \
	(attr)->enumerations = enum_vals; \
	(attr)->default_val.int_value = default_item; \
} while(0)

static hid_gpmi_script_info_t *choose_script(const char **operations, int *operation)
{
	pcb_hid_attribute_t attr[3];
	pcb_hid_attr_val_t result[3];
	char **scrl, **s;
	hid_gpmi_script_info_t *i;
	int n, res;

	n = gpmi_hid_scripts_count();

	scrl = malloc(sizeof(char *) * (n+1));
	for(i = hid_gpmi_script_info, n = 0; i != NULL; i = i->next, n++) {
		char *basename;

		basename = strrchr(i->name, PCB_DIR_SEPARATOR_C);
		if (basename == NULL)
			basename = i->name;
		else
			basename++;

		scrl[n] = Concat(basename, "\t", i->module_name, NULL);
	}
	scrl[n] = NULL;

	attr_make_enum(&attr[0],  "script", "Select an item from the list of scripts loaded", (const char **)scrl, -1);

	if (operations != NULL)
		attr_make_enum(&attr[1],  "operation", "Choose what to do with the script", operations, *operation);

	res = gui->attribute_dialog(attr, 1 + (operations != NULL), result, "GPMI manage scripts - select script", "Select one of the scripts already loaded");

	/* free scrl slots before return */
	for(s = scrl; *s != NULL; s++)
		free(*s);

	if (res) {
		if (operation != NULL)
			*operation = -1;
		return NULL;
	}

	if ((operations != NULL) && (operation != NULL))
		*operation = result[1].int_value;

/*	printf("res=%d\n", result[0].int_value);*/

	if (result[0].int_value != -1) {
		for(i = hid_gpmi_script_info, n = result[0].int_value; i != NULL && n != 0; i = i->next, n--);
/*		printf("name=%s\n", i->name);*/
		return i;
	}
	return NULL;
}

static hid_gpmi_script_info_t *load_script(void)
{
	char *fn, *ext;
	hid_gpmi_script_info_t *info;
	int default_mod = -1;
	pcb_hid_attribute_t attr[3];
	pcb_hid_attr_val_t result[3];
	char *exts[] = {
		".tcl",    "tcl",
		".lua",    "lua",
		".awk",    "mawk",
		".mawk",   "mawk",
		".py",     "python",
		".scm",    "scheme",
		".rb",     "mruby",
		".ruby",   "mruby",
		".st",     "stutter",
		".pas",    "ghli",
		".pl",     "perl",
		".php",    "php",
		".sh",     "cli",
		".bash",   "cli",
		NULL,      NULL
	};
	const char *modules[] = { "tcl", "lua", "mawk", "python","scheme", "mruby",
	                          "stutter", "ghli", "perl", "php", "cli", NULL };


	fn = gui->fileselect("Load script", "Load a GPMI script", NULL, NULL, "gpmi_load_script", HID_FILESELECT_READ);

	if (fn == NULL)
		return NULL;

	ext = strrchr(fn, '.');
	if (ext != NULL) {
		char **s;
		const char **i;
		/* find the extension in the extension->module pairs */
		for(s = exts; s[0] != NULL; s+=2)
			if (strcmp(ext, s[0]) == 0)
				break;

		/* if found, look up the "default enum value" for that module */
		if (s[1] != NULL) {
			int n;
			for(i = modules, n = 0; *i != NULL; i++,n++) {
				if (strcmp(*i, s[1]) == 0) {
					default_mod = n;
					break;
				}
			}
		}
	}

	attr_make_enum(&attr[0],  "module", "Select a GPMI module to interpret the script", modules, default_mod);

	if (gui->attribute_dialog(attr, 1, result, "GPMI manage scripts - select module", "Select one of GPMI modules to interpret the script"))
		return NULL;

	if (result[0].int_value < 0)
		return NULL;

	info = hid_gpmi_load_module(NULL, modules[result[0].int_value], fn, NULL);
	if (info == NULL)
		gui->report_dialog("GPMI script load", "Error loading the script.\nPlease consult the message log for details.");
	return info;
}

static void script_details(hid_gpmi_script_info_t *i)
{
	pcb_hid_attribute_t attr[4];
	pcb_hid_attr_val_t result[4];
	char *cf;

	cf = i->conffile_name == NULL ? "<none>" : i->conffile_name;


	attr_make_label_str(&attr[0], "File name:   ", i->name, "File name of the script (if not absolute, it's relative to the config file)");
	attr_make_label_str(&attr[1], "GPMI module: ", i->module_name, "Name of the GPMI module that is interpreting the script");
	attr_make_label_str(&attr[2], "Config file: ", cf, "Name of config file that requested the script to be loaded ");
	gui->attribute_dialog(attr, 3, result, "GPMI manage scripts - script details", "Displaying detailed info on a script already loaded");
	free((char *)attr[0].name);
	free((char *)attr[1].name);
	free((char *)attr[2].name);
}

void gpmi_hid_manage_scripts(void)
{
	hid_gpmi_script_info_t *i;
	static const char *err_no_script = "Error: you didn't select a script";
#define CONSULT "Please consult the message log for details."

	const char *operations[] = {"show details...", "reload", "unload", "unload and remove from the config file", "load a new script...", "load a new script and add it in the config...", NULL};
	int op = 0;
	i = choose_script(operations, &op);
	switch(op) {
		case 0:
			if (i != NULL)
				script_details(i);
			else
				gui->report_dialog("GPMI script details", err_no_script);
			break;
		case 1:
			if (i != NULL) {
				i = hid_gpmi_reload_module(i);
				if (i == NULL)
					gui->report_dialog("GPMI script reload", "Error reloading the script.\nThe script is now unloaded.\n" CONSULT "\n(e.g. there may be syntax errors in the script source).");
			}
			else
				gui->report_dialog("GPMI script reload", err_no_script);
			break;
		case 2:
			if (i != NULL) {
				if (gpmi_hid_script_unload(i) != 0)
					gui->report_dialog("GPMI script unload", "Error unloading the script.\n" CONSULT "\n");
			}
			else
				gui->report_dialog("GPMI script unload", err_no_script);
			break;
		case 3:
			if (i != NULL) {
				int r1, r2;
				r1 = gpmi_hid_script_remove(i);
				r2 = gpmi_hid_script_unload(i);
				if (r1 || r2) {
					char *msg;
					msg = Concat("Error:", 
					             (r1 ? "couldnt't remove the script from the config file;" : ""), 
					             (r2 ? "couldnt't unload the script;" : ""),
					             "\n" CONSULT "\n", NULL);
					gui->report_dialog("GPMI script unload and remove", msg);
					free(msg);
				}
			else
				gui->report_dialog("GPMI script unload and remove", err_no_script);
			}
			break;
		case 4:
			load_script();
			break;
		case 5:
			i = load_script();
			if (i != NULL) {
				if (gpmi_hid_script_addcfg(i) != 0)
					gui->report_dialog("GPMI script add to config", "Error adding the script in user configuration.\n" CONSULT "\n");
			}
			break;
	}
}
