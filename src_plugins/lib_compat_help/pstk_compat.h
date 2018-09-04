#ifndef PCB_PSTK_COMPAT_H
#define PCB_PSTK_COMPAT_H

#include "obj_pstk.h"

typedef enum {
	PCB_PSTK_COMPAT_ROUND = 0,
	PCB_PSTK_COMPAT_SQUARE = 1,
	PCB_PSTK_COMPAT_SHAPED = 2, /* old pcb-rnd pin shapes */
	PCB_PSTK_COMPAT_SHAPED_END = 16, /* old pcb-rnd pin shapes */
	PCB_PSTK_COMPAT_OCTAGON,
	PCB_PSTK_COMPAT_INVALID
} pcb_pstk_compshape_t;

/* Create a padstack that emulates an old-style via - register proto as needed */
pcb_pstk_t *pcb_pstk_new_compat_via(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_coord_t pad_dia, pcb_coord_t clearance, pcb_coord_t mask, pcb_pstk_compshape_t shp, pcb_bool plated);

/* Convert an existing padstack to old-style via and return broken down parameters */
pcb_bool pcb_pstk_export_compat_via(pcb_pstk_t *ps, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t *drill_dia, pcb_coord_t *pad_dia, pcb_coord_t *clearance, pcb_coord_t *mask, pcb_pstk_compshape_t *cshape, pcb_bool *plated);

/* Create a padstack that emulates an old-style pad - register proto as needed */
pcb_pstk_t *pcb_pstk_new_compat_pad(pcb_data_t *data, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thickness, pcb_coord_t clearance, pcb_coord_t mask, pcb_bool square, pcb_bool nopaste, pcb_bool onotherside);

/* Convert an existing padstack to old-style pad and return broken down parameters */
pcb_bool pcb_pstk_export_compat_pad(pcb_pstk_t *ps, pcb_coord_t *x1, pcb_coord_t *y1, pcb_coord_t *x2, pcb_coord_t *y2, pcb_coord_t *thickness, pcb_coord_t *clearance, pcb_coord_t *mask, pcb_bool *square, pcb_bool *nopaste);

/* Convert padstack flags to old pin/via flag. Use only in gEDA/PCB
   compatibility code: io_pcb and io_lihata. */
pcb_flag_t pcb_pstk_compat_pinvia_flag(pcb_pstk_t *ps, pcb_pstk_compshape_t cshape);

#define PCB_PSTK_VIA_COMPAT_FLAGS (PCB_FLAG_CLEARLINE | PCB_FLAG_SELECTED | PCB_FLAG_FOUND | PCB_FLAG_WARN | PCB_FLAG_USETHERMAL | PCB_FLAG_LOCK)

/* Create a padstack that mimics the old gEDA/PCB via (or pin). Should not
   be used anywhere but io_pcb and io_lihata. */
pcb_pstk_t *pcb_old_via_new(pcb_data_t *data, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, pcb_flag_t Flags);

#endif
