/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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

/* prototypes for misc routines - PCB data type dependent ones */

#ifndef	PCB_MISC_H
#define	PCB_MISC_H

#include <stdlib.h>
#include "config.h"
#include <genvector/gds_char.h>
#include "mymem.h"


void r_delete_element(DataTypePtr, ElementTypePtr);
void SetLineBoundingBox(LineTypePtr);
void SetArcBoundingBox(ArcTypePtr);
void SetPointBoundingBox(PointTypePtr);
void SetPinBoundingBox(PinTypePtr);
void SetPadBoundingBox(PadTypePtr);
void SetPolygonBoundingBox(PolygonTypePtr);
void SetElementBoundingBox(DataTypePtr, ElementTypePtr, FontTypePtr);
void CountHoles(int *, int *, const BoxType *);
BoxTypePtr GetDataBoundingBox(DataTypePtr);
void SetFontInfo(FontTypePtr);

void SetTextBoundingBox(FontTypePtr, TextTypePtr);

BoxTypePtr GetObjectBoundingBox(int, void *, void *, void *);

BoxTypePtr GetArcEnds(ArcTypePtr);
void ChangeArcAngles(LayerTypePtr, ArcTypePtr, Angle, Angle);
void ChangeArcRadii(LayerTypePtr, ArcTypePtr, Coord, Coord);
char *UniqueElementName(DataTypePtr, char *);
void AttachForCopy(Coord, Coord);

/* Return a relative rotation for an element, useful only for
   comparing two similar footprints.  */
int ElementOrientation(ElementType * e);



void QuitApplication(void);

/* Returns a string with info about this copy of pcb. */
char *GetInfoString(void);

void SaveOutputWindow(void);
void CenterDisplay(Coord, Coord);

char *EvaluateFilename(const char *, const char *, const char *, const char *);

pcb_bool IsDataEmpty(DataTypePtr);
pcb_bool IsPasteEmpty(int);

const char *pcb_author(void);


#endif /* PCB_MISC_H */
