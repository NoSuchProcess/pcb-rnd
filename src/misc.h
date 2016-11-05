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
#include "mymem.h"


void r_delete_element(DataTypePtr, ElementTypePtr);
void SetElementBoundingBox(DataTypePtr, ElementTypePtr, FontTypePtr);
void SetFontInfo(FontTypePtr);

BoxTypePtr GetObjectBoundingBox(int, void *, void *, void *);

char *UniqueElementName(DataTypePtr, char *);
void AttachForCopy(Coord, Coord);

/* Return a relative rotation for an element, useful only for
   comparing two similar footprints.  */
int ElementOrientation(ElementType * e);

char *EvaluateFilename(const char *, const char *, const char *, const char *);


#endif /* PCB_MISC_H */
