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

#ifndef PCB_BUILD_RUN_H
#define PCB_BUILD_RUN_H

void pcb_quit_app(void);

/* Returns a string that has a bunch of information about this program. */
char *pcb_get_info_program(void);

/* Returns a string that has a bunch of information about the copyrights. */
char *pcb_get_info_copyright(void);

/* Returns a string about how the program is licensed. */
char *pcb_get_info_license(void);

/* Returns a string that has a bunch of information about the websites. */
char *pcb_get_info_websites(const char **url_out);

/* Returns a string as the concatenation of pcb_get_info_program() and pcb_get_info_websites() */
char *pcb_get_info_comments(void);

/* Returns a string that has a bunch of information about the options selected at compile time. */
char *pcb_get_info_compile_options(void);

/* Author's name: either from the config system (typically from the design) or
   as a last resort from the OS */
const char *pcb_author(void);

void pcb_catch_signal(int Signal);

#endif
