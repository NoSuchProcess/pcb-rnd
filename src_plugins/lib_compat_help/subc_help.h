#ifndef PCB_SUBC_HELP_H
#define PCB_SUBC_HELP_H

#include "obj_subc.h"
#include "layer.h"

/* Look up a layer by lyt and comb (and name, if req_name_match is true);
   if not found and alloc is true, allocate a new layer with the given name.
   Return NULL on error. */
pcb_layer_t *pcb_subc_get_layer(pcb_subc_t *sc, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_bool_t alloc, const char *name, pcb_bool req_name_match);

/* Create refdes text on the top silk layer (creates the layer if needed).
   Returns the text object, or NULL on error. Does not set the subc refdes
   attribute. */
pcb_text_t *pcb_subc_add_refdes_text(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale, pcb_bool bottom);

/* Returns the refdes text objects; must be DYNTEXT and must contain the
   refdes attribute printing. If there's are multiple objects, silk is
   preferred. */
pcb_text_t *pcb_subc_get_refdes_text(pcb_subc_t *sc);

#endif
