/* Take an arc specified by start, end and signed delta (sweep) angle in degree
   and compute center, radius and start/end angle in radian. Coord system is
   x+ to the right, y+ upward, positive angles CCW */
int arc_start_end_delta(double sx, double sy, double ex, double ey, double deg, double *cx, double *cy, double *r, double *srad, double *erad);
