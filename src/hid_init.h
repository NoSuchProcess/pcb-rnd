#ifndef PCB_HID_INIT_H
#define PCB_HID_INIT_H

#include <puplug/puplug.h>
#include <genvector/vtp0.h>
#include "hid.h"

#define PCBHL_ACTION_ARGS_WIDTH 5

/* NULL terminated list of all static HID structures.  Built by
   hid_register_hid, used by hid_find_*() and pcb_hid_enumerate().  The
   order in this list is the same as the order of hid_register_hid
   calls.  */
extern pcb_hid_t **pcb_hid_list;

/* Count of entries in the above.  */
extern int pcb_hid_num_hids;

/* Call this as soon as possible from main().  No other HID calls are
   valid until this is called.  */
void pcb_hid_init(void);

/* Call this at exit */
void pcb_hid_uninit(void);

/* When PCB runs in interactive mode, this is called to instantiate
   one GUI HID which happens to be the GUI.  This HID is the one that
   interacts with the mouse and keyboard.  */
pcb_hid_t *pcb_hid_find_gui(const char *preference);

/* Finds the one printer HID and instantiates it.  */
pcb_hid_t *pcb_hid_find_printer(void);

/* Finds the indicated exporter HID and instantiates it.  */
pcb_hid_t *pcb_hid_find_exporter(const char *);

/* This returns a NULL-terminated array of available HIDs.  The only
   real reason to use this is to locate all the export-style HIDs. */
pcb_hid_t **pcb_hid_enumerate(void);

/* HID internal interfaces.  These may ONLY be called from the HID
   modules, not from the common PCB code.  */

/* A HID may use this if it does not need command line arguments in
   any special format; for example, the Lesstif HID needs to use the
   Xt parser, but the Postscript HID can use this function.  */
int pcb_hid_parse_command_line(int *argc, char ***argv);

/* Called by the init funcs, used to set up pcb_hid_list.  */
extern void pcb_hid_register_hid(pcb_hid_t * hid);
void pcb_hid_remove_hid(pcb_hid_t * hid);

typedef struct pcb_plugin_dir_s pcb_plugin_dir_t;
struct pcb_plugin_dir_s {
	char *path;
	int num_plugins;
	pcb_plugin_dir_t *next;
};

extern pcb_plugin_dir_t *pcb_plugin_dir_first, *pcb_plugin_dir_last;

/* Safe file name for inclusion in export file comments/headers; if the
   user requested in the config, this becomes the basename of filename,
   else it is the full file name */
const char *pcb_hid_export_fn(const char *filename);


/*** main(), initialize ***/

typedef struct {
	enum {
		DO_SOMETHING,
		DO_PRINT,
		DO_EXPORT,
		DO_GUI
	} do_what;
	int hid_argc;
	const char *main_action, *main_action_hint;
	vtp0_t plugin_cli_conf;
	char **hid_argv_orig, **hid_argv;
	const char *hid_name;
	const char **action_args;
	int autopick_gui;
} pcbhl_main_args_t;

/* call this before anything, to switch locale to "C" permanently */
void pcb_fix_locale(void);

void pcb_hidlib_init1(void (*conf_core_init)(void)); /* before CLI argument parsing; conf_core_init should conf_reg() at least the hidlib related nodes */
void pcb_hidlib_init2(const pup_buildin_t *buildins); /* after CLI argument parsing */
void pcb_hidlib_uninit(void);

int pcbhl_main_arg_match(const char *in, const char *shrt, const char *lng);

/* parse arguments using the gui; if fails and fallback is enabled, try the next gui */
int pcb_gui_parse_arguments(int autopick_gui, int *hid_argc, char **hid_argv[]);

#endif
