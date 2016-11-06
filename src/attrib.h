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

/* attribute lists */

#ifndef PCB_ATTRIB_H
#define PCB_ATTRIB_H

typedef struct AttributeListType AttributeListType, *AttributeListTypePtr;

typedef struct {
	char *name;
	char *value;
} AttributeType, *AttributeTypePtr;

struct AttributeListType {
	int Number, Max;
	AttributeType *List;
};

/* Returns NULL if the name isn't found, else the value for that named
   attribute.  */
char *AttributeGetFromList(AttributeListType * list, const char *name);
/* Adds an attribute to the list.  If the attribute already exists,
   whether it's replaced or a second copy added depends on
   REPLACE.  Returns non-zero if an existing attribute was replaced.  */
int AttributePutToList(AttributeListType * list, const char *name, const char *value, int replace);
/* Simplistic version: Takes a pointer to an object, looks up attributes in it.  */
#define AttributeGet(OBJ,name) AttributeGetFromList (&(OBJ->Attributes), name)
/* Simplistic version: Takes a pointer to an object, sets attributes in it.  */
#define AttributePut(OBJ,name,value) AttributePutToList (&(OBJ->Attributes), name, value, 1)
/* Remove an attribute by name; returns number of items removed  */
int AttributeRemoveFromList(AttributeListType * list, const char *name);
/* Simplistic version of Remove.  */
#define AttributeRemove(OBJ, name) AttributeRemoveFromList (&(OBJ->Attributes), name)

void FreeAttributeListMemory(AttributeListTypePtr list);

#endif
