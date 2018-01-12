#ifndef PCB_SUBC_HELP_H
#define PCB_SUBC_HELP_H

#include "obj_subc.h"
#include "layer.h"

/* Look up a layer by lyt and comb; if not found and alloc is true, allocate
   a new layer with the given name. Return NULL on error. */
pcb_layer_t *pcb_subc_get_layer(pcb_subc_t *sc, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_bool_t alloc, const char *name);

#endif
