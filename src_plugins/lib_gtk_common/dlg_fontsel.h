#include <gtk/gtk.h>
#include "global_typedefs.h"
#include "hid.h"
#include "glue.h"

/* type is PCB_TYPE_TEXT or PCB_TYPE_ELEMENT_NAME */
void pcb_gtk_dlg_fontsel(pcb_gtk_common_t *com, pcb_layer_t *txtly, pcb_text_t *txt, int type, int modal);

