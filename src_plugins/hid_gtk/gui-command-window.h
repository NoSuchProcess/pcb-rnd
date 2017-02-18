#include "pcb_bool.h"

void ghid_handle_user_command(pcb_bool raise);
void ghid_command_window_show(pcb_bool raise);
char *ghid_command_entry_get(const char *prompt, const char *command);
void ghid_command_use_command_window_sync(void);
