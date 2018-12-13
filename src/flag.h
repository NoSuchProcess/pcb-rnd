/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef PCB_FLAG_H
#define PCB_FLAG_H

#include "globalconst.h"

/* Nobody should know about the internals of this except the macros in
   macros.h that access it.  This structure must be simple-assignable
   for now.  */
typedef struct pcb_unknown_flag_s pcb_unknown_flag_t;
struct pcb_unknown_flag_s {
	char *str;
	pcb_unknown_flag_t *next;
};

#define PCB_DYNFLAG_WORD unsigned long
#define PCB_DYNFLAG_WLEN ((PCB_DYNFLAG_BLEN-1) / sizeof(PCB_DYNFLAG_WORD)+1)
typedef PCB_DYNFLAG_WORD pcb_dynflag_t[PCB_DYNFLAG_WLEN];

typedef struct {
	unsigned long f;                           /* generic statically assigned flag bits */
	pcb_dynflag_t df;                          /* dynamically allocated flag bits */
	unsigned char t[(PCB_MAX_LAYER + 1) / 2];  /* thermals */
	unsigned char q;                           /* square geometry flag - need to keep only for .pcb compatibility */
	pcb_unknown_flag_t *unknowns;
} pcb_flag_t;

extern pcb_flag_t no_flags;

/*** object flags ***/
typedef enum {
	PCB_FLAG_NO           = 0x00000,
	PCB_FLAG_PIN          = 0x00001,
	PCB_FLAG_VIA          = 0x00002,
	PCB_FLAG_FOUND        = 0x00004,
	PCB_FLAG_HOLE         = 0x00008,
	PCB_FLAG_NOPASTE      = 0x00008,
	PCB_FLAG_RAT          = 0x00010,
	PCB_FLAG_PININPOLY    = 0x00010,
	PCB_FLAG_CLEARPOLY    = 0x00010,
	PCB_FLAG_HIDENAME     = 0x00010,
	PCB_FLAG_DISPLAYNAME  = 0x00020,
	PCB_FLAG_CLEARLINE    = 0x00020,
	PCB_FLAG_FULLPOLY     = 0x00020,
	PCB_FLAG_SELECTED     = 0x00040,
	PCB_FLAG_ONSOLDER     = 0x00080,
	PCB_FLAG_AUTO         = 0x00080,
	PCB_FLAG_SQUARE       = 0x00100,
	PCB_FLAG_RUBBEREND    = 0x00200,
	PCB_FLAG_WARN         = 0x00200,
	PCB_FLAG_USETHERMAL   = 0x00400,
	PCB_FLAG_ONSILK       = 0x00400, /* Obsolete, old files used this to indicate lines drawn on silk. (Used by io_pcb for compatibility.) */
	PCB_FLAG_OCTAGON      = 0x00800,
	PCB_FLAG_DRC          = 0x01000,
	PCB_FLAG_LOCK         = 0x02000,
	PCB_FLAG_EDGE2        = 0x04000,
	PCB_FLAG_VISIT        = 0x08000, /* marker to avoid re-visiting an object */
	PCB_FLAG_NONETLIST    = 0x10000,
	PCB_FLAG_MINCUT       = 0x20000, /* used by the mincut short find code */
	PCB_FLAG_ONPOINT      = 0x40000, /* crosshair is on line point or arc point */
	PCB_FLAG_TERMNAME     = 0x80000, 
	PCB_FLAG_DRC_INTCONN  = 0x100000,/* Set for objects are put on the DRC mark because of an intconn */
	PCB_FLAG_CLEARPOLYPOLY= 0x200000,
	PCB_FLAG_DYNTEXT      = 0x400000,
	PCB_FLAG_FLOATER      = 0x800000
/*	PCB_FLAG_NOCOPY     = (PCB_FLAG_FOUND | CONNECTEDFLAG | PCB_FLAG_ONPOINT)*/
} pcb_flag_values_t;

#define PCB_FLAGS               0x01ffffff	/* all used flags */

#define PCB_SHOWNUMBERFLAG          0x00000001
#define PCB_LOCALREFFLAG            0x00000002
#define PCB_CHECKPLANESFLAG         0x00000004
#define PCB_SHOWPCB_FLAG_DRC        0x00000008
#define PCB_RUBBERBANDFLAG          0x00000010
#define PCB_DESCRIPTIONFLAG         0x00000020
#define PCB_NAMEONPCBFLAG           0x00000040
#define PCB_AUTOPCB_FLAG_DRC        0x00000080
#define PCB_ALLDIRECTIONFLAG        0x00000100
#define PCB_SWAPSTARTDIRFLAG        0x00000200
#define PCB_UNIQUENAMEFLAG          0x00000400
#define PCB_CLEARNEWFLAG            0x00000800
#define PCB_SNAPPCB_FLAG_PIN        0x00001000
#define PCB_SHOWMASKFLAG            0x00002000
#define PCB_THINDRAWFLAG            0x00004000
#define PCB_ORTHOMOVEFLAG           0x00008000
#define PCB_LIVEROUTEFLAG           0x00010000
#define PCB_THINDRAWPOLYFLAG        0x00020000
#define PCB_LOCKNAMESFLAG           0x00040000
#define PCB_ONLYNAMESFLAG           0x00080000
#define PCB_NEWPCB_FLAG_FULLPOLY    0x00100000
#define PCB_HIDENAMESFLAG           0x00200000
#define PCB_ENABLEPCB_FLAG_MINCUT   0x00400000



