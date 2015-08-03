#include <gpmi.h>

gpmi_define_event(ACTE_action)(const char *name, int argc, int x, int y);

/* register an action in PCB - will generate event ACTE_action */
int action_register(const char *name, const char *need_xy, const char *description, const char *syntax, const char *context);

/* extract argument argn for the current action (makes sense only in an ACTE_action event handler */
const char *action_arg(int argn);

/* call an existing action */
int action(const char *cmdline);
