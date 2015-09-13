#include "global.h"
void rats_patch_append(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2);

/* Create [NETLIST_EDITED] from [NETLIST_INPUT] applying the patch */
void rats_patch_apply(PCBTypePtr pcb);
