#include <gpmi.h>
#include "src/pcb-printf.h"

#undef snprintf
#define snprintf pcb_snprintf

/* Generated when an action registered by the script is executed.
   Arguments:
    name: name of the action (as registed using function action_register())
    argc: number of arguments. Arguments can be accessed using function action_arg
    x, y: optional coords, if need_xy was not empty at action_register */
gpmi_define_event(ACTE_action)(const char *name, int argc, int x, int y);

/* Generated right after gui initialization, before the gui main loop.
   Arguments:
    argc: number of arguments the gui was initialized with.
    argv[]: arguments the gui was initialized with - unaccessible for the scripts. */
gpmi_define_event(ACTE_gui_init)(int argc, char **argv);


/* Generated right before unloading a script to give the script a chance
   to clean up.
   Arguments:
    conffile: the name of the config file that originally triggered laoding the script, or empty if the script was loaded from the gui. */
gpmi_define_event(ACTE_unload)(const char *conffile);

/* Register an action in PCB - when the action is executed, event
   ACTE_action is generated with the action name.
   Multiple actions can be registered. Any action registered by the script
   will trigger an ACTE_event sent to the script.
   Arguments:
    name: name of the action
    need_xy: the question the user is asked when he needs to choose a coordinate; if empty, no coordinate is asked
    description: description of the action (for the help)
    syntax: syntax of the action (for the help)
   Returns 0 on success.
 */
int action_register(const char *name, const char *need_xy, const char *description, const char *syntax);

/* extract the (argn)th event argument for the current action (makes sense only in an ACTE_action event handler */
const char *action_arg(int argn);

/* call an existing action using PCB syntax (e.g. foo(1, 2, 3))
   Returns non-zero on error; generally returns value of the action
   (which is also non-zero on error). */
int action(const char *cmdline);

/* Create a new menu or submenu at path. Missing parents are created
   automatically with empty action, mnemonic, hotkey and tooltip.
   Arguments:
    path: the full path of the new menu
    action: this action is executed when the user clicks on the menu
    mnemonic: which letter to underline in the menu text (will be the fast-jump-there key once the menu is open)
    hotkey: when this key is pressed in the main gui, the action is also triggered; the format is modifiers&lt;Key&gt;letter, where modifiers is Alt, Shift or Ctrl. This is the same syntax that is used in the .res files.
    tooltip: short help text */
void create_menu(const char *path, const char *action, const char *mnemonic, const char *hotkey, const char *tooltip);
