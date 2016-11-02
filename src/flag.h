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

#ifndef PCB_FLAG_H
#define PCB_FLAG_H

#include "globalconst.h"

/* Nobody should know about the internals of this except the macros in
   macros.h that access it.  This structure must be simple-assignable
   for now.  */
typedef struct unknown_flag_s unknown_flag_t;
struct unknown_flag_s {
	char *str;
	unknown_flag_t *next;
};

typedef struct {
	unsigned long f;							/* generic flags */
	unsigned char t[(MAX_LAYER + 1) / 2];	/* thermals */
	unsigned char q;							/* square geometry flag */
	unsigned char int_conn_grp;
	unknown_flag_t *unknowns;
} FlagType, *FlagTypePtr;



extern FlagType no_flags;


/* For passing modified flags to other functions. */
FlagType MakeFlags(unsigned int);
FlagType OldFlags(unsigned int);
FlagType AddFlags(FlagType, unsigned int);
FlagType MaskFlags(FlagType, unsigned int);
void EraseFlags(FlagType *f);
#define		NoFlags() MakeFlags(0)

/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changing and testing
 */
#define	SET_FLAG(F,P)		((P)->Flags.f |= (F))
#define	CLEAR_FLAG(F,P)		((P)->Flags.f &= (~(F)))
#define	TEST_FLAG(F,P)		((P)->Flags.f & (F) ? 1 : 0)
#define	TOGGLE_FLAG(F,P)	((P)->Flags.f ^= (F))
#define	ASSIGN_FLAG(F,V,P)	((P)->Flags.f = ((P)->Flags.f & (~(F))) | ((V) ? (F) : 0))
#define TEST_FLAGS(F,P)         (((P)->Flags.f & (F)) == (F) ? 1 : 0)

typedef enum {
	PCB_CHGFLG_CLEAR,
	PCB_CHGFLG_SET,
	PCB_CHGFLG_TOGGLE
} pcb_change_flag_t;

#define CHANGE_FLAG(how, F, P) \
do { \
	switch(how) { \
		case PCB_CHGFLG_CLEAR:  CLEAR_FLAG(F, P); break; \
		case PCB_CHGFLG_SET:    SET_FLAG(F, P); break; \
		case PCB_CHGFLG_TOGGLE: TOGGLE_FLAG(F, P); break; \
	} \
} while(0)

#define FLAGS_EQUAL(F1,F2)	(memcmp (&F1, &F2, sizeof(FlagType)) == 0)

#define THERMFLAG(L)		(0xf << (4 *((L) % 2)))

#define TEST_THERM(L,P)		((P)->Flags.t[(L)/2] & THERMFLAG(L) ? 1 : 0)
#define GET_THERM(L,P)		(((P)->Flags.t[(L)/2] >> (4 * ((L) % 2))) & 0xf)
#define CLEAR_THERM(L,P)	(P)->Flags.t[(L)/2] &= ~THERMFLAG(L)
#define ASSIGN_THERM(L,V,P)	(P)->Flags.t[(L)/2] = ((P)->Flags.t[(L)/2] & ~THERMFLAG(L)) | ((V)  << (4 * ((L) % 2)))


#define GET_SQUARE(P)		((P)->Flags.q)
#define CLEAR_SQUARE(P)		(P)->Flags.q = 0
#define ASSIGN_SQUARE(V,P)	(P)->Flags.q = V


#define GET_INTCONN(P)		((P)->Flags.int_conn_grp)

extern int mem_any_set(unsigned char *, int);
#define TEST_ANY_THERMS(P)	mem_any_set((P)->Flags.t, sizeof((P)->Flags.t))

#endif
