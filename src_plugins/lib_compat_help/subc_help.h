#ifndef PCB_SUBC_HELP_H
#define PCB_SUBC_HELP_H

#include "obj_subc.h"
#include "layer.h"

/* Create dynamic text on the top silk layer (creates the layer if needed).
   Returns the text object, or NULL on error. Does not set any subc
   attribute. The refdes version is just a shorthand for the pattern. */
pcb_text_t *pcb_subc_add_dyntex(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale, pcb_bool bottom, const char *pattern);
pcb_text_t *pcb_subc_add_refdes_text(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, unsigned direction, int scale, pcb_bool bottom);

/* Returns the refdes text objects; must be DYNTEXT and must contain the
   refdes attribute printing. If there's are multiple objects, silk is
   preferred. */
pcb_text_t *pcb_subc_get_refdes_text(pcb_subc_t *sc);

#endif
