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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_PLUG_IO_H
#define PCB_PLUG_IO_H

#include "library.h"
#include "global_typedefs.h"
#include "conf.h"

typedef enum { /* I/O type bitmask; each bit is one thing to save or load, not all formats support all things */
	PCB_IOT_PCB        = 1,
	PCB_IOT_FOOTPRINT  = 2,
	PCB_IOT_FONT       = 4,
	PCB_IOT_BUFFER     = 8
} pcb_plug_iot_t;

/**************************** API definition *********************************/
struct pcb_plug_io_s {
	pcb_plug_io_t *next;
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
	int (*fmt_support_prio)(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt);

	/* Test if the given board is readable by the plugin. The plugin should return
	   1 if it can handle the file or 0 if it can not. This check is not a deep
	   syntax analysis; the plugin should read barely enough headers to decide if
	   the file contains a the format it expect, then return error from parse_pcb
	   if there are syntax errors later. The file is open for read and positioned
	   to file begin in f */
	int (*test_parse_pcb)(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f);

	/* Attempt to load a pcb design from Filename to Ptr.
	   Conf subtree at settings_dest is replaced by settings loaded from the
	   file unless it's CFR_invalid.
	   Return 0 on success. */
	int (*parse_pcb)(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest);

	/* Attempt to load an element from Filename to Ptr. Return 0 on success. */
	int (*parse_element)(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name);

	/* Attempt to load fonts from a file. Return 0 on success. */
	int (*parse_font)(pcb_plug_io_t *ctx, pcb_font_t *Ptr, const char *Filename);


	/* Write the buffer to a file. Return 0 on success. */
	int (*write_buffer)(pcb_plug_io_t *ctx, FILE *f, pcb_buffer_t *buff, pcb_bool elem_only);

	/* Write element data to a file. Return 0 on success. */
	int (*write_element)(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *e);

	/* Write PCB to f; there's a copy of the file we are going to
	   "overwrite", named in old_filename and the new file name we are
	   using is new_filename. The file names are NULL if we are saving
	   into a pipe. If emergency is true, do the safest save possible,
	   don't mind formatting and extras.
	   Return 0 on success. */
	int (*write_pcb)(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, pcb_bool emergency);

	/* Attempt to write the font to file. Return 0 on success. */
	int (*write_font)(pcb_plug_io_t *ctx, pcb_font_t *font, const char *Filename);

	const char *default_fmt;
	const char *description;
	const char *default_extension; /* used to generate save-as filename */
	const char *fp_extension;      /* used to generate save-as filename for footprints */
	const char *mime_type;

	int save_preference_prio; /* all available save plugins are sorted by this before presenting them to the user to choose one */
};
extern pcb_plug_io_t *pcb_plug_io_chain;


/********** hook wrappers **********/
int pcb_parse_pcb(pcb_board_t *Ptr, const char *Filename, const char *fmt, int load_settings, int ignore_missing);
int pcb_parse_element(pcb_data_t *Ptr, const char *name);
int pcb_parse_font(pcb_font_t *Ptr, const char *Filename);
int pcb_write_buffer(FILE *f, pcb_buffer_t *buff, const char *fmt, pcb_bool elem_only);
int pcb_write_element_data(FILE *f, pcb_data_t *e, const char *fmt);
int pcb_write_font(pcb_font_t *Ptr, const char *Filename, const char *fmt);

/********** common function used to be part of file.[ch] and friends **********/
FILE *pcb_check_and_open_file(const char *, pcb_bool, pcb_bool, pcb_bool *, pcb_bool *);
FILE *pcb_open_connection_file(void);
int pcb_save_pcb(const char *, const char *fmt);
#define PCB_INHIBIT_BOARD_CHANGED 0x20
int pcb_load_pcb(const char *name, const char *fmt, pcb_bool, int how); /* how: 0=normal pcb; 1=default.pcb, 2=misc (do not load settings) | 0x10: ignore missing file, | PCB_INHIBIT_BOARD_CHANGED: do not send a board changed event */
void pcb_enable_autosave(void);
void pcb_backup(void);
void pcb_save_in_tmp(void);
void pcb_emergency_save(void);
void pcb_disable_emergency_save(void);
int pcb_revert_pcb(void);
int pcb_save_buffer_elements(const char *, const char *fmt);
void pcb_sort_netlist(void);
void pcb_print_quoted_string(FILE *, const char *);
void pcb_library_sort(pcb_lib_t *lib);
void pcb_set_some_route_style();
int pcb_write_pcb_file(const char *Filename, pcb_bool thePcb, const char *fmt, pcb_bool emergency, pcb_bool elem_only);
int pcb_write_pipe(const char *, pcb_bool, const char *fmt, pcb_bool elem_only);
void pcb_set_design_dir(const char *fn);

#ifndef HAS_ATEXIT
void pcb_tmp_data_save(void);
void pcb_tmp_data_remove(void);
#endif

/********** helpers **********/

/* wish we had more than 32 IO plugins... */
#define PCB_IO_MAX_FORMATS 32

/* A list of format plugins available for a given purpose */
typedef struct {
	int len;
	const pcb_plug_io_t *plug[PCB_IO_MAX_FORMATS+1];
	char *digest[PCB_IO_MAX_FORMATS+1];           /* string that contains the format identifier and the description (strdup'd) */
	const char *extension[PCB_IO_MAX_FORMATS+1];  /* default file extension, with the leading . (not strdup'd) */
} pcb_io_formats_t;

typedef enum {
	PCB_IOL_EXT_NONE,
	PCB_IOL_EXT_BOARD,
	PCB_IOL_EXT_FP
} pcb_io_list_ext_t;

/* Search all io plugins to see if typ/wr is supported. Return an ordered
   list in out. If do_digest is non-zero, fill in the digest field. Returns
   number of suitable io plugins. Call pcb_io_list_free() on out when it is not
   needed anymore. */
int pcb_io_list(pcb_io_formats_t *out, pcb_plug_iot_t typ, int wr, int do_digest, pcb_io_list_ext_t ext);
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
	pcb_plug_io_t *plug;
	int prio;
} pcb_find_io_t;
int pcb_find_io(pcb_find_io_t *available, int avail_len, pcb_plug_iot_t typ, int is_wr, const char *fmt);

void pcb_io_uninit(void);

#endif
