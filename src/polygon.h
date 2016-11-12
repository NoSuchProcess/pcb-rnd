/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for polygon editing routines */

#ifndef	PCB_POLYGON_H
#define	PCB_POLYGON_H

#include "config.h"
#include "flag.h"
#include "rtree.h"
#include "math_helper.h"
#include "polyarea.h"

/* Implementation constants */

#define POLY_CIRC_SEGS 40
#define POLY_CIRC_SEGS_F ((float)POLY_CIRC_SEGS)

/* adjustment to make the segments outline the circle rather than connect
 * points on the circle: 1 - cos (\alpha / 2) < (\alpha / 2) ^ 2 / 2
 */
#define POLY_CIRC_RADIUS_ADJ (1.0 + M_PI / POLY_CIRC_SEGS_F * \
                                    M_PI / POLY_CIRC_SEGS_F / 2.0)

/* polygon diverges from modelled arc no more than MAX_ARC_DEVIATION * thick */
#define POLY_ARC_MAX_DEVIATION 0.02

/* Prototypes */

void polygon_init(void);
pcb_cardinal_t polygon_point_idx(pcb_polygon_t *polygon, pcb_point_t *point);
pcb_cardinal_t polygon_point_contour(pcb_polygon_t *polygon, pcb_cardinal_t point);
pcb_cardinal_t prev_contour_point(pcb_polygon_t *polygon, pcb_cardinal_t point);
pcb_cardinal_t next_contour_point(pcb_polygon_t *polygon, pcb_cardinal_t point);
pcb_cardinal_t GetLowestDistancePolygonPoint(pcb_polygon_t *, pcb_coord_t, pcb_coord_t);
pcb_bool RemoveExcessPolygonPoints(pcb_layer_t *, pcb_polygon_t *);
void GoToPreviousPoint(void);
void ClosePolygon(void);
void CopyAttachedPolygonToLayer(void);
int PolygonHoles(pcb_polygon_t * ptr, const pcb_box_t * range, int (*callback) (pcb_pline_t *, void *user_data), void *user_data);
int PlowsPolygon(pcb_data_t *, int, void *, void *,
								 pcb_r_dir_t (*callback) (pcb_data_t *, pcb_layer_t *, pcb_polygon_t *, int, void *, void *));
void ComputeNoHoles(pcb_polygon_t * poly);
pcb_polyarea_t *ContourToPoly(pcb_pline_t *);
pcb_polyarea_t *PolygonToPoly(pcb_polygon_t *);
pcb_polyarea_t *RectPoly(pcb_coord_t x1, pcb_coord_t x2, pcb_coord_t y1, pcb_coord_t y2);
pcb_polyarea_t *CirclePoly(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius);
pcb_polyarea_t *OctagonPoly(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, int style);
pcb_polyarea_t *LinePoly(pcb_line_t * l, pcb_coord_t thick);
pcb_polyarea_t *ArcPoly(pcb_arc_t * l, pcb_coord_t thick);
pcb_polyarea_t *PinPoly(pcb_pin_t * l, pcb_coord_t thick, pcb_coord_t clear);
pcb_polyarea_t *BoxPolyBloated(pcb_box_t * box, pcb_coord_t radius);
void frac_circle(pcb_pline_t *, pcb_coord_t, pcb_coord_t, pcb_vector_t, int);
int InitClip(pcb_data_t * d, pcb_layer_t * l, pcb_polygon_t * p);
void RestoreToPolygon(pcb_data_t *, int, void *, void *);
void ClearFromPolygon(pcb_data_t *, int, void *, void *);

pcb_bool IsPointInPolygon(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_polygon_t *);
pcb_bool IsPointInPolygonIgnoreHoles(pcb_coord_t, pcb_coord_t, pcb_polygon_t *);
pcb_bool IsRectangleInPolygon(pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_coord_t, pcb_polygon_t *);
pcb_bool isects(pcb_polyarea_t *, pcb_polygon_t *, pcb_bool);
pcb_bool MorphPolygon(pcb_layer_t *, pcb_polygon_t *);
void NoHolesPolygonDicer(pcb_polygon_t * p, const pcb_box_t * clip, void (*emit) (pcb_pline_t *, void *), void *user_data);
void PolyToPolygonsOnLayer(pcb_data_t *, pcb_layer_t *, pcb_polyarea_t *, pcb_flag_t);

void square_pin_factors(int style, double *xm, double *ym);


#endif