/* Convert flags to flags, set everything else to 0 */
pcb_flag_t pcb_flag_make(unsigned int src);

/* set bits of src in dst */
pcb_flag_t pcb_flag_add(pcb_flag_t dst, unsigned int src);

/* clear (unset) bits of src in dst */
pcb_flag_t pcb_flag_mask(pcb_flag_t dst, unsigned int src);

/* destroy flags: clear all bits and free fields */
void pcb_flag_erase(pcb_flag_t *f);

#define		pcb_no_flags() pcb_flag_make(0)

/* ---------------------------------------------------------------------------
 * some routines for flag setting, clearing, changing and testing
 */
#define PCB_FLAG_SET(F,P)       ((P)->Flags.f |= (F))
#define PCB_FLAG_CLEAR(F,P)     ((P)->Flags.f &= (~(F)))
#define PCB_FLAG_TEST(F,P)      ((P)->Flags.f & (F) ? 1 : 0)
#define PCB_FLAG_TOGGLE(F,P)    ((P)->Flags.f ^= (F))
#define PCB_FLAG_ASSIGN(F,V,P)  ((P)->Flags.f = ((P)->Flags.f & (~(F))) | ((V) ? (F) : 0))
#define PCB_FLAGS_TEST(F,P)     (((P)->Flags.f & (F)) == (F) ? 1 : 0)

typedef enum {
	PCB_CHGFLG_CLEAR,
	PCB_CHGFLG_SET,
	PCB_CHGFLG_TOGGLE
} pcb_change_flag_t;

#define PCB_FLAG_CHANGE(how, F, P) \
do { \
	switch(how) { \
		case PCB_CHGFLG_CLEAR:  PCB_FLAG_CLEAR(F, P); break; \
		case PCB_CHGFLG_SET:    PCB_FLAG_SET(F, P); break; \
		case PCB_CHGFLG_TOGGLE: PCB_FLAG_TOGGLE(F, P); break; \
	} \
} while(0)

/* Ignores dynamic flags and thermals */
int pcb_flag_eq(pcb_flag_t *f1, pcb_flag_t *f2);
#define PCB_FLAG_EQ(F1,F2)	pcb_flag_eq(&(F1), &(F2))

#define PCB_FLAG_THERM(L)		(0xf << (4 *((L) % 2)))

#define PCB_FLAG_THERM_TEST(L,P)		((P)->Flags.t[(L)/2] & PCB_FLAG_THERM(L) ? 1 : 0)
#define PCB_FLAG_THERM_GET(L,P)		(((P)->Flags.t[(L)/2] >> (4 * ((L) % 2))) & 0xf)
#define PCB_FLAG_THERM_CLEAR(L,P)	(P)->Flags.t[(L)/2] &= ~PCB_FLAG_THERM(L)
#define PCB_FLAG_THERM_ASSIGN(L,V,P)	(P)->Flags.t[(L)/2] = ((P)->Flags.t[(L)/2] & ~PCB_FLAG_THERM(L)) | ((V)  << (4 * ((L) % 2)))
#define PCB_FLAG_THERM_ASSIGN_(L,V,F)	(F).t[(L)/2] = (((F).t[(L)/2] & ~PCB_FLAG_THERM(L)) | ((V)  << (4 * ((L) % 2))))


#define PCB_FLAG_SQUARE_GET(P)		((P)->Flags.q)
#define PCB_FLAG_SQUARE_CLEAR(P)		(P)->Flags.q = 0
#define PCB_FLAG_SQUARE_ASSIGN(V,P)	(P)->Flags.q = V

/* Returns 1 if any of the bytes in arr is non-zero */
int pcb_mem_any_set(unsigned char *arr, int arr_len);
#define PCB_FLAG_THERM_TEST_ANY(P)	pcb_mem_any_set((P)->Flags.t, sizeof((P)->Flags.t))

/*** Dynamic flags ***/
#define PCB_DFLAG_SET(flg, dynf) (flg)->df[(dynf) / sizeof(PCB_DYNFLAG_WORD)] |= (1 << (dynf) % sizeof(PCB_DYNFLAG_WORD))
#define PCB_DFLAG_CLR(flg, dynf) (flg)->df[(dynf) / sizeof(PCB_DYNFLAG_WORD)] &= ~(1 << (dynf) % sizeof(PCB_DYNFLAG_WORD))
#define PCB_DFLAG_TEST(flg, dynf, val) (!!((flg)->df[(dynf) / sizeof(PCB_DYNFLAG_WORD)] & (1 << (dynf) % sizeof(PCB_DYNFLAG_WORD))))
#define PCB_DFLAG_PUT(flg, dynf, val) ((val) ? PCB_DFLAG_SET((flg), (dynf)) : PCB_DFLAG_CLR((flg), (dynf)))

extern const char *pcb_dynflag_cookie[PCB_DYNFLAG_BLEN];

typedef int pcb_dynf_t;
#define PCB_DYNF_INVALID (-1)
pcb_dynf_t pcb_dynflag_alloc(const char *cookie);
void pcb_dynflag_free(pcb_dynf_t dynf);

void pcb_dynflag_uninit(void);

#endif
