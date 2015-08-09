typedef struct hid_gpmi_script_info_s hid_gpmi_script_info_t;

struct hid_gpmi_script_info_s {
	char *name;
	char *module_name;
	char *conffile_name;
	gpmi_module *module;
	
	hid_gpmi_script_info_t *next;
};

extern hid_gpmi_script_info_t *hid_gpmi_script_info;

/* Load a GPMI module; if i is NULL, allocate a new slot on the list, else unload
   i first and place the new module in place of i. */
hid_gpmi_script_info_t *hid_gpmi_load_module(hid_gpmi_script_info_t *i, const char *module_name, const char *params, const char *config_file_name);

/* Reload a script - useful if the source of the script has changed
   Reloads a module already loaded; return NULL on error. */
hid_gpmi_script_info_t *hid_gpmi_reload_module(hid_gpmi_script_info_t *i);

/* look up a module by name - slow linear search */
hid_gpmi_script_info_t *hid_gpmi_lookup(const char *name);

void hid_gpmi_load_dir(const char *dir, int add_pkg_path);
char *gpmi_hid_asm_scriptname(const void *info, const char *file_name);

/* Return the number of scripts (gpmi modules) loaded */
int gpmi_hid_scripts_count();

/* Unload a script (also removes i from the list) - temporary effect,
   the script will be loaded again on the next startup */
int gpmi_hid_script_unload(hid_gpmi_script_info_t *i);

/* Remove a script from the config file (but do not unload it now) */
int gpmi_hid_script_remove(hid_gpmi_script_info_t *i);

/* Edit a config file so that the script is in it */
int gpmi_hid_script_addcfg(hid_gpmi_script_info_t *i);
