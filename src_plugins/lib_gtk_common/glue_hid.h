#include "hid.h"

void ghid_glue_hid_init(pcb_hid_t *dst);
void gtkhid_do_export(pcb_hid_t *hid, pcb_hidlib_t *hidlib, pcb_hid_attr_val_t *options);
int gtkhid_parse_arguments(int *argc, char ***argv);

