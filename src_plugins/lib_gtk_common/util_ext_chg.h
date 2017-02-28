#ifndef PCB_GTK_UTIL_EXT_CHG_H
#define PCB_GTK_UTIL_EXT_CHG_H

#include <glib.h>
#include "pcb_bool.h"

typedef struct {
	GTimeVal our_mtime;
	GTimeVal last_seen_mtime;
} pcb_gtk_ext_chg_t;

pcb_bool check_externally_modified(pcb_gtk_ext_chg_t *ec);
void update_board_mtime_from_disk(pcb_gtk_ext_chg_t *ec);


#endif
