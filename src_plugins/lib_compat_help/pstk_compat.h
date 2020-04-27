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

/* Create a padstack that emulates an old-style via - register proto as needed
   if id <= 0, allocate a new id automatically */
pcb_pstk_t *pcb_pstk_new_compat_via(pcb_data_t *data, long int id, rnd_coord_t x, rnd_coord_t y, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t clearance, rnd_coord_t mask, pcb_pstk_compshape_t shp, rnd_bool plated);

/* Convert an existing padstack to old-style via and return broken down parameters */
rnd_bool pcb_pstk_export_compat_via(pcb_pstk_t *ps, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t *drill_dia, rnd_coord_t *pad_dia, rnd_coord_t *clearance, rnd_coord_t *mask, pcb_pstk_compshape_t *cshape, rnd_bool *plated);

/* Create a padstack that emulates an old-style pad - register proto as needed 
   If id <= 0, allocate an id automatically. */
pcb_pstk_t *pcb_pstk_new_compat_pad(pcb_data_t *data, long int id, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness, rnd_coord_t clearance, rnd_coord_t mask, rnd_bool square, rnd_bool nopaste, rnd_bool onotherside);

/* Convert an existing padstack to old-style pad and return broken down parameters */
rnd_bool pcb_pstk_export_compat_pad(pcb_pstk_t *ps, rnd_coord_t *x1, rnd_coord_t *y1, rnd_coord_t *x2, rnd_coord_t *y2, rnd_coord_t *thickness, rnd_coord_t *clearance, rnd_coord_t *mask, rnd_bool *square, rnd_bool *nopaste);

typedef enum {
	PCB_PSTKCOMP_OLD_OCTAGON = 1
} pcb_pstk_compat_t;

/* Convert padstack flags to old pin/via flag. Use only in gEDA/PCB
   compatibility code: io_pcb and io_lihata. */
pcb_flag_t pcb_pstk_compat_pinvia_flag(pcb_pstk_t *ps, pcb_pstk_compshape_t cshape, pcb_pstk_compat_t compat);

#define PCB_PSTK_VIA_COMPAT_FLAGS (PCB_FLAG_CLEARLINE | PCB_FLAG_SELECTED | PCB_FLAG_FOUND | PCB_FLAG_WARN | PCB_FLAG_USETHERMAL | PCB_FLAG_LOCK)

/* Create a padstack that mimics the old gEDA/PCB via (or pin).
   If id <= 0, allocate a new ID automatically.
   Should not be used anywhere but io_pcb and io_lihata. */
pcb_pstk_t *pcb_old_via_new(pcb_data_t *data, long int id, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Thickness, rnd_coord_t Clearance, rnd_coord_t Mask, rnd_coord_t DrillingHole, const char *Name, pcb_flag_t Flags);

#endif
