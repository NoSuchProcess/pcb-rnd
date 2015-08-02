typedef struct script_info_s script_info_t;

struct script_info_s {
	char *name;
	char *module_name;
	char *conffile_name;
	gpmi_module *module;
	
	script_info_t *next;
};

extern script_info_t *script_info;

gpmi_module *hid_gpmi_load_module(const char *module_name, const char *params, const char *config_file_name);
void hid_gpmi_load_dir(const char *dir);
char *gpmi_hid_asm_scriptname(const void *info, const char *file_name);
