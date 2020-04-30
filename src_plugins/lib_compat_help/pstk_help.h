#ifndef PCB_PSTK_HELP_H
#define PCB_PSTK_HELP_H

#include "obj_pstk.h"
#include <genvector/vtp0.h>

/* create a new adstack that contains only a hole, but no shapes */
pcb_pstk_t *pcb_pstk_new_hole(pcb_data_t *data, rnd_coord_t x, rnd_coord_t y, rnd_coord_t drill_dia, rnd_bool plated);

/* Convert an array of shapes, terminated by a shape with layer_mask=0, into
   a padstack. */
pcb_pstk_t *pcb_pstk_new_from_shape(pcb_data_t *data, rnd_coord_t x, rnd_coord_t y, rnd_coord_t drill_dia, rnd_bool plated, rnd_coord_t glob_clearance, pcb_pstk_shape_t *shape);

/* Convert a vector of (pcb_any_obj_t *) into zero or more padstacks. Remove
   objects that are converted  from both data and objs. New padstacks are
   placed back in objs. Quiet controls log messages. Return the number
   of padstacks created. Return -1 on error.
   WARNING: O(n^2) loops, assuming there are only a dozen of objects passed. */
int pcb_pstk_vect2pstk_thr(pcb_data_t *data, vtp0_t *objs, rnd_bool_t quiet); /* thru-hole pins only */
int pcb_pstk_vect2pstk_smd(pcb_data_t *data, vtp0_t *objs, rnd_bool_t quiet); /* smd pads only */

/* Same as above, but convert both; returns -1 only if both failed */
int pcb_pstk_vect2pstk(pcb_data_t *data, vtp0_t *objs, rnd_bool_t quiet);

/*** shape generators ***/
void pcb_shape_rect(pcb_pstk_shape_t *shape, rnd_coord_t width, rnd_coord_t height);
void pcb_shape_oval(pcb_pstk_shape_t *shape, rnd_coord_t width, rnd_coord_t height);

/* trapezoid: deform the rectangle so that upper horizontal edge is smaller
   by dx and lower horizontal edge is larger by dx; same happens to the vertical
   edges and dy. */
void pcb_shape_rect_trdelta(pcb_pstk_shape_t *shape, rnd_coord_t width, rnd_coord_t height, rnd_coord_t dx, rnd_coord_t dy);

/* Create a regular octagon shape */
void pcb_shape_octagon(pcb_pstk_shape_t *dst, rnd_coord_t radiusx, rnd_coord_t radiusy);


#endif
