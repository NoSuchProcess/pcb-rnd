/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#ifndef PCB_PLUG_IO_H
#define PCB_PLUG_IO_H
/**************************** API definition *********************************/
typedef struct plug_io_s plug_io_t;
struct plug_io_s {
	plug_io_t *next;
	void *plugin_data;

	/* Attempt to load a pcb design from Filename to Ptr. Return 0 on success. */
	int (*parse_pcb)(plug_io_t *ctx, PCBTypePtr Ptr, char *Filename);

	/* Attempt to load an element from Filename to Ptr. Return 0 on success. */
	int (*parse_element)(plug_io_t *ctx, DataTypePtr Ptr, const char *name);

	/* Attempt to load fonts from a file. Return 0 on success. */
	int (*parse_font)(plug_io_t *ctx, FontTypePtr Ptr, char *Filename);


	/* Write the buffer to a file. Return 0 on success. */
	int (*write_buffer)(plug_io_t *ctx, FILE *f, BufferType *buff);

	/* Write element data to a file. Return 0 on success. */
	int (*write_element)(plug_io_t *ctx, FILE *f, DataTypePtr e);

	/* Writes PCB to a file. Return 0 on success. */
	int (*write_pcb)(plug_io_t *ctx, FILE *f);

};
extern plug_io_t *plug_io_chain;


/********** hook wrappers **********/
int ParsePCB(PCBTypePtr Ptr, char *Filename);
int ParseElement(DataTypePtr Ptr, const char *name);
int ParseFont(FontTypePtr Ptr, char *Filename);
int WriteBuffer(FILE *f, BufferType *buff);
int WriteElementData(FILE *f, DataTypePtr e);
int WritePCB(FILE *f);


/********** common function used to be part of file.[ch] and friends **********/
FILE *CheckAndOpenFile(char *, bool, bool, bool *, bool *);
FILE *OpenConnectionDataFile(void);
int SavePCB(char *);
int LoadPCB(char *, bool, bool);
void EnableAutosave(void);
void Backup(void);
void SaveInTMP(void);
void EmergencySave(void);
void DisableEmergencySave(void);
int RevertPCB(void);
int ImportNetlist(char *);
int SaveBufferElements(char *);
void sort_netlist(void);
void PrintQuotedString(FILE *, const char *);
void sort_library(LibraryTypePtr lib);
void set_some_route_style();
int WritePCBFile(char *);
int WritePipe(char *, bool);
int real_load_pcb(char *Filename, bool revert, bool require_font, bool is_misc);


#ifndef HAS_ATEXIT
#ifdef HAS_ON_EXIT
void GlueEmergencySave(int, caddr_t);
#else
void SaveTMPData(void);
void RemoveTMPData(void);
#endif
#endif

#endif
