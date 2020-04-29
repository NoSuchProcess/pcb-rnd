#include <librnd/core/hid.h>
#include <librnd/core/event.h>
#include <librnd/core/conf_hid.h>

extern rnd_conf_hid_id_t ghid_conf_id;

void pcb_gtk_conf_init(void);
void pcb_gtk_conf_uninit(void);

/* Parses string_path to expand and select the corresponding path in tree view. */
void pcb_gtk_config_set_cursor(const char *string_path);



