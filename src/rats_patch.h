#include "global.h"
void rats_patch_append(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2);

/* Create [NETLIST_EDITED] from [NETLIST_INPUT] applying the patch */
void rats_patch_make_edited(PCBTypePtr pcb);

/* apply a single patch record on [NETLIST_EDITED]; returns non-zero on failure */
int rats_patch_apply(PCBTypePtr pcb, rats_patch_line_t *patch);

/* find the net entry for a pin (slow) */
LibraryMenuTypePtr rats_patch_find_net4pin(PCBTypePtr pcb, const char *pin);
