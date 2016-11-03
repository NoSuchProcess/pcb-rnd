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

#include "library.h"
#include "global.h"
#include "conf.h"

typedef enum { /* I/O type bitmask; each bit is one thing to save or load, not all formats support all things */
	PCB_IOT_PCB        = 1,
	PCB_IOT_FOOTPRINT  = 2,
	PCB_IOT_FONT       = 4,
	PCB_IOT_BUFFER     = 8
} plug_iot_t;

/**************************** API definition *********************************/
struct plug_io_s {
	plug_io_t *next;
	void *plugin_data;

	/* Check if the plugin supports format fmt, for writing (if wr != 0) or for
	   reading (if wr == 0) for I/O type typ. Return 0 if not supported or an
	   integer priority if supported. The higher the prio is the more likely
	   the plugin gets the next operation on the file. Base prio should be
	   100 for native formats.
	   NOTE: if any format can be chosen for output, the user will have to pick
	         one; for ordering plugins in that case, the format-neutral
	         save_preference_prio field is used.
	   */
	int (*fmt_support_prio)(plug_io_t *ctx, plug_iot_t typ, int wr, const char *fmt);

	/* Attempt to load a pcb design from Filename to Ptr.
	   Conf subtree at settings_dest is replaced by settings loaded from the
	   file unless it's CFR_invalid.
	   Return 0 on success. */
	int (*parse_pcb)(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest);

	/* Attempt to load an element from Filename to Ptr. Return 0 on success. */
	int (*parse_element)(plug_io_t *ctx, DataTypePtr Ptr, const char *name);

	/* Attempt to load fonts from a file. Return 0 on success. */
	int (*parse_font)(plug_io_t *ctx, FontTypePtr Ptr, const char *Filename);


	/* Write the buffer to a file. Return 0 on success. */
	int (*write_buffer)(plug_io_t *ctx, FILE *f, BufferType *buff);

	/* Write element data to a file. Return 0 on success. */
	int (*write_element)(plug_io_t *ctx, FILE *f, DataTypePtr e);

	/* Write PCB to f; there's a copy of the file we are going to
	   "overwrite", named in old_filename and the new file name we are
	   using is new_filename. The file names are NULL if we are saving
	   into a pipe. If emergency is true, do the safest save possible,
	   don't mind formatting and extras.
	   Return 0 on success. */
	int (*write_pcb)(plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, pcb_bool emergency);

	const char *default_fmt;
	const char *description;

	int save_preference_prio; /* all available save plugins are sorted by this before presenting them to the user to choose one */
};
extern plug_io_t *plug_io_chain;


/********** hook wrappers **********/
int ParsePCB(PCBTypePtr Ptr, const char *Filename, const char *fmt, int load_settings);
int ParseElement(DataTypePtr Ptr, const char *name);
int ParseFont(FontTypePtr Ptr, char *Filename);
int WriteBuffer(FILE *f, BufferType *buff, const char *fmt);
int WriteElementData(FILE *f, DataTypePtr e, const char *fmt);

/********** common function used to be part of file.[ch] and friends **********/
FILE *CheckAndOpenFile(const char *, pcb_bool, pcb_bool, pcb_bool *, pcb_bool *);
FILE *OpenConnectionDataFile(void);
int SavePCB(const char *, const char *fmt);
int LoadPCB(const char *name, const char *fmt, pcb_bool, int how); /* how: 0=normal pcb; 1=default.pcb, 2=misc (do not load settings) */
void EnableAutosave(void);
void Backup(void);
void SaveInTMP(void);
void EmergencySave(void);
void DisableEmergencySave(void);
int RevertPCB(void);
int SaveBufferElements(const char *, const char *fmt);
void pcb_sort_netlist(void);
void PrintQuotedString(FILE *, const char *);
void sort_library(LibraryTypePtr lib);
void set_some_route_style();
int WritePCBFile(const char *Filename, pcb_bool thePcb, const char *fmt, pcb_bool emergency);
int WritePipe(const char *, pcb_bool, const char *fmt);

#ifndef HAS_ATEXIT
void SaveTMPData(void);
void RemoveTMPData(void);
#endif

/********** helpers **********/

/* wish we had more than 32 IO plugins... */
#define PCB_IO_MAX_FORMATS 32

/* A list of format plugins available for a given purpose */
typedef struct {
	int len;
	const plug_io_t *plug[PCB_IO_MAX_FORMATS+1];
	char *digest[PCB_IO_MAX_FORMATS+1];           /* string that contains the format identifier and the description */
} pcb_io_formats_t;

/* Search all io plugins to see if typ/wr is supported. Return an ordered
   list in out. If do_digest is non-zero, fill in the digest field. Returns
   number of suitable io plugins. Call pcb_io_list_free() on out when it is not
   needed anymore. */
int pcb_io_list(pcb_io_formats_t *out, plug_iot_t typ, int wr, int do_digest);
void pcb_io_list_free(pcb_io_formats_t *out);

extern int pcb_io_err_inhibit;
#define pcb_io_err_inhibit_inc() pcb_io_err_inhibit++
#define pcb_io_err_inhibit_dec() \
do { \
	if (pcb_io_err_inhibit > 0) \
		pcb_io_err_inhibit--; \
} while(0)

/* Find all plugins that can handle typ/is_wr and build an ordered list in available[],
   highest prio first. If fmt is NULL, use the default fmt for each plugin.
   Return the length of the array. */
typedef struct {
	plug_io_t *plug;
	int prio;
} pcb_find_io_t;
int pcb_find_io(pcb_find_io_t *available, int avail_len, plug_iot_t typ, int is_wr, const char *fmt);


/* generic file name template substitution callbacks for pcb_strdup_subst:
    %P    pid
    %F    load-time file name of the current pcb
    %B    basename (load-time file name of the current pcb without path)
    %D    dirname (load-time file path of the current pcb, without file name, with trailing slash, might be ./)
    %N    name of the current pcb
    %T    wall time (Epoch)
*/
int pcb_build_fn_cb(gds_t *s, const char **input);

#endif
