#include <gpmi.h>

gpmi_define_event(ACTE_action)(const char *name, int argc, int x, int y);
gpmi_define_event(ACTE_gui_init)(int argc, char **argv);
gpmi_define_event(ACTE_unload)(const char *conffile);

/* register an action in PCB - will generate event ACTE_action */
int action_register(const char *name, const char *need_xy, const char *description, const char *syntax, const char *context);

/* extract argument argn for the current action (makes sense only in an ACTE_action event handler */
const char *action_arg(int argn);

/* call an existing action */
int action(const char *cmdline);

/* Create a new menu at the given path in the menu system */
void create_menu(const char *path, const char *action, const char *mnemonic, const char *hotkey, const char *tooltip);
